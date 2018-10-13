/*
 * Sanssh2 - SFTP client using libssh2
 * Author: SAG
 * Date: 10/09/2018
 *
 * Download a file from remote ssh server.
 *
 * usage: sanssh hostname user /tmp/file [private key]
 * private key defaults to %USERPROFILE%\.ssh\id_rsa
 */
#include "sanssh2.h"
#include <stdio.h>
#include <assert.h>


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
	
	SOCKET sock;
	SOCKADDR_IN sin;
	LIBSSH2_SESSION *session;
	LIBSSH2_SFTP *sftp;
	

	if (argc == 2 && strcmp(argv[1],"-V") == 0) {
		printf("sanssh2 1.0.2\n");
		return 0;
	}
	
	if (argc < 5) {
		usage(argv[0]);
		return 1;
	}

	// initialize windows socket
	WSADATA wsadata;
	int err;
	err = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (err != 0) {
		fprintf(stderr, "WSAStartup failed with error: %d\n", err);
		return 1;
	}
	
	// resolve hostname
	hostname = argv[1];
	HOSTENT *he;
	he = gethostbyname(hostname);
	if (!he) {
		fprintf(stderr, "host not found\n");
		return 1;
	}
	sin.sin_addr.s_addr = **(int**)he->h_addr_list;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(22);
	printf("host       : %s\n", hostname);
	//printf("IP         : %s\n", inet_ntoa(sin.sin_addr));

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
	
	printf("username   : %s\n", username);
	printf("private key: %s\n", pkey);
	
	// init ssh
    rc = libssh2_init(0);
    if (rc) {
		int err = libssh2_session_last_error(session, &errmsg, NULL, 0);		
		assert(rc == err);
        fprintf(stderr, "ssh initialization failed: (%d) %s\n", err, errmsg);
        return 1;
    }

    /* The application code is responsible for creating the socket
     * and establishing the connection  */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(sock, (SOCKADDR*)(&sin), sizeof(SOCKADDR_IN)) != 0) {
        fprintf(stderr, "failed to connect!\n");
        return -1;
    }

    /* Create a session instance */
	session = libssh2_session_init();
	if (!session)
		return -1;

	/* non-blocking */
	//libssh2_session_set_blocking(session, 0);
	/* blocking */
	libssh2_session_set_blocking(session, 1);

	/* ... start it up. This will trade welcome banners, exchange keys,
	* and setup crypto, compression, and MAC layers	*/
	rc = libssh2_session_handshake(session, sock);
	//while ((rc = libssh2_session_handshake(session, sock)) == LIBSSH2_ERROR_EAGAIN);
	if (rc) {
		fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
		return -1;
	}

	// authenticate
	rc = libssh2_userauth_publickey_fromfile(session, username, NULL, pkey, NULL);
	//while ((rc = libssh2_userauth_publickey_fromfile(
	//	session, username, NULL, pkey, NULL)) == LIBSSH2_ERROR_EAGAIN);
	if (rc) {
	    fprintf(stderr, "\tAuthentication by public key failed: %d\n", rc);
	    goto shutdown;
	} 

	// init sftp channel
	sftp = libssh2_sftp_init(session);
	if (!sftp) {
		fprintf(stderr, "Unable to init SFTP session\n");
		goto shutdown;
	}
	/* do {
		sftp = libssh2_sftp_init(session);
		if ((!sftp) && (libssh2_session_last_errno(session) !=
			LIBSSH2_ERROR_EAGAIN)) {
			fprintf(stderr, "Unable to init SFTP session\n");
			goto shutdown;
		}
	} while (!sftp); */

	/* default mode is blocking */
	//libssh2_session_set_blocking(session, 1);

	// read in blocking mode
	//san_read(session, sftp, remotefile, localfile);
	
	// read in non-blocking mode
	san_read_async(sock, session, sftp, remotefile, localfile);

	
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

	/* close sftp chanel */
	libssh2_sftp_shutdown(sftp);

shutdown:
    libssh2_session_disconnect(session, "sanssh2 disconnected");
    libssh2_session_free(session);
    libssh2_exit();
	closesocket(sock);
	WSACleanup();
	fprintf(stderr, "done.\n");
	return 0;
}
