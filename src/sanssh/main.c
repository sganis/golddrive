/*
 * Sanssh
 * Author: SAG
 * Date: 10/09/2018
 *
 * Download a file from remote ssh server.
 *
 * usage: sanssh.exe hostname user /tmp/file
 * ssh key must be %USERPROFILE%\.ssh\id_rsa
 *
 */

#include "libssh2_config.h"
#include <libssh2.h>
#include <libssh2_sftp.h>

#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
# ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#define BUFFER_SIZE 32767


BOOL FileExists(const char* path)
{
	DWORD dwAttrib = GetFileAttributesA(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int main(int argc, char *argv[])
{
	const char *hostname = "";
	const char *username = "";
	const char *password = "";
	const char *remotefile = "";
	const char *localfile = "C:\\Temp\\file.bin";

    SOCKET sock;
	SOCKADDR_IN sin;
	char pkey[MAX_PATH];
	int rc;
	LIBSSH2_SESSION *session;
    LIBSSH2_SFTP *sftp_channel;
    LIBSSH2_SFTP_HANDLE *sftp_handle;

	if (argc < 4) {
		printf("usage: %s <ip address> <user> <remove file>\n", argv[0]);
		exit(1);
	}

	// initialize socket
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

	if (argc > 4) {
		strcpy_s(pkey, MAX_PATH, argv[4]);
	} else {
		// get public key
		char profile[BUFFER_SIZE];
		ExpandEnvironmentStringsA("%USERPROFILE%", profile, BUFFER_SIZE);
		//printf("user profile: %s\n", profile);
		strcpy_s(pkey, MAX_PATH, profile);
		strcat_s(pkey, MAX_PATH, "\\.ssh\\id_rsa");
	}

	if (!FileExists(pkey)) {
		printf("error: cannot read private key: %s\n", pkey);
		exit(1);
	}
	
	printf("username   : %s\n", username);
	printf("private key: %s\n", pkey);

	
	
	// init ssh
    rc = libssh2_init (0);
    if (rc != 0) {
        fprintf(stderr, "libssh2 initialization failed, exit %d\n", rc);
        return 1;
    }

    /*
     * The application code is responsible for creating the socket
     * and establishing the connection
     */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(sock, (SOCKADDR*)(&sin), sizeof(SOCKADDR_IN)) != 0) {
        fprintf(stderr, "failed to connect!\n");
        return -1;
    }

    /* Create a session instance */
    session = libssh2_session_init();
    if(!session)
        return -1;

    /* Since we have set non-blocking, tell libssh2 we are blocking */
    libssh2_session_set_blocking(session, 1);

    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers */
    rc = libssh2_session_handshake(session, sock);
    if(rc) {
        fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
        return -1;
    }
	// authenticate
	rc = libssh2_userauth_publickey_fromfile(session, username, NULL, pkey, NULL);
	if (rc) {
	    fprintf(stderr, "\tAuthentication by public key failed!\n");
	    goto shutdown;
	} else {
	    //fprintf(stderr, "\tAuthentication by public key succeeded.\n");
	}

    // init sftp channel
    sftp_channel = libssh2_sftp_init(session);

    if (!sftp_channel) {
        fprintf(stderr, "Unable to init SFTP session\n");
        goto shutdown;
    }

    /* Request a file via SFTP */
    sftp_handle = libssh2_sftp_open(sftp_channel, remotefile, LIBSSH2_FXF_READ, 0);
    if (!sftp_handle) {
        fprintf(stderr, "Unable to open file with SFTP: %ld\n",
                libssh2_sftp_last_error(sftp_channel));
        goto shutdown;
    }
    fprintf(stderr, "donwloading %s -> %s...\n", remotefile, localfile);
	
	FILE *file;
	unsigned bytesWritten = 0;
	if (fopen_s(&file, localfile, "wb")) {
		fprintf(stderr, "error opening %s for writing\n", localfile);
		goto shutdown;
	}
	
	int total = 0;
	size_t bytesize = sizeof(char);
	int buf_size = 2 * 1024 * 1024;
	ULONG tend, tstart;
	tstart = (ULONG)time(NULL);
	//fprintf(stdout, "time: %lu\n", tstart);
    do {
        char *mem = (char*)malloc(buf_size);

        /* loop until we fail */
        //fprintf(stderr, "libssh2_sftp_read()!\n");
        rc = libssh2_sftp_read(sftp_handle, mem, buf_size);
        if (rc > 0) {
			//printf("bytes read: %d\n", rc);
            //write(1, mem, rc);
			if ((bytesWritten = fwrite(mem, bytesize, rc, file)) == -1)
			{
				switch (errno)
				{
				case EBADF:
					perror("Bad file descriptor!");
					break;
				case ENOSPC:
					perror("No space left on device!");
					break;
				case EINVAL:
					perror("Invalid parameter: buffer was NULL!");
					break;
				default:
					// An unrelated error occured   
					perror("Unexpected error!");
				}
				break;
			}
			total += rc;
        } else {
            break;
        }
		free(mem);
    } while (1);

	tend = (unsigned long)time(NULL);
	//fprintf(stdout, "time: %lu\n", tend);
	if (file) {
		fclose(file);
	}	
	printf("bytes     : %d\n", total);
	printf("elapsed   : %ld secs.\n", (tend - tstart));
	printf("speed     : %d MB/s.\n", (int)(total/1024.0/1024.0/(double)(tend - tstart)));
	libssh2_sftp_close(sftp_handle);
    libssh2_sftp_shutdown(sftp_channel);

shutdown:
    libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
    libssh2_session_free(session);
    closesocket(sock);
    fprintf(stderr, "done.\n");
    libssh2_exit();
    return 0;
}
