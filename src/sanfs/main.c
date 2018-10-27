#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include "util.h"
#include "sanssh.h"
#include "cache.h"

/* global variables */
size_t				g_sftp_calls;
size_t				g_sftp_cached_calls;
SANSSH *			g_ssh_ht;
CRITICAL_SECTION	g_ssh_lock;
CACHE_ATTRIBUTES *	g_attributes_ht;
CRITICAL_SECTION	g_attributes_lock;
SAN_HANDLE *		g_handle_open_ht;
SAN_HANDLE *		g_handle_close_ht;
CRITICAL_SECTION	g_handle_lock;
CMD_ARGS *			g_cmd_args;

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

static void usage(const char* prog)
{
    fprintf(stderr, "usage: %s host port user drive [pkey]\n", prog);
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
		printf("sanfs 1.0.1\nbuild date: %s\n", __DATE__);
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
	if (!InitializeCriticalSectionAndSpinCount(&g_ssh_lock, 0x00000400)
		|| !InitializeCriticalSectionAndSpinCount(&g_attributes_lock, 0x00000400)
		|| !InitializeCriticalSectionAndSpinCount(&g_handle_lock, 0x00000400))
		return 1;
	g_ssh_ht = NULL;
	g_attributes_ht = NULL;
	g_handle_open_ht = NULL;
	g_handle_close_ht = NULL;

	g_sftp_calls = 0;
	g_sftp_cached_calls = 0;

	g_cmd_args = malloc(sizeof(CMD_ARGS));
	strcpy_s(g_cmd_args->host, 65, host);
	g_cmd_args->port = port;
	strcpy_s(g_cmd_args->user, 21, user);
	strcpy_s(g_cmd_args->pkey, MAX_PATH, pkey);


	//g_ssh = san_init(host, port, user, pkey);
	SANSSH* ssh = get_ssh();
	if (!ssh) {
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
		//"-oVolumePrefix=/sanfs/linux,uid=-1,gid=-1,rellinks",
		"-oVolumePrefix=/sanfs/linux,uid=-1,gid=-1,rellinks,FileInfoTimeout=1000,DirInfoTimeout=3000",
		//"-s",
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
	DeleteCriticalSection(&g_ssh_lock);
	DeleteCriticalSection(&g_attributes_lock);
	DeleteCriticalSection(&g_handle_lock);
	san_finalize();

	return rc;
}
