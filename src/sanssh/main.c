/*
 * Sanssh2 - SFTP client using libssh2
 * Author: SAG
 * Date: 10/09/2018
 *
 * Download a file from remote ssh server.
 *
 * usage: sanssh hostname port user pass /tmp/file.bin c:\temp\file.bin [private key]
 * private key defaults to %USERPROFILE%\.ssh\id_rsa
 */
#include <stdio.h>
#include <assert.h>
#include "sanssh.h"

int main(int argc, char *argv[])
{
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
	password = argv[4];
	remotefile = argv[5];
	localfile = argv[6];

	// get public key
	char profile[MAX_PATH];
	ExpandEnvironmentStringsA("%USERPROFILE%", profile, MAX_PATH);
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
		fprintf(stderr, "Error initializing sanssh: %s\n", error);
		return 1;
	}

	printf("starting test...\n");
	int start = time(NULL);
	
	const char* path = "/tmp/folder";

	/* mkdir */
	san_mkdir(sanssh, path);
	
	/* stat */
	SANSTAT attrs;
	san_stat(sanssh, path, &attrs);
	print_stat(path, &attrs);


	/* statfvs */
	SANSTATVFS st;
	san_statvfs(sanssh, path, &st);
	print_statvfs(path, &st);

	/* rmdir */
	san_rmdir(sanssh, "/tmp/folder");

	/* readdir */
	san_readdir(sanssh, "/tmp");

	// read in blocking mode
	start = time(NULL);
	san_write(sanssh, remotefile, localfile);
	printf("upload: %d secs.\n", time(NULL) - start);

	// read in blocking mode
	char newlocal[1000];
	strcpy(newlocal, localfile);
	strcat(newlocal, ".downloaded");
	start = time(NULL);
	san_read(sanssh, remotefile, newlocal);
	printf("download: %d secs.\n", time(NULL) - start);
	// read in non-blocking mode
	//san_read_async(sanssh, remotefile, localfile);


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
	
	printf("total test time: %d secs.\n", time(NULL) - start);

	/* close */
	san_finalize(sanssh);
	
	return 0;
}
