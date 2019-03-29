#include <libssh2_sftp.h> 
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "util.h"
#include "golddrive.h"


SANSSH * san_init_ssh(const char* hostname,	int port, const char* username, const char* pkey)
{
	int rc;
	char *errmsg;
	int errlen;
	SOCKADDR_IN sin;
	HOSTENT *he;
	SOCKET sock;
	LIBSSH2_SESSION* ssh = NULL;
	LIBSSH2_CHANNEL *channel;
	LIBSSH2_SFTP* sftp = NULL;
	int thread = GetCurrentThreadId();

	// initialize windows socket
	WSADATA wsadata;
	rc = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (rc != 0) {
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: WSAStartup failed, rc=%d\n", 
			time_mu(), thread, __func__, __LINE__, rc);
		return 0;
	}

	// resolve hostname	
	he = gethostbyname(hostname);
	if (!he) {
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: host not found: %s\n", 
			time_mu(), thread, __func__, __LINE__, hostname);
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
			time_mu(), thread, __func__, __LINE__, rc);
		return 0;
	}

	/* create socket  */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	rc = connect(sock, (SOCKADDR*)(&sin), sizeof(SOCKADDR_IN));
	if (rc) {
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: "
			"failed to open socket, connect() rc=%d\n",
			time_mu(), thread, __func__, __LINE__, rc);
		return 0;
	}

	/* Create a session instance */
	ssh = libssh2_session_init();
	if (!ssh) {
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: "
			"failed allocate memory for ssh session\n",
			time_mu(), thread, __func__, __LINE__);
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
			time_mu(), thread, __func__, __LINE__, rc, errmsg);
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
			time_mu(), thread, __func__, __LINE__, rc, errmsg);
		return 0;
	}

	// init ssh channel
	channel = libssh2_channel_open_session(ssh);
	if (!channel) {
		rc = libssh2_session_last_error(ssh, &errmsg, &errlen, 0);
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: "
			"failed to start ssh channel [rc=%d, %s]\n",
			time_mu(), thread, __func__, __LINE__, rc, errmsg);
		return 0;
	}
	

	// init sftp channel
	sftp = libssh2_sftp_init(ssh);
	if (!sftp) {
		rc = libssh2_session_last_error(ssh, &errmsg, &errlen, 0);
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: "
			"failed to start sftp session [rc=%d, %s]\n",
			time_mu(), thread, __func__, __LINE__, rc, errmsg);
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
	g_ssh = malloc(sizeof(SANSSH));
	g_ssh->socket = sock;
	g_ssh->ssh = ssh;
	g_ssh->sftp = sftp;
	g_ssh->thread = GetCurrentThreadId();
	//if (!InitializeCriticalSectionAndSpinCount(&sanssh->lock, 0x00000400))
	//	return 0;
	return g_ssh;
}

int san_finalize(void)
{
	/* free the hash table contents */
	//SANSSH* ssh, *tmp;
	//unsigned int sessions;
	/* FIXME: not sure if I can shutdown ssh sessions from different thread
	* let's try anyways */
	//HASH_ITER(hh, g_ssh_ht, ssh, tmp) {
		libssh2_sftp_shutdown(g_ssh->sftp);
		libssh2_session_disconnect(g_ssh->ssh, "ssh session disconnected");
		libssh2_session_free(g_ssh->ssh);
		libssh2_exit();
		closesocket(g_ssh->socket);
		//HASH_DEL(g_ssh_ht, ssh);
		free(g_ssh);
	//}

	//libssh2_sftp_shutdown(g_g_ssh->sftp);
	//libssh2_session_disconnect(g_ssh->ssh, "g_ssh disconnected");
	//libssh2_session_free(g_ssh->ssh);
	//libssh2_exit();
	//closesocket(g_ssh->socket);
	//WSACleanup();
	//free(g_ssh);

	return 0;
}

void *f_init(struct fuse_conn_info *conn, struct fuse_config *conf)
{
	conn->want |= (conn->capable & FUSE_CAP_READDIRPLUS);
	return fuse_get_context()->private_data;
}

int waitsocket(SANSSH* ssh)
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
	FD_SET(ssh->socket, &fd);
	/* now make sure we wait in the correct direction */
	dir = libssh2_session_block_directions(ssh->ssh);
	if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
		readfd = &fd;
	if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
		writefd = &fd;
	rc = select((int)ssh->socket + 1, readfd, writefd, NULL, &timeout);
	return rc;
}

int f_getattr(const char *path, struct fuse_stat *stbuf, struct fuse_file_info *fi)
{
	debug("%s\n", path);
	int rc;
	const char *p = path;
	if (fi) {		
		size_t fd = fi_fd(fi);
		SAN_HANDLE* sh = (SAN_HANDLE*)fd;
		p = sh->path;
		info("FSTAT HANDLE: %zu:%zu, %s\n", (size_t)sh, (size_t)sh->handle, sh->path);
	}
	rc = san_stat(p, stbuf);

#if LOGLEVEL==DEBUG
	//debug("end %d %s\n", rc, path);
	char perm[10];
	mode_human(stbuf->st_mode, perm);
	debug("%s %d %d %s\n", perm, stbuf->st_uid, stbuf->st_gid, path);
#endif

	return rc;
}

int san_stat(const char * path, struct fuse_stat *stbuf)
{
	int rc = 0;
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	memset(stbuf, 0, sizeof *stbuf);
	//debug("before realpath: %s\n", path);
	//if (strncmp(path, "/~", 2) == 0) {
	//	printf("\n");
	//}
	realpath(path);
	debug("%s\n", path);

#if USE_CACHE
	CACHE_ATTRIBUTES* cattrs = NULL;
	memset(&attrs, 0, sizeof attrs);
	cattrs = ht_attributes_find(p);
	if (!cattrs) {			
#endif
		assert(g_ssh);
		assert(g_ssh->sftp);
		
		lock();
		rc = libssh2_sftp_stat_ex(g_ssh->sftp, path, (int)strlen(path), 
			LIBSSH2_SFTP_STAT, &attrs);		
		g_sftp_calls++;		
		unlock();
		if (rc) {
			san_error(path);
		}
#if USE_CACHE
		// added to cache
		cattrs = malloc(sizeof(CACHE_ATTRIBUTES));
		strcpy(cattrs->path, p);
		cattrs->attrs = attrs; /* shallow copy is ok */
		ht_attributes_add(cattrs);
	}
	else {
		debug("CACHE: %s\n", cattrs->path);
		attrs = cattrs->attrs;
		g_sftp_cached_calls++;
	}
#endif
	copy_attributes(stbuf, &attrs);
	//print_permissions(path, &attrs);

#if STATS
	// stats	
	debug("sftp calls cached/total: %zd/%zd (%.1f%% cached)\n",
		g_sftp_cached_calls, g_sftp_calls,
		(g_sftp_cached_calls * 100 / (double)g_sftp_calls));
#endif
	debug("rc: %d\n", -rc);
	return -rc;
}

//int san_fstat(size_t fd, struct fuse_stat *stbuf)
//{
//	int rc = 0;
//	SAN_HANDLE* sh = (SAN_HANDLE*)fd;
//	LIBSSH2_SFTP_HANDLE* handle = sh->handle;
//	LIBSSH2_SFTP_ATTRIBUTES attrs;
//	memset(stbuf, 0, sizeof *stbuf);
//	
//	lock();
//	rc = libssh2_sftp_fstat(handle, &attrs);
//	g_sftp_calls++;
//	if (rc) {
//		error("cannot get fstat from handle: %zu:%zu\n", (size_t)sh, (size_t)handle);
//		san_error("wrong handle");
//	}
//	else {
//		copy_attributes(stbuf, &attrs);
//	}
//	unlock();	
//	//free(attrs);
//
//#if STATS
//	// stats	
//	g_sftp_calls++;
//	debug("sftp calls cached/total: %zd/%zd (%.1f%% cached)\n",
//		g_sftp_cached_calls, g_sftp_calls,
//		(g_sftp_cached_calls * 100 / (double)g_sftp_calls));
//#endif
//	return rc ? -1 : 0;
//}

int f_statfs(const char * path, struct fuse_statvfs *stbuf)
{
	realpath(path);
	info("%s\n", path);
	int rc = 0;
	LIBSSH2_SFTP_STATVFS stvfs;

	lock();
	rc = libssh2_sftp_statvfs(g_ssh->sftp, path, strlen(path), &stvfs);
	g_sftp_calls++;
	if (rc) {
		san_error(path);
	}
	unlock();
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

int f_opendir(const char *path, struct fuse_file_info *fi)
{
	realpath(path);
	info("%s\n", path);
	DIR *dirp = fi_dirp(fi);
	SAN_HANDLE *sh;
	unsigned int mode = 0;
	
	
	sh = san_open(path, FILE_ISDIR, mode, fi);
	if (!sh)
		return -1;

	size_t pathlen = strlen(path);
	if (0 < pathlen && '/' == path[pathlen - 1])
		pathlen--;
	dirp = malloc(sizeof *dirp + pathlen + 2); /* sets errno */
	if (0 == dirp) 
		return -1;
	memset(dirp, 0, sizeof *dirp);
	dirp->san_handle = sh;
	memcpy(dirp->path, path, pathlen);
	dirp->path[pathlen + 0] = '/';
	dirp->path[pathlen + 1] = '\0';
	
	int rc = (fi_setdirp(fi, dirp), 0);

	return rc;
}

size_t san_dirfd(DIR *dirp)
{
	return (size_t)dirp->san_handle;
}

SAN_HANDLE * san_open(const char *path, int is_dir, unsigned int mode, 
	struct fuse_file_info *fi)
{
	LIBSSH2_SFTP_HANDLE* handle;
	SAN_HANDLE *sh = malloc(sizeof(SAN_HANDLE));
	debug("OPEN SAN_HANDLE: %zu, %s\n", (size_t)sh, path);
	
	unsigned int pflags;
	if ((fi->flags & O_ACCMODE) == O_RDONLY) {
		pflags = LIBSSH2_FXF_READ;
	}
	else if ((fi->flags & O_ACCMODE) == O_WRONLY) {
		pflags = LIBSSH2_FXF_WRITE;
		if (!is_dir) {
			// check for hard link
			char cmd[MAX_PATH + 10], out[100], err[100];
			sprintf_s(cmd, sizeof cmd, "stat -c%%h %s\n", path);
			run_command(cmd, out, err);
			int hlinks = atoi(out);
			if (hlinks > 1) {
				//error("opening for writing hard linked file: %s\n"
				//	"number of links: %d\n", path, hlinks);
				
				// backup file
				char backup[PATH_MAX];
				sprintf_s(backup, sizeof backup, "%s.%zd.hlink", path, time_mu());
				f_rename(path, backup, 0);

				// create new file
				sprintf_s(cmd, sizeof cmd, "touch %s\n", path);
				run_command(cmd, out, err);
			}
		}
	}
	else if ((fi->flags & O_ACCMODE) == O_RDWR) {
		pflags = LIBSSH2_FXF_READ | LIBSSH2_FXF_WRITE;
	}
	else
		return 0;

	if (fi->flags & O_CREAT)
		pflags |= LIBSSH2_FXF_CREAT;

	if (fi->flags & O_EXCL)
		pflags |= LIBSSH2_FXF_EXCL;

	if (fi->flags & O_TRUNC)
		pflags |= LIBSSH2_FXF_TRUNC;

	if (fi->flags & O_APPEND)
		pflags |= LIBSSH2_FXF_APPEND;

	info("%s, pflags: %d\n", path, pflags);
	sh->flags = pflags;
	sh->is_dir = is_dir;
	sh->mode = mode;
	strcpy_s(sh->path, MAX_PATH, path);

	lock();
	if (sh->is_dir)
		handle = libssh2_sftp_open_ex(g_ssh->sftp, sh->path, (int)strlen(sh->path),
			sh->flags, sh->mode, LIBSSH2_SFTP_OPENDIR);
	else
		handle = libssh2_sftp_open_ex(g_ssh->sftp, sh->path, (int)strlen(sh->path),
			sh->flags, sh->mode, LIBSSH2_SFTP_OPENFILE);
	g_sftp_calls++;
	
	if (!handle) {
		int rc;
		san_error(sh->path);
		unlock();
		return 0;
	}
	unlock();
	
	info("OPEN HANDLE : %zu:%zu: %s, flags: %d, mode: %d\n",
		(size_t)sh,(size_t)handle, sh->path, sh->flags, sh->mode);

	sh->handle = handle;
	return sh;
}

int f_readdir(const char *path, void *buf, fuse_fill_dir_t filler, fuse_off_t off,
	struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
	info("%s\n", path);
	int rc = 0;
	DIR *dirp = fi_dirp(fi);
	SAN_HANDLE *sh = dirp->san_handle;
	LIBSSH2_SFTP_HANDLE* handle = sh->handle;
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	char fname[FILENAME_MAX];
	//char longentry[512];
	//struct dirent *de;
	//struct fuse_stat *stbuf;

	lock();
	libssh2_sftp_rewind(handle);
	g_sftp_calls++;

	for (;;) {
		//de = san_readdir_entry(dirp);

		rc = libssh2_sftp_readdir(handle, fname, FILENAME_MAX, &attrs);
		//rc = libssh2_sftp_readdir_ex(handle, fname, FILENAME_MAX, longentry, 512, &attrs);
		g_sftp_calls++;

		/* skip hidden files */
		if (rc > 1							/* file name length 2 or more	*/				
			&& !g_fs.hidden				/* user parameter not set		*/
			&& strncmp(fname, ".", 1) == 0  /* file name starts with .		*/
			&& strncmp(fname, "..", 2)) {	/* file name is not ..			*/
			//printf("skipping hidden file: %s\n", fname);
			continue;
		}

		if (rc < 0) {
			san_error(sh->path);
			break; // or continue?
		}
		if (rc == 0) {
			// no more files
			rc = 0;
			break;
		}

		//if (longentry[0] != '\0') {
		//	printf("%s\n", longentry);
		//}
		// resolve symbolic links
		if (attrs.permissions & LIBSSH2_SFTP_S_IFLNK) {
			char fullpath[MAX_PATH];
			strcpy_s(fullpath, MAX_PATH, dirp->path);
			int pathlen = (int)strlen(dirp->path);
			if (!(pathlen > 0 && dirp->path[pathlen - 1] == '/'))
				strcat_s(fullpath, 2, "/");
			strcat_s(fullpath, FILENAME_MAX, fname);
			memset(&attrs, 0, sizeof attrs);
			rc = libssh2_sftp_stat_ex(g_ssh->sftp, fullpath,
				(int)strlen(fullpath), LIBSSH2_SFTP_STAT, &attrs);
			g_sftp_calls++;

			if (rc) {
				san_error(fullpath);
			}
		}

		//stbuf = &dirp->de.d_stat;
		copy_attributes(&dirp->de.d_stat, &attrs);
		strcpy_s(dirp->de.d_name, FILENAME_MAX, fname);

#if STATS
		// stats	
		//debug("sftp calls cached/total: %zd/%zd (%.1f%% cached)\n",
		//		g_sftp_cached_calls, g_sftp_calls, 
		//		(g_sftp_cached_calls*100/(double)g_sftp_calls));
#endif

		//de = &dirp->de;

		//if (!de) {
		//	break;
		//}
		if (0 != filler(buf, dirp->de.d_name, &dirp->de.d_stat, 0, FUSE_FILL_DIR_PLUS)) {
			rc = -ENOMEM;
			break;
		}			
	}
	unlock();
	return rc;

}

int f_releasedir(const char *path, struct fuse_file_info *fi)
{
	info("%s\n", path);
	DIR *dirp = fi_dirp(fi);
	SAN_HANDLE *sh = dirp->san_handle;
	san_close(sh);
	free(dirp);
	dirp = NULL;
	return 0;
}

int san_close(SAN_HANDLE* sh)
{
	int rc;
	LIBSSH2_SFTP_HANDLE* handle;
	handle = sh->handle;
	lock();
	rc = libssh2_sftp_close_handle(handle);
	g_sftp_calls++;
	unlock();
	info("CLOSE HANDLE: %zu:%zu\n", (size_t)sh, (size_t)handle);
	free(sh);
	sh = NULL;
	return 0;
}

int f_create(const char *path, fuse_mode_t mode, struct fuse_file_info *fi)
{
	realpath(path);
	info("%s\n", path);
	SAN_HANDLE *sh;
	int rc;
	fuse_mode_t mod = mode & 438; // 438 = 0666 to remove execution bit

	sh = san_open(path, FILE_ISREG, mod, fi);
	

	if (!sh)
		return -1;

	rc = (fi_setfd(fi, (size_t)sh), 0);

	return rc ? -1 : 0;
}

int f_open(const char *path, struct fuse_file_info *fi)
{
	realpath(path);
	info("%s\n", path);
	unsigned int mode = 0;
	int rc;
	SAN_HANDLE *sh;
	
	sh = san_open(path, FILE_ISREG, mode, fi);
	
	if (!sh) 
		return -1;

	rc = (fi_setfd(fi, (size_t)sh), 0);
	return rc ? -1 : 0;
}

int f_read(const char *path, char *buf, size_t size, fuse_off_t offset, struct fuse_file_info *fi)
{
	//info("%s\n", path);
	(void)path;
	size_t fd = fi_fd(fi);
	size_t curpos;
	SAN_HANDLE* sh = (SAN_HANDLE*)fd;	
	LIBSSH2_SFTP_HANDLE* handle = sh->handle;
	
	debug("READING HANDLE: %zu:%zu size: %zu\n", (size_t)sh, (size_t)handle, size);

	lock();
	curpos = libssh2_sftp_tell64(handle);
	if (offset != curpos)
		libssh2_sftp_seek64(handle, offset);
	ssize_t bytesread = 0;
	ssize_t total = 0;
	size_t chunk = size;
	char *pos = buf;	
	
	while (chunk) {
		bytesread = libssh2_sftp_read(handle, pos, chunk);
		g_sftp_calls++;
		if (bytesread <= 0) {
			if (bytesread < 0) {
				int rc;
				san_error("ERROR: Unable to read chuck of file\n");
				total = -1;
			}
			break;
		}	
		pos += bytesread;
		total += bytesread;
		chunk -= bytesread;
	}
	unlock();
	
	debug("FINISH READING HANDLE %zu, bytes: %zu\n", (size_t)handle, total);

	return total >= 0 ? (int)total: -1;
}

int f_write(const char *path, const char *buf, size_t size, fuse_off_t offset, struct fuse_file_info *fi)
{
	(void)path;
	size_t fd = fi_fd(fi);
	SAN_HANDLE* sh = (SAN_HANDLE*)fd;
	LIBSSH2_SFTP_HANDLE* handle = sh->handle;
	size_t curpos;

	debug("WRITING HANDLE: %zu:%zu size: %zu\n", (size_t)sh, (size_t)handle, size);

	// working in hard links
	if (offset == 0) {
		printf("writing first buffer to file: %s", path);
	}
	else {
		printf("writing %s", path);
	}


	lock();
	curpos = libssh2_sftp_tell64(handle);
	if (offset != curpos)
		libssh2_sftp_seek64(handle, offset);
	//printf("thread buffer size    offset         bytes read     bytes written  total bytes\n");
	ssize_t byteswritten = 0;
	ssize_t total = 0;
	size_t chunk = size;
	const char* pos = buf;

	while (chunk) {
		byteswritten = libssh2_sftp_write(handle, pos, chunk);
		g_sftp_calls++;
		if (byteswritten <= 0) {
			if (byteswritten < 0) {
				error("ERROR: Unable to write chuck of data\n");
				total = -1;
			}
			break;
		}
		pos += byteswritten;
		total += byteswritten;
		chunk -= byteswritten;
		//printf("%-7ld%-15d%-15ld%-15d%-15d%-15ld\n",thread,
		//	size, offset, bytesread, bytesread, total);

	}
	unlock();
	
	debug("FINISH WRITING HANDLE %zu, bytes: %zu\n", (size_t)handle, total);

	return total >= 0 ? (int)total : -1;
}

int f_release(const char *path, struct fuse_file_info *fi)
{
	debug("%s\n", path);
	SAN_HANDLE* sh = (SAN_HANDLE*)fi_fd(fi);
	return san_close(sh);	
}


int f_rename(const char *from, const char *to, unsigned int flags)
{
	int rc;
	realpath(from);
	realpath(to);
	info("%s -> %s\n", from, to);
	size_t fromlen = strlen(from);
	size_t tolen = strlen(to);
	lock();
	rc = libssh2_sftp_rename_ex(g_ssh->sftp, from, (int)fromlen, to, (int)tolen,
		LIBSSH2_SFTP_RENAME_OVERWRITE | LIBSSH2_SFTP_RENAME_ATOMIC | LIBSSH2_SFTP_RENAME_NATIVE);
	g_sftp_calls++;
	unlock();

	if (rc) {

		if (tolen + 8 < PATH_MAX) {
			char totmp[PATH_MAX];
			strcpy(totmp, to);
			gen_random(totmp + tolen, 8);
			rc = f_rename(to, totmp, 0);
			if (!rc) {
				rc = f_rename(from, to, 0);
				if (!rc)
					rc = f_unlink(totmp);
				else
					f_rename(totmp, to, 0);
			}
			if (rc) {
				rc = libssh2_sftp_last_error(g_ssh->sftp);
				san_error(from);
				san_error(to);
			}
		}
	}
	return rc ? -1 : 0;

}

int f_mkdir(const char * path, fuse_mode_t  mode)
{
	int rc;
	realpath(path);
	info("%s\n", path);
	info("MODE: %u\n", mode);

	//unsigned int mod = LIBSSH2_SFTP_S_IRWXU |
	//				   LIBSSH2_SFTP_S_IRWXG |
	//				   LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH;
	//info("MOD : %u\n", mod);
	lock();
	rc = libssh2_sftp_mkdir_ex(g_ssh->sftp, path, (int)strlen(path), mode);
	g_sftp_calls++;
	unlock();
	if (rc) {
		san_error(path);
	}
	

	return rc ? -1 : 0;
}

int f_rmdir(const char * path)
{
	int rc;
	realpath(path);
	info("%s\n", path);

	lock();
	rc = libssh2_sftp_rmdir_ex(g_ssh->sftp, path, (int)strlen(path));
	g_sftp_calls++;
	if (rc) {
		san_error(path);
	}
	unlock();
	

	return rc ? -1 : 0;
}

int f_unlink(const char *path)
{
	int rc;
	realpath(path);
	info("%s\n", path);
	lock();
	rc = libssh2_sftp_unlink_ex(g_ssh->sftp, path, (int)strlen(path));
	g_sftp_calls++;
	if (rc) {
		san_error(path);
	}
	unlock();
	

	return rc ? -1 : 0;
}

int f_truncate(const char *path, fuse_off_t size, struct fuse_file_info *fi)
{
	int rc;
	realpath(path);
	info("%s\n", path);
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	lock();
	rc = libssh2_sftp_stat_ex(g_ssh->sftp, path, (int)strlen(path),
		LIBSSH2_SFTP_STAT, &attrs);
	g_sftp_calls++;
	if (rc) {
		san_error(path);
		return rc;
	}
	attrs.filesize = size;
	rc = libssh2_sftp_stat_ex(g_ssh->sftp, path, (int)strlen(path),
		LIBSSH2_SFTP_SETSTAT, &attrs);
	g_sftp_calls++;
	if (rc) {
		san_error(path);
		return rc;
	}
	unlock();
	

	return rc;
}

int f_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	realpath(path);
	info("%s\n", path);
	SAN_HANDLE* sh = (SAN_HANDLE*)fi_fd(fi);
	
	return f_flush(sh->path, fi);
}

int f_flush(const char *path, struct fuse_file_info *fi)
{
	info("%s\n", path);
	// no need as this is sync write always ?
	return 0;
}

int f_utimens(const char *path, const struct fuse_timespec tv[2], struct fuse_file_info *fi)
{
	info("%s\n", path);
	return utimensat(AT_FDCWD, path, tv, AT_SYMLINK_NOFOLLOW);
}

int utimensat(int dirfd, const char *path, const struct fuse_timespec times[2], int flag)
{
	/* ignore dirfd and assume that it is always AT_FDCWD */
	/* ignore flag and assume that it is always AT_SYMLINK_NOFOLLOW */

	return 0;

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

int run_command(const char *cmd, char *out, char *err)
{
	LIBSSH2_CHANNEL *channel;
	int rc;
	ssize_t bytesread = 0;
	size_t offset = 0;
	char *errmsg;
	//lock();
	channel = libssh2_channel_open_session(g_ssh->ssh);
	//unlock();
	if (!channel) {
		rc = libssh2_session_last_error(g_ssh->ssh, &errmsg, NULL, 0);
		debug("ERROR: unable to init ssh chanel, rc=%d, %s\n", rc, errmsg);
		return 1;
	}

	libssh2_channel_set_blocking(channel, 0);
	while ((rc = libssh2_channel_exec(channel, cmd)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(g_ssh);

	if (rc != 0) {
		rc = libssh2_session_last_error(g_ssh->ssh, &errmsg, NULL, 0);
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
			waitsocket(g_ssh);
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
			waitsocket(g_ssh);
		else
			break;
	}
	/* get exit code */
	while ((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(g_ssh);
	if (rc == 0)
		rc = libssh2_channel_get_exit_status(channel);
	else
		rc = 127;

finish:
	libssh2_channel_set_blocking(channel, 1);
	libssh2_channel_free(channel);
	return (int)rc;
}

int san_threads(int n, int c)
{
	// guess number of threads in this app
	// n: ThreadCount arg
	// c: number of cores
	// w: winfsp threads = n < 1 ? c : max(2, n) + 1
	// t: total = w + c + main thread
	return (n < 1 ? c : max(2, n)) + c + 2;
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


//void ht_ssh_add(SANSSH *value)
//{
//	ht_ssh_lock(1);
//	HASH_ADD_INT(g_ssh_ht, thread, value);
//	ht_ssh_lock(0);
//	debug("new ssh connection added\n");
//}
//void ht_ssh_del(SANSSH *value)
//{
//	HASH_DEL(g_ssh_ht, value);			/* value: pointer to delete */
//	free(value);						/* optional; it's up to you! */
//}
//SANSSH * ht_ssh_find(int thread)
//{
//	SANSSH *value = NULL;
//	HASH_FIND_INT(g_ssh_ht, &thread, value);
//	return value;
//}
//
//SANSSH* get_ssh(void)
//{
//	int thread = GetCurrentThreadId();
//	SANSSH *ssh = ht_ssh_find(thread);
//	if (!ssh) {
//		ssh = san_init_ssh(g_cmd_args->host, g_cmd_args->port,
//			g_cmd_args->user, g_cmd_args->pkey);
//		if (ssh) {
//			ht_ssh_add(ssh);
//			info("new ssh session created in thread %d\n", thread);
//		}
//		else {
//			error("cannot create ssh session, terminating...\n");
//			exit(1);
//		}
//	}
//	return ssh;
//}
//
//// lock add and delete functions before calling
//void ht_handle_close_add(SAN_HANDLE *value)
//{
//	HASH_ADD_PTR(g_handle_close_ht, id , value);
//}
//void ht_handle_close_del(SAN_HANDLE *value) 
//{
//	HASH_DEL(g_handle_close_ht, value);  /* value: pointer to delete */
//}
//
//SAN_HANDLE * ht_handle_close_find(void * id)
//{
//	SAN_HANDLE *value = NULL;
//	HASH_FIND_PTR(g_handle_close_ht, &id, value);
//	return value;
//}
//
//void ht_handle_add(SAN_HANDLE *sh, HANDLE_T *value)
//{
//	HASH_ADD_INT(sh->handles, thread, value);
//	sh->thread_count++;
//}
//void ht_handle_del(SAN_HANDLE *sh, HANDLE_T *value)
//{
//	HASH_DEL(sh->handles, value);  /* value: pointer to delete */
//	sh->thread_count--;
//}
//HANDLE_T * ht_handle_find(SAN_HANDLE *sh, int thread)
//{
//	HANDLE_T *value = NULL;
//	HASH_FIND_INT(sh->handles, &thread, value);
//	return value;
//}
//

