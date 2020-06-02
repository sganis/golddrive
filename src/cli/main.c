#include "util.h"
#include "gd.h"
#include "cache.h"
#include <direct.h>						/* _mkdir */
#include <openssl/opensslv.h>			/* to get version only */
//#include <Shlwapi.h>					/* PathRemoveFileSpecA */
//#pragma comment(lib, "shlwapi.lib")

/* global variables */
GDSSH*				g_ssh;
size_t				g_sftp_calls;
size_t				g_cache_calls;
CACHE_STAT*			g_cache_stat_ht;
SRWLOCK				g_ssh_lock;
SRWLOCK				g_log_lock;
SRWLOCK				g_cache_stat_lock;
GDCONFIG			g_conf;
char*				g_logfile;
char*				g_logurl;

static void* f_init(struct fuse_conn_info* conn, 
	struct fuse_config* conf)
{
#if defined(FUSE_CAP_READDIRPLUS)
	conn->want |= (conn->capable & FUSE_CAP_READDIRPLUS);
#endif
	//conn->want |= (conn->capable & FSP_FUSE_CAP_STAT_EX);
#if defined(FSP_FUSE_USE_STAT_EX) && defined(FSP_FUSE_CAP_STAT_EX)
	conn->want |= (conn->capable & FSP_FUSE_CAP_STAT_EX);
#endif
	return fuse_get_context()->private_data;
}

static int f_statfs(const char* path, struct fuse_statvfs* stbuf)
{
	realpath(path);
	return -1 != gd_statvfs(path, stbuf) ? 0 : -errno;
}

static int f_getattr(const char* path, struct fuse_stat* stbuf, 
	struct fuse_file_info* fi)
{
	int rc;
	if (0 == fi)
	{
		realpath(path);
		rc = -1 != gd_stat(path, stbuf) ? 0 : -errno;
	}
	else
	{
		intptr_t fd = fi_fd(fi);
		rc = -1 != gd_fstat(fd, stbuf) ? 0 : -errno;
	}
	//if (rc) {
	//	// debug
	//	int err = -errno;
	//}

	//printf("f_getattr: %s, rc=%d\n", path, rc);

	return rc;
}

static int f_readlink(const char* path, char* buf, size_t size)
{
	realpath(path);
	int rc = -1 != gd_readlink(path, buf, size) ? 0 : -errno;
	/*if (rc) {
		int err = -errno;
	}*/
	return rc;
}


static int f_unlink(const char* path)
{
	realpath(path);
	int rc = -1 != gd_unlink(path) ? 0 : -errno;
	/*if (rc) {
		int err = -errno;
	}*/
	return rc;
}



static int f_create(const char* path, fuse_mode_t mode, 
	struct fuse_file_info* fi)
{
	// printf("f_create: %s, mode=%d, flags=%d\n", path, mode, fi->flags);

	realpath(path);
	intptr_t fd;
	// remove execution bit in files
	// fuse_mode_t mod = mode & 0666; // int 438 
	fuse_mode_t mod = mode;
	int rc = -1 != (fd = gd_open(path, fi->flags, mod)) ? 
		(fi_setfd(fi, fd), 0) : -errno;
	return rc;
}

static int f_truncate(const char* path, fuse_off_t size, 
	struct fuse_file_info* fi)
{
	if (0 == fi)
	{
		realpath(path);
		return -1 != gd_truncate(path, size) ? 0 : -errno;
	}
	else
	{
		intptr_t fd = fi_fd(fi);
		return -1 != gd_ftruncate(fd, size) ? 0 : -errno;
	}
}

static int f_open(const char* path, struct fuse_file_info* fi)
{
	// printf("f_open: %s, flags=%d\n", path, fi->flags);

	realpath(path);
	intptr_t fd;
	int rc = -1 != (fd = gd_open(path, fi->flags, 0)) ? 
		(fi_setfd(fi, fd), 0) : -errno;
	/*if (rc != 0) {
		printf("error: f_open: %s, flags=%d\n", path, fi->flags);
	}*/
	return rc;
}

static int f_read(const char* path, char* buf, size_t size, 
	fuse_off_t off, struct fuse_file_info* fi)
{
	intptr_t fd = fi_fd(fi);
	int nb;
	int rc = -1 != (nb = gd_read(fd, buf, size, off)) ? nb : -errno;

	return rc;

}

static int f_write(const char* path, const char* buf, 
	size_t size, fuse_off_t off, struct fuse_file_info* fi)
{
	int rc = 0;
	intptr_t fd = fi_fd(fi);
	int nb;
	rc = -1 != (nb = gd_write(fd, buf, size, off)) ? nb : -errno;

	//if(size != nb)
	//	printf("f_write error: %s, flags=%d, size=%zu, rc=%d\n", path, fi->flags, size, rc);

	return rc;
}


static int f_release(const char* path, struct fuse_file_info* fi)
{
	intptr_t fd = fi_fd(fi);
	return gd_close(fd);
}

static int f_rename(const char* oldpath, const char* newpath, 
	unsigned int flags)
{
	realpath(newpath);
	realpath(oldpath);
	int rc = -1 != gd_rename(oldpath, newpath) ? 0 : -errno;
	/*if (rc) {
		int err = -errno;
	}*/
	return rc;
}

static int f_opendir(const char* path, struct fuse_file_info* fi)
{
	realpath(path);
	GDDIR* dirp;
	return 0 != (dirp = gd_opendir(path)) ? 
		(fi_setdirp(fi, dirp), 0) : 
		-errno;
}

static int f_readdir(const char* path, void* buf, 
	fuse_fill_dir_t filler, fuse_off_t off,
	struct fuse_file_info* fi, enum fuse_readdir_flags flags)
{
	GDDIR* dirp = fi_dirp(fi);
	struct GDDIRENT* de;

	gd_rewinddir(dirp);

	for (;;) {
		errno = 0;
		de = gd_readdir(dirp);
		if (de == 0)
			break;

		if (0 != filler(buf, de->d_name, &de->d_stat, 
			0, FUSE_FILL_DIR_PLUS))
			return -ENOMEM;
	}

	return -errno;
}

static int f_releasedir(const char* path, 
	struct fuse_file_info* fi)
{
	GDDIR* dirp = fi_dirp(fi);
	return gd_closedir(dirp);
}

static int f_mkdir(const char* path, fuse_mode_t  mode)
{
	realpath(path);
	int rc = -1 != gd_mkdir(path, mode) ? 0 : -errno;
	/*if (rc) {
		int err = -errno;
	}*/
	return rc;
}

static int f_rmdir(const char* path)
{
	realpath(path);
	//int rc;
	// rmdir fails if hidden files are not shown
	//if (!g_fs.hidden)
	//	rc = gd_rm_hidden(path);

	int rc = -1 != gd_rmdir(path) ? 0 : -errno;
	/*if (rc) {
		int err = -errno;
	}*/
	//ShowLastError();
	return rc;
}


static int f_utimens(const char* path, 
	const struct fuse_timespec tv[2], 
	struct fuse_file_info* fi)
{
	realpath(path);
	return -1 != gd_utimens(path, tv, fi) ? 0 : -errno;
}

static int f_fsync(const char* path, 
	int datasync, struct fuse_file_info* fi)
{
	intptr_t fd = fi_fd(fi);
	return -1 != gd_fsync(fd) ? 0 : -errno;
}

static int f_flush(const char* path, struct fuse_file_info* fi)
{
	intptr_t fd = fi_fd(fi);
	return -1 != gd_flush(fd) ? 0 : -errno;
}

//int f_chmod(const char* path, fuse_mode_t mode, struct fuse_file_info* fi)
//{
//	realpath(path);
//	//char cmd[1000], out[1000], err[1000];
//	//sprintf_s(cmd, sizeof cmd, "chmod 777 \"%s\"\n", path);
//	//run_command(cmd, out, err);
//	return -1 != gd_chmod(path, mode) ? 0 : -errno;
//}
//
//int f_chown(const char* path, fuse_uid_t uid, fuse_gid_t gid, struct fuse_file_info* fi)
//{
//	realpath(path);
//	return -1 != gd_chown(path, uid, gid) ? 0 : -errno;
//}
//int f_mknod(const char* path, fuse_mode_t mode, fuse_dev_t dev)
//{
//	return 0;
//}
//
//int f_setxattr(const char* path, const char* name, const char* value, size_t size, int flags)
//{
//	realpath(path);
//	return -1 != gd_setxattr(path, name, value, size, flags) ? 0 : -errno;
//}
//
//int f_getxattr(const char* path, const char* name, char* value, size_t size)
//{
//	realpath(path);
//	int nb;
//	return -1 != (nb = gd_getxattr(path, name, value, size)) ? nb : -errno;
//}
//
//int f_listxattr(const char* path, char* namebuf, size_t size)
//{
//	realpath(path);
//	int nb;
//	return -1 != (nb = gd_listxattr(path, namebuf, size)) ? nb : -errno;
//}
//
//int f_removexattr(const char* path, const char* name)
//{
//	realpath(path);
//	return -1 != gd_removexattr(path, name) ? 0 : -errno;
//}


/* supported fs operations */
static struct fuse_operations fs_ops = {
	.init = f_init,
	.getattr = f_getattr,
	.opendir = f_opendir,
	.readdir = f_readdir,
	.releasedir = f_releasedir,
	.readlink = f_readlink,
	.mkdir = f_mkdir,
	.unlink = f_unlink,
	.rmdir = f_rmdir,
	.rename = f_rename,
	.truncate = f_truncate,
	.utimens = f_utimens,
	.open = f_open,
	.flush = f_flush,
	.fsync = f_fsync,
	.release = f_release,
	.read = f_read,
	.write = f_write,
	.statfs = f_statfs,
	.create = f_create,
	//.mknod = f_mknod,
	//.access = f_access,
	//.symlink = f_symlink,
	//.link = f_link,
	//.chmod = f_chmod,
	//.chown = f_chown,
	//.setxattr = f_setxattr,
	//.getxattr = f_getxattr,
	//.listxattr = f_listxattr,
	//.removexattr = f_removexattr,
#if defined(FSP_FUSE_USE_STAT_EX)
	//.chflags = f_chflags,
#endif
};

enum {
	KEY_HELP,
	KEY_VERSION,
};

#define fs_OPT(t, p, v) { t, offsetof(GDCONFIG, p), v }

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
	fs_OPT("keeplink",          keeplink, 1),
	fs_OPT("fastrm",            fastrm, 1),
	fs_OPT("audit",             audit, 1),
	fs_OPT("block",             block, 1),
	fs_OPT("buffer=%u",         buffer, 0),
	fs_OPT("cipher=%s",         cipher, 0),

	FUSE_OPT_KEY("--version",      KEY_VERSION),
	FUSE_OPT_KEY("--help",         KEY_HELP),
	FUSE_OPT_END
};

static int fs_opt_proc(
	void *data, const char *arg, 
	int key, struct fuse_args *outargs)
{
	char exepath[MAX_PATH];
	char version[100];
	//char arch[3];
	//sprintf_s(arch, 3, "%d", sizeof(void*) * 8);

	switch (key) {
	case FUSE_OPT_KEY_NONOPT:
		if (!g_conf.drive) {
			g_conf.drive = strdup(arg);
			return 0;
		}
		if (!g_conf.remote) {
			g_conf.remote = strdup(arg);
			return 0;
		}
		fprintf(stderr, "golddrive: invalid argument '%s'\n", arg);
		return -1;
	//case FUSE_OPT_KEY_OPT:
	//	return 1;
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
			"    -h HOST, -o host=HOST      ssh server name or IP\n"
			"    -u USER, -o user=USER      user to connect to ssh server, default: current user\n"
			"    -k PKEY, -o pkey=PKEY      private key, default: %%USERPROFILE%%\\.ssh\\id_rsa\n"
			"    -p PORT, -o port=PORT      server port, default: 22\n"
			"    -o keeplink                hard links are not removed before overwriting data\n"
			"    -o fastrm                  use /bin/rm -rf command in host to delete folders\n"
			"    -o audit                   enable auditing by logging read and write events\n"
			"    -o block                   disable non-blocking protocol mode\n"
			"    -o cipher                  cipher for symetric encryption, comma-separated list\n"
			"    -o buffer=BYTES            read/write block size in bytes, default: 65535\n"
			"\n"
			"WinFsp-FUSE options:\n"
			"    -s                         disable multi-threaded operation\n"
			"    -d, -o debug               enable debug output\n"
			"    -o create_umask=MASK       file creation umask permissions\n"
			"    -o DebugLog=FILE           debug log file (requires -d)\n"
			"    -o FileInfoTimeout=N       metadata timeout (millis, -1 for data caching)\n"
			"    -o DirInfoTimeout=N        directory info timeout (millis)\n"
			"    -o VolumeInfoTimeout=N     volume info timeout (millis)\n"
			"    -o EaTimeout=N             extended attribute timeout (millis)\n"
			"    -o KeepFileCache           do not discard cache when files are closed\n"
			"    -o ThreadCount             number of file system dispatcher threads\n"
		);
		//fuse_opt_add_arg(outargs, "-h");
		//fuse_main(outargs->argc, outargs->argv, &fs_ops, NULL);
		exit(1);

	case KEY_VERSION:		
		GetModuleFileNameA(NULL, exepath, MAX_PATH);
		//char* version = calloc(100, sizeof(char));
		get_file_version(exepath, version);
		fprintf(stderr, "Golddrive %s %d-bit %s\n", 
			version, PLATFORM_BITS, __DATE__);
#ifdef USE_LIBSSH
		fprintf(stderr, "LibSSH %s\n", ssh_version(0));
#else
		fprintf(stderr, "Libssh2 %s\n", libssh2_version(0));
#endif
		fprintf(stderr, "%s\n", OPENSSL_VERSION_TEXT);
		fprintf(stderr, "FUSE %s\n", fuse_pkgversion());
		exit(0);
	}
	return 1;
}

static int parse_remote(GDCONFIG* fs)
{
	
	if (!fs->remote)
		return -1;

	char* npath, * locuser, * user, * host, * port, * p;

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
		gd_log("Invalid service name, "
			"only '\\\\golddrive' is supported: %s\n", 
			fs->remote);
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
	
	fs->root = strdup(p);	
	fs->has_root = 0;
	/* mount root by default, prepend a slash before path 
	 * if needed in remote linux file system
	 * not in windows */
	if (p && *p != '/') {
		char s[MAX_PATH];
		strcpy(s, "/");
		strcat(s, p);
		free(fs->root);
		fs->root = strdup(s);
		fs->has_root = strlen(fs->root) > 1;
	}

	free(npath);
	return 0;
}

static int load_config_file(GDCONFIG* fs)
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
	char jsonfile[MAX_PATH];
	sprintf_s(jsonfile, MAX_PATH, 
		"%s\\Golddrive\\config.json", appdata);
	fs->json = strdup(jsonfile);
	rc = load_json(fs);
	return rc;
		
}

static void init_logging(GDCONFIG* fs)
{
	g_logfile = fs->logfile;

	if (!g_logfile) {
		char* appdata = getenv("LOCALAPPDATA");
		char f[MAX_PATH];
		sprintf_s(f, MAX_PATH, "%s\\Golddrive", appdata);	
		g_logfile = malloc(MAX_PATH);
		sprintf_s(g_logfile, MAX_PATH, "%s\\golddrive.log", f);
		if (!directory_exists(f))
			_mkdir(f);		
	}
	// touch file
	FILE* log = fopen(g_logfile, "a");
	if (log != NULL) {
		//fprintf(log, "starting logging to file %s\n", g_logfile);
		fclose(log);
	}
	else {
		fprintf(stderr, "cannot initialize logging file %s\n", g_logfile);
		free(g_logfile);
		g_logfile = 0;
	}
}

int main(int argc, char *argv[])
{
	// load fuse driver
	if (FspLoad(0) != STATUS_SUCCESS) {
		fprintf(stderr,
			"failed to load winfsp driver, "
			"either dll not present or wrong version\n");
		return -1;
	}

	// init cache table
	g_cache_stat_ht = NULL;

	// parameters
	int rc;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	memset(&g_conf, 0, sizeof(g_conf));
	rc = fuse_opt_parse(&args, &g_conf, fs_opts, fs_opt_proc);
	if (rc || argc < 2 || 
		!g_conf.drive || strlen(g_conf.drive) < 2) {
		fprintf(stderr, "bad arguments, try --help\n");
		return 1;
	}

	// load arguments from config file
	load_config_file(&g_conf);

	// logging
	init_logging(&g_conf);

	// set arguments
	g_conf.drive[0] = toupper(g_conf.drive[0]);
	g_conf.letter = g_conf.drive[0];
	if (!g_conf.port)
		g_conf.port = 22;
	if (!g_conf.buffer)
		g_conf.buffer = BUFFER_SIZE;

	// parse network path
	if (parse_remote(&g_conf))
		return 1;
	
	// user in lower case
	if (!g_conf.user)
		g_conf.user = getenv("USERNAME");
	char* u = g_conf.user;
	for (; *u; ++u)
		*u = tolower(*u);

	// private key
	if (!g_conf.pkey) {
		char* profile = getenv("USERPROFILE");
		g_conf.pkey = malloc(MAX_PATH);
		sprintf_s(g_conf.pkey, MAX_PATH, 
			"%s\\.ssh\\id_golddrive_%s", 
			profile, g_conf.user);
		//sprintf_s(g_fs.pkey, MAX_PATH, "%s\\.ssh\\id_rsa", profile);
	}


	// show parameters
	gd_log("Arguments:\n");
	gd_log("drive    = %s\n", g_conf.drive);
	gd_log("remote   = %s\n", g_conf.remote);
	gd_log("mountp   = %s\n", g_conf.mountpoint);
	gd_log("user     = %s\n", g_conf.user);
	gd_log("host     = %s\n", g_conf.host);
	gd_log("port     = %d\n", g_conf.port);
	gd_log("root     = %s\n", g_conf.root);
	gd_log("pkey     = %s\n", g_conf.pkey);

	// winfsp arguments
	char volprefix[256], volname[256], prefix[256];
	strcpy(prefix, g_conf.remote);
	if (str_contains(g_conf.remote, ":"))
		str_replace(g_conf.remote, ":", "", prefix);
	sprintf_s(volprefix, sizeof(volprefix), 
		"-oVolumePrefix=%s", prefix);
	sprintf_s(volname, sizeof(volname), 
		"-ovolname=%s", g_conf.mountpoint);
	//gd_log("Prefix   = %s\n", volprefix);

	int pos = 1;
	fuse_opt_insert_arg(&args, pos++, volprefix);
	fuse_opt_insert_arg(&args, pos++, volname);
	fuse_opt_insert_arg(&args, pos++, 
		"-oFileSystemName=Golddrive");
	//fuse_opt_insert_arg(&args, pos++, 
	//	"-oFileInfoTimeout=5000,DirInfoTimeout=5000,VolumeInfoTimeout=5000");
	fuse_opt_insert_arg(&args, pos++, 
		"-orellinks,dothidden,uid=-1,gid=-1,umask=000,create_umask=000");
	
	// config file arguments
	if (g_conf.args && strcmp(g_conf.args, "") != 0) {
		fuse_opt_insert_arg(&args, pos++, g_conf.args);
		gd_log("args     = %s\n", g_conf.args);
	}
	
	// drive must be the last argument for winfsp
	rc = fuse_opt_parse(&args, &g_conf, fs_opts, fs_opt_proc);
	fuse_opt_add_arg(&args, g_conf.drive);

	// print arguments
	gd_log("buffer   = %u\n", g_conf.buffer);
	gd_log("keeplink = %u\n", g_conf.keeplink);
	gd_log("fastrm   = %u\n", g_conf.fastrm);
	gd_log("audit    = %u\n", g_conf.audit);
	gd_log("block    = %u\n", g_conf.block);
	if (g_conf.cipher)
		gd_log("cipher   = %s\n", g_conf.cipher);

	if (g_conf.usageurl) 
		gd_log("usage    = %s\n", g_conf.usageurl);

	gd_log("WinFsp arguments:\n");
	for (int i = 1; i < args.argc; i++)
		gd_log("arg %d    = %s\n", i, args.argv[i]);
	
	// check existance of private key before trying to ssh
	if (!file_exists(g_conf.pkey)) {
		gd_log("cannot read private key: %s\n", g_conf.pkey);
		return 1;
	}

	// initialize thread locks
	InitializeSRWLock(&g_ssh_lock);
	InitializeSRWLock(&g_log_lock);
	InitializeSRWLock(&g_cache_stat_lock);
	g_cache_calls = 0;
	g_sftp_calls = 0;

	// initialize ssh
	g_ssh = gd_init_ssh();
	if (!g_ssh)
		return 1;

	// usage
	HANDLE* uh = gd_usage("connected");

	// get uid
	char cmd[COMMAND_SIZE], out[COMMAND_SIZE], err[COMMAND_SIZE];
	//snprintf(cmd, sizeof(cmd), "hostname", g_fs.user);
	//rc = run_command_channel_exec(cmd, out, err);

	//snprintf(cmd, sizeof(cmd), "id -u %s", g_fs.user);
	snprintf(cmd, sizeof(cmd), "id -u %s", g_conf.user);

	// bencharmk commands
	//LARGE_INTEGER frequency, start, end;
	//double interval;
	//QueryPerformanceFrequency(&frequency);
	//QueryPerformanceCounter(&start);

	//// code to be measured
	//for (int u = 0; u < 100; u++) {
	//	memset(out, 0, COMMAND_SIZE);
	//	memset(err, 0, COMMAND_SIZE);
	//	rc = run_command_channel_exec(cmd, out, err);
	//	//printf("out: %s, err: %s\n", out, err);
	//}
	//QueryPerformanceCounter(&end);
	//interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
	//printf("\ncommand execution time: %f\n\n", interval);

	gd_lock();
	rc = run_command_channel_exec(cmd, out, err);
	gd_unlock();

	if (rc == 0) {
		g_conf.remote_uid = atoi(out);
		gd_log("uid      = %d\n", g_conf.remote_uid);
	}
	gd_lock();
	rc = run_command_channel_exec("echo $HOME", out, err);
	gd_unlock();
	if (rc == 0) {
		g_conf.home = malloc(sizeof out);
		strcpy_s(g_conf.home, sizeof out, out);
		gd_log("home     = %s\n", g_conf.home);
	}

	// number of threads
	//printf("Threads = %d\n", gd_threads(5, get_number_of_processors()));


	// mount
	rc = fuse_main(args.argc, args.argv, &fs_ops, NULL);
	
	// cleanup
	if (uh) {
		WaitForSingleObject(uh, 10000);
		CloseHandle(uh);
	}
	// this prouces disconnection delays
	//uh = gd_usage("disconnected");
	////if (uh) {
	//	WaitForSingleObject(uh, 10000);
	//	CloseHandle(uh);
	//}
	gd_finalize();
	return rc;
}
