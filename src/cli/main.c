#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <fuse_opt.h>
#include "util.h"
#include "golddrive.h"
#include <Shlwapi.h> /* PathRemoveFileSpecA */
#pragma comment(lib, "shlwapi.lib")

/* global variables */
size_t		g_sftp_calls;
size_t		g_sftp_cached_calls;
SANSSH *	g_ssh;
SRWLOCK		g_ssh_lock;
fs_config	g_fs;
char*		g_logfile;

/* supported fs operations */
static struct fuse_operations fs_ops = {
	.init = f_init,
	.statfs = f_statfs,
	.getattr = f_getattr,
	.opendir = f_opendir,
	.readdir = f_readdir,
	.releasedir = f_releasedir,
	.unlink = f_unlink,
	.rename = f_rename,
	.create = f_create,
	.open = f_open,
	.read = f_read,
	.write = f_write,
	.release = f_release,
	/*.flush = f_flush,*/
	.mkdir = f_mkdir,
	.rmdir = f_rmdir,
	.truncate = f_truncate,
	.fsync = f_fsync,
	.utimens = f_utimens,
};

enum {
	KEY_HELP,
	KEY_VERSION,
};

#define fs_OPT(t, p, v) { t, offsetof(fs_config, p), v }

static struct fuse_opt fs_opts[] = {
	fs_OPT("host=%s",           host, 0),
	fs_OPT("-h %s",             host, 0),
	fs_OPT("-h=%s",             host, 0),
	fs_OPT("user=%s",           user, 0),
	fs_OPT("-u %s",             user, 0),
	fs_OPT("-u=%s",             user, 0),
	fs_OPT("port=%d",           port, 0),
	fs_OPT("-p %d",             port, 0),
	fs_OPT("-p=%d",             port, 0),
	fs_OPT("pkey=%s",           pkey, 0),
	fs_OPT("-k %s",             pkey, 0),
	fs_OPT("-k=%s",             pkey, 0),
	fs_OPT("hidden",		    hidden, 1),

	FUSE_OPT_KEY("--version",      KEY_VERSION),
	FUSE_OPT_KEY("--help",         KEY_HELP),
	FUSE_OPT_END
};

static int fs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	char exepath[MAX_PATH];
	switch (key) {
	case FUSE_OPT_KEY_NONOPT:
		if (!g_fs.drive) {
			g_fs.drive = strdup(arg);
			return 0;
		}
		if (!g_fs.remote) {
			g_fs.remote = strdup(arg);
			return 0;
		}
		fprintf(stderr, "golddrive: invalid argument '%s'\n", arg);
		return -1;
	case KEY_HELP:
		fprintf(stderr,
			"\n"
			"Usage: golddrive drive [remote] [options]\n"
			"\n"
			"drive : letter and colon (ex Z:)\n"
			"remote: remote network path as \\\\golddrive\\[[locuser=]user@]host[!port][\\path]\n"
			"Options:\n"
			"    --help                     print this help\n"
			"    --version                  print version\n"
			"    -o opt,[opt...]            mount options, -o=opt or -oopt is also valid\n"						
			"    -h HOST, -o host=HOST      ssh server name or IP, default: random from config file\n"
			"    -u USER, -o user=USER      user to connect to ssh server, default: current user\n"
			"    -k PKEY, -o pkey=PKEY      private key, default: %%USERPROFILE%%\\.ssh\\id_rsa\n"
			"    -p PORT, -o port=PORT      server port, default: 22\n"
			"    -o hidden                  show hidden files\n"
			"\n"
			"WinFsp-FUSE options:\n"
			"    -s                         disable multi-threaded operation\n"
			"    -d, -o debug               enable debug output\n"
			"    -o create_umask=MASK       set file umask permissions (octal, overriten by server)\n"
			"    -o rellinks                interpret absolute symlinks as volume relative\n"
			"    -o DebugLog=FILE           debug log file (requires -d)\n"
			"    -o FileInfoTimeout=N       metadata timeout (millis, -1 for data caching)\n"
			"    -o DirInfoTimeout=N        directory info timeout (millis)\n"
			"    -o VolumeInfoTimeout=N     volume info timeout (millis)\n"
			"    -o KeepFileCache           do not discard cache when files are closed\n"
			"    -o ThreadCount             number of file system dispatcher threads\n"
		);
		//fuse_opt_add_arg(outargs, "-h");
		//fuse_main(outargs->argc, outargs->argv, &fs_ops, NULL);
		exit(1);

	case KEY_VERSION:
		
		GetModuleFileNameA(NULL, exepath, MAX_PATH);
		//PathRemoveFileSpecA(appdir);
		char* version = calloc(100, sizeof(char));
		get_file_version(exepath, version);
		fprintf(stderr, "Golddrive v%s\nBuild date: %s\n", version, __DATE__);
		free(version);
		fuse_opt_add_arg(outargs, "--version");
		fuse_main(outargs->argc, outargs->argv, &fs_ops, NULL);
		exit(0);
	}
	return 1;
}

//char **new_argv(int count, ...)
//{
//	va_list args;
//	int i;
//	char **argv = malloc((count + 1) * sizeof(char*));
//	char *temp;
//	va_start(args, count);
//	for (i = 0; i < count; i++) {
//		temp = va_arg(args, char*);
//		argv[i] = malloc(sizeof(temp));
//		argv[i] = temp;
//	}
//	argv[i] = NULL;
//	va_end(args);
//	return argv;
//}

static int parse_network_path(fs_config* fs)
{
	char *npath, *locuser, *user, *host, *port, *p;
	
	/* translate backslash to forward slash */
	for (p = fs->remote; *p; p++)
		if ('\\' == *p)
			*p = '/';

	npath = strdup(fs->remote);
	/* remove first slash if it has 2 slashes // */
	size_t len = strlen(npath);
	if (len > 2 && npath[0] == '/' && npath[1] == '/') {
		memcpy(fs->remote, npath + 1, len - 1);
		fs->remote[len - 1] = '\0';
		len--;
	}

	if (strncmp(fs->remote, "/golddrive/", 11) != 0) {
		gdlog("Invalid service name, only '\\\\golddrive' is supported: %s\n", fs->remote);
		return -1;
	}

	char mountpoint[256];
	memcpy(mountpoint, fs->remote + 11, len-11);
	mountpoint[len-11] = '\0';
	fs->mountpoint = strdup(mountpoint);
	
	
	
	/* get service name (\\golddrive\) */
	p = npath;
	while ('/' == *p)
		p++;
	//service = p;
	while (*p && '/' != *p)
		p++;
	if (*p)
		*p++ = '\0';

	

	/* parse instance name (syntax: [locuser=]user@host!port/path) */
	locuser = 0;
	user = 0;
	port = 0;
	host = p;
	while (*p && '/' != *p) {
		if ('=' == *p) {
			*p = '\0';
			locuser = host;
			host = p + 1;
		} else if ('@' == *p) {
			*p = '\0';
			user = host;
			host = p + 1;
		} else if ('!' == *p) {
			*p = '\0';
			port = p + 1;
		}
		p++;
	}
	if (*p)
		*p++ = '\0';
	if(host)
		fs->host = strdup(host);
	if (locuser)
		fs->locuser = strdup(locuser);
	if(user)
		fs->user = strdup(user);
	if(port)
		fs->port = atoi(port);
	
	/* mount root by default, prepend a slash before path if needed */
	fs->path = strdup(p);
	if (*p != '/') {
		char s[256];
		strcpy(s, "/");
		strcat(s, p);
		free(fs->path);
		fs->path = strdup(s);
	}
	free(npath);
	return 0;
}

static int load_config_file(fs_config* fs)
{
	int rc = 0;
	//// app dir
	//char appdir[MAX_PATH];
	//GetModuleFileNameA(NULL, appdir, MAX_PATH);
	//PathRemoveFileSpecA(appdir);
	//rc = load_ini(appdir, fs);
	//if (!rc)
	//	rc = load_json(fs);
	//return rc;
	char* appdata = getenv("LOCALAPPDATA");
	char jsonfile[PATH_MAX];
	sprintf_s(jsonfile, MAX_PATH, "%s\\golddrive\\config.json", appdata);
	fs->json = strdup(jsonfile);
	rc = load_json(fs);
	return rc;
		
}

static void init_logging()
{
	char* appdata = getenv("LOCALAPPDATA");
	char logfolder[PATH_MAX];
	sprintf_s(logfolder, MAX_PATH, "%s\\golddrive", appdata);
	
	if (!directory_exists(logfolder)) 
		_mkdir(logfolder);
	if (directory_exists(logfolder)) {
		g_logfile = malloc(PATH_MAX);
		sprintf_s(g_logfile, MAX_PATH, "%s\\golddrive.log", logfolder);
	}
	else {
		g_logfile = 0;	
	}
}

int main(int argc, char *argv[])
{
	// load fuse driver
	if (FspLoad(0) != STATUS_SUCCESS) {
		fprintf(stderr, "failed to load winfsp driver, either dll not present or wrong version\n");
		return -1;
	}

	// logging
	init_logging();
	

	// parameters
	int rc;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	memset(&g_fs, 0, sizeof(g_fs));
	rc = fuse_opt_parse(&args, &g_fs, fs_opts, fs_opt_proc);
	if (rc) {
		fprintf(stderr, "bad arguments, try --help\n");
		return -1;
	}

	if (!g_fs.drive || strlen(g_fs.drive) < 2 || argc < 2) {
		fuse_opt_add_arg(&args, "--help");
		fuse_opt_parse(&args, &g_fs, fs_opts, fs_opt_proc);
		return 1;
	}
	
	fuse_opt_parse(&args, &g_fs, fs_opts, fs_opt_proc);

	// load missing arguments from config file
	g_fs.drive[0] = toupper(g_fs.drive[0]);
	g_fs.letter = g_fs.drive[0];
	load_config_file(&g_fs);

	// parse network path
	if(g_fs.remote)
		parse_network_path(&g_fs);



	// finally setup missing arguments with defaults
	// user
	if (!g_fs.user) {
		g_fs.user = getenv("USERNAME");
	}
	// private key
	if (!g_fs.pkey) {
		//char profile[BUFFER_SIZE];
		//ExpandEnvironmentStringsA("%USERPROFILE%", profile, BUFFER_SIZE);
		char* profile = getenv("USERPROFILE");
		g_fs.pkey = malloc(MAX_PATH);
		sprintf_s(g_fs.pkey, MAX_PATH, "%s\\.ssh\\id_rsa-%s-golddrive", profile, g_fs.user);
		//strcpy_s(g_fs.pkey, MAX_PATH, profile);
		//strcat_s(g_fs.pkey, MAX_PATH, "\\.ssh\\id_rsa-");
	}
	if (!file_exists(g_fs.pkey)) {
		fprintf(stderr, "error: cannot read private key: %s\n", g_fs.pkey);
		return 1;
	}
	if (!g_fs.port) {
		g_fs.port = 22;
	}

//	if (!g_fs.host && g_fs.hostcount > 0) {
//		// pick random host
//		g_fs.host = g_fs.hostlist[randint(0,g_fs.hostcount-1)];
//	}

	// show parameters
	gdlog("Golddrive arguments:\n");
	gdlog("drive   = %s\n", g_fs.drive);
	gdlog("remote  = %s\n", g_fs.remote);
	gdlog("mountp  = %s\n", g_fs.mountpoint);
	gdlog("user    = %s\n", g_fs.user);
	gdlog("host    = %s\n", g_fs.host);
	gdlog("port    = %d\n", g_fs.port);
	gdlog("pkey    = %s\n", g_fs.pkey);
	gdlog("hidden  = %d\n", g_fs.hidden);


	// initiaize small read/write lock
	InitializeSRWLock(&g_ssh_lock);

	g_sftp_calls = 0;
	g_sftp_cached_calls = 0;

	g_ssh = san_init_ssh(g_fs.host, g_fs.port, g_fs.user, g_fs.pkey);
	if (!g_ssh) 
		return 1;

	// get uid
	char cmd[100], out[100], err[100];
	
	snprintf(cmd,sizeof(cmd), "id -u %s\n", g_fs.user);
	rc = run_command(cmd, out, err);
	if (rc == 0) {
		out[strcspn(out, "\r\n")] = 0;
		g_fs.remote_uid = atoi(out);
		gdlog("uid     = %d\n", g_fs.remote_uid);
	}
	rc = run_command("echo $HOME\n", out, err);
	if (rc == 0) {
		out[strcspn(out, "\r\n")] = 0;
		g_fs.home = malloc(sizeof out);
		strcpy_s(g_fs.home, sizeof out, out);
		gdlog("home    = %s\n", g_fs.home);
	}

	// number of threads
	//printf("Threads = %d\n", san_threads(5, get_number_of_processors()));

	// run fuse main
	char volprefix[256];
	if(g_fs.remote)
		sprintf_s(volprefix, sizeof(volprefix), "-oVolumePrefix=%s", g_fs.remote);
	else
		sprintf_s(volprefix, sizeof(volprefix), "-oVolumePrefix=/golddrive/%c", g_fs.letter);
	gdlog("Prefix  = %s\n", g_fs.remote);
	gdlog("WinFsp arguments:\n");
	fuse_opt_add_arg(&args, volprefix);
	char volname[256];
	sprintf_s(volname, sizeof(volname), "-ovolname=%s", g_fs.mountpoint);
	fuse_opt_add_arg(&args, volname);
	fuse_opt_add_arg(&args, "-oFileSystemName=Golddrive");
	fuse_opt_add_arg(&args, "-orellinks");
	fuse_opt_add_arg(&args, "-ouid=-1,gid=-1");

	// add json args
	if (g_fs.args && strcmp(g_fs.args, "") != 0) {
		fuse_opt_add_arg(&args, g_fs.args);
		fuse_opt_parse(&args, &g_fs, fs_opts, fs_opt_proc);
	}
	// drive must be the last argument for winfsp
	fuse_opt_add_arg(&args, g_fs.drive);

	// debug arguments
	for (int i = 1; i < args.argc; i++)
		gdlog("arg %d   = %s\n", i, args.argv[i]);

	rc = fuse_main(args.argc, args.argv, &fs_ops, NULL);
	
	// cleanup
	InitializeSRWLock(&g_ssh_lock);
	san_finalize();
	return rc;
}
