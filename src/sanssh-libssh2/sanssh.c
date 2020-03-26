#include "sanssh.h"
#include <stdio.h>

void usage(const char* prog)
{
	printf("Sanssh2 - sftp client using libssh2\n");
	printf("usage: %s [options] <host> <port> <user> <remote file> <local file> [public key]\n", prog);
	printf("options:\n");
	printf("    -V           show version\n");
}
int file_exists(const char* path)
{
	DWORD attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}
void get_filetype(unsigned long perm, char* filetype)
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

SANSSH * san_init(const char* hostname,	int port, const char* username, 
	const char* pkey, char* error)
{
	int rc;
	char *errmsg;
	SOCKADDR_IN sin;
	HOSTENT *he;
	SOCKET sock;
	LIBSSH2_SESSION* ssh;
	LIBSSH2_SFTP* sftp;

	// initialize windows socket
	WSADATA wsadata;
	rc = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (rc != 0) {
		sprintf(error, "WSAStartup failed with error %d\n", rc);
		return 0;
	}

	// resolve hostname	
	he = gethostbyname(hostname);
	if (!he) {
		sprintf(error, "host not found");
		return 0;
	}
	sin.sin_addr.s_addr = **(int**)he->h_addr_list;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	// init ssh
	rc = libssh2_init(0);
	if (rc) {
		sprintf(error, "ssh initialization %d\n", rc);
		return 0;
	}

	/* create socket  */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(sock, (SOCKADDR*)(&sin), sizeof(SOCKADDR_IN)) != 0) {
		sprintf(error, "failed to open socket");
		return 0;
	}

	/* Create a session instance */
	ssh = libssh2_session_init();
	if (!ssh) {
		sprintf(error, "failed to initialize ssh session");
		return 0;
	}
		
	/* non-blocking */
	//libssh2_session_set_blocking(session, 0);
	/* blocking */
	libssh2_session_set_blocking(ssh, 1);

	/* ... start it up. This will trade welcome banners, exchange keys,
	* and setup crypto, compression, and MAC layers	*/
	rc = libssh2_session_handshake(ssh, sock);
	//while ((rc = libssh2_session_handshake(session, sock)) == LIBSSH2_ERROR_EAGAIN);
	if (rc) {
		sprintf(error, "failure establishing ssh handshake whith error %d", rc);
		return 0;
	}

	// authenticate
	rc = libssh2_userauth_publickey_fromfile(ssh, username, NULL, pkey, NULL);
	//while ((rc = libssh2_userauth_publickey_fromfile(
	//	session, username, NULL, pkey, NULL)) == LIBSSH2_ERROR_EAGAIN);
	if (rc) {
		sprintf(error, "authentication by public key failed with error %d", rc);
		return 0;
	}

	// init sftp channel
	sftp = libssh2_sftp_init(ssh);
	if (!sftp) {
		sprintf(error, "failure to init sftp session");
		return 0;
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
	SANSSH* sanssh = malloc(sizeof(SANSSH));
	if (sanssh) {
		sanssh->socket = sock;
		sanssh->ssh = ssh;
		sanssh->sftp = sftp;
	}
	return sanssh;
}
int san_finalize(SANSSH *sanssh)
{
	libssh2_sftp_shutdown(sanssh->sftp);
	libssh2_session_disconnect(sanssh->ssh, "sanssh2 disconnected");
	libssh2_session_free(sanssh->ssh);
	libssh2_exit();
	closesocket(sanssh->socket);
	WSACleanup();
	free(sanssh);
	return 0;
}
int waitsocket(SANSSH *sanssh)
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
	FD_SET(sanssh->socket, &fd);
	/* now make sure we wait in the correct direction */
	dir = libssh2_session_block_directions(sanssh->ssh);
	if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
		readfd = &fd;
	if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
		writefd = &fd;
	rc = select(sanssh->socket + 1, readfd, writefd, NULL, &timeout);
	return rc;
}
int san_read(SANSSH *sanssh, const char * remotefile, const char * localfile)
{
	LIBSSH2_SFTP_HANDLE *handle;

	/* Since we have set non-blocking, tell libssh2 we are blocking */
	

	/* Request a file via SFTP */
	handle = libssh2_sftp_open(sanssh->sftp, remotefile, LIBSSH2_FXF_READ, 0);
	if (!handle) {
		fprintf(stderr, "Unable to open file with SFTP: %ld\n",
			libssh2_sftp_last_error(sanssh->sftp));
		return 0;
	}

	FILE *file;
	unsigned bytesWritten = 0;
	if (fopen_s(&file, localfile, "wb")) {
		fprintf(stderr, "error opening %s for writing\n", localfile);
		return 0;
	}
	int bytesread;
	size_t total = 0;
	size_t bytesize = sizeof(char);
	size_t byteswritten = 0;
	int bufsize = 2 * 1024 * 1024;
	//int bufsize = 64 * 1024;
	int start;
	int duration;

	fprintf(stderr, "donwloading %s -> %s...\n", remotefile, localfile);
	//printf("buffer size    bytes read     bytes written  total bytes\n");
	start = time(NULL);
	char *mem = (char*)malloc(bufsize);
	for (;;) {
		bytesread = libssh2_sftp_read(handle, mem, bufsize);
		if (bytesread == 0)
			break;
		byteswritten = fwrite(mem, bytesize, bytesread, file);
		total += bytesread;
		//printf("%-15d%-15d%-15ld%-15ld\n", bufsize, bytesread, byteswritten, total);
	}
	free(mem);
	duration = time(NULL) - start;
	if(file)
		fclose(file);
	printf("bytes     : %d\n", total);
	printf("elapsed   : %ld secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)(duration)));

	libssh2_sftp_close(handle);
	return 0;
}
int san_write(SANSSH* sanssh, const char* remotefile, const char* localfile)
{
	LIBSSH2_SFTP_HANDLE* handle;

	/* Since we have set non-blocking, tell libssh2 we are blocking */
	//libssh2_session_set_blocking(sanssh->ssh, 1);

	/* Request a file via SFTP */
	handle = libssh2_sftp_open(sanssh->sftp, remotefile,
		LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
		LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR );
	if (!handle) {
		fprintf(stderr, "Unable to open file with SFTP: %ld\n",
			libssh2_sftp_last_error(sanssh->sftp));
		return 0;
	}

	FILE* file;
	
	if (fopen_s(&file, localfile, "rb")) {
		fprintf(stderr, "error opening %s for reading\n", localfile);
		return 0;
	}
	int bytesread;
	int total = 0;
	size_t bytesize = sizeof(char);
	size_t rc = 0;
	int bufsize = 2 * 1024 * 1024;
	//int bufsize = 64 * 1024;
	int start;
	int duration;

	fprintf(stderr, "uploading %s -> %s...\n", localfile, remotefile);
	//printf("buffer size    bytes read     bytes written  total bytes\n");
	start = time(NULL);
	char* mem = (char*)malloc(bufsize);
	char* ptr;

	do {
		bytesread = fread(mem, 1, sizeof(mem), file);
		if (bytesread == 0)
			break;
		ptr = mem;
		do {
			rc = libssh2_sftp_write(handle, ptr, bytesread);
			if (rc < 0)
				break;
			ptr += rc;
			bytesread -= rc;
			total += rc;
		} while (bytesread);
	} while (rc > 0);

	free(mem);
	duration = time(NULL) - start;
	if(file)
		fclose(file);
	printf("bytes     : %d\n", total);
	printf("elapsed   : %d secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)(duration)));

	libssh2_sftp_close(handle);
	return 0;
}
int san_read_async(SANSSH *sanssh, const char * remotefile, const char * localfile)
{
	LIBSSH2_SFTP_HANDLE *handle;
	int rc;
	int spin = 0;
	int total = 0;

	/* Since we have set non-blocking, tell libssh2 we are non-blocking */
	//libssh2_session_set_blocking(sanssh->ssh, 0);

	//do {
	//	sftp_channel = libssh2_sftp_init(sanssh->ssh);
	//	if (!sftp_channel) {
	//		if (libssh2_session_last_errno(session) == LIBSSH2_ERROR_EAGAIN) {
	//			spin++;
	//			waitsocket(sock, session); /* now we wait */
	//		}
	//		else {
	//			fprintf(stderr, "Unable to init SFTP session\n");
	//			return -1;
	//		}
	//	}
	//} while (!sftp_channel);

	/* Request a file via SFTP */
	do {
		handle = libssh2_sftp_open(sanssh->sftp, remotefile,
			LIBSSH2_FXF_READ, 0);
		if (!handle) {
			if (libssh2_session_last_errno(sanssh->ssh) != LIBSSH2_ERROR_EAGAIN) {
				fprintf(stderr, "Unable to open file with SFTP\n");
				return -2;
			}
			else {
				//fprintf(stderr, "non-blocking open\n");
				spin++;
				waitsocket(sanssh); /* now we wait */
			}
		}
	} while (!handle);

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
		while ((rc = libssh2_sftp_read(handle, mem, buf_size))
			== LIBSSH2_ERROR_EAGAIN) {
			spin++;
			waitsocket(sanssh); /* now we wait */
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
	printf("bytes     : %d\n", total);
	printf("spin      : %d\n", spin);
	printf("duration  : %ld secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)duration));

	libssh2_sftp_close(handle);
	return 0;
}
int san_write_async(SANSSH* sanssh, const char* remotefile, const char* localfile)
{
	LIBSSH2_SFTP_HANDLE* handle;
	int rc;
	int spin = 0;
	size_t total = 0;

	/* Since we have set non-blocking, tell libssh2 we are non-blocking */
	//libssh2_session_set_blocking(sanssh->ssh, 0);

	//do {
	//	sftp_channel = libssh2_sftp_init(sanssh->ssh);
	//	if (!sftp_channel) {
	//		if (libssh2_session_last_errno(session) == LIBSSH2_ERROR_EAGAIN) {
	//			spin++;
	//			waitsocket(sock, session); /* now we wait */
	//		}
	//		else {
	//			fprintf(stderr, "Unable to init SFTP session\n");
	//			return -1;
	//		}
	//	}
	//} while (!sftp_channel);

	/* Request a file via SFTP */
	do {
		handle = libssh2_sftp_open(sanssh->sftp, remotefile,
			LIBSSH2_FXF_READ, 0);
		if (!handle) {
			if (libssh2_session_last_errno(sanssh->ssh) != LIBSSH2_ERROR_EAGAIN) {
				fprintf(stderr, "Unable to open file with SFTP\n");
				return -2;
			}
			else {
				//fprintf(stderr, "non-blocking open\n");
				spin++;
				waitsocket(sanssh); /* now we wait */
			}
		}
	} while (!handle);

	FILE* file;
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
		char* mem = (char*)malloc(buf_size);
		while ((rc = libssh2_sftp_read(handle, mem, buf_size))
			== LIBSSH2_ERROR_EAGAIN) {
			spin++;
			waitsocket(sanssh); /* now we wait */
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
	printf("bytes     : %d\n", total);
	printf("spin      : %d\n", spin);
	printf("duration  : %ld secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)duration));

	libssh2_sftp_close(handle);
	return 0;
}
int san_mkdir(SANSSH *sanssh, const char * path)
{
	int block = libssh2_session_get_blocking(sanssh->ssh);
	int rc;
	rc = (int)libssh2_sftp_mkdir(sanssh->sftp, path,
		LIBSSH2_SFTP_S_IRWXU |
		LIBSSH2_SFTP_S_IRWXG |
		LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_mkdir failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
	return rc;
}
int san_rmdir(SANSSH *sanssh, const char * path)
{
	int rc = 0;
	rc = libssh2_sftp_rmdir(sanssh->sftp, path);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_rmdir failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
	return rc;
}
int san_stat(SANSSH *sanssh, const char * path, SANSSH_ATTRIBUTES *attrs)
{
	int rc = 0;
	LIBSSH2_SFTP_ATTRIBUTES att;
	rc = libssh2_sftp_stat(sanssh->sftp, path, &att);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_stat failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
	copy_attributes(attrs, &att);
	return rc;
}
int san_lstat(SANSSH *sanssh, const char * path, SANSSH_ATTRIBUTES*attrs)
{
	int rc = 0;
	rc = libssh2_sftp_lstat(sanssh->sftp, path, attrs);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_lstat failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
	return rc;
}
void print_stat(const char* path, SANSSH_ATTRIBUTES *attrs)
{
	printf("path:  %s\n", path);
	printf("flags: %ld\n", attrs->flags);
	printf("size:  %lld\n", attrs->filesize);
	printf("uid:   %ld\n", attrs->uid);
	printf("gid:   %ld\n", attrs->gid);
	printf("mode:  %ld\n", attrs->mode);
	printf("atime: %ld\n", attrs->atime);
	printf("mtime: %ld\n", attrs->mtime);
}
int san_statvfs(SANSSH *sanssh, const char * path, SANSSH_STATVFS *st)
{
	int rc = 0;
	rc = libssh2_sftp_statvfs(sanssh->sftp, path, strlen(path), st);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_statvfs failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
	return rc;
}
void print_statvfs(const char* path, SANSSH_STATVFS *st)
{
	printf("path:    %s\n", path);
	printf("bsize:   %lld\n", st->f_bsize);    	/* file system block size */
	printf("frsize:  %lld\n", st->f_frsize);   	/* fragment size */
	printf("blocks:  %lld\n", st->f_blocks);   	/* size of fs in f_frsize units */
	printf("bfree:   %lld\n", st->f_bfree);    	/* # free blocks */
	printf("bavail:  %lld\n", st->f_bavail);   	/* # free blocks for non-root */
	printf("files:   %lld\n", st->f_files);    	/* # inodes */
	printf("ffree:   %lld\n", st->f_ffree);    	/* # free inodes */
	printf("favail:  %lld\n", st->f_favail);   	/* # free inodes for non-root */
	printf("fsid:    %lld\n", st->f_fsid);     	/* file system ID */
	printf("flag:    %lld\n", st->f_flag);     	/* mount flags */
	printf("namemax: %lld\n", st->f_namemax);  	/* maximum filename length */

}
LIBSSH2_SFTP_HANDLE * san_open(SANSSH *sanssh, const char *path, long mode)
{
	LIBSSH2_SFTP_HANDLE * handle;
	handle = libssh2_sftp_open(sanssh->sftp, path,
		LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
		LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
		LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
	if (!handle) {
		fprintf(stderr, "Unable to open file\n");
	}
	return handle;
}
LIBSSH2_SFTP_HANDLE * san_opendir(SANSSH *sanssh, const char *path)
{
	LIBSSH2_SFTP_HANDLE * handle;
	handle = libssh2_sftp_opendir(sanssh->sftp, path);
	if (!handle) {
		fprintf(stderr, "Unable to open directory\n");
	}
	return handle;
}
int san_close_handle(LIBSSH2_SFTP_HANDLE *handle)
{
	return libssh2_sftp_close_handle(handle);
}
int san_rename(SANSSH *sanssh, const char *source, const char *destination)
{
	int rc;
	rc = libssh2_sftp_rename(sanssh->sftp, source, destination);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_rename failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
	return rc;

}
int san_delete(SANSSH *sanssh, const char *filename)
{
	int rc;
	rc = libssh2_sftp_unlink(sanssh->sftp, filename);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_unlink failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
	return rc;
}
int san_realpath(SANSSH *sanssh, const char *path, char *target)
{
	int rc;
	rc = libssh2_sftp_realpath(sanssh->sftp, path, target, MAX_PATH);
	if (rc < 0) {
		fprintf(stderr, "libssh2_sftp_readlink failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
	return rc;
}
int san_readdir(SANSSH *sanssh, const char *path)
{
	int rc = 0;
	LIBSSH2_SFTP_HANDLE *handle;
	handle = libssh2_sftp_opendir(sanssh->sftp, path);
	if (!handle) {
		fprintf(stderr, "Unable to open dir with SFTP\n");
		return libssh2_sftp_last_error(sanssh->sftp);
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
int run_command(SANSSH *sanssh,	const char *cmd, char *out, char *err)
{
	LIBSSH2_CHANNEL *channel;
	int rc;
	int bytes = 0;
	int errlen = 0;
	char *errmsg;
	channel = libssh2_channel_open_session(sanssh->ssh);
	if (!channel) {
		int rc = libssh2_session_last_error(sanssh->ssh, &errmsg, NULL, 0);
		fprintf(stderr, "Unable to init ssh chanel: (%d) %s\n", rc, errmsg);
		return 1;
	}

	//libssh2_channel_set_blocking(channel, 0);

	while ((rc = libssh2_channel_exec(channel, cmd)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(sanssh);

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
			waitsocket(sanssh);
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
			waitsocket(sanssh);
		else
			break;
	}
	/* get exit code */
	while ((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(sanssh);
	if (rc == 0)
		rc = libssh2_channel_get_exit_status(channel);
	else
		rc = 127;
	//libssh2_channel_set_blocking(channel, 1);
	libssh2_channel_free(channel);
	return rc;
}

void copy_attributes(SANSSH_ATTRIBUTES* stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs)
{


	//int isdir = LIBSSH2_SFTP_S_ISDIR(attrs->permissions);
	//memset(stbuf, 0, sizeof *stbuf);
	stbuf->flags = attrs->flags;
	stbuf->uid = attrs->uid;
	stbuf->gid = attrs->gid;
	stbuf->mode = 0777 | ((LIBSSH2_SFTP_S_ISDIR(attrs->permissions)) ? S_IFDIR : 0);
	stbuf->filesize = attrs->filesize;
	stbuf->atime = attrs->atime;
	stbuf->mtime = attrs->mtime;
	/*if (LIBSSH2_SFTP_S_ISLNK(attrs->permissions)) {
		int a = attrs->permissions;
	}*/
#if defined(FSP_FUSE_USE_STAT_EX)
	//	if (hidden) {
	//		stbuf->st_flags |= FSP_FUSE_UF_READONLY;
	//	}
#endif
}