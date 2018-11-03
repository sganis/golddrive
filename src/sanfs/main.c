#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include "util.h"
#include "sanfs.h"
#include "cache.h"

#define VERSION "1.1.0"

/* global variables */
size_t				g_sftp_calls;
size_t				g_sftp_cached_calls;
SANSSH *			g_ssh;
CACHE_ATTRIBUTES *	g_attributes_ht;
CRITICAL_SECTION	g_attributes_lock;
SAN_HANDLE *		g_handle_open_ht;
SAN_HANDLE *		g_handle_close_ht;
SRWLOCK				g_ssh_lock;

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


struct sanfs_config {
	char *host;
	int port;
	char *user;
	char *pkey;
	char *drive;
	//int uid;
	//int gid;
	//char *volumeprefix;
};

static struct sanfs_config sanfs;

enum {
	KEY_HELP,
	KEY_VERSION,
};

#define SANFS_OPT(t, p, v) { t, offsetof(struct sanfs_config, p), v }

static struct fuse_opt sanfs_opts[] = {
	SANFS_OPT("port=%i",           port, 0),
	SANFS_OPT("user=%s",           user, 0),
	SANFS_OPT("pkey=%s",           pkey, 0),
	//SANFS_OPT("uid=%d",            uid, 0),
	//SANFS_OPT("gid=%d",            gid, 0),
	//SANFS_OPT("VolumePrefix=%s",   volumeprefix, 0),

	FUSE_OPT_KEY("-V",             KEY_VERSION),
	FUSE_OPT_KEY("--version",      KEY_VERSION),
	FUSE_OPT_KEY("-h",             KEY_HELP),
	FUSE_OPT_KEY("--help",         KEY_HELP),
	FUSE_OPT_END
};

static int sanfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	switch (key) {
	case FUSE_OPT_KEY_NONOPT:
		if (!sanfs.host) {
			sanfs.host = strdup(arg);
			return 0;
		}
		if (!sanfs.drive) {
			sanfs.drive = strdup(arg);
			return 0;
		}
		fprintf(stderr, "sanfs: invalid argument '%s'\n", arg);
		return -1;
	case KEY_HELP:
		fprintf(stderr,
			"\n"
			"Usage: sanfs.exe [options] host drive\n"
			"\n"
			"Options:\n"
			"    -h, --help                 print this help\n"
			"    -V, --version              print version\n"
			"    -o opt,[opt...]            mount options\n"
			"    -o user=USER               user to connect to ssh server, default: current user\n"
			"    -o port=PORT               server port, default: 22\n"
			"    -o pkey=PKEY               private key, default: %%USERPROFILE%%\\.ssh\\id_rsa\n"
			"\n"
			"WinFsp-FUSE options:\n"
			"    -d, -o debug               enable debug output\n"
			"    -s                         disable multi-threaded operation\n"
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

int main(int argc, char *argv[])
{
	// load fuse dll
	if (FspLoad(0) != STATUS_SUCCESS) {
		fprintf(stderr, "failed to load winfsp driver, either dll not present or wrong version\n");
		return -1;
	}

	int rc;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	memset(&sanfs, 0, sizeof(sanfs));
	sanfs.port = 22;
	sanfs.user = getenv("USERNAME");
	fuse_opt_parse(&args, &sanfs, sanfs_opts, sanfs_opt_proc);

	if (argc < 3) {
		fuse_opt_add_arg(&args, "-h");
		fuse_opt_parse(&args, &sanfs, sanfs_opts, sanfs_opt_proc);
		return 1;
	}
	
	// get public key
	if (!sanfs.pkey) {
		char profile[BUFFER_SIZE];
		ExpandEnvironmentStringsA("%USERPROFILE%", profile, BUFFER_SIZE);
		sanfs.pkey = malloc(MAX_PATH);
		strcpy_s(sanfs.pkey, MAX_PATH, profile);
		strcat_s(sanfs.pkey, MAX_PATH, "\\.ssh\\id_rsa");
	}
	if (!file_exists(sanfs.pkey)) {
		fprintf(stderr, "error: cannot read private key: %s\n", sanfs.pkey);
		return 1;
	}

	
	fuse_opt_parse(&args, &sanfs, sanfs_opts, sanfs_opt_proc);

	printf("host  = %s\n", sanfs.host);
	printf("drive = %s\n", sanfs.drive);
	printf("port  = %d\n", sanfs.port);
	printf("user  = %s\n", sanfs.user);
	printf("pkey  = %s\n", sanfs.pkey);

	// initiaize small read/write lock
	InitializeSRWLock(&g_ssh_lock);

	g_sftp_calls = 0;
	g_sftp_cached_calls = 0;

	g_ssh = san_init_ssh(sanfs.host, sanfs.port, sanfs.user, sanfs.pkey);
	if (!g_ssh) 
		return 1;

	// get uid
	char cmd[100], out[100], err[100];
	int uid=-1, gid=-1;
	snprintf(cmd,sizeof(cmd), "id -u %s\n", sanfs.user);
	rc = run_command(cmd, out, err);
	if (rc == 0) {
		out[strcspn(out, "\r\n")] = 0;
		uid = atoi(out);
		printf("uid=%d\n", uid);
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
	printf("Threads needed: %d\n", san_threads(5, get_number_of_processors()));

	// run fuse main
	char volprefix[256], volname[256];
	sprintf_s(volprefix, sizeof(volprefix), "-oVolumePrefix=/sanfs/%s@%s", sanfs.user, sanfs.host);
	sprintf_s(volname, sizeof(volname), "-ovolname=%s@%s", sanfs.user, sanfs.host);
	fuse_opt_add_arg(&args, volprefix);
	fuse_opt_add_arg(&args, volname);
	//fuse_opt_add_arg(&args, "-ouid=-1,gid=-1,create_umask=007,mask=007");
	fuse_opt_add_arg(&args, "-ouid=-1,gid=-1,create_umask=002");
	fuse_opt_add_arg(&args, "-oFileSystemName=SANFS");
	fuse_opt_add_arg(&args, "-orellinks,FileInfoTimeout=3000,DirInfoTimeout=3000");
	fuse_opt_parse(&args, &sanfs, sanfs_opts, sanfs_opt_proc);
	fuse_opt_add_arg(&args, sanfs.drive);
	// debug arguments
	for (int i = 1; i < args.argc; i++)
		printf("arg %d = %s\n", i, args.argv[i]);

	rc = fuse_main(args.argc, args.argv, &sanfs_ops, NULL);

	// cleanup
	InitializeSRWLock(&g_ssh_lock);
	san_finalize();
	return rc;
}
