#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <fuse_opt.h>
#include "util.h"
#include "sanfs.h"
#include "cache.h"
#include <Shlwapi.h> /* PathRemoveFileSpecA */
#pragma comment(lib, "shlwapi.lib")

#define VERSION "1.1.2"

/* global variables */
size_t				g_sftp_calls;
size_t				g_sftp_cached_calls;
SANSSH *			g_ssh;
SRWLOCK				g_ssh_lock;
sanfs_config		g_sanfs;

/* supported fs operations */
static struct fuse_operations sanfs_ops = {
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

#define SANFS_OPT(t, p, v) { t, offsetof(sanfs_config, p), v }

static struct fuse_opt sanfs_opts[] = {
	SANFS_OPT("host=%s",           host, 0),
	SANFS_OPT("-h %s",             host, 0),
	SANFS_OPT("-h=%s",             host, 0),
	SANFS_OPT("user=%s",           user, 0),
	SANFS_OPT("-u %s",             user, 0),
	SANFS_OPT("-u=%s",             user, 0),
	SANFS_OPT("port=%d",           port, 0),
	SANFS_OPT("-p %d",             port, 0),
	SANFS_OPT("-p=%d",             port, 0),
	SANFS_OPT("pkey=%s",           pkey, 0),
	SANFS_OPT("-k %s",             pkey, 0),
	SANFS_OPT("-k=%s",             pkey, 0),
	SANFS_OPT("hidden",			   hidden, 1),

	FUSE_OPT_KEY("--version",      KEY_VERSION),
	FUSE_OPT_KEY("--help",         KEY_HELP),
	FUSE_OPT_END
};

static int sanfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	switch (key) {
	case FUSE_OPT_KEY_NONOPT:
		if (!g_sanfs.drive) {
			g_sanfs.drive = strdup(arg);
			return 0;
		}
		fprintf(stderr, "sanfs: invalid argument '%s'\n", arg);
		return -1;
	case KEY_HELP:
		fprintf(stderr,
			"\n"
			"Usage: sanfs.exe drive [options]\n"
			"\n"
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
			"    -o umask=MASK              set file permissions (octal)\n"
			"    -o create_umask=MASK       set newly created file permissions (octal)\n"
			"    -o rellinks                interpret absolute symlinks as volume relative\n"
			"    -o DebugLog=FILE           debug log file (requires -d)\n"
			"    -o FileInfoTimeout=N       metadata timeout (millis, -1 for data caching)\n"
			"    -o DirInfoTimeout=N        directory info timeout (millis)\n"
			"    -o VolumeInfoTimeout=N     volume info timeout (millis)\n"
			"    -o KeepFileCache           do not discard cache when files are closed\n"
			"    -o ThreadCount             number of file system dispatcher threads\n"
		);
		//fuse_opt_add_arg(outargs, "-h");
		//fuse_main(outargs->argc, outargs->argv, &sanfs_ops, NULL);
		exit(1);

	case KEY_VERSION:
		fprintf(stderr, "Sanfs v%s\nBuild date: %s\n", VERSION, __DATE__);
		fuse_opt_add_arg(outargs, "--version");
		fuse_main(outargs->argc, outargs->argv, &sanfs_ops, NULL);
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

static int load_config_file(sanfs_config* sanfs)
{
	int rc = 0;
	// app dir
	char appdir[MAX_PATH];
	GetModuleFileNameA(NULL, appdir, MAX_PATH);
	PathRemoveFileSpecA(appdir);
	rc = load_ini(appdir, sanfs);
	if (!rc)
		rc = load_json(sanfs);
	return rc;
}

int main(int argc, char *argv[])
{
	// load fuse dll
	if (FspLoad(0) != STATUS_SUCCESS) {
		fprintf(stderr, "failed to load winfsp driver, either dll not present or wrong version\n");
		return -1;
	}

	int rc;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	memset(&g_sanfs, 0, sizeof(g_sanfs));
	fuse_opt_parse(&args, &g_sanfs, sanfs_opts, sanfs_opt_proc);

	if (argc < 2) {
		fuse_opt_add_arg(&args, "--help");
		fuse_opt_parse(&args, &g_sanfs, sanfs_opts, sanfs_opt_proc);
		return 1;
	}
	
	fuse_opt_parse(&args, &g_sanfs, sanfs_opts, sanfs_opt_proc);

	// load missing arguments from config file
	load_config_file(&g_sanfs);

	// finally setup missing arguments with defaults
	// user
	if (!g_sanfs.user) {
		g_sanfs.user = getenv("USERNAME");
	}
	// private key
	if (!g_sanfs.pkey) {
		char profile[BUFFER_SIZE];
		ExpandEnvironmentStringsA("%USERPROFILE%", profile, BUFFER_SIZE);
		g_sanfs.pkey = malloc(MAX_PATH);
		sprintf_s(g_sanfs.pkey, MAX_PATH, "%s\\.ssh\\id_rsa-%s-golddrive", profile, g_sanfs.user);
		//strcpy_s(g_sanfs.pkey, MAX_PATH, profile);
		//strcat_s(g_sanfs.pkey, MAX_PATH, "\\.ssh\\id_rsa-");
	}
	if (!file_exists(g_sanfs.pkey)) {
		fprintf(stderr, "error: cannot read private key: %s\n", g_sanfs.pkey);
		return 1;
	}
	if (!g_sanfs.port)
		g_sanfs.port = 22;

	// show parameters
	printf("host    = %s\n", g_sanfs.host);
	printf("drive   = %s\n", g_sanfs.drive);
	printf("port    = %d\n", g_sanfs.port);
	printf("user    = %s\n", g_sanfs.user);
	printf("pkey    = %s\n", g_sanfs.pkey);
	printf("hidden  = %d\n", g_sanfs.hidden);

	// initiaize small read/write lock
	InitializeSRWLock(&g_ssh_lock);

	g_sftp_calls = 0;
	g_sftp_cached_calls = 0;

	g_ssh = san_init_ssh(g_sanfs.host, g_sanfs.port, g_sanfs.user, g_sanfs.pkey);
	if (!g_ssh) 
		return 1;

	// get uid
	char cmd[100], out[100], err[100];
	
	snprintf(cmd,sizeof(cmd), "id -u %s\n", g_sanfs.user);
	rc = run_command(cmd, out, err);
	if (rc == 0) {
		out[strcspn(out, "\r\n")] = 0;
		g_sanfs.remote_uid = atoi(out);
		printf("uid     = %d\n", g_sanfs.remote_uid);
	}
	rc = run_command("echo $HOME\n", out, err);
	if (rc == 0) {
		out[strcspn(out, "\r\n")] = 0;
		g_sanfs.home = malloc(sizeof out);
		strcpy_s(g_sanfs.home, sizeof out, out);
		printf("home    = %s\n", g_sanfs.home);
	}

	// get gid
	//snprintf(cmd, sizeof(cmd), "id -g %s\n", user);
	//rc = run_command(cmd, out, err);
	//if (rc == 0) {
	//	out[strcspn(out, "\r\n")] = 0;
	//	gid = atoi(out);
	//	printf("gid=%d\n", gid);
	//}
		
	// get number of links
	//int nlinks;
	//const char *path = "~";
	//snprintf(cmd, sizeof(cmd), "stat -c %%h %s\n", path);
	//rc = run_command(cmd, out, err);
	//if (rc == 0) {
	//	out[strcspn(out, "\r\n")] = 0;
	//	nlinks = atoi(out);
	//	printf("%s nlinks=%d\n", path, nlinks);
	//}

	// number of threads
	//printf("Threads = %d\n", san_threads(5, get_number_of_processors()));

	// run fuse main
	char volprefix[256];
	sprintf_s(volprefix, sizeof(volprefix), "-oVolumePrefix=/sanfs/%s@%s", g_sanfs.user, g_sanfs.host);
	fuse_opt_add_arg(&args, volprefix);
	char volname[256];
	sprintf_s(volname, sizeof(volname), "-ovolname=%s@%s", g_sanfs.user, g_sanfs.host);
	fuse_opt_add_arg(&args, volname);
	fuse_opt_add_arg(&args, "-oFileSystemName=SANFS");
	fuse_opt_add_arg(&args, "-orellinks");
	//fuse_opt_add_arg(&args, "-ouid=-1,gid=-1,create_umask=007,mask=007");
	fuse_opt_parse(&args, &g_sanfs, sanfs_opts, sanfs_opt_proc);
	
	// no need to parse the drive, it must be the last argument
	fuse_opt_add_arg(&args, g_sanfs.drive);

	// debug arguments
	for (int i = 1; i < args.argc; i++)
		printf("arg %d   = %s\n", i, args.argv[i]);

	rc = fuse_main(args.argc, args.argv, &sanfs_ops, NULL);
	
	// cleanup
	InitializeSRWLock(&g_ssh_lock);
	san_finalize();
	return rc;
}
