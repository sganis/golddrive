// src/cli/main.c
#include "util.h"
#include "gd.h"
#include "cache.h"
#include "parse.h"
#include "pool.h"
#include <direct.h>
#include <openssl/opensslv.h>

/* global variables (g_ssh is thread-local, defined in pool.c) */
size_t				g_sftp_calls;
size_t				g_cache_calls;
CACHE_INODE*		g_cache_inode_ht;
SRWLOCK				g_log_lock;
SRWLOCK				g_cache_inode_lock;
SRWLOCK				g_cache_stat_lock;
GDCONFIG			g_conf;
char*				g_logfile;
char*				g_logurl;

HANDLE g_keepalive_thread = NULL;
HANDLE g_keepalive_stop_event = NULL;

DWORD WINAPI gd_keepalive_thread(LPVOID param)
{
    HANDLE stop_event = (HANDLE)param;
    int seconds_to_next;
    DWORD wait_result;

    /* wait 5 seconds for mount to complete */
    wait_result = WaitForSingleObject(stop_event, 5000);
    if (wait_result == WAIT_OBJECT_0) {
        return 0;
    }

    while (1) {
        wait_result = WaitForSingleObject(stop_event, 30000);

        if (wait_result == WAIT_OBJECT_0) {
            break;
        }

        for (int i = 0; i < g_pool.size; i++) {
            GDSSH* c = g_pool.conn[i];
            if (c && c->ssh) {
                gd_lock_conn(c);
                libssh2_keepalive_send(c->ssh, &seconds_to_next);
                gd_unlock();
            }
        }
    }

    return 0;
}

/* Check if an error indicates a dead SSH connection that may be recoverable */
static int is_connection_error(int err)
{
	return err == -EIO || err == -ECONNRESET || err == -ENOTCONN;
}

/* Try to reconnect and retry an operation once.
 * Returns the original error if reconnection fails. */
#define RETRY_ON_DISCONNECT(call) do {                     \
	int _rc = (call);                                      \
	if (_rc != 0 && is_connection_error(_rc)) {            \
		GDSSH* _c = g_ssh;  /* connection the failed op used */ \
		gd_log("Connection error (%d), attempting reconnect\n", _rc); \
		gd_lock_conn(_c);                                  \
		if (gd_reconnect(_c) == 0) {                        \
			gd_unlock();                                   \
			_rc = (call);                                  \
		} else {                                           \
			gd_unlock();                                   \
		}                                                  \
	}                                                      \
	return _rc;                                            \
} while(0)

static void* f_init(struct fuse_conn_info* conn, struct fuse_config* conf)
{
	(void)conf;
#if defined(FUSE_CAP_READDIRPLUS)
	conn->want |= (conn->capable & FUSE_CAP_READDIRPLUS);
#endif
#if defined(FSP_FUSE_USE_STAT_EX) && defined(FSP_FUSE_CAP_STAT_EX)
	conn->want |= (conn->capable & FSP_FUSE_CAP_STAT_EX);
#endif
	return fuse_get_context()->private_data;
}

static int f_statfs_impl(const char* path, struct fuse_statvfs* stbuf)
{
	realpath(path);
	return -1 != gd_statvfs(path, stbuf) ? 0 : -errno;
}
static int f_statfs(const char* path, struct fuse_statvfs* stbuf)
{
	RETRY_ON_DISCONNECT(f_statfs_impl(path, stbuf));
}

static int f_getattr_impl(const char* path, struct fuse_stat* stbuf,
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
	return rc;
}
static int f_getattr(const char* path, struct fuse_stat* stbuf,
	struct fuse_file_info* fi)
{
	RETRY_ON_DISCONNECT(f_getattr_impl(path, stbuf, fi));
}

static int f_readlink(const char* path, char* buf, size_t size)
{
	realpath(path);
	int rc = -1 != gd_readlink(path, buf, size) ? 0 : -errno;
	return rc;
}

static int f_unlink(const char* path)
{
	realpath(path);
	int rc = -1 != gd_unlink(path) ? 0 : -errno;
	return rc;
}

static int f_create(const char* path, fuse_mode_t mode,
	struct fuse_file_info* fi)
{
	realpath(path);
	intptr_t fd;
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
	realpath(path);
	intptr_t fd;
	int rc = -1 != (fd = gd_open(path, fi->flags, 0)) ?
		(fi_setfd(fi, fd), 0) : -errno;
	return rc;
}

static int f_read(const char* path, char* buf, size_t size,
	fuse_off_t off, struct fuse_file_info* fi)
{
	(void)path;
	intptr_t fd = fi_fd(fi);
	int nb;
	int rc = -1 != (nb = gd_read(fd, buf, size, off)) ? nb : -errno;
	if (rc != 0 && is_connection_error(rc)) {
		gd_log("Read connection error (%d), attempting reconnect\n", rc);
		GDSSH* c = g_ssh;
		gd_lock_conn(c);
		gd_reconnect(c);
		gd_unlock();
	}
	return rc;
}

static int f_write(const char* path, const char* buf,
	size_t size, fuse_off_t off, struct fuse_file_info* fi)
{
	(void)path;
	intptr_t fd = fi_fd(fi);
	int nb;
	int rc = -1 != (nb = gd_write(fd, buf, size, off)) ? nb : -errno;
	if (rc != 0 && is_connection_error(rc)) {
		gd_log("Write connection error (%d), attempting reconnect\n", rc);
		GDSSH* c = g_ssh;
		gd_lock_conn(c);
		gd_reconnect(c);
		gd_unlock();
	}
	return rc;
}

static int f_release(const char* path, struct fuse_file_info* fi)
{
	(void)path;
	intptr_t fd = fi_fd(fi);
	return gd_close(fd);
}

static int f_rename(const char* oldpath, const char* newpath,
	unsigned int flags)
{
	(void)flags;
	realpath(newpath);
	realpath(oldpath);
	int rc = -1 != gd_rename(oldpath, newpath) ? 0 : -errno;
	return rc;
}

static int f_opendir_impl(const char* path, struct fuse_file_info* fi)
{
	realpath(path);
	GDDIR* dirp;
	return 0 != (dirp = gd_opendir(path)) ?
		(fi_setdirp(fi, dirp), 0) :
		-errno;
}
static int f_opendir(const char* path, struct fuse_file_info* fi)
{
	RETRY_ON_DISCONNECT(f_opendir_impl(path, fi));
}

static int f_readdir(const char* path, void* buf,
	fuse_fill_dir_t filler, fuse_off_t off,
	struct fuse_file_info* fi, enum fuse_readdir_flags flags)
{
	(void)path; (void)off; (void)flags;
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
	(void)path;
	GDDIR* dirp = fi_dirp(fi);
	return gd_closedir(dirp);
}

static int f_mkdir(const char* path, fuse_mode_t  mode)
{
	realpath(path);
	int rc = -1 != gd_mkdir(path, mode) ? 0 : -errno;
	return rc;
}

static int f_rmdir(const char* path)
{
	realpath(path);
	int rc = -1 != gd_rmdir(path) ? 0 : -errno;
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
	(void)path; (void)datasync;
	intptr_t fd = fi_fd(fi);
	return -1 != gd_fsync(fd) ? 0 : -errno;
}

static int f_flush(const char* path, struct fuse_file_info* fi)
{
	(void)path;
	intptr_t fd = fi_fd(fi);
	return -1 != gd_flush(fd) ? 0 : -errno;
}

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
#if defined(FSP_FUSE_USE_STAT_EX)
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
	fs_OPT("audit",             audit, 1),
	fs_OPT("buffer=%u",         buffer, 0),
	fs_OPT("connections=%d",    connections, 0),
	fs_OPT("cipher=%s",         cipher, 0),

	FUSE_OPT_KEY("--version",      KEY_VERSION),
	FUSE_OPT_KEY("--help",         KEY_HELP),
	FUSE_OPT_END
};

static int fs_opt_proc(
	void *data, const char *arg,
	int key, struct fuse_args *outargs)
{
	(void)data; (void)outargs;
	char exepath[MAX_PATH];
	char version[100];

	switch (key) {
	case FUSE_OPT_KEY_NONOPT:
		if (!g_conf.drive) {
			g_conf.drive = strdup(arg);
			if (!g_conf.drive) return -1;
			return 0;
		}
		if (!g_conf.remote) {
			g_conf.remote = strdup(arg);
			if (!g_conf.remote) return -1;
			return 0;
		}
		fprintf(stderr, "golddrive: invalid argument '%s'\n", arg);
		return -1;
	case KEY_HELP:
		fprintf(stderr,
			"\n"
			"Usage: golddrive drive [remote] [options]\n"
			"\n"
			"drive : letter and colon (like Z:)\n"
			"remote: remote network path like \\\\golddrive\\[[locuser=]user@]host[!port][\\path]\n"
			"Options:\n"
			"    -o opt1,[opt2,...]         mount options\n"
			"    --help                     show this help\n"
			"    --version                  show version\n"
			"    -h HOST, -o host=HOST      ssh server name or IP\n"
			"    -u USER, -o user=USER      user to connect to ssh server, default: current user\n"
			"    -k PKEY, -o pkey=PKEY      private key, default: %%USERPROFILE%%\\.ssh\\id_rsa\n"
			"    -p PORT, -o port=PORT      server port, default: 22\n"
			"    -o keeplink                hard links are not removed before overwriting data\n"
			"    -o audit                   enable auditing by logging read and write events\n"
			"    -o cipher                  cipher for symmetric encryption, comma-separated list\n"
			"    -o buffer=BYTES            read/write block size in bytes, default: 65535\n"
			"    -o connections=N           SSH connection-pool size (1-16), default: 4\n"
			"    -o create_umask=MASK       file creation umask permissions\n"
			"    -o DebugLog=FILE           debug log file (requires -d)\n"
			"    -o FileInfoTimeout=N       metadata timeout (millis, -1 for data caching)\n"
			"    -o DirInfoTimeout=N        directory info timeout (millis)\n"
			"    -o VolumeInfoTimeout=N     volume info timeout (millis)\n"
			"    -o EaTimeout=N             extended attribute timeout (millis)\n"
			"    -o KeepFileCache           do not discard cache when files are closed\n"
			"    -o ThreadCount             number of file system dispatcher threads\n"
			"    -s                         disable multi-threaded operation\n"
			"    -d, -o debug               enable debug output\n"
		);
		exit(1);

	case KEY_VERSION:
		GetModuleFileNameA(NULL, exepath, MAX_PATH);
		get_file_version(exepath, version);
		fprintf(stderr, "Golddrive %s %d-bit %s\n",
			version, PLATFORM_BITS, __DATE__);
		fprintf(stderr, "Libssh2 %s\n", libssh2_version(0));
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

	/* translate backslash to forward slash (in place; VolumePrefix reads this) */
	for (char* p = fs->remote; *p; p++)
		if ('\\' == *p)
			*p = '/';

	/* remove the first slash when the remote starts with "//" */
	size_t len = strlen(fs->remote);
	if (len > 2 && fs->remote[0] == '/' && fs->remote[1] == '/')
		memmove(fs->remote, fs->remote + 1, len);

	/* field parsing lives in the unit-tested pure helper (src/cli/parse.c) */
	gd_remote r;
	if (parse_remote_str(fs->remote, &r) != 0)
		return -1;

	fs->service = strdup(r.service);
	fs->mountpoint = strdup(r.mountpoint);
	fs->host = strdup(r.host);
	fs->root = strdup(r.root);
	if (!fs->service || !fs->mountpoint || !fs->host || !fs->root)
		return -1;
	if (r.has_locuser) {
		fs->locuser = strdup(r.locuser);
		if (!fs->locuser) return -1;
	}
	if (r.has_user) {
		fs->user = strdup(r.user);
		if (!fs->user) return -1;
	}
	if (r.has_port)
		fs->port = r.port;
	fs->has_root = r.has_root;
	return 0;
}

static int load_config_file(GDCONFIG* fs)
{
	int rc = 0;
	char* appdata = getenv("LOCALAPPDATA");
	if (!appdata) {
		fprintf(stderr, "LOCALAPPDATA environment variable not set\n");
		return 1;
	}
	char jsonfile[MAX_PATH];
	sprintf_s(jsonfile, MAX_PATH,
		"%s\\Golddrive\\config.json", appdata);
	fs->json = strdup(jsonfile);
	if (!fs->json) return 1;
	rc = load_json(fs);
	return rc;
}

static void init_logging(GDCONFIG* fs)
{
	g_logfile = fs->logfile;

	if (!g_logfile) {
		char* appdata = getenv("LOCALAPPDATA");
		if (!appdata) {
			fprintf(stderr, "LOCALAPPDATA not set, logging disabled\n");
			return;
		}
		char f[MAX_PATH];
		sprintf_s(f, MAX_PATH, "%s\\Golddrive", appdata);
		g_logfile = malloc(MAX_PATH);
		if (!g_logfile) {
			fprintf(stderr, "out of memory for log file path\n");
			return;
		}
		sprintf_s(g_logfile, MAX_PATH, "%s\\golddrive.log", f);
		if (!directory_exists(f))
			_mkdir(f);
	}
	/* touch file */
	FILE* log = fopen(g_logfile, "a");
	if (log != NULL) {
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
	/* load fuse driver */
	if (FspLoad(0) != STATUS_SUCCESS) {
		fprintf(stderr,
			"failed to load winfsp driver, "
			"either dll not present or wrong version\n");
		return -1;
	}

	/* init cache table */
	g_cache_inode_ht = NULL;

	/* parameters */
	int rc;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	memset(&g_conf, 0, sizeof(g_conf));
	rc = fuse_opt_parse(&args, &g_conf, fs_opts, fs_opt_proc);
	if (rc || argc < 2 ||
		!g_conf.drive || strlen(g_conf.drive) < 2) {
		fprintf(stderr, "bad arguments, try --help\n");
		return 1;
	}

	/* load arguments from config file */
	load_config_file(&g_conf);

	/* logging */
	init_logging(&g_conf);

	/* set arguments */
	g_conf.drive[0] = (char)toupper(g_conf.drive[0]);
	g_conf.letter = g_conf.drive[0];
	if (!g_conf.port)
		g_conf.port = 22;
	if (!g_conf.buffer)
		g_conf.buffer = BUFFER_SIZE;
	if (!g_conf.connections)
		g_conf.connections = 4;

	/* parse network path */
	if (parse_remote(&g_conf))
		return 1;

	/* user in lower case */
	if (!g_conf.user)
		g_conf.user = getenv("USERNAME");
	if (!g_conf.user) {
		fprintf(stderr, "error: USERNAME environment variable not set\n");
		return 1;
	}
	char* u = g_conf.user;
	for (; *u; ++u)
		*u = (char)tolower(*u);

	/* private key */
	if (!g_conf.pkey || strlen(g_conf.pkey) == 0) {
		char* profile = getenv("USERPROFILE");
		if (!profile) {
			fprintf(stderr, "USERPROFILE environment variable not set\n");
			return 1;
		}
		g_conf.pkey = malloc(MAX_PATH);
		if (!g_conf.pkey) {
			fprintf(stderr, "out of memory\n");
			return 1;
		}
		sprintf_s(g_conf.pkey, MAX_PATH, "%s\\.ssh\\id_rsa", profile);
	}

	/* show parameters */
	gd_log("Arguments:\n");
	gd_log("drive    = %s\n", g_conf.drive);
	gd_log("remote   = %s\n", g_conf.remote);
	gd_log("mountp   = %s\n", g_conf.mountpoint);
	gd_log("user     = %s\n", g_conf.user);
	gd_log("host     = %s\n", g_conf.host);
	gd_log("port     = %d\n", g_conf.port);
	gd_log("root     = %s\n", g_conf.root);
	gd_log("pkey     = %s\n", g_conf.pkey);

	/* winfsp arguments */
	char volprefix[256], volname[256], prefix[256];
	strcpy_s(prefix, sizeof(prefix), g_conf.remote);
	if (str_contains(g_conf.remote, ":"))
		str_replace(g_conf.remote, ":", "", prefix, sizeof(prefix));
	sprintf_s(volprefix, sizeof(volprefix),	"-oVolumePrefix=%s", prefix);
	sprintf_s(volname, sizeof(volname), "-ovolname=%s", g_conf.mountpoint);
	gd_log("Prefix   = %s\n", volprefix);

	int pos = 1;
	fuse_opt_insert_arg(&args, pos++, volprefix);
	fuse_opt_insert_arg(&args, pos++, volname);
	fuse_opt_insert_arg(&args, pos++, "-oFileSystemName=Golddrive");
	fuse_opt_insert_arg(&args, pos++, "-oFileInfoTimeout=10000");
	fuse_opt_insert_arg(&args, pos++, "-oDirInfoTimeout=10000");
	fuse_opt_insert_arg(&args, pos++, "-oVolumeInfoTimeout=20000");
	fuse_opt_insert_arg(&args, pos++, "-orellinks");
	fuse_opt_insert_arg(&args, pos++, "-odothidden");
	fuse_opt_insert_arg(&args, pos++, "-ouid=-1,gid=-1");
	fuse_opt_insert_arg(&args, pos++, "-oumask=000,create_umask=000");

	/* config file arguments */
	if (g_conf.args && strcmp(g_conf.args, "") != 0) {
		fuse_opt_insert_arg(&args, pos++, g_conf.args);
		gd_log("args     = %s\n", g_conf.args);
	}

	/* drive must be the last argument for winfsp */
	fuse_opt_add_arg(&args, g_conf.drive);

	/* print arguments */
	gd_log("buffer   = %u\n", g_conf.buffer);
	gd_log("keeplink = %u\n", g_conf.keeplink);
	gd_log("audit    = %u\n", g_conf.audit);
	if (g_conf.cipher)
		gd_log("cipher   = %s\n", g_conf.cipher);
	if (g_conf.usageurl)
		gd_log("usage    = %s\n", g_conf.usageurl);

	gd_log("Arguments:\n");
	for (int i = 1; i < args.argc; i++)
		gd_log("arg %d    = %s\n", i, args.argv[i]);

	/* check existence of private key before trying to ssh */
	if (!file_exists(g_conf.pkey)) {
		gd_log("cannot read private key: %s\n", g_conf.pkey);
		return 1;
	}

	/* initialize thread locks */
	InitializeSRWLock(&g_log_lock);
	InitializeSRWLock(&g_cache_inode_lock);
	InitializeSRWLock(&g_cache_stat_lock);
	g_cache_calls = 0;
	g_sftp_calls = 0;

	/* one-time Winsock/libssh2 init, then build the SSH connection pool */
	if (gd_global_init() != 0)
		return 1;
	if (gd_pool_init(g_conf.connections) < 1) {
		gd_log("failed to establish any SSH connection\n");
		return 1;
	}
	gd_log("connections = %d\n", g_pool.size);

	/* keepalive event */
	g_keepalive_stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_keepalive_stop_event) {
		g_keepalive_thread = CreateThread(NULL, 0, gd_keepalive_thread,
										g_keepalive_stop_event, 0, NULL);
		if (!g_keepalive_thread) {
			gd_log("Warning: Failed to create keepalive thread\n");
			CloseHandle(g_keepalive_stop_event);
			g_keepalive_stop_event = NULL;
		}
	} else {
		gd_log("Warning: Failed to create keepalive stop event\n");
	}

	/* usage */
	HANDLE* uh = gd_usage("CONNECTED", "");

	/* get uid */
	char cmd[COMMAND_SIZE], out[COMMAND_SIZE], err[COMMAND_SIZE];
	snprintf(cmd, sizeof(cmd), "id -u %s", g_conf.user);

	gd_lock();
	rc = run_command_channel_exec(cmd, out, err);
	gd_unlock();

	if (rc == 0) {
		/* get last line, ignore warnings */
		size_t outlen = strlen(out);
		if (outlen > 0)
			out[outlen - 1] = '\0';
		g_conf.remote_uid = atoi(out);
		if (g_conf.remote_uid == 0 && strchr(out, '\n') != NULL) {
			int i, lastnl = -1;
			for (i = 0; i <= (int)outlen; i++)
				if (out[i] == '\n')
					lastnl = i;
			if (lastnl > -1)
				g_conf.remote_uid = atoi(out + lastnl);
		}
		gd_log("uid      = %d\n", g_conf.remote_uid);
	}
	gd_lock();
	rc = run_command_channel_exec("echo $HOME", out, err);
	gd_unlock();
	if (rc == 0) {
		g_conf.home = malloc(sizeof out);
		if (!g_conf.home) {
			gd_log("out of memory for home path\n");
			return 1;
		}
		strcpy_s(g_conf.home, sizeof out, out);
		gd_log("home     = %s\n", g_conf.home);
	}

	/* mount */
	rc = fuse_main(args.argc, args.argv, &fs_ops, NULL);

	/* cleanup keepalive */
	if (g_keepalive_thread) {
		if (g_keepalive_stop_event) {
			SetEvent(g_keepalive_stop_event);
		}
		WaitForSingleObject(g_keepalive_thread, 1000);
		CloseHandle(g_keepalive_thread);
		g_keepalive_thread = NULL;
		if (g_keepalive_stop_event) {
			CloseHandle(g_keepalive_stop_event);
			g_keepalive_stop_event = NULL;
		}
	}

	/* cleanup */
	if (uh) {
		WaitForSingleObject(uh, 10000);
		CloseHandle(uh);
	}

	return gd_finalize(rc);
}
