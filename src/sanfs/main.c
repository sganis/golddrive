#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include "util.h"
#include "sanssh.h"
#include "cache.h"
#include "winposix.h"

/* global variables */
CACHE_ATTRIBUTES *	g_attributes_map;
size_t				g_sftp_calls;
size_t				g_sftp_cached_calls;
SANSSH *			g_sanssh_pool;
SAN_HANDLE *		g_handle_close_ht;
CRITICAL_SECTION	g_ssh_critical_section;
CMD_ARGS *			g_cmd_args;


static int fs_mkdir(const char *path, fuse_mode_t mode)
{
	debug("%s\n", path);
	return san_mkdir(path, mode);
}

static int fs_rmdir(const char *path)
{
	debug("%s\n", path);
	return san_rmdir(path);
}

static int fs_truncate(const char *path, fuse_off_t size, struct fuse_file_info *fi)
{
    if (0 == fi)  {
        return san_truncate(path, size);
    } else {
		int fd = fi_fd(fi);
        return san_ftruncate(fd, size);
    }
}

static int fs_read(const char *path, char *buf, size_t size, 
	fuse_off_t off, struct fuse_file_info *fi)
{
	//debug(path);
	//printf("thread req size  offset    path\n");
	//printf("%-7ld%-10ld%-10ld%s\n", GetCurrentThreadId(), size, off, path);

	int fd = fi_fd(fi);
	ssize_t nb = san_read(fd, buf, size, off);
	if (nb >= 0)
		return (int)nb;
	else
		return -1;
}

static int fs_write(const char *path, const char *buf, size_t size, 
	fuse_off_t off,  struct fuse_file_info *fi)
{
	debug("%s\n", path);
	return -1;

    //int fd = fi_fd(fi);
    //int nb;
    //return -1 != (nb = pwrite(fd, buf, size, off)) ? nb : -errno;
}


static int fs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	debug("%s\n", path);
	int fd = fi_fd(fi);
    return san_fsync(fd);
}

static int fs_utimens(const char *path, const struct fuse_timespec tv[2], struct fuse_file_info *fi)
{
	debug("%s\n", path);
	return -1;
    //return san_utimensat(AT_FDCWD, path, tv, AT_SYMLINK_NOFOLLOW);
}

static struct fuse_operations fs_ops = {
	.init		= san_init,
	.statfs		= san_statfs,
	.getattr	= san_getattr,
	.opendir	= san_opendir,
	.readdir	= san_readdir,
	.releasedir = san_releasedir,
	.unlink		= san_unlink,
	.rename		= san_rename,
	.create		= san_create,
	.open		= san_open,
    .read		= fs_read,
    .write		= fs_write,
    .release	= san_release,
	.mkdir		= fs_mkdir,
	.rmdir		= fs_rmdir,
	.truncate	= fs_truncate,
	.fsync		= fs_fsync,
    .utimens	= fs_utimens,
};

static void usage(const char* prog)
{
    fprintf(stderr, "usage: %s host user drive [pkey]\n", prog);
    exit(2);
}

char **new_argv(int count, ...)
{
	va_list args;
	int i;
	char **argv = malloc((count + 1) * sizeof(char*));
	char *temp;
	va_start(args, count);
	for (i = 0; i < count; i++) {
		temp = va_arg(args, char*);
		argv[i] = malloc(sizeof(temp));
		argv[i] = temp;
	}
	argv[i] = NULL;
	va_end(args);
	return argv;
}

int main(int argc, char *argv[])
{
	char *host = "";
	char *user = "";
	int port = 22;
	char drive[3];
	char pkey[MAX_PATH];
	int rc;

	if (argc == 2 && strcmp(argv[1], "-V") == 0) {
		printf("sanfs 1.0.0\n");
		return 0;
	}

	if (argc < 5) {
		usage(argv[0]);
		return 1;
	}

	host = argv[1];
	port = atoi(argv[2]);
	user = argv[3];
	strcpy_s(drive, 3, argv[4]);
	
	// get public key
	if (argc > 5) {
		strcpy_s(pkey, MAX_PATH, argv[5]);
	}
	else {
		char profile[BUFFER_SIZE];
		ExpandEnvironmentStringsA("%USERPROFILE%", profile, BUFFER_SIZE);
		strcpy_s(pkey, MAX_PATH, profile);
		strcat_s(pkey, MAX_PATH, "\\.ssh\\id_rsa");
	}
	if (!file_exists(pkey)) {
		printf("error: cannot read private key: %s\n", pkey);
		exit(1);
	}
	printf("host       : %s\n", host);
	printf("port       : %d\n", port);
	printf("user       : %s\n", user);
	printf("private key: %s\n", pkey);


	// Initialize global variables
	if (!InitializeCriticalSectionAndSpinCount(&g_ssh_critical_section, 0x00000400))
		return 1;
	g_attributes_map = NULL;
	g_sftp_calls = 0;
	g_sftp_cached_calls = 0;

	g_cmd_args = malloc(sizeof(CMD_ARGS));
	strcpy_s(g_cmd_args->host, 65, host);
	g_cmd_args->port = port;
	strcpy_s(g_cmd_args->user, 21, user);
	strcpy_s(g_cmd_args->pkey, MAX_PATH, pkey);


	//g_sanssh = san_init(host, port, user, pkey);
	SANSSH* sanssh = get_sanssh();
	if (!sanssh) {
		return 1;
	}


	// get uid
	char cmd[100], out[100], err[100];
	int uid=-1, gid=-1;
	snprintf(cmd,sizeof(cmd), "id -u %s\n", user);
	rc = run_command(cmd, out, err);
	if (rc == 0) {
		// trim newline
		//trim_str(out, strlen(out));
		out[strcspn(out, "\r\n")] = 0;
		uid = atoi(out);
		printf("uid=%d\n", uid);
	}
	// get gid
	snprintf(cmd, sizeof(cmd), "id -g %s\n", user);
	rc = run_command(cmd, out, err);
	if (rc == 0) {
		out[strcspn(out, "\r\n")] = 0;
		gid = atoi(out);
		printf("gid=%d\n", gid);
	}
		

	// fuse arguments, is this needed?
	//PTFS ptfs = { 0 };
	//char name[] = "";
	//ptfs.rootdir = malloc(strlen(name) + 1);
	//strcpy_s(ptfs.rootdir, 255, name);

	argc = 3;
	argv = new_argv(argc, argv[0], 
		"-oVolumePrefix=/sanfs/linux,uid=-1,gid=-1,rellinks",
		//"-oThreadCount=5",
		drive);
	//argv = new_argv(2, argv[0], "-h");


	// debug arguments
	for (int i = 0; i < argc; i++)
		printf("arg %d = %s\n", i, argv[i]);

	// number of threads
	printf("Threads needed: %d\n", san_threads(5, get_number_of_processors()));

	// load fuse dll
	if (FspLoad(0) != STATUS_SUCCESS) {
		fprintf(stderr, "failed to load winfsp driver, either dll not present or wrong version\n");
		return -1;
	}
	// run fuse main
    rc = fuse_main(argc, argv, &fs_ops, 0);

	// cleanup
	DeleteCriticalSection(&g_ssh_critical_section);
	san_finalize();

	return rc;
}
