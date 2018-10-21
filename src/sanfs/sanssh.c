#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <libssh2_sftp.h>
#include <stdio.h>
#include <assert.h>
#include "util.h"
#include "sanssh.h"
#include "cache.h"

extern size_t			g_sftp_calls;
extern size_t			g_sftp_cached_calls;
extern CRITICAL_SECTION g_ssh_critical_section;


int san_threads(int n, int c)
{
	// guess number of threads in this app
	// n: ThreadCount arg
	// c: number of cores
	// w: winfsp threads = n < 1 ? c : max(2, n) + 1
	// t: total = w + c + main thread
	return (n < 1 ? c : max(2, n)) + c + 2;
}


int file_exists(const char* path)
{
	DWORD attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}
void get_filetype(unsigned long perm, char* filetype)
{
	if (LIBSSH2_SFTP_S_ISLNK(perm))			strcpy_s(filetype, 4, "LNK");
	else if (LIBSSH2_SFTP_S_ISREG(perm))	strcpy_s(filetype, 4, "REG");
	else if (LIBSSH2_SFTP_S_ISDIR(perm))	strcpy_s(filetype, 4, "DIR");
	else if (LIBSSH2_SFTP_S_ISCHR(perm))	strcpy_s(filetype, 4, "CHR");
	else if (LIBSSH2_SFTP_S_ISBLK(perm))	strcpy_s(filetype, 4, "BLK");
	else if (LIBSSH2_SFTP_S_ISFIFO(perm))	strcpy_s(filetype, 4, "FIF");
	else if (LIBSSH2_SFTP_S_ISSOCK(perm))	strcpy_s(filetype, 4, "SOC");
	else									strcpy_s(filetype, 4, "NAN");
}
void copy_attributes(struct fuse_stat *stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs)
{
	//memset(stbuf, 0, sizeof *stbuf);
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
SANSSH * san_init(const char* hostname,	int port, 
	const char* username, const char* pkey)
{
	int rc;
	char *errmsg;
	int errlen;
	SOCKADDR_IN sin;
	HOSTENT *he;
	SOCKET sock;
	LIBSSH2_SESSION* ssh = NULL;
	LIBSSH2_SFTP* sftp = NULL;
	int thread_id = GetCurrentThreadId();

	// initialize windows socket
	WSADATA wsadata;
	rc = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (rc != 0) {
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: WSAStartup failed, rc=%d\n", 
			time_ms(), thread_id, __func__, __LINE__, rc);
		return 0;
	}

	// resolve hostname	
	he = gethostbyname(hostname);
	if (!he) {
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: host not found: %s\n", 
			time_ms(), thread_id, __func__, __LINE__, hostname);
		return 0;
	}
	sin.sin_addr.s_addr = **(int**)he->h_addr_list;
	sin.sin_family = AF_INET;
	sin.sin_port = htons((u_short)port);

	// init ssh
	rc = libssh2_init(0);
	if (rc) {
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: "
			"failed to initialize crypto library, rc=%d\n",
			time_ms(), thread_id, __func__, __LINE__, rc);
		return 0;
	}

	/* create socket  */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	rc = connect(sock, (SOCKADDR*)(&sin), sizeof(SOCKADDR_IN));
	if (rc) {
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: "
			"failed to open socket, connect() rc=%d\n",
			time_ms(), thread_id, __func__, __LINE__, rc);
		return 0;
	}

	/* Create a session instance */
	ssh = libssh2_session_init();
	if (!ssh) {
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: "
			"failed allocate memory for ssh session\n",
			time_ms(), thread_id, __func__, __LINE__);
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
		rc = libssh2_session_last_error(ssh, &errmsg, &errlen, 0);
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: "
			"failed to complete ssh handshake [rc=%d, %s]\n",
			time_ms(), thread_id, __func__, __LINE__, rc, errmsg);
		return 0;
	}

	// authenticate
	rc = libssh2_userauth_publickey_fromfile(ssh, username, NULL, pkey, NULL);
	//while ((rc = libssh2_userauth_publickey_fromfile(
	//	session, username, NULL, pkey, NULL)) == LIBSSH2_ERROR_EAGAIN);
	if (rc) {
		rc = libssh2_session_last_error(ssh, &errmsg, &errlen, 0);
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: "
			"authentication by public key failed [rc=%d, %s]\n",
			time_ms(), thread_id, __func__, __LINE__, rc, errmsg);
		return 0;
	}

	// init sftp channel
	sftp = libssh2_sftp_init(ssh);
	if (!sftp) {
		rc = libssh2_session_last_error(ssh, &errmsg, &errlen, 0);
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: "
			"failed to start sftp session [rc=%d, %s]\n",
			time_ms(), thread_id, __func__, __LINE__, rc, errmsg);
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
	sanssh->thread_id = GetCurrentThreadId();
	return sanssh;
}
int san_finalize()
{
	/* free the hash table contents */
	SANSSH* sanssh, *tmp;
	unsigned int sessions;
	sessions = HASH_COUNT(g_sanssh_pool);
	debug("there are %u ssh sessions\n", sessions);
	/* FIXME: not sure if I can shutdown ssh sessions from different thread 
	 * let's try anyways */
	HASH_ITER(hh, g_sanssh_pool, sanssh, tmp) {
		libssh2_sftp_shutdown(sanssh->sftp);
		libssh2_session_disconnect(sanssh->ssh, "sanssh session disconnected");
		libssh2_session_free(sanssh->ssh);
		libssh2_exit();
		closesocket(sanssh->socket);
		HASH_DEL(g_sanssh_pool, sanssh);
		free(sanssh);
	}
	
	//libssh2_sftp_shutdown(g_sanssh->sftp);
	//libssh2_session_disconnect(g_sanssh->ssh, "g_sanssh disconnected");
	//libssh2_session_free(g_sanssh->ssh);
	//libssh2_exit();
	//closesocket(g_sanssh->socket);
	//WSACleanup();
	//free(g_sanssh);

	return 0;
}
int waitsocket(SANSSH* sanssh)
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
	rc = select((int)sanssh->socket + 1, readfd, writefd, NULL, &timeout);
	return rc;
}

int san_fstat(int fd, struct fuse_stat *stbuf)
{
	int rc = 0;
	LIBSSH2_SFTP_HANDLE* handle = (LIBSSH2_SFTP_HANDLE*)(intptr_t)fd;
	
	//LIBSSH2_SFTP_ATTRIBUTES *attrs = NULL;
	//attrs = malloc(sizeof(LIBSSH2_SFTP_ATTRIBUTES));
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	memset(stbuf, 0, sizeof *stbuf);
	debug("LIBSSH2_SFTP_HANDLE: %zd\n", (intptr_t)handle);
	//lock();
	rc = libssh2_sftp_fstat(handle, &attrs);
	if (rc) {
		debug("ERROR: cannot get fstat from handle: %zd\n", (intptr_t)handle);
		san_error("handle");
	}
	else {
		copy_attributes(stbuf, &attrs);
	}
	//unlock();	
	//free(attrs);

#if STATS
	// stats	
	g_sftp_calls++;
	debug("sftp calls cached/total: %zd/%zd (%.1f%% cached)\n",
		g_sftp_cached_calls, g_sftp_calls,
		(g_sftp_cached_calls * 100 / (double)g_sftp_calls));
#endif
	return rc;
}

int san_stat(const char * path, struct fuse_stat *stbuf)
{
	int rc = 0;
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	//LIBSSH2_SFTP_ATTRIBUTES *attrs = NULL;
	CACHE_ATTRIBUTES* cattrs = NULL;
	memset(stbuf, 0, sizeof *stbuf);
#if USE_CACHE
	cattrs = cache_attributes_find(path);
#endif
	if (!cattrs) {
		//attrs = malloc(sizeof(LIBSSH2_SFTP_ATTRIBUTES));
		//lock();
		// allways follow links
		//rc = libssh2_sftp_stat(g_sanssh->sftp, path, attrs);

		//rc = libssh2_sftp_stat_ex(g_sanssh->sftp, path, (int)strlen(path),
		//	LIBSSH2_SFTP_STAT, attrs);
		//lock();
		
		rc = libssh2_sftp_stat_ex(get_sanssh()->sftp, path, (int)strlen(path),
									LIBSSH2_SFTP_STAT, &attrs);
		//unlock();
		if (rc) {
			printf("rc: %d\n", rc);
			san_error(path);
			//free(attrs);
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
		//unlock();		
	}
	//else {
	//	debug_cached(cattrs->path);
	//	attrs = cattrs->attrs;
	//	g_sftp_cached_calls++;
	//}

	copy_attributes(stbuf, &attrs);
	//print_permissions(path, attrs);

#if STATS
	// stats	
	g_sftp_calls++;
	debug("sftp calls cached/total: %zd/%zd (%.1f%% cached)\n",
		g_sftp_cached_calls, g_sftp_calls,
		(g_sftp_cached_calls * 100 / (double)g_sftp_calls));
#endif
	return rc;
}

int san_statvfs(const char * path, struct fuse_statvfs *stbuf)
{
	int rc = 0;
	LIBSSH2_SFTP_STATVFS stvfs;
	//lock();
	rc = libssh2_sftp_statvfs(get_sanssh()->sftp, path, strlen(path), &stvfs);
	//unlock();
	if (rc) {
		san_error(path);
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

int san_mkdir(const char * path, fuse_mode_t  mode)
{
	int rc;
	//rc = libssh2_sftp_mkdir(g_sanssh->sftp, path,
	//	LIBSSH2_SFTP_S_IRWXU |
	//	LIBSSH2_SFTP_S_IRWXG |
	//	LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH);
	rc = libssh2_sftp_mkdir_ex(get_sanssh()->sftp, path, (int)strlen(path),
		LIBSSH2_SFTP_S_IRWXU |
		LIBSSH2_SFTP_S_IRWXG |
		LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH);
	if (rc) {
		san_error(path);
	}
	return rc ? -1 : 0;
}

int san_rmdir(const char * path)
{
	int rc;
	//rc = libssh2_sftp_rmdir(g_sanssh->sftp, path);
	rc = libssh2_sftp_rmdir_ex(get_sanssh()->sftp, path, (int)strlen(path));
	if (rc) {
		san_error(path);
	}
	return rc ? -1 : 0;
}

DIR * san_opendir(const char *path)
{
	LIBSSH2_SFTP_HANDLE * handle;
	//lock();
	//handle = libssh2_sftp_opendir(g_sanssh->sftp, path);
	handle = libssh2_sftp_open_ex(get_sanssh()->sftp, path, (int)strlen(path),
									0, 0, LIBSSH2_SFTP_OPENDIR);
	//unlock();
	if (!handle) {
		san_error(path);
		return 0;
	}
	debug("LIBSSH2_SFTP_HANDLE: %zd open %s\n", (ssize_t)handle, path);

	size_t pathlen = strlen(path);
	if (0 < pathlen && '/' == path[pathlen - 1])
		pathlen--;

	DIR *dirp = malloc(sizeof *dirp + pathlen + 2); /* sets errno */
	if (0 == dirp) {
		return 0;
	}
	
	memset(dirp, 0, sizeof *dirp);
	dirp->handle = handle;
	memcpy(dirp->path, path, pathlen);
	dirp->path[pathlen + 0] = '/';
	dirp->path[pathlen + 1] = '\0';

	return dirp;

}

int san_dirfd(DIR *dirp)
{
	return (int)(intptr_t)dirp->handle;
}

void san_rewinddir(DIR *dirp)
{
	//lock();
	libssh2_sftp_rewind(dirp->handle);
	//unlock();
}

struct dirent *san_readdir(DIR *dirp)
{
	struct fuse_stat *stbuf = &dirp->de.d_stat;
	memset(stbuf, 0, sizeof *stbuf);
	int rc;
	char fname[512];
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	assert(dirp->handle);
	//lock();
	rc = libssh2_sftp_readdir(dirp->handle, fname, sizeof(fname), &attrs);
	//unlock();
	g_sftp_calls++;

	if (rc > 0) {
		// resolve symbolic links
		if (attrs.permissions & LIBSSH2_SFTP_S_IFLNK) {
			char fullpath[MAX_PATH];
			strcpy_s(fullpath, MAX_PATH, dirp->path);
			int pathlen = (int)strlen(dirp->path);
			if (!(pathlen > 0 && dirp->path[pathlen - 1] == '/'))
				strcat_s(fullpath, 2, "/");
			strcat_s(fullpath, FILENAME_MAX, fname);
			memset(&attrs, 0, sizeof attrs);
			//lock();
			//rc = libssh2_sftp_stat(g_sanssh->sftp, fullpath, &attrs);
			rc = libssh2_sftp_stat_ex(get_sanssh()->sftp, fullpath,
							(int)strlen(fullpath), LIBSSH2_SFTP_STAT, &attrs);
			//unlock();
			if (rc) {
				san_error(fullpath);
			}
			////printf("fullpath: %s\n", fullpath);
			//int realpathlen = san_realpath(fullpath, realpath);
			////printf("realpath: %s\n", realpath);
			//int r;
			//memset(&attrs, 0, sizeof attrs);
			//lock();
			//r = libssh2_sftp_lstat(g_sanssh->sftp, realpath, &attrs);
			//unlock();
			//if (r) {
			//	sftp_error(g_sanssh, realpath, r);
			//}
		}
	}
	else {
		// no more files
		return 0;
	}
	
	copy_attributes(stbuf, &attrs);
	strcpy_s(dirp->de.d_name, FILENAME_MAX, fname);
#if STATS
	// stats	
	debug("sftp calls cached/total: %zd/%zd (%.1f%% cached)\n",
			g_sftp_cached_calls, g_sftp_calls, 
			(g_sftp_cached_calls*100/(double)g_sftp_calls));
#endif

	return &dirp->de;

}

int san_closedir(DIR *dirp)
{
	int fd = (int)(intptr_t)dirp->handle;
	san_close(fd);
	free(dirp);
	dirp = NULL;
	return 0;
}

void mode_human(unsigned long mode, char* human)
{
	human[0] = mode & LIBSSH2_SFTP_S_IRUSR ? 'r' : '-';
	human[1] = mode & LIBSSH2_SFTP_S_IWUSR ? 'w' : '-';
	human[2] = mode & LIBSSH2_SFTP_S_IXUSR ? 'x' : '-';
	human[3] = mode & LIBSSH2_SFTP_S_IRGRP ? 'r' : '-';
	human[4] = mode & LIBSSH2_SFTP_S_IWGRP ? 'w' : '-';
	human[5] = mode & LIBSSH2_SFTP_S_IXGRP ? 'x' : '-';
	human[6] = mode & LIBSSH2_SFTP_S_IROTH ? 'r' : '-';
	human[7] = mode & LIBSSH2_SFTP_S_IWOTH ? 'w' : '-';
	human[8] = mode & LIBSSH2_SFTP_S_IXOTH ? 'x' : '-';
	human[9] = '\0';
}

void print_permissions(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs)
{
	char perm[10];
	mode_human(attrs->permissions, perm);
	printf("%s %d %d %s\n", perm, attrs->uid, attrs->gid, path);
}

void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs)
{
	printf("path:  %s\n", path);
	printf("flags: %ld\n", attrs->flags);
	printf("size:  %zd\n", attrs->filesize);
	printf("uid:   %ld\n", attrs->uid);
	printf("gid:   %ld\n", attrs->gid);
	printf("mode:  %ld\n", attrs->permissions);
	printf("atime: %ld\n", attrs->atime);
	printf("mtime: %ld\n", attrs->mtime);
}

void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st)
{
	printf("path:    %s\n", path);
	printf("bsize:   %zd\n", st->f_bsize);    	/* file system block size */
	printf("frsize:  %zd\n", st->f_frsize);   	/* fragment size */
	printf("blocks:  %zd\n", st->f_blocks);   	/* size of fs in f_frsize units */
	printf("bfree:   %zd\n", st->f_bfree);    	/* # free blocks */
	printf("bavail:  %zd\n", st->f_bavail);   	/* # free blocks for non-root */
	printf("files:   %zd\n", st->f_files);    	/* # inodes */
	printf("ffree:   %zd\n", st->f_ffree);    	/* # free inodes */
	printf("favail:  %zd\n", st->f_favail);   	/* # free inodes for non-root */
	printf("fsid:    %zd\n", st->f_fsid);     	/* file system ID */
	printf("flag:    %zd\n", st->f_flag);     	/* mount flags */
	printf("namemax: %zd\n", st->f_namemax);  	/* maximum filename length */

}

int san_open(const char *path, long mode)
{
	LIBSSH2_SFTP_HANDLE * handle;
	//lock();
	//handle = libssh2_sftp_open(g_sanssh->sftp, path, LIBSSH2_FXF_READ, 0);
	handle = libssh2_sftp_open_ex(get_sanssh()->sftp, path, (int)strlen(path),
							LIBSSH2_FXF_READ, mode, LIBSSH2_SFTP_OPENFILE);

	//unlock();
	//handle = libssh2_sftp_open(g_sanssh->sftp, path,
	//	LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
	//	LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
	//	LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
	if (!handle) {
		san_error(path);
	}
	debug("LIBSSH2_SFTP_HANDLE: %zd: %s\n", (ssize_t)handle, path);
	return(int)(intptr_t)handle;
}

ssize_t san_read(int fd, void *buf, size_t nbyte, fuse_off_t offset)
{
	//size_t thread = GetCurrentThreadId();
	LIBSSH2_SFTP_HANDLE* handle = (LIBSSH2_SFTP_HANDLE*)(intptr_t)fd;
	debug("LIBSSH2_SFTP_HANDLE: %zd read from descriptor: %d\n", (intptr_t)handle, fd);
	ssize_t curpos;
	//lock();
	curpos = libssh2_sftp_tell64(handle);
	if (offset != curpos)
		libssh2_sftp_seek64(handle, offset);
	//printf("thread buffer size    offset         bytes read     bytes written  total bytes\n");
	ssize_t bytesread = 0;
	ssize_t total = 0;
	size_t size = nbyte;
	size_t off = 0;
	char *mem = malloc(size);
	while (size) {		
		bytesread = libssh2_sftp_read(handle, mem, size);
		if (bytesread < 0) {
			debug("ERROR: Unable to read file\n");
			return -1;
		} else if (bytesread == 0) {
			break;			
		}
		//memcpy(buffer, selectedText + offset, size);
		//return strlen(selectedText) - offset;
		memcpy((char*)buf + off, mem, bytesread);
		total += bytesread;
		off += bytesread;
		//printf("%-7ld%-15d%-15ld%-15d%-15d%-15ld\n",thread,
		//	size, offset, bytesread, bytesread, total);
		size -= bytesread;
	}
	//unlock();
	free(mem);

	return total;
}

int san_close(int fd)
{
	int rc;
	LIBSSH2_SFTP_HANDLE* handle = (LIBSSH2_SFTP_HANDLE*)(intptr_t)fd;
	//lock();
	// I don't know what thread is calling this function
	// need to protect
	EnterCriticalSection(&g_ssh_critical_section);
	rc = libssh2_sftp_close_handle(handle);
	LeaveCriticalSection(&g_ssh_critical_section);
	//unlock();
	debug("LIBSSH2_SFTP_HANDLE: %zd closed\n", (intptr_t)handle);
	handle = NULL;
	return rc;
}

int san_rename(const char *source, const char *destination)
{
	int rc;
	//rc = libssh2_sftp_rename(g_sanssh->sftp, source, destination);
	rc = libssh2_sftp_rename_ex(get_sanssh()->sftp,
				source, (int)strlen(source), 
				destination, (int)strlen(destination), 
				LIBSSH2_SFTP_RENAME_OVERWRITE | 
				LIBSSH2_SFTP_RENAME_ATOMIC | 
				LIBSSH2_SFTP_RENAME_NATIVE);
	if (rc) {
		san_error(source);
	}
	return rc ? -1 : 0;

}

int san_unlink(const char *path)
{
	int rc;
	//rc = libssh2_sftp_unlink(g_sanssh->sftp, path);
	rc = libssh2_sftp_unlink_ex(get_sanssh()->sftp, path, (int)strlen(path));
	if (rc) {
		san_error(path);
	}
	return rc ? -1 : 0;
}

int san_truncate(const char *path, fuse_off_t size)
{
	return -1;
	
	//HANDLE h = CreateFileA(path,
	//	FILE_WRITE_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	//	0,
	//	OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	//if (INVALID_HANDLE_VALUE == h)
	//	return error();

	//int res = san_ftruncate((int)(intptr_t)h, size);

	//CloseHandle(h);

	//return res;
	
}

int san_ftruncate(int fd, fuse_off_t size)
{
	return -1;
	//HANDLE h = (HANDLE)(intptr_t)fd;
	//FILE_END_OF_FILE_INFO EndOfFileInfo;

	//EndOfFileInfo.EndOfFile.QuadPart = size;

	//if (!SetFileInformationByHandle(h, FileEndOfFileInfo, &EndOfFileInfo, sizeof EndOfFileInfo))
	//	return error();

	//return 0;
}

int san_fsync(int fd)
{
	//HANDLE h = (HANDLE)(intptr_t)fd;

	//if (!FlushFileBuffers(h))
	//	return error();

	return 0;
}

int utime(const char *path, const struct fuse_utimbuf *timbuf)
{
	return -1;
	//if (0 == timbuf)
	//	return utimensat(AT_FDCWD, path, 0, AT_SYMLINK_NOFOLLOW);
	//else
	//{
	//	struct fuse_timespec times[2];
	//	times[0].tv_sec = timbuf->actime;
	//	times[0].tv_nsec = 0;
	//	times[1].tv_sec = timbuf->modtime;
	//	times[1].tv_nsec = 0;
	//	return utimensat(AT_FDCWD, path, times, AT_SYMLINK_NOFOLLOW);
	//}
}

int utimensat(int dirfd, const char *path, const struct fuse_timespec times[2], int flag)
{
	return -1;
	///* ignore dirfd and assume that it is always AT_FDCWD */
	///* ignore flag and assume that it is always AT_SYMLINK_NOFOLLOW */

	//HANDLE h = CreateFileA(path,
	//	FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	//	0,
	//	OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	//if (INVALID_HANDLE_VALUE == h)
	//	return error();

	//UINT64 LastAccessTime, LastWriteTime;
	//if (0 == times)
	//{
	//	FILETIME FileTime;
	//	GetSystemTimeAsFileTime(&FileTime);
	//	LastAccessTime = LastWriteTime = *(PUINT64)&FileTime;
	//}
	//else
	//{
	//	FspPosixUnixTimeToFileTime((void *)&times[0], &LastAccessTime);
	//	FspPosixUnixTimeToFileTime((void *)&times[1], &LastWriteTime);
	//}

	//int res = SetFileTime(h,
	//	0, (PFILETIME)&LastAccessTime, (PFILETIME)&LastWriteTime) ? 0 : error();

	//CloseHandle(h);

	//return res;
}

int setcrtime(const char *path, const struct fuse_timespec *tv)
{
	(void*)path;
	return -1;
	//HANDLE h = CreateFileA(path,
	//	FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	//	0,
	//	OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	//if (INVALID_HANDLE_VALUE == h)
	//	return error();

	//UINT64 CreationTime;
	//FspPosixUnixTimeToFileTime((void *)tv, &CreationTime);

	//int res = SetFileTime(h,
	//	(PFILETIME)&CreationTime, 0, 0) ? 0 : error();

	//CloseHandle(h);

	//return res;
}
//int san_realpath(const char *path, char *target)
//{
//	int rc;
//	lock();
//	rc = libssh2_sftp_realpath(g_sanssh->sftp, path, target, MAX_PATH);
//	unlock();
//	if (rc < 0) {
//		sftp_error(g_sanssh, path, rc);
//	}
//	return rc;
//}

int run_command(const char *cmd, char *out, char *err)
{
	LIBSSH2_CHANNEL *channel;
	int rc;
	ssize_t bytesread = 0;
	size_t offset = 0;
	char *errmsg;
	//lock();
	channel = libssh2_channel_open_session(get_sanssh()->ssh);
	//unlock();
	if (!channel) {
		rc = libssh2_session_last_error(get_sanssh()->ssh, &errmsg, NULL, 0);
		debug("ERROR: unable to init ssh chanel, rc=%d, %s\n", rc, errmsg);
		return 1;
	}

	libssh2_channel_set_blocking(channel, 0);
	while ((rc = libssh2_channel_exec(channel, cmd)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(get_sanssh());

	if (rc != 0) {
		rc = libssh2_session_last_error(get_sanssh()->ssh, &errmsg, NULL, 0);
		debug("ERROR: unable to execute command, rc=%d, %s\n", rc, errmsg);
		goto finish;
	}

	/* read stdout */
	out[0] = '\0';
	for (;;) {
		do {
			char buffer[0x4000];
			//lock();
			bytesread = libssh2_channel_read(channel, buffer, sizeof(buffer));
			//unlock();
			if (bytesread > 0) {
				//strncat(out, buffer, bytesread);
				memcpy(out + offset, buffer, bytesread);
				offset += bytesread;
			}
		} while (bytesread > 0);

		if (bytesread == LIBSSH2_ERROR_EAGAIN)
			waitsocket(get_sanssh());
		else
			break;
	}

	/* read stderr */
	err[0] = '\0';
	offset = 0;
	for (;;) {
		do {
			char buffer[0x4000];
			//lock();
			bytesread = libssh2_channel_read_stderr(channel, buffer, sizeof(buffer));
			//unlock();
			if (bytesread > 0) {
				//strncat(err, buffer, bytesread);
				memcpy(out + offset, buffer, bytesread);
				offset += bytesread;
			}
		} while (bytesread > 0);

		if (bytesread == LIBSSH2_ERROR_EAGAIN)
			waitsocket(get_sanssh());
		else
			break;
	}
	/* get exit code */
	while ((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(get_sanssh());
	if (rc == 0)
		rc = libssh2_channel_get_exit_status(channel);
	else
		rc = 127;

finish:
	libssh2_channel_set_blocking(channel, 1);
	libssh2_channel_free(channel);
	return (int)rc;
}

void sanssh_pool_add(SANSSH *value)
{
	HASH_ADD_INT(g_sanssh_pool, thread_id, value);
	debug("new ssh connection added\n");
}

SANSSH * sanssh_pool_find(int thread_id)
{
	SANSSH *value = NULL;
	HASH_FIND_INT(g_sanssh_pool, &thread_id, value);
	return value;
}

SANSSH* get_sanssh(void)
{
	int current_thread_id = GetCurrentThreadId();
	SANSSH *sanssh = sanssh_pool_find(current_thread_id);
	if (!sanssh) {
		size_t t = time_ms();
		sanssh = san_init(g_cmd_args->host, g_cmd_args->port,
			g_cmd_args->user, g_cmd_args->pkey);
		if (sanssh) {
			EnterCriticalSection(&g_ssh_critical_section);
			sanssh_pool_add(sanssh);
			LeaveCriticalSection(&g_ssh_critical_section);
			debug("time to create new ssh session: %zd secs\n", time_ms() - t);
		}
		
	}
	return sanssh;
}