/*
 * Sanssh2 - SFTP client using libssh2
 * Author: SAG
 * Date: 10/09/2018
 *
 * Download a file from remote ssh server.
 *
 * usage: sanssh hostname port user /tmp/file.bin c:\temp\file.bin [private key]
 * private key defaults to %USERPROFILE%\.ssh\id_rsa
 */
#include <stdio.h>
#include <assert.h>

#define USE_LIBSSH

#include "sanssh.h"


int main(int argc, char *argv[])
{
	int start = time(NULL);

	char *hostname = "";
	int port = 22;
	char *username = "";
	char *password = "";
	char *remotefile = "";
	char *localfile = "";
	char pkey[MAX_PATH];
	int rc;
	char *errmsg;

	if (argc == 2 && strcmp(argv[1],"-V") == 0) {
		printf("sanssh 1.0\n");
		return 0;
	}
	
	if (argc < 6) {
		usage(argv[0]);
		return 1;
	}

	hostname = argv[1];
	port = atoi(argv[2]);
	username = argv[3];
	remotefile = argv[4];
	localfile = argv[5];

	// get public key
	char profile[BUFFER_SIZE];
	ExpandEnvironmentStringsA("%USERPROFILE%", profile, BUFFER_SIZE);
	strcpy_s(pkey, MAX_PATH, profile);
	strcat_s(pkey, MAX_PATH, "\\.ssh\\id_rsa");
	
	if (!file_exists(pkey)) {
		printf("error: cannot read private key: %s\n", pkey);
		exit(1);
	}
	printf("host       : %s\n", hostname);
	printf("port       : %d\n", port);
	printf("username   : %s\n", username);
	printf("private key: %s\n", pkey);
	
	char error[ERROR_LEN];
	printf("connecting...\n");
	SANSSH* sanssh = san_init(hostname, port, username, pkey, error);
	if (!sanssh) {
		fprintf(stderr, "Error initializing sanssh2: %s\n", error);
		return 1;
	}
	printf("starting test...\n");

	
	const char* path = "/tmp/sftp_folder";

	/* mkdir */
	san_mkdir(sanssh, "/tmp/sftp_folder");


	
	/* stat */
	SANSSH_ATTRIBUTES attrs;
	san_stat(sanssh, path, &attrs);
	print_stat(path, &attrs);


	/* lstat */
	//san_lstat(sftp_channel, path, &attrs);
	//print_stat(path, &attrs);

	/* statfvs */
	SANSSH_STATVFS st;
	san_statvfs(sanssh, path, &st);
	print_statvfs(path, &st);

	/* rmdir */
	san_rmdir(sanssh, "/tmp/sftp_folder");


	//char target[MAX_PATH];
	//san_realpath(sftp, path, target);
	//printf("path:     %s\n", path);
	//printf("realpath: %s\n", target);

	/* readdir */
	san_readdir(sanssh, "/tmp");



	// read in blocking mode
	san_write(sanssh, remotefile, localfile);


	// read in blocking mode
	san_read(sanssh, remotefile, localfile);

	// read in non-blocking mode
	san_read_async(sanssh, remotefile, localfile);


	/* run command */
	//const char * cmd = "ls -l /path 2>/dev/null";
	//char out[10000], eer[10000];
	//rc = run_command(sock, session, cmd, out, eer);
	//printf("cmd: %s\n", cmd);
	//printf("out: %s\n", out);
	//printf("err: %s\n", eer);
	//printf("rc : %d\n", rc);
	//
	
	//for(int i=0; i<100; i++)
	//	run_command(sock, session, cmd, out, eer);
	//
	
	printf("duration: %d secs.\n", time(NULL) - start);

	/* close */
	san_finalize(sanssh);
	
	return 0;
}
