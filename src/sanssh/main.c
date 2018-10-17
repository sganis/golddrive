/*
 * Sanssh2 - SFTP client using libssh2
 * Author: SAG
 * Date: 10/09/2018
 *
 * Download a file from remote ssh server.
 *
 * usage: sanssh hostname user /tmp/file.bin c:\temp\file.bin [private key]
 * private key defaults to %USERPROFILE%\.ssh\id_rsa
 */
#include <stdio.h>
#include <assert.h>
#include "sanssh.h"


int main(int argc, char *argv[])
{
	const char *hostname = "";
	const char *username = "";
	const char *password = "";
	const char *remotefile = "";
	const char *localfile = "";
	char pkey[MAX_PATH];
	int rc;
	char *errmsg;

	if (argc == 2 && strcmp(argv[1],"-V") == 0) {
		printf("sanssh 1.0.0\n");
		return 0;
	}
	
	if (argc < 5) {
		usage(argv[0]);
		return 1;
	}

	hostname = argv[1];
    username = argv[2];
	remotefile = argv[3];
	localfile = argv[4];

	// get public key
	if (argc > 5) {
		strcpy_s(pkey, MAX_PATH, argv[5]);
	} else {
		char profile[BUFFER_SIZE];
		ExpandEnvironmentStringsA("%USERPROFILE%", profile, BUFFER_SIZE);
		strcpy_s(pkey, MAX_PATH, profile);
		strcat_s(pkey, MAX_PATH, "\\.ssh\\id_rsa");
	}
	if (!file_exists(pkey)) {
		printf("error: cannot read private key: %s\n", pkey);
		exit(1);
	}
	printf("host       : %s\n", hostname);
	printf("username   : %s\n", username);
	printf("private key: %s\n", pkey);
	
	char error[ERROR_LEN];
	SANSSH* sanssh = san_init(hostname, username, pkey, error);
	if (!sanssh) {
		fprintf(stderr, "Error initializing sanssh2: %s\n", error);
		return 1;
	}
	// read in blocking mode
	san_read(sanssh, remotefile, localfile);
	
	// read in non-blocking mode
	//san_read_async(sanssh, remotefile, localfile);

	
	//const char* path = "/tmp/sftp_folder";

	/* mkdir */
	//san_mkdir(sftp, "/tmp/sftp_folder");

	/* rmdir */
	//san_rmdir(sftp, "/tmp/sftp_folder");

	//LIBSSH2_SFTP_ATTRIBUTES attrs;
	/* stat */
	//san_stat(sftp, path, &attrs);
	//print_stat(path, &attrs);

	/* lstat */
	//san_lstat(sftp_channel, path, &attrs);
	//print_stat(path, &attrs);

	/* statfvs */
	//LIBSSH2_SFTP_STATVFS st;
	//san_statvfs(sftp, path, &st);
	//print_statvfs(path, &st);

	//char target[MAX_PATH];
	//san_realpath(sftp, path, target);
	//printf("path:     %s\n", path);
	//printf("realpath: %s\n", target);

	/* readdir */
	//san_readdir(sftp, "/tmp");


	/* run command */
	//const char * cmd = "ls -l /path 2>/dev/null";
	//char out[10000], eer[10000];
	//rc = run_command(sock, session, cmd, out, eer);
	//printf("cmd: %s\n", cmd);
	//printf("out: %s\n", out);
	//printf("err: %s\n", eer);
	//printf("rc : %d\n", rc);
	//
	//int start = time(NULL);

	//for(int i=0; i<100; i++)
	//	run_command(sock, session, cmd, out, eer);
	//
	//printf("duration: %d secs.\n", time(NULL) - start);

	/* close */
	san_finalize(sanssh);
	
	return 0;
}
