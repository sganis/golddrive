/*
 * Sanssh
 * Author: SAG
 * Date: 10/09/2018
 *
 * Download a file from remote ssh server.
 *
 * usage: sanssh hostname user /tmp/file [private key]
 * private key defaults to %USERPROFILE%\.ssh\id_rsa
 */

#include "libssh2_config.h"
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <winsock2.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#define BUFFER_SIZE 32767

static void usage(const char* prog);
static int file_exists(const char* path);
static int waitsocket(int socket_fd, LIBSSH2_SESSION *session);
static int san_read_block(LIBSSH2_SESSION *session, LIBSSH2_SFTP *sftp_channel,
	const char * remotefile, const char * localfile);
static int san_read_nonblock(SOCKET sock, LIBSSH2_SESSION *session,
	LIBSSH2_SFTP *sftp_channel, const char * remotefile, const char * localfile);
static int san_mkdir(LIBSSH2_SFTP *sftp, const char * path);
static int san_rmdir(LIBSSH2_SFTP *sftp, const char * path);
static int san_stat(LIBSSH2_SFTP *sftp, const char * path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
static int san_lstat(LIBSSH2_SFTP *sftp, const char * path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
static int san_statvfs(LIBSSH2_SFTP *sftp, const char * path, LIBSSH2_SFTP_STATVFS *st);
static void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
static void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st);
LIBSSH2_SFTP_HANDLE * san_open(LIBSSH2_SFTP *sftp, const char *path, long mode);
LIBSSH2_SFTP_HANDLE * san_opendir(LIBSSH2_SFTP *sftp, const char *path);
int san_close_handle(LIBSSH2_SFTP_HANDLE *handle);
int san_rename(LIBSSH2_SFTP *sftp, const char *source, const char *destination);
int san_delete(LIBSSH2_SFTP *sftp, const char *filename);
int san_realpath(LIBSSH2_SFTP *sftp, const char *path, char *target);

// static int san_readdir();
// static int san_write();


int main(int argc, char *argv[])
{
	const char *hostname = "";
	const char *username = "";
	const char *password = "";
	const char *remotefile = "";
	const char *localfile = "";
	char pkey[MAX_PATH];
	int rc, errlen;
	char *errmsg;
	
	SOCKET sock;
	SOCKADDR_IN sin;
	LIBSSH2_SESSION *session;
	LIBSSH2_SFTP *sftp_channel;

	if (argc == 2 && strcmp(argv[1],"-V") == 0) {
		printf("sanssh 1.0.1\n");
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
		int err = libssh2_session_last_error(session, &errmsg, &errlen, 0);		
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
	
	
    /* This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers */
	rc = libssh2_session_handshake(session, sock);
	if (rc) {
		fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
		return -1;
	}
	// authenticate
	rc = libssh2_userauth_publickey_fromfile(session, username, NULL, pkey, NULL);
	if (rc) {
	    fprintf(stderr, "\tAuthentication by public key failed: %d\n", rc);
	    goto shutdown;
	} 
	
	// init sftp channel
	sftp_channel = libssh2_sftp_init(session);
	if (!sftp_channel) {
		fprintf(stderr, "Unable to init SFTP session\n");
		goto shutdown;
	}
	/* default mode is blocking */
	libssh2_session_set_blocking(session, 1);

	// read in blocking mode
	//read_block(session, sftp_channel, remotefile, localfile);
	
	// read in non-blocking mode
	//read_nonblock(sock, session, sftp_channel, remotefile, localfile);

	
	const char* path = "/tmp/sftp_folder";

	/* mkdir */
	//san_mkdir(sftp_channel, "/tmp/sftp_folder");

	/* rmdir */
	//san_rmdir(sftp_channel, "/tmp/sftp_folder");

	LIBSSH2_SFTP_ATTRIBUTES attrs;
	/* stat */
	//san_stat(sftp_channel, path, &attrs);
	//print_stat(path, &attrs);

	/* lstat */
	//san_lstat(sftp_channel, path, &attrs);
	//print_stat(path, &attrs);

	/* statfvs */
	LIBSSH2_SFTP_STATVFS st;
	san_statvfs(sftp_channel, path, &st);
	print_statvfs(path, &st);

	char target[MAX_PATH];
	san_realpath(sftp_channel, path, target);
	printf("path:     %s\n", path);
	printf("realpath: %s\n", target);

	/* close sftp chanel */
	libssh2_sftp_shutdown(sftp_channel);

shutdown:
    libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
    libssh2_session_free(session);
    closesocket(sock);
    fprintf(stderr, "done.\n");
    libssh2_exit();
    return 0;
}

static void usage(const char* prog)
{
	printf("My own sftp client!\n");
	printf("usage: %s [options] <host> <user> <remote file> <local file> [public key]\n", prog);
	printf("options:\n");
	printf("    -V           show version\n");
}

static int file_exists(const char* path)
{
	DWORD attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

static int waitsocket(int socket_fd, LIBSSH2_SESSION *session)
{
	struct timeval timeout;
	int rc;
	fd_set fd;
	fd_set *writefd = NULL;
	fd_set *readfd = NULL;
	int dir;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	FD_ZERO(&fd);
	FD_SET(socket_fd, &fd);
	/* now make sure we wait in the correct direction */
	dir = libssh2_session_block_directions(session);
	if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
		readfd = &fd;
	if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
		writefd = &fd;
	rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);
	return rc;
}
static int san_read_block(LIBSSH2_SESSION *session, LIBSSH2_SFTP *sftp_channel,
	const char * remotefile, const char * localfile)
{
	LIBSSH2_SFTP_HANDLE *sftp_handle;

	/* Since we have set non-blocking, tell libssh2 we are blocking */
	libssh2_session_set_blocking(session, 1);

	/* Request a file via SFTP */
	sftp_handle = libssh2_sftp_open(sftp_channel, remotefile, LIBSSH2_FXF_READ, 0);
	if (!sftp_handle) {
		fprintf(stderr, "Unable to open file with SFTP: %ld\n",
			libssh2_sftp_last_error(sftp_channel));
		return;
	}

	FILE *file;
	unsigned bytesWritten = 0;
	if (fopen_s(&file, localfile, "wb")) {
		fprintf(stderr, "error opening %s for writing\n", localfile);
		return;
	}
	int rc;
	size_t total = 0;
	size_t bytesize = sizeof(char);
	int buf_size = 2 * 1024 * 1024;
	ULONG tend, tstart;

	fprintf(stderr, "donwloading %s -> %s...\n", remotefile, localfile);

	tstart = (ULONG)time(NULL);
	do {
		char *mem = (char*)malloc(buf_size);
		rc = libssh2_sftp_read(sftp_handle, mem, buf_size);
		if (rc > 0) {
			fwrite(mem, bytesize, rc, file);
			total += rc;
		}
		else {
			break;
		}
		free(mem);
	} while (1);
	tend = (unsigned long)time(NULL);

	fclose(file);
	printf("bytes     : %ld\n", total);
	printf("elapsed   : %ld secs.\n", (tend - tstart));
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)(tend - tstart)));

	libssh2_sftp_close(sftp_handle);

}

static int san_read_nonblock(SOCKET sock, LIBSSH2_SESSION *session,
	LIBSSH2_SFTP *sftp_channel, const char * remotefile, const char * localfile)
{
	LIBSSH2_SFTP_HANDLE *sftp_handle;
	int rc;
	int spin = 0;
	size_t total = 0;

	/* Since we have set non-blocking, tell libssh2 we are non-blocking */
	libssh2_session_set_blocking(session, 0);

	do {
		sftp_channel = libssh2_sftp_init(session);
		if (!sftp_channel) {
			if (libssh2_session_last_errno(session) == LIBSSH2_ERROR_EAGAIN) {
				spin++;
				waitsocket(sock, session); /* now we wait */
			}
			else {
				fprintf(stderr, "Unable to init SFTP session\n");
				return -1;
			}
		}
	} while (!sftp_channel);

	/* Request a file via SFTP */
	do {
		sftp_handle = libssh2_sftp_open(sftp_channel, remotefile,
			LIBSSH2_FXF_READ, 0);
		if (!sftp_handle) {
			if (libssh2_session_last_errno(session) != LIBSSH2_ERROR_EAGAIN) {
				fprintf(stderr, "Unable to open file with SFTP\n");
				return -2;
			}
			else {
				//fprintf(stderr, "non-blocking open\n");
				spin++;
				waitsocket(sock, session); /* now we wait */
			}
		}
	} while (!sftp_handle);

	FILE *file;
	if (fopen_s(&file, localfile, "wb")) {
		fprintf(stderr, "error opening %s for writing\n", localfile);
		return -3;
	}

	size_t bytesize = sizeof(char);
	unsigned bytesWritten = 0;
	int buf_size = 2 * 1024 * 1024;
	int tend, tstart;
	fprintf(stderr, "donwloading %s -> %s...\n", remotefile, localfile);
	tstart = (ULONG)time(NULL);

	do {
		char *mem = (char*)malloc(buf_size);
		while ((rc = libssh2_sftp_read(sftp_handle, mem, buf_size))
			== LIBSSH2_ERROR_EAGAIN) {
			spin++;
			waitsocket(sock, session); /* now we wait */
		}
		if (rc > 0) {
			fwrite(mem, bytesize, rc, file);
			total += rc;
		}
		else {
			break;
		}
	} while (1);

	tend = time(NULL);
	fclose(file);
	printf("bytes     : %ld\n", total);
	printf("spin      : %d\n", spin);
	printf("elapsed   : %ld secs.\n", (tend - tstart));
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)(tend - tstart)));

	libssh2_sftp_close(sftp_handle);
	return 0;
}

static int san_mkdir(LIBSSH2_SFTP *sftp, const char * path)
{
	int rc;
	rc = libssh2_sftp_mkdir(sftp, path,
		LIBSSH2_SFTP_S_IRWXU |
		LIBSSH2_SFTP_S_IRWXG |
		LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_mkdir failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sftp));
	}
	return rc;
}

static int san_rmdir(LIBSSH2_SFTP *sftp, const char * path)
{
	int rc = 0;
	rc = libssh2_sftp_rmdir(sftp, path);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_rmdir failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sftp));
	}
	return rc;
}
static int san_stat(LIBSSH2_SFTP *sftp, const char * path, 
	LIBSSH2_SFTP_ATTRIBUTES *attrs)
{
	int rc = 0;
	rc = libssh2_sftp_stat(sftp, path, attrs);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_stat failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sftp));
	}
	return rc;
}
static int san_lstat(LIBSSH2_SFTP *sftp, const char * path, 
	LIBSSH2_SFTP_ATTRIBUTES *attrs)
{
	int rc = 0;
	rc = libssh2_sftp_lstat(sftp, path, attrs);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_lstat failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sftp));
	}
	return rc;
}

static void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs)
{
	printf("path:  %s\n", path);
	printf("flags: %ld\n", attrs->flags);
	printf("size:  %ld\n", attrs->filesize);
	printf("uid:   %ld\n", attrs->uid);
	printf("gid:   %ld\n", attrs->gid);
	printf("mode:  %ld\n", attrs->permissions);
	printf("atime: %ld\n", attrs->atime);
	printf("mtime: %ld\n", attrs->mtime);
}

static int san_statvfs(LIBSSH2_SFTP *sftp, const char * path, LIBSSH2_SFTP_STATVFS *st)
{
	int rc = 0;
	rc = libssh2_sftp_statvfs(sftp, path, strlen(path), st);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_statvfs failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sftp));
	}
	return rc;
}
static void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st)
{
	printf("path:    %s\n", path);
	printf("bsize:   %ld\n", st->f_bsize);    	/* file system block size */
	printf("frsize:  %ld\n", st->f_frsize);   	/* fragment size */
	printf("blocks:  %ld\n", st->f_blocks);   	/* size of fs in f_frsize units */
	printf("bfree:   %ld\n", st->f_bfree);    	/* # free blocks */
	printf("bavail:  %ld\n", st->f_bavail);   	/* # free blocks for non-root */
	printf("files:   %ld\n", st->f_files);    	/* # inodes */
	printf("ffree:   %ld\n", st->f_ffree);    	/* # free inodes */
	printf("favail:  %ld\n", st->f_favail);   	/* # free inodes for non-root */
	printf("fsid:    %ld\n", st->f_fsid);     	/* file system ID */
	printf("flag:    %ld\n", st->f_flag);     	/* mount flags */
	printf("namemax: %ld\n", st->f_namemax);  	/* maximum filename length */

}

LIBSSH2_SFTP_HANDLE * san_open(LIBSSH2_SFTP *sftp, const char *path, long mode)
{
	LIBSSH2_SFTP_HANDLE * handle;
	handle = libssh2_sftp_open(sftp, path,
			LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
			LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
			LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
	if (!handle) {
		fprintf(stderr, "Unable to open file\n");
	}
	return handle;
}
LIBSSH2_SFTP_HANDLE * san_opendir(LIBSSH2_SFTP *sftp, const char *path)
{
	LIBSSH2_SFTP_HANDLE * handle;
	handle = libssh2_sftp_opendir(sftp, path);
	if (!handle) {
		fprintf(stderr, "Unable to open directory\n");
	}
	return handle;
}
int san_close_handle(LIBSSH2_SFTP_HANDLE *handle)
{
	return libssh2_sftp_close_handle(handle);
}
int san_rename(LIBSSH2_SFTP *sftp, const char *source, const char *destination)
{
	int rc;
	rc = libssh2_sftp_rename(sftp, source, destination);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_rename failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sftp));
	}
	return rc;

}

int san_delete(LIBSSH2_SFTP *sftp, const char *filename)
{
	int rc;
	rc = libssh2_sftp_unlink(sftp, filename);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_unlink failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sftp));
	}
	return rc;
}

int san_realpath(LIBSSH2_SFTP *sftp, const char *path, char *target)
{
	int rc;
	rc = libssh2_sftp_realpath(sftp, path, target, MAX_PATH);
	if (rc < 0) {
		fprintf(stderr, "libssh2_sftp_readlink failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sftp));
	}
	return rc;
}
