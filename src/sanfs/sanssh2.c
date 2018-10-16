#include <stdio.h>
#include <assert.h>
#include "util.h"
#include "sanssh2.h"
#include "cache.h"

extern CRITICAL_SECTION g_critical_section;
extern size_t			g_sftp_calls;
extern size_t			g_sftp_cached_calls;


void lock()
{
	EnterCriticalSection(&g_critical_section);
}
void unlock()
{
	LeaveCriticalSection(&g_critical_section);
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
void copy_attributes(struct fuse_stat *stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs)
{
	memset(stbuf, 0, sizeof *stbuf);
	stbuf->st_uid = attrs->uid;
	stbuf->st_gid = attrs->gid;
	stbuf->st_mode = attrs->permissions;
	stbuf->st_size = attrs->filesize;
	stbuf->st_birthtim.tv_sec = attrs->mtime;
	stbuf->st_atim.tv_sec = attrs->atime;
	stbuf->st_mtim.tv_sec = attrs->mtime;
	stbuf->st_ctim.tv_sec = attrs->mtime;
	stbuf->st_nlink = 1;
}
SANSSH * san_init(const char* hostname,	const char* username, 
	const char* pkey, char* error)
{
	int rc;
	char *errmsg;
	SOCKADDR_IN sin;
	HOSTENT *he;
	int port = 22;
	SOCKET sock;
	LIBSSH2_SESSION* ssh = NULL;
	LIBSSH2_SFTP* sftp = NULL;

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
		rc = libssh2_session_last_error(ssh, &errmsg, NULL, 0);
		snprintf(error, "ssh initialization %d: %s", rc, errmsg);
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
	sanssh->socket = sock;
	sanssh->ssh = ssh;
	sanssh->sftp = sftp;
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
	libssh2_session_set_blocking(sanssh->ssh, 1);

	/* Request a file via SFTP */
	handle = libssh2_sftp_open(sanssh->sftp, remotefile, LIBSSH2_FXF_READ, 0);
	if (!handle) {
		fprintf(stderr, "Unable to open file with SFTP: %ld\n",
			libssh2_sftp_last_error(sanssh->sftp));
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
		rc = libssh2_sftp_read(handle, mem, bufsize);
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

	libssh2_sftp_close(handle);
	return 0;
}
int san_read_async(SANSSH *sanssh, const char * remotefile, const char * localfile)
{
	LIBSSH2_SFTP_HANDLE *handle;
	int rc;
	int spin = 0;
	size_t total = 0;

	/* Since we have set non-blocking, tell libssh2 we are non-blocking */
	libssh2_session_set_blocking(sanssh->ssh, 0);

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
	printf("bytes     : %ld\n", total);
	printf("spin      : %d\n", spin);
	printf("duration  : %ld secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)duration));

	libssh2_sftp_close(handle);
	return 0;
}
int san_mkdir(SANSSH *sanssh, const char * path)
{
	int rc;
	rc = libssh2_sftp_mkdir(sanssh->sftp, path,
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
int san_stat(SANSSH *sanssh, const char * path, struct fuse_stat *stbuf)
{
	return san_lstat(sanssh, path, stbuf);
}
int san_lstat(SANSSH *sanssh, const char * path, struct fuse_stat *stbuf)
{
	int rc = 0;
	LIBSSH2_SFTP_ATTRIBUTES *attrs = NULL;
	CACHE_ATTRIBUTES* cattrs = NULL;
#if USE_CACHE
	cattrs = cache_attributes_find(path);
#endif
	if (!cattrs) {
		debug(path);
		attrs = malloc(sizeof(LIBSSH2_SFTP_ATTRIBUTES));
		lock();
		rc = libssh2_sftp_lstat(sanssh->sftp, path, attrs);
		unlock();
		
		if (rc) {
			sftp_error(sanssh, path, rc);
			free(attrs);
		}
		else {
#if USE_CACHE
			// added to cache
			cattrs = malloc(sizeof(CACHE_ATTRIBUTES));
			strcpy(cattrs->path, path);
			cattrs->attrs = attrs;
			cache_attributes_add(cattrs);
#endif
		}
	}
	else {
		debug_cached(cattrs->path);
		attrs = cattrs->attrs;
		g_sftp_cached_calls++;
	}

	copy_attributes(stbuf, attrs);

	// stats	
	g_sftp_calls++;
	printf("sftp calls cached/total: %ld/%ld\n", g_sftp_cached_calls, g_sftp_calls);
	return rc;
}
int san_statvfs(SANSSH *sanssh, const char * path, struct fuse_statvfs *stbuf)
{
	int rc = 0;
	LIBSSH2_SFTP_STATVFS stvfs;
	lock();
	rc = libssh2_sftp_statvfs(sanssh->sftp, path, strlen(path), &stvfs);
	unlock();
	if (rc) {
		sftp_error(sanssh, path, rc);
	}
	memset(stbuf, 0, sizeof *stbuf);
	stbuf->f_bsize = stvfs.f_bsize;
	stbuf->f_frsize = stvfs.f_frsize;
	stbuf->f_blocks = stvfs.f_blocks;
	stbuf->f_bfree = stvfs.f_bfree;
	stbuf->f_bavail = stvfs.f_bavail;
	stbuf->f_fsid = stvfs.f_fsid;
	stbuf->f_namemax = stvfs.f_namemax;
	return rc;
}
DIR * san_opendir(SANSSH *sanssh, const char *path)
{
	LIBSSH2_SFTP_HANDLE * handle;
	lock();
	handle = libssh2_sftp_opendir(sanssh->sftp, path);
	unlock();
	if (!handle) {
		sftp_error(sanssh, path, 0);
	}
	
	size_t pathlen = strlen(path);
	if (0 < pathlen && '/' == path[pathlen - 1])
		pathlen--;

	DIR *dirp = malloc(sizeof *dirp + pathlen + 3); /* sets errno */
	if (0 == dirp)
		return 0;
	
	memset(dirp, 0, sizeof *dirp);
	dirp->h = handle;
	memcpy(dirp->path, path, pathlen);
	dirp->path[pathlen + 0] = '/';
	dirp->path[pathlen + 1] = '*';
	dirp->path[pathlen + 2] = '\0';

	return dirp;

}
int san_dirfd(DIR *dirp)
{
	return (int)dirp->h;
}
void san_rewinddir(DIR *dirp)
{
	lock();
	libssh2_sftp_rewind(dirp->h);
	unlock();
}
struct dirent *san_readdir(DIR *dirp)
{
	struct fuse_stat *stbuf = &dirp->de.d_stat;
	int rc;
	char fname[512];
	char longentry[512];
	LIBSSH2_SFTP_ATTRIBUTES* attrs;
	CACHE_ATTRIBUTES* cattrs = NULL;
//#if USE_CACHE
//	cattrs = cache_attributes_find(dirp->path);
//#endif
	if (!cattrs) {
		debug(dirp->path);
		attrs = malloc(sizeof(LIBSSH2_SFTP_ATTRIBUTES));
		assert(dirp->h);
		lock();
		rc = libssh2_sftp_readdir_ex(dirp->h, fname, sizeof(fname), longentry, sizeof(longentry), attrs);
		unlock();
		if (rc > 0) {
			if (strcmp(fname, "bin") == 0) {
				// bin dir
				//printf("bin:\n");
			}
//#if USE_CACHE
//			// added to cache
//			cattrs = malloc(sizeof(CACHE_ATTRIBUTES));
//			strcpy(cattrs->path, dirp->path);
//			cattrs->attrs = attrs;
//			cache_attributes_add(cattrs);
//#endif
		}
		else {
			// no more files
			return 0;
		}
	}
	else {
		debug_cached(cattrs->path);
		attrs = cattrs->attrs;
		g_sftp_cached_calls++;
	}
	
	copy_attributes(stbuf, attrs);
	strcpy(dirp->de.d_name, fname);

	// stats	
	g_sftp_calls++;
	printf("sftp calls cached/total: %ld/%ld\n", g_sftp_cached_calls, g_sftp_calls);


	return &dirp->de;

}
int san_closedir(DIR *dirp)
{
	int rc = san_close_handle(dirp->h);
	free(dirp);
	return rc;
}
void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs)
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
void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st)
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
int san_close_handle(LIBSSH2_SFTP_HANDLE *handle)
{
	int rc;
	lock();
	rc = libssh2_sftp_close_handle(handle);
	unlock();
	return rc;
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
int run_command(SANSSH *sanssh,	const char *cmd, char *out, char *err)
{
	LIBSSH2_CHANNEL *channel;
	int rc;
	int bytes = 0;
	int errlen;
	char *errmsg;
	channel = libssh2_channel_open_session(sanssh->ssh);
	if (!channel) {
		int rc = libssh2_session_last_error(sanssh->ssh, &errmsg, NULL, 0);
		fprintf(stderr, "Unable to init ssh chanel: (%d) %s\n", rc, errmsg);
		return 1;
	}

	libssh2_channel_set_blocking(channel, 0);

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
	libssh2_channel_set_blocking(channel, 1);
	libssh2_channel_free(channel);
	return rc;
}

int run_command_shell(SANSSH *sanssh, const char *cmd, char *out, char *err)
{
	LIBSSH2_CHANNEL *channel;
	int rc;
	int bytes = 0;
	char *errmsg;

	if (!(channel = libssh2_channel_open_session(sanssh->ssh))) {
		int rc = libssh2_session_last_error(sanssh->ssh, &errmsg, NULL, 0);
		fprintf(stderr, "Unable to init ssh chanel: (%d) %s\n", rc, errmsg);
		return 1;
	}

	/* set env var */
	libssh2_channel_setenv(channel, "FOO", "bar");

	/* Request a terminal with 'vanilla' terminal emulation
	* See /etc/termcap for more options
	*/
	if (libssh2_channel_request_pty(channel, "vanilla")) {
		int rc = libssh2_session_last_error(sanssh->ssh, &errmsg, NULL, 0);
		fprintf(stderr, "Failed requesting pty: (%d) %s\n", rc, errmsg);
		return 1;
	}

	/* Open a SHELL on that pty */
	if (libssh2_channel_shell(channel)) {
		int rc = libssh2_session_last_error(sanssh->ssh, &errmsg, NULL, 0);
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

