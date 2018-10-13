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

//static int file_exists(const char* path);
//static int waitsocket(int socket_fd, LIBSSH2_SESSION *session);
//static int san_mkdir(LIBSSH2_SFTP *sftp, const char *path);
//static int san_rmdir(LIBSSH2_SFTP *sftp, const char *path);
//static int san_stat(LIBSSH2_SFTP *sftp, const char *path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
//static int san_lstat(LIBSSH2_SFTP *sftp, const char *path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
//static int san_statvfs(LIBSSH2_SFTP *sftp, const char *path, LIBSSH2_SFTP_STATVFS *st);
//static int san_close_handle(LIBSSH2_SFTP_HANDLE *handle);
//static int san_rename(LIBSSH2_SFTP *sftp, const char *source, const char *destination);
//static int san_delete(LIBSSH2_SFTP *sftp, const char *filename);
//static int san_realpath(LIBSSH2_SFTP *sftp, const char *path, char *target);
//static int san_readdir(LIBSSH2_SFTP *sftp, const char *path);
//static int san_read(LIBSSH2_SESSION *session, LIBSSH2_SFTP *sftp, 
//	const char * remotefile, const char * localfile);
//static int san_read_async(SOCKET sock, LIBSSH2_SESSION *session, LIBSSH2_SFTP *sftp, 
//	const char * remotefile, const char * localfile);
//static LIBSSH2_SFTP_HANDLE * san_open(LIBSSH2_SFTP *sftp, const char *path, long mode);
//static LIBSSH2_SFTP_HANDLE * san_opendir(LIBSSH2_SFTP *sftp, const char *path);
//static void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
//static void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st);
//static void get_filetype(unsigned long perm, char* filetype);
//static void usage(const char* prog);
//static int run_command(int sock, LIBSSH2_SESSION *session, 
//	const char *cmd, char *out, char *err);

static void usage(const char* prog)
{
	printf("Sanssh2 - sftp client using libssh2\n");
	printf("usage: %s [options] <host> <user> <remote file> <local file> [public key]\n", prog);
	printf("options:\n");
	printf("    -V           show version\n");
}
static int file_exists(const char* path)
{
	DWORD attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}
static void get_filetype(unsigned long perm, char* filetype)
{
	if (LIBSSH2_SFTP_S_ISLNK(perm))			strcpy(filetype, "LNK");
	else if (LIBSSH2_SFTP_S_ISREG(perm))	strcpy(filetype, "REG");
	else if (LIBSSH2_SFTP_S_ISDIR(perm))	strcpy(filetype, "DIR");
	else if (LIBSSH2_SFTP_S_ISCHR(perm))	strcpy(filetype, "CHR");
	else if (LIBSSH2_SFTP_S_ISBLK(perm))	strcpy(filetype, "BLK");
	else if (LIBSSH2_SFTP_S_ISFIFO(perm))	strcpy(filetype, "FIF");
	else if (LIBSSH2_SFTP_S_ISSOCK(perm))	strcpy(filetype, "SOC");
	else									strcpy(filetype, "NAN");
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
static int san_read(LIBSSH2_SESSION *session, LIBSSH2_SFTP *sftp_channel,
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
	int bufsize = 2 * 1024 * 1024;
	int start;
	int duration;

	fprintf(stderr, "donwloading %s -> %s...\n", remotefile, localfile);
	start = time(NULL);
	char *mem = (char*)malloc(bufsize);
	for (;;) {	
		rc = libssh2_sftp_read(sftp_handle, mem, bufsize);
		if (rc == 0)
			break;
		fwrite(mem, bytesize, rc, file);
		total += rc;	
	} 
	free(mem);
	duration = time(NULL) - start;

	fclose(file);
	printf("bytes     : %ld\n", total);
	printf("elapsed   : %ld secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)(duration)));

	libssh2_sftp_close(sftp_handle);
	return 0;
}
static int san_read_async(SOCKET sock, LIBSSH2_SESSION *session,
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
	int start;
	int duration;
	fprintf(stderr, "donwloading %s -> %s...\n", remotefile, localfile);
	start = time(NULL);

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

	duration = time(NULL) - start;
	fclose(file);
	printf("bytes     : %ld\n", total);
	printf("spin      : %d\n", spin);
	printf("duration  : %ld secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)duration));

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
static LIBSSH2_SFTP_HANDLE * san_open(LIBSSH2_SFTP *sftp, const char *path, long mode)
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
static LIBSSH2_SFTP_HANDLE * san_opendir(LIBSSH2_SFTP *sftp, const char *path)
{
	LIBSSH2_SFTP_HANDLE * handle;
	handle = libssh2_sftp_opendir(sftp, path);
	if (!handle) {
		fprintf(stderr, "Unable to open directory\n");
	}
	return handle;
}
static int san_close_handle(LIBSSH2_SFTP_HANDLE *handle)
{
	return libssh2_sftp_close_handle(handle);
}
static int san_rename(LIBSSH2_SFTP *sftp, const char *source, const char *destination)
{
	int rc;
	rc = libssh2_sftp_rename(sftp, source, destination);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_rename failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sftp));
	}
	return rc;

}
static int san_delete(LIBSSH2_SFTP *sftp, const char *filename)
{
	int rc;
	rc = libssh2_sftp_unlink(sftp, filename);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_unlink failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sftp));
	}
	return rc;
}
static int san_realpath(LIBSSH2_SFTP *sftp, const char *path, char *target)
{
	int rc;
	rc = libssh2_sftp_realpath(sftp, path, target, MAX_PATH);
	if (rc < 0) {
		fprintf(stderr, "libssh2_sftp_readlink failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sftp));
	}
	return rc;
}
static int san_readdir(LIBSSH2_SFTP *sftp, const char *path)
{
	int rc = 0;
	LIBSSH2_SFTP_HANDLE *handle;
	handle = libssh2_sftp_opendir(sftp, path);
	if (!handle) {
		fprintf(stderr, "Unable to open dir with SFTP\n");
		return libssh2_sftp_last_error(sftp);
	}
	do {
		char mem[512];
		char longentry[512];
		LIBSSH2_SFTP_ATTRIBUTES attrs;
		rc = libssh2_sftp_readdir_ex(handle, mem, sizeof(mem),
			longentry, sizeof(longentry), &attrs);
		if (rc > 0) {
			/* rc is the length of the file name in the mem buffer */
			char filetype[4];
			get_filetype(attrs.permissions, &filetype);
			if (longentry[0] != '\0') {
				printf("%3s %10ld %5ld %5ld %s\n",
					filetype, attrs.filesize, attrs.uid, attrs.gid,
					longentry);
			}
			else {
				if (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
					/* this should check what permissions it is
					and print the output accordingly */
					printf("--fix----- ");
				}
				else {
					printf("---------- ");
				}
				if (attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID) {
					printf("%4ld %4ld ", attrs.uid, attrs.gid);
				}
				else {
					printf("   -    - ");
				}
				if (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) {
					printf("%8ld ", attrs.filesize);
				}
				printf("%s\n", mem);
			}
		}
		else {
			break;
		}
	} while (1);

	libssh2_sftp_closedir(handle);
}
static int run_command(int sock, LIBSSH2_SESSION *session,
	const char *cmd, char *out, char *err)
{
	LIBSSH2_CHANNEL *channel;
	int rc;
	int bytes = 0;
	int errlen;
	char *errmsg;
	channel = libssh2_channel_open_session(session);
	if (!channel) {
		int rc = libssh2_session_last_error(session, &errmsg, &errlen, 0);
		fprintf(stderr, "Unable to init ssh chanel: (%d) %s\n", rc, errmsg);
		return 1;
	}
	libssh2_channel_set_blocking(channel, 0);
	while ((rc = libssh2_channel_exec(channel, cmd))
		== LIBSSH2_ERROR_EAGAIN) {
		waitsocket(sock, session);
	}
	if (rc != 0) {
		fprintf(stderr, "Error\n");
		return 1;
	}

	/* read stdout */
	for (;;) {
		do {
			char buffer[0x4000];
			rc = libssh2_channel_read(channel, buffer, sizeof(buffer));
			if (rc > 0) {
				bytes += rc;
				strncat(out, buffer, rc);
			}
		} while (rc > 0);

		if (rc == LIBSSH2_ERROR_EAGAIN)
			waitsocket(sock, session);
		else
			break;
	}

	/* read stderr */
	for (;;) {
		do {
			char buffer[0x4000];
			rc = libssh2_channel_read_stderr(channel, buffer, sizeof(buffer));
			if (rc > 0) {
				bytes += rc;
				strncat(err, buffer, rc);
			}
		} while (rc > 0);

		if (rc == LIBSSH2_ERROR_EAGAIN)
			waitsocket(sock, session);
		else
			break;
	}
	/* get exit code */
	while ((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(sock, session);
	if (rc == 0)
		rc = libssh2_channel_get_exit_status(channel);
	else
		rc = 127;
	libssh2_channel_set_blocking(channel, 1);
	libssh2_channel_free(channel);
	return rc;
}

static int run_command_shell(LIBSSH2_SESSION *session,
	const char *cmd, char *out, char *err)
{
	LIBSSH2_CHANNEL *channel;
	int rc;
	int bytes = 0;
	char *errmsg;

	if (!(channel = libssh2_channel_open_session(session))) {
		int rc = libssh2_session_last_error(session, &errmsg, NULL, 0);
		fprintf(stderr, "Unable to init ssh chanel: (%d) %s\n", rc, errmsg);
		return 1;
	}

	/* set env var */
	libssh2_channel_setenv(channel, "FOO", "bar");

	/* Request a terminal with 'vanilla' terminal emulation
	* See /etc/termcap for more options
	*/
	if (libssh2_channel_request_pty(channel, "vanilla")) {
		int rc = libssh2_session_last_error(session, &errmsg, NULL, 0);
		fprintf(stderr, "Failed requesting pty: (%d) %s\n", rc, errmsg);
		return 1;
	}

	/* Open a SHELL on that pty */
	if (libssh2_channel_shell(channel)) {
		int rc = libssh2_session_last_error(session, &errmsg, NULL, 0);
		fprintf(stderr, "Unable to request shell on allocated pty: (%d) %s\n", rc, errmsg);
		return 1;
	}

	/* At this point the shell can be interacted with using
	* libssh2_channel_read()
	* libssh2_channel_read_stderr()
	* libssh2_channel_write()
	* libssh2_channel_write_stderr()
	*
	* Blocking mode may be (en|dis)abled with: libssh2_channel_set_blocking()
	* If the server send EOF, libssh2_channel_eof() will return non-0
	* To send EOF to the server use: libssh2_channel_send_eof()
	*/

	if (channel) {
		libssh2_channel_close(channel);
		libssh2_channel_free(channel);
		channel = NULL;
	}
}

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
	LIBSSH2_SFTP *sftp;
	

	if (argc == 2 && strcmp(argv[1],"-V") == 0) {
		printf("sanssh2 1.0.1\n");
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

	/* close sftp chanel */
	libssh2_sftp_shutdown(sftp);

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

shutdown:
    libssh2_session_disconnect(session, "sanssh2 disconnected");
    libssh2_session_free(session);
    libssh2_exit();
	closesocket(sock);
	WSACleanup();
	fprintf(stderr, "done.\n");
	return 0;
}
