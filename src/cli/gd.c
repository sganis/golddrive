// src/cli/gd.c
#include <assert.h>
#include "config.h"
#include "util.h"
#include "gd.h"
#include "cache.h"
#include "parse.h"
#include "net.h"
#include "pool.h"
#include <Winhttp.h>

/* process-wide one-time init: Winsock + libssh2 crypto. Call once before
 * building the connection pool. */
int gd_global_init(void)
{
	WSADATA wsadata;
	int rc = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (rc != 0) {
		gd_log("WSAStartup failed, rc=%d\n", rc);
		return -1;
	}
	rc = libssh2_init(0);
	if (rc) {
		gd_log("failed to initialize crypto library, rc=%d\n", rc);
		WSACleanup();
		return -1;
	}
	return 0;
}

GDSSH* gd_init_ssh(void)
{
	int rc;
	char* errmsg = 0;
	int errlen;
	SOCKET sock = INVALID_SOCKET;
	LIBSSH2_SESSION* ssh = NULL;
	LIBSSH2_SFTP* sftp = NULL;
	LIBSSH2_CHANNEL* channel = NULL;
	int thread = GetCurrentThreadId();

	/* create a session instance (Winsock + libssh2 init done once in
	 * gd_global_init before the pool is built) */
	ssh = libssh2_session_init();
	if (!ssh) {
		gd_log("%zd: %d :ERROR: %s: %d: "
			"failed allocate memory for ssh session\n",
			time_mu(), thread, __func__, __LINE__);
		goto fail;
	}

	/* encryption */
	if (g_conf.cipher) {
		rc = libssh2_session_method_pref(ssh,
			LIBSSH2_METHOD_CRYPT_CS, g_conf.cipher);
		rc = libssh2_session_method_pref(ssh,
			LIBSSH2_METHOD_CRYPT_SC, g_conf.cipher);

		if (rc) {
			rc = libssh2_session_last_error(ssh, &errmsg, &errlen, 0);
			gd_log("%zd: %d :ERROR: %s: %d: "
				"failed to set cipher [rc=%d, %s]\n",
				time_mu(), thread, __func__, __LINE__, rc, errmsg);
			goto fail;
		}
	}

	/* blocking mode */
	libssh2_session_set_blocking(ssh, 1);

	/* resolve (IPv4 or IPv6) and connect */
	sock = gd_tcp_connect(g_conf.host, g_conf.port);
	if (sock == INVALID_SOCKET) {
		gd_log("%zd: %d :ERROR: %s: %d: "
			"failed to connect to %s:%d (host not found or port closed)\n",
			time_mu(), thread, __func__, __LINE__, g_conf.host, g_conf.port);
		goto fail;
	}

	/* start it up: trade welcome banners, exchange keys,
	 * and setup crypto, compression, and MAC layers */
	while ((rc = libssh2_session_handshake(ssh, sock)) ==
		LIBSSH2_ERROR_EAGAIN)
		Sleep(10);

	if (rc) {
		rc = libssh2_session_last_error(ssh, &errmsg, &errlen, 0);
		gd_log("%zd: %d :ERROR: %s: %d: "
			"failed to complete ssh handshake [rc=%d, %s]\n",
			time_mu(), thread, __func__, __LINE__, rc, errmsg);
		goto fail;
	}

	/* verify host key against known_hosts */
	{
		LIBSSH2_KNOWNHOSTS* nh = libssh2_knownhost_init(ssh);
		if (nh) {
			char knownhosts_path[MAX_PATH];
			char* profile = getenv("USERPROFILE");
			if (profile) {
				sprintf_s(knownhosts_path, MAX_PATH,
					"%s\\.ssh\\known_hosts", profile);
				libssh2_knownhost_readfile(nh, knownhosts_path,
					LIBSSH2_KNOWNHOST_FILE_OPENSSH);
			}
			size_t hostkey_len;
			int hostkey_type;
			const char* hostkey = libssh2_session_hostkey(ssh,
				&hostkey_len, &hostkey_type);
			if (hostkey) {
				int kh_type = gd_knownhost_keytype(hostkey_type);
				struct libssh2_knownhost* host = NULL;
				int check = libssh2_knownhost_checkp(nh,
					g_conf.host, g_conf.port,
					hostkey, hostkey_len,
					LIBSSH2_KNOWNHOST_TYPE_PLAIN |
					LIBSSH2_KNOWNHOST_KEYENC_RAW |
					kh_type, &host);
				if (check == LIBSSH2_KNOWNHOST_CHECK_MISMATCH) {
					gd_log("%zd: %d :ERROR: %s: %d: "
						"host key mismatch for %s, possible MITM attack\n",
						time_mu(), thread, __func__, __LINE__,
						g_conf.host);
					libssh2_knownhost_free(nh);
					goto fail;
				}
				if (check == LIBSSH2_KNOWNHOST_CHECK_NOTFOUND && profile) {
					/* TOFU: add host key on first connection */
					libssh2_knownhost_addc(nh,
						g_conf.host, NULL,
						hostkey, hostkey_len,
						NULL, 0,
						LIBSSH2_KNOWNHOST_TYPE_PLAIN |
						LIBSSH2_KNOWNHOST_KEYENC_RAW |
						kh_type, NULL);
					libssh2_knownhost_writefile(nh,
						knownhosts_path,
						LIBSSH2_KNOWNHOST_FILE_OPENSSH);
					gd_log("Added host key for %s to known_hosts\n",
						g_conf.host);
				}
			}
			libssh2_knownhost_free(nh);
		}
	}

	/* keepalive */
	libssh2_keepalive_config(ssh, 1, 60);

	gd_log("Session symmetric encryption:\n\t%s\n",
		libssh2_session_methods(ssh, LIBSSH2_METHOD_CRYPT_CS));
	gd_log("Session key exchange:\n\t%s\n",
		libssh2_session_methods(ssh, LIBSSH2_METHOD_KEX));
	gd_log("Session host keys:\n\t%s\n",
		libssh2_session_methods(ssh, LIBSSH2_METHOD_HOSTKEY));
	gd_log("Session MAC:\n\t%s\n",
		libssh2_session_methods(ssh, LIBSSH2_METHOD_MAC_CS));

	/* authenticate with keys */
	while ((rc = libssh2_userauth_publickey_fromfile(
		ssh, g_conf.user, NULL, g_conf.pkey, NULL)) ==
		LIBSSH2_ERROR_EAGAIN)
		Sleep(10);

	if (rc) {
		rc = libssh2_session_last_error(ssh, &errmsg, &errlen, 0);
		gd_log("%zd: %d :ERROR: %s: %d: "
			"authentication by public key failed [rc=%d, %s]\n",
			time_mu(), thread, __func__, __LINE__, rc, errmsg);
		goto fail;
	}

	/* init sftp channel */
	{
		int retries = 0;
		const int max_retries = 50;
		do {
			sftp = libssh2_sftp_init(ssh);
			if ((!sftp) && (libssh2_session_last_errno(ssh) !=
				LIBSSH2_ERROR_EAGAIN))
			{
				gd_log("%zd: %d :ERROR: %s: %d: "
					"failed to start sftp session [rc=%d, %s]\n",
					time_mu(), thread, __func__, __LINE__, rc, errmsg);
				goto fail;
			}
			if (!sftp) Sleep(100);
		} while (!sftp && ++retries < max_retries);
		if (!sftp) {
			gd_log("%zd: %d :ERROR: %s: %d: "
				"timeout waiting for sftp session\n",
				time_mu(), thread, __func__, __LINE__);
			goto fail;
		}
	}

	{
		int retries = 0;
		const int max_retries = 50;
		do {
			channel = libssh2_channel_open_session(ssh);
			if ((!channel) && (libssh2_session_last_errno(ssh) !=
				LIBSSH2_ERROR_EAGAIN))
				break;
			if (!channel) Sleep(100);
		} while (!channel && ++retries < max_retries);
	}
	if (!channel) {
		rc = libssh2_session_last_error(ssh, &errmsg, NULL, 0);
		log_error("ERROR: invalid channel to run commands, rc=%d, %s\n", rc, errmsg);
		goto fail;
	}
	else {
		while ((rc = libssh2_channel_shell(channel)) ==
			LIBSSH2_ERROR_EAGAIN)
			Sleep(10);
		if (rc) {
			rc = libssh2_session_last_error(ssh, &errmsg, NULL, 0);
			gd_log("cannot request shell: [rc=%d, %s]\n", rc, errmsg);
			goto fail;
		}
	}

	GDSSH* s = malloc(sizeof(GDSSH));
	if (!s) {
		gd_log("%zd: %d :ERROR: %s: %d: out of memory\n",
			time_mu(), thread, __func__, __LINE__);
		goto fail;
	}
	s->socket = sock;
	s->ssh = ssh;
	s->sftp = sftp;
	s->channel = channel;
	s->thread = GetCurrentThreadId();
	InitializeSRWLock(&s->lock);

	return s;

fail:
	/* free whatever this connection acquired; each freed only if acquired.
	 * libssh2_exit/WSACleanup are process-global (done once in gd_finalize). */
	if (channel) libssh2_channel_free(channel);
	if (sftp) libssh2_sftp_shutdown(sftp);
	if (ssh) libssh2_session_free(ssh);
	if (sock != INVALID_SOCKET) closesocket(sock);
	return 0;
}

/* gracefully tear down one connection and free its struct */
void gd_conn_free(GDSSH* c)
{
	int retries;
	if (!c)
		return;
	if (c->channel) {
		retries = 50;
		while (libssh2_channel_close(c->channel) ==
			LIBSSH2_ERROR_EAGAIN && retries-- > 0)
			waitsocket(c);
		retries = 50;
		while (libssh2_channel_free(c->channel) ==
			LIBSSH2_ERROR_EAGAIN && retries-- > 0)
			waitsocket(c);
	}
	if (c->sftp) {
		retries = 50;
		while (libssh2_sftp_shutdown(c->sftp) ==
			LIBSSH2_ERROR_EAGAIN && retries-- > 0)
			waitsocket(c);
	}
	if (c->ssh) {
		retries = 50;
		while (libssh2_session_disconnect(c->ssh, "ssh session disconnected") ==
			LIBSSH2_ERROR_EAGAIN && retries-- > 0)
			waitsocket(c);
		retries = 50;
		while (libssh2_session_free(c->ssh) ==
			LIBSSH2_ERROR_EAGAIN && retries-- > 0)
			waitsocket(c);
	}
	closesocket(c->socket);
	free(c);
}

/* rebuild connection c in place; caller must hold c->lock. Returns 0 on success */
int gd_reconnect(GDSSH* c)
{
	gd_log("Attempting SSH reconnection...\n");
	int retries;
	if (!c)
		return -1;

	/* tear down the dead session (abrupt) */
	if (c->channel) {
		retries = 10;
		while (libssh2_channel_close(c->channel) ==
			LIBSSH2_ERROR_EAGAIN && retries-- > 0)
			Sleep(10);
		libssh2_channel_free(c->channel);
		c->channel = NULL;
	}
	if (c->sftp) {
		retries = 10;
		while (libssh2_sftp_shutdown(c->sftp) ==
			LIBSSH2_ERROR_EAGAIN && retries-- > 0)
			Sleep(10);
		c->sftp = NULL;
	}
	if (c->ssh) {
		libssh2_session_disconnect(c->ssh, "reconnecting");
		libssh2_session_free(c->ssh);
		c->ssh = NULL;
	}
	closesocket(c->socket);
	c->socket = INVALID_SOCKET;

	/* build a fresh connection and adopt its session into c, keeping c's lock
	 * (held by the caller); discard the temporary shell */
	GDSSH* n = gd_init_ssh();
	if (!n) {
		gd_log("SSH reconnection failed\n");
		return -1;
	}
	c->socket = n->socket;
	c->ssh = n->ssh;
	c->sftp = n->sftp;
	c->channel = n->channel;
	c->thread = n->thread;
	free(n);

	gd_log("SSH reconnection successful\n");
	return 0;
}

int gd_finalize(int error)
{
	log_info("FINALIZE\n");
	gd_pool_free();
	libssh2_exit();
	WSACleanup();
	printf("sftp calls: %zu\n", g_sftp_calls);
	return error;
}

int gd_stat(const char* path, struct fuse_stat* stbuf)
{
	log_info("%s\n", path);
	int rc = 0;

	LIBSSH2_SFTP_ATTRIBUTES attrs;
	gd_lock();
	while ((rc = libssh2_sftp_stat_ex(
		g_ssh->sftp, path, (int)strlen(path),
		LIBSSH2_SFTP_LSTAT, &attrs)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	gd_unlock();

	log_debug("rc=%d, %s\n", rc, path);
	if (rc < 0) {
		gd_error(path);
		rc = error();

		if (errno == EIO) {
			gd_log("I/O error detected, connection may be lost.");
		}
	}
	copy_attributes(stbuf, &attrs);

	/* generate inode */
	CACHE_INODE* inode_hash = cache_inode_find(path);
	if (!inode_hash) {
		inode_hash = malloc(sizeof * inode_hash);
		if (inode_hash) {
			inode_hash->inode = hash_path(path);
			strcpy_s(inode_hash->path, MAX_PATH, path);
			cache_inode_add(inode_hash);
			stbuf->st_ino = inode_hash->inode;
		}
	}
	else {
		stbuf->st_ino = inode_hash->inode;
	}

	log_info("DONE\n");
	return rc;
}

int gd_fstat(intptr_t fd, struct fuse_stat* stbuf)
{
	int rc = 0;
	GDHANDLE* sh = (GDHANDLE*)fd;

	LIBSSH2_SFTP_HANDLE* handle = sh->file_handle;
	if (!handle) {
		errno = EBADF;
		return -1;
	}

	log_info("FSTAT: %s\n", sh->path);

	LIBSSH2_SFTP_ATTRIBUTES attrs;

	gd_lock_conn(sh->conn);
	while ((rc = libssh2_sftp_fstat_ex(handle, &attrs, 0)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	gd_unlock();

	log_debug("rc=%d, %s\n", rc, sh->path);
	if (rc < 0) {
		gd_error(sh->path);
		rc = error();
	}
	copy_attributes(stbuf, &attrs);
	log_info("DONE\n");
	return rc;
}

int gd_readlink(const char* path, char* buf, size_t size)
{
	log_info("READLINK: %s, size=%zu\n", path, size);
	log_debug("%s, size=%zu, buf=%s\n", path, size, buf);
	int rc;
	if (size == 0) {
		errno = EINVAL;
		return -1;
	}

	char* target = malloc(MAX_PATH);
	if (!target) {
		errno = ENOMEM;
		return -1;
	}
	/* rc is number of bytes in target */
	gd_lock();
	while ((rc = libssh2_sftp_symlink_ex(
		g_ssh->sftp, path, (int)strlen(path),
		target, MAX_PATH, LIBSSH2_SFTP_READLINK)) ==
			LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	gd_unlock();

	log_debug("rc=%d, %s\n", rc, path);
	if (rc < 0) {
		free(target);
		if (strcmp(path, g_conf.root) != 0) {
			gd_error(path);
			return error();
		}
		return 0;
	}
	if (rc >= (int)size || rc >= MAX_PATH) {
		free(target);
		errno = ENAMETOOLONG;
		return -1;
	}
	target[rc] = '\0';

	/* collapse double slashes and strip a trailing slash */
	if (normalize_link_path(target, buf, size) < 0) {
		free(target);
		errno = ENAMETOOLONG;
		return -1;
	}
	free(target);
	log_info("DONE\n");
	return 0;
}

int gd_mkdir(const char* path, fuse_mode_t mode)
{
	int rc = 0;
	log_info("%s, mode=%u\n", path, mode);

	/* check if file already exists */
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	gd_lock();
	while ((rc = libssh2_sftp_stat_ex(
		g_ssh->sftp, path, (int)strlen(path),
		LIBSSH2_SFTP_LSTAT, &attrs)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	gd_unlock();
	if (rc == 0) {
		errno = EEXIST;
		return -1;
	}

	gd_lock();
	while ((rc = libssh2_sftp_mkdir_ex(
		g_ssh->sftp, path, (int)strlen(path), mode)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	gd_unlock();

	if (rc < 0) {
		gd_error(path);
		rc = error();
	}
	log_info("DONE\n");
	return rc;
}

int gd_unlink(const char* path)
{
	int rc = 0;
	log_info("%s\n", path);

	gd_lock();
	while ((rc = libssh2_sftp_unlink_ex(
		g_ssh->sftp, path, (int)strlen(path))) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	gd_unlock();

	if (rc) {
		gd_error(path);
		rc = error();
	}

	if (g_conf.audit) {
		gd_log("%s: DELETE: %s\n", g_conf.user, path);
	}
	log_info("DONE\n");
	return rc;
}

int gd_rmdir(const char* path)
{
	int rc = 0;
	log_info("%s\n", path);

	gd_lock();
	while ((rc = libssh2_sftp_rmdir_ex(
		g_ssh->sftp, path, (int)strlen(path))) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	gd_unlock();

	if (rc < 0) {
		gd_error(path);
		rc = error();
	}

	log_info("DONE\n");
	return rc;
}

static int _gd_rename(const char* from, const char* to)
{
	log_info("%s -> %s\n", from, to);
	int rc = 0;
	gd_lock();
	while ((rc = libssh2_sftp_rename_ex(g_ssh->sftp,
		from, (int)strlen(from), to, (int)strlen(to),
		LIBSSH2_SFTP_RENAME_OVERWRITE)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	gd_unlock();

	if (rc < 0) {
		gd_error(from);
		rc = error();
	}

	if (g_conf.audit) {
		gd_log("%s: RENAME: %s -> %s\n", g_conf.user, from, to);
	}

	log_info("DONE\n");
	return rc;
}

int gd_rename(const char* from, const char* to)
{
	int rc = 0;
	log_info("%s -> %s\n", from, to);
	size_t tolen = strlen(to);
	rc = _gd_rename(from, to);

	if (rc) {
		if (tolen + 8 < MAX_PATH) {
			char totmp[MAX_PATH];
			strcpy_s(totmp, MAX_PATH, to);
			gd_random_string(totmp + tolen, 8);
			rc = _gd_rename(to, totmp);

			if (!rc) {
				rc = _gd_rename(from, to);
				if (!rc)
					rc = gd_unlink(totmp);
				else
					_gd_rename(totmp, to);
			}
			if (rc) {
				gd_error(from);
				gd_error(to);
			}
		}
	}
	log_info("DONE\n");
	return rc ? -1 : 0;
}

int gd_truncate(const char* path, fuse_off_t size)
{
	int rc = 0;
	log_info("%s, size=%zu\n", path, size);
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	attrs.flags = LIBSSH2_SFTP_ATTR_SIZE;
	attrs.filesize = size;
	gd_lock();
	while ((rc = libssh2_sftp_stat_ex(g_ssh->sftp, path, (int)strlen(path),
		LIBSSH2_SFTP_SETSTAT, &attrs)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	if (rc < 0) {
		gd_error(path);
		rc = error();
	}
	gd_unlock();

	log_info("DONE\n");
	return rc;
}

int gd_ftruncate(intptr_t fd, fuse_off_t size)
{
	GDHANDLE* sh = (GDHANDLE*)fd;
	return gd_truncate(sh->path, size);
}

intptr_t gd_open(const char* path, int flags, unsigned int mode)
{
	log_info("%s\n", path);
	int rc;
	GDHANDLE* sh = malloc(sizeof(GDHANDLE));
	if (!sh) {
		errno = ENOMEM;
		return -1;
	}
	sh->file_handle = 0;
	sh->dir_handle = 0;
	strcpy_s(sh->path, MAX_PATH, path);
	sh->mode = mode;
	sh->dir = 0;

	LIBSSH2_SFTP_HANDLE* handle = 0;
	unsigned int pflags;
	if ((flags & O_ACCMODE) == O_RDONLY) {
		pflags = LIBSSH2_FXF_READ;
	}
	else if ((flags & O_ACCMODE) == O_WRONLY) {
		pflags = LIBSSH2_FXF_WRITE;
	}
	else if ((flags & O_ACCMODE) == O_RDWR) {
		pflags = LIBSSH2_FXF_READ | LIBSSH2_FXF_WRITE;
	}
	else {
		free(sh);
		return -EINVAL;
	}

	if (flags & O_CREAT)
		pflags |= GD_CREAT;
	if (flags & O_EXCL)
		pflags |= GD_EXCL;
	if (flags & O_TRUNC)
		pflags |= GD_TRUNC;
	if (flags & O_APPEND)
		pflags |= GD_APPEND;

	sh->flags = pflags;

	/* check if file has hard links */
	if (g_conf.keeplink == 0) {
		struct fuse_stat stbuf;
		if (!gd_stat(path, &stbuf)) {
			if (sh->flags == LIBSSH2_FXF_WRITE
				|| sh->flags == (LIBSSH2_FXF_READ | LIBSSH2_FXF_WRITE)) {
				gd_check_hlink(path);
			}
		}
	}

	gd_lock();
	do {
		handle = libssh2_sftp_open_ex(
			g_ssh->sftp, sh->path, (int)strlen(sh->path), sh->flags, sh->mode, LIBSSH2_SFTP_OPENFILE);
		g_sftp_calls++;
		if (!handle && libssh2_session_last_errno(g_ssh->ssh) !=
			LIBSSH2_ERROR_EAGAIN)
			break;
	} while (!handle);

	if (!handle) {
		gd_error(sh->path);
		gd_unlock();
		free(sh);
		return error();
	}
	gd_unlock();

	sh->file_handle = handle;
	sh->conn = g_ssh;	/* pin the handle to the connection that opened it */

	log_info("OPEN HANDLE : %zu:%zu: %s, flags=%d, mode=%d\n",
		(size_t)sh, (size_t)handle, sh->path, sh->flags, sh->mode);
	log_info("DONE\n");
	return (intptr_t)sh;
}

int gd_read(intptr_t fd, void* buf, size_t size, fuse_off_t offset)
{
	int rc = 0;
	GDHANDLE* sh = (GDHANDLE*)fd;
	int total = 0;
	size_t chunk = size;
	char* pos = buf;

	if (g_conf.audit && offset == 0 && size > 0) {
		gd_log("%s: READ: %s\n", g_conf.user, sh->path);
	}

	LIBSSH2_SFTP_HANDLE* handle = sh->file_handle;

	log_info("READING HANDLE: %zu size=%zu, offset=%zu\n",
		(size_t)handle, size, offset);

	gd_lock_conn(sh->conn);
	libssh2_sftp_seek64(handle, offset);

	size_t bsize;
	do {
		bsize = chunk < g_conf.buffer ? chunk : g_conf.buffer;
		while ((rc = (int)libssh2_sftp_read(handle, pos, bsize)) ==
			LIBSSH2_ERROR_EAGAIN) {
			waitsocket(g_ssh);
			g_sftp_calls++;
		}
		if (rc <= 0)
			break;
		pos += rc;
		total += rc;
		chunk -= rc;
	} while (chunk);

	if (rc < 0) {
		gd_error("ERROR: Unable to read chunk of file\n");
		rc = error();
		if (rc)
			total = -1;
	}
	gd_unlock();

	log_debug("FINISH READING HANDLE %zu, bytes: %zu\n", (size_t)handle, total);
	return total;
}

int gd_write(intptr_t fd, const void* buf, size_t size, fuse_off_t offset)
{
	GDHANDLE* sh = (GDHANDLE*)fd;
	int rc;
	int total = 0;
	size_t chunk = size;
	const char* pos = buf;

	if (g_conf.audit && offset == 0 && size > 0) {
		gd_log("%s: WRITE: %s\n", g_conf.user, sh->path);
	}

	LIBSSH2_SFTP_HANDLE* handle = sh->file_handle;
	log_info("WRITING HANDLE: %zu size: %zu\n", (size_t)handle, size);

	gd_lock_conn(sh->conn);
	libssh2_sftp_seek64(handle, offset);

	size_t bsize;
	do {
		bsize = chunk < g_conf.buffer ? chunk : g_conf.buffer;
		while ((rc = (int)libssh2_sftp_write(handle, pos, bsize)) ==
			LIBSSH2_ERROR_EAGAIN) {
			waitsocket(g_ssh);
			g_sftp_calls++;
		}
		if (rc <= 0)
			break;
		pos += rc;
		total += rc;
		chunk -= rc;
	} while (chunk);

	if (rc < 0) {
		gd_error("ERROR: Unable to write chunk of data\n");
		rc = error();
		if (rc)
			total = -1;
	}
	gd_unlock();

	log_debug("FINISH WRITING %zu, bytes: %zu\n", (size_t)handle, total);
	return total;
}

int gd_statvfs(const char* path, struct fuse_statvfs* stbuf)
{
	log_info("%s\n", path);
	int rc = 0;
	LIBSSH2_SFTP_STATVFS stvfs;

	gd_lock();
	while ((rc = libssh2_sftp_statvfs(g_ssh->sftp, path, strlen(path), &stvfs)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	if (rc < 0) {
		gd_error(path);
		rc = error();
	}
	gd_unlock();

	memset(stbuf, 0, sizeof(struct fuse_statvfs));
	stbuf->f_bsize = stvfs.f_bsize;		/* file system block size */
	stbuf->f_frsize = stvfs.f_frsize;		/* fragment size */
	stbuf->f_blocks = stvfs.f_blocks;		/* size of fs in f_frsize units */
	stbuf->f_bfree = stvfs.f_bfree;		/* free blocks */
	stbuf->f_bavail = stvfs.f_bavail;		/* free blocks for non-root */
	stbuf->f_files = stvfs.f_files;			/* inodes */
	stbuf->f_ffree = stvfs.f_ffree;		/* free inodes */
	stbuf->f_favail = stvfs.f_favail;		/* free inodes for non-root */
	stbuf->f_namemax = stvfs.f_namemax;	/* maximum filename length */

	return rc;
}

int gd_close(intptr_t fd)
{
	int rc = 0;
	GDHANDLE* sh = (GDHANDLE*)fd;
	LIBSSH2_SFTP_HANDLE* handle;
	handle = sh->file_handle;
	if (!handle) {
		free(sh);
		errno = EBADF;
		return -1;
	}
	log_info("CLOSE HANDLE: %zu:%zu\n", (size_t)sh, (size_t)handle);
	gd_lock_conn(sh->conn);
	while ((rc = libssh2_sftp_close_handle(handle)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	if (rc < 0) {
		gd_error(sh->path);
		rc = error();
	}

	free(sh);
	sh = NULL;
	log_info("DONE\n");
	gd_unlock();

	return rc;
}

GDDIR* gd_opendir(const char* path)
{
	log_info("%s\n", path);
	int rc = 0;
	GDDIR* dirp = 0;
	GDHANDLE* sh = malloc(sizeof(GDHANDLE));
	if (!sh) {
		errno = ENOMEM;
		return 0;
	}
	sh->file_handle = 0;
	sh->dir_handle = 0;
	log_debug("OPEN GDHANDLE: %zu, %s\n", (size_t)sh, path);

	LIBSSH2_SFTP_HANDLE* handle;
	gd_lock();
	do {
		handle = libssh2_sftp_open_ex(g_ssh->sftp, path,
			(int)strlen(path), 0, 0, LIBSSH2_SFTP_OPENDIR);
		g_sftp_calls++;
		if (!handle && libssh2_session_last_errno(g_ssh->ssh) !=
			LIBSSH2_ERROR_EAGAIN)
			break;
	} while (!handle);
	if (!handle) {
		gd_error(path);
		free(sh);
		gd_unlock();
		rc = error();
		return 0;
	}

	size_t pathlen = strlen(path);
	if (0 < pathlen && '/' == path[pathlen - 1])
		pathlen--;

	dirp = malloc(sizeof * dirp + pathlen + 2);
	if (0 == dirp) {
		while (libssh2_sftp_close_handle(handle) == LIBSSH2_ERROR_EAGAIN)
			waitsocket(g_ssh);
		free(sh);
		gd_unlock();
		return 0;
	}

	strcpy_s(sh->path, MAX_PATH, path);
	sh->dir_handle = handle;
	sh->conn = g_ssh;	/* pin the dir handle to the connection that opened it */
	sh->dir = 1;
	memset(dirp, 0, sizeof * dirp);
	dirp->handle = sh;
	memcpy(dirp->path, path, pathlen);
	dirp->path[pathlen + 0] = '/';
	dirp->path[pathlen + 1] = '\0';

	gd_unlock();
	log_info("DONE\n");
	return dirp;
}

void gd_rewinddir(GDDIR* dirp)
{
	log_info("%s\n", dirp->path);
	GDHANDLE* sh = dirp->handle;
	LIBSSH2_SFTP_HANDLE* handle = sh->dir_handle;
	gd_lock_conn(sh->conn);
	libssh2_sftp_seek64(handle, 0);
	g_sftp_calls++;
	gd_unlock();
	log_info("DONE\n");
}

struct GDDIRENT* gd_readdir(GDDIR* dirp)
{
	int rc;
	GDHANDLE* sh = dirp->handle;

	LIBSSH2_SFTP_HANDLE* handle = sh->dir_handle;
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	memset(&attrs, 0, sizeof attrs);
	char fname[FILENAME_MAX];

	gd_lock_conn(sh->conn);
	while ((rc = libssh2_sftp_readdir(
		handle, fname, FILENAME_MAX, &attrs)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	if (rc < 0) {
		gd_error(dirp->path);
		gd_unlock();
		rc = error0();
		return 0;
	}
	gd_unlock();
	if (rc == 0) {
		return 0;
	}
	strcpy_s(dirp->de.d_name, FILENAME_MAX, fname);
	dirp->de.dir = LIBSSH2_SFTP_S_ISDIR(attrs.permissions);
	copy_attributes(&dirp->de.d_stat, &attrs);
	return &dirp->de;
}

int gd_closedir(GDDIR* dirp)
{
	int rc = 0;
	if (!dirp)
		return 0;

	GDHANDLE* dirfh = dirp->handle;
	LIBSSH2_SFTP_HANDLE* handle = dirfh->dir_handle;
	log_info("CLOSE HANDLE: %zu:%zu\n", (size_t)dirfh, (size_t)handle);

	gd_lock_conn(dirfh->conn);
	while ((rc = libssh2_sftp_close_handle(handle)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	if (rc < 0) {
		gd_error(dirfh->path);
		rc = error();
	}

	free(dirfh);
	dirfh = NULL;
	free(dirp);
	dirp = NULL;
	gd_unlock();
	log_info("DONE\n");
	return rc;
}

intptr_t gd_dirfd(GDDIR* dirp)
{
	log_info("gd_dirfd\n");
	return (intptr_t)dirp->handle;
}

int gd_check_hlink(const char* path)
{
	log_info("%s\n", path);
	int rc = 0;
	char cmd[COMMAND_SIZE];
	char out[COMMAND_SIZE];

	/* FIXME: use stat cmd until we get hlinks from sftp v6 */
	/* Validate path: reject shell metacharacters to prevent command injection */
	for (const char* p = path; *p; p++) {
		if (*p == '`' || *p == '$' || *p == '(' || *p == ')' ||
			*p == ';' || *p == '|' || *p == '&' || *p == '\n' || *p == '\r') {
			log_error("ERROR: path contains unsafe characters: %s\n", path);
			errno = EINVAL;
			return -1;
		}
	}
	sprintf_s(cmd, sizeof cmd, "/usr/bin/stat -c%%h \"%s\"", path);
	gd_lock();
	rc = run_command_channel_exec(cmd, out, 0);
	gd_unlock();
	int hlinks = 0;
	if (!rc) {
		hlinks = atoi(out);
	}
	if (hlinks > 1) {
		char backup[MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		rc = _splitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, fname,
				_MAX_FNAME, ext, _MAX_EXT);
		if (rc != 0) {
			gd_error(path);
			rc = error();
			return rc;
		}

		rc = sprintf_s(backup, sizeof backup, "%s.%s_%s_%zu.hlink",	dir, fname, ext, time_mu());
		rc = gd_rename(path, backup);

		if (rc) {
			gd_error(path);
			rc = error();
		}
		else {
			gd_lock();
			LIBSSH2_SFTP_HANDLE* handle;
			/* FIXME: AND mode with -o create_umask arg */
			unsigned int mode = 0660;
			unsigned flags = LIBSSH2_FXF_READ | LIBSSH2_FXF_WRITE
				| LIBSSH2_FXF_CREAT | LIBSSH2_FXF_EXCL;

			do {
				handle = libssh2_sftp_open_ex(
					g_ssh->sftp, path, (int)strlen(path),
					flags, mode, LIBSSH2_SFTP_OPENFILE);
				g_sftp_calls++;
				if (!handle && libssh2_session_last_errno(g_ssh->ssh) !=
					LIBSSH2_ERROR_EAGAIN)
					break;
			} while (!handle);
			if (handle) {
				while ((rc = libssh2_sftp_close_handle(handle)) ==
					LIBSSH2_ERROR_EAGAIN) {
					waitsocket(g_ssh);
					g_sftp_calls++;
				}
			}
			gd_unlock();
		}
	}
	log_info("DONE\n");
	return rc;
}

int gd_utimens(const char* path, const struct fuse_timespec tv[2], struct fuse_file_info* fi)
{
	(void)fi;
	log_info("%s\n", path);
	int rc = 0;

	LIBSSH2_SFTP_ATTRIBUTES attrs;
	attrs.flags = LIBSSH2_SFTP_ATTR_ACMODTIME;
	attrs.atime = (unsigned long)tv->tv_sec;
	attrs.mtime = (unsigned long)tv->tv_sec;
	gd_lock();
	while ((rc = libssh2_sftp_stat_ex(
		g_ssh->sftp, path, (int)strlen(path),
		LIBSSH2_SFTP_SETSTAT, &attrs)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	if (rc < 0) {
		gd_error(path);
		rc = error();
	}
	gd_unlock();
	log_info("DONE\n");
	return rc;
}

int gd_flush(intptr_t fd)
{
	log_info("gd_flush\n");
	return gd_fsync(fd);
}

int gd_fsync(intptr_t fd)
{
	int rc = 0;
	GDHANDLE* sh = (GDHANDLE*)fd;
	LIBSSH2_SFTP_HANDLE* handle = sh->file_handle;
	if (!handle) {
		errno = EBADF;
		return -1;
	}
	log_info("%s\n", sh->path);
	gd_lock_conn(sh->conn);
	while ((rc = libssh2_sftp_fsync(handle)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	if (rc < 0) {
		gd_error(sh->path);
		rc = error();
	}
	gd_unlock();
	log_info("DONE\n");
	return rc;
}

int run_command_channel_exec(const char* cmd, char* out, char* err)
{
	int rc = 0;
	int rcode = 0;
	size_t offset = 0;

	memset(out, 0, COMMAND_SIZE);
	if (err)
		memset(err, 0, COMMAND_SIZE);

	if (!g_ssh || !g_ssh->ssh || !g_ssh->channel) {
		log_error("ERROR: ssh session not initialized\n");
		return -1;
	}
	LIBSSH2_CHANNEL* channel = g_ssh->channel;
	/* Note: caller must hold the connection lock (gd_lock/gd_lock_conn) so
	 * g_ssh and its channel stay valid for the duration of this call */
	char* errmsg;
	char buffer[COMMAND_SIZE];
	memset(buffer, 0, COMMAND_SIZE);

	if (!channel) {
		rc = libssh2_session_last_error(g_ssh->ssh, &errmsg, NULL, 0);
		log_error("ERROR: invalid channel to run commands, rc=%d, %s\n", rc, errmsg);
		return rc;
	}

	char newcmd[COMMAND_SIZE];
	snprintf(newcmd, COMMAND_SIZE, "%s;echo RCODE=$?\n", cmd);
	size_t len = strlen(newcmd);

	while ((rc = libssh2_channel_flush_ex(channel, LIBSSH2_CHANNEL_FLUSH_ALL)) ==
		LIBSSH2_ERROR_EAGAIN)
		Sleep(10);

	while ((rc = (int)libssh2_channel_write(channel, newcmd, len)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}

	if (rc == len) {
		for (;;) {
			while ((rc = (int)libssh2_channel_read_ex(
				channel, 0, buffer, COMMAND_SIZE)) ==
				LIBSSH2_ERROR_EAGAIN) {
				waitsocket(g_ssh);
				g_sftp_calls++;
			}
			if (rc <= 0)
				break;
			if (offset + rc > COMMAND_SIZE - 1)
				rc = COMMAND_SIZE - 1 - (int)offset;
			if (rc <= 0)
				break;
			memcpy(out + offset, buffer, rc);
			offset += rc;
			out[offset] = '\0';
			if (libssh2_channel_eof(channel))
				break;
			if (strstr(out, "RCODE="))
				break;
		}

		int rc_code = 0;
		int sentinel_off = extract_rcode(out, &rc_code);
		if (sentinel_off >= 0) {
			rcode = rc_code;
			out[sentinel_off] = '\0';
		}

		if (err && libssh2_poll_channel_read(channel, 1)) {
			offset = 0;
			for (;;) {
				while ((rc = (int)libssh2_channel_read_ex(
					channel, 1, buffer, sizeof(buffer))) ==
					LIBSSH2_ERROR_EAGAIN) {
					waitsocket(g_ssh);
					g_sftp_calls++;
				}
				if (rc <= 0)
					break;
				if (offset + rc > COMMAND_SIZE - 1)
					rc = COMMAND_SIZE - 1 - (int)offset;
				if (rc <= 0)
					break;
				memcpy(err + offset, buffer, rc);
				offset += rc;
				err[offset] = '\0';
				if (libssh2_channel_eof(channel))
					break;
				if(strstr(err, "\n"))
					break;
			}
			err[min(strcspn(err, "\n"), COMMAND_SIZE - 1)] = '\0';
		}
	}

	return rcode;
}

void copy_attributes(struct fuse_stat* stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs)
{
	if (!attrs)
		return;
	memset(stbuf, 0, sizeof * stbuf);
	stbuf->st_uid = attrs->uid;
	stbuf->st_gid = attrs->gid;
	stbuf->st_mode = attrs->permissions;
	stbuf->st_size = attrs->filesize;
	stbuf->st_birthtim.tv_sec = attrs->mtime;
	stbuf->st_atim.tv_sec = attrs->atime;
	stbuf->st_mtim.tv_sec = attrs->mtime;
	stbuf->st_ctim.tv_sec = attrs->mtime;
	stbuf->st_nlink = 1;
#if defined(FSP_FUSE_USE_STAT_EX)
#endif
}

int waitsocket(GDSSH* ssh)
{
	struct timeval timeout;
	int rc;
	fd_set fd;
	fd_set* writefd = NULL;
	fd_set* readfd = NULL;
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

int get_ssh_error(GDSSH* ssh)
{
	int rc = libssh2_session_last_errno(ssh->ssh);
	if (rc > 0 || rc < -47)
		rc = -48; /* ssh unknown */
	if (rc == LIBSSH2_ERROR_SFTP_PROTOCOL) {
		rc = libssh2_sftp_last_error(ssh->sftp);
		if (rc < 0 || rc>21)
			rc = 22; /* sftp unknown */
	}
	/* rc < 0: ssh error, rc > 0: sftp error */
	return rc;
}

int map_error(int rc)
{
	if (rc == LIBSSH2_FX_OK || rc == LIBSSH2_FX_EOF)
		return rc;

	if (rc < 0 || rc >= 22)
		return EIO;

	switch (rc) {
	case LIBSSH2_FX_NO_SUCH_FILE:
	case LIBSSH2_FX_NO_SUCH_PATH:
	case LIBSSH2_FX_INVALID_FILENAME:
	case LIBSSH2_FX_NOT_A_DIRECTORY:
	case LIBSSH2_FX_UNKNOWN_PRINCIPAL:
	case LIBSSH2_FX_NO_MEDIA:
		return ENOENT;
	case LIBSSH2_FX_PERMISSION_DENIED:
	case LIBSSH2_FX_WRITE_PROTECT:
	case LIBSSH2_FX_LOCK_CONFLICT:
	case LIBSSH2_FX_LINK_LOOP:
		return EACCES;
	case LIBSSH2_FX_QUOTA_EXCEEDED:
	case LIBSSH2_FX_NO_SPACE_ON_FILESYSTEM:
		return ENOMEM;
	case LIBSSH2_FX_FAILURE:
		return EPERM;
	case LIBSSH2_FX_FILE_ALREADY_EXISTS:
		return EEXIST;
	case LIBSSH2_FX_DIR_NOT_EMPTY:
		return ENOTEMPTY;
	case LIBSSH2_FX_BAD_MESSAGE:
		return EBADMSG;
	case LIBSSH2_FX_NO_CONNECTION:
		return ENOTCONN;
	case LIBSSH2_FX_CONNECTION_LOST:
		return ECONNABORTED;
	case LIBSSH2_FX_OP_UNSUPPORTED:
		return EOPNOTSUPP;
	case LIBSSH2_FX_INVALID_HANDLE:
		return EBADF;
	default:
		return EIO;
	}
}

void libssh2_logger(LIBSSH2_SESSION* session,
	void* context, const char* data, size_t length)
{
	(void)session; (void)context; (void)length;
	printf("libssh2: %s\n", data);
}

int gd_threads(int n, int c)
{
	/* guess number of threads in this app
	 * n: ThreadCount arg, c: number of cores
	 * w: winfsp threads = n < 1 ? c : max(2, n) + 1
	 * t: total = w + c + main thread */
	return (n < 1 ? c : max(2, n)) + c + 2;
}

int load_json(GDCONFIG* fs)
{
	if (!file_exists(fs->json)) {
		fprintf(stderr, "cannot read json file: %s\n", fs->json);
		return 1;
	}
	char* JSON_STRING = 0;
	size_t size = 0;
	FILE* fp = fopen(fs->json, "r");
	if (!fp) {
		fprintf(stderr, "cannot open json file: %s\n", fs->json);
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);
	JSON_STRING = calloc(size + 1, sizeof(char));
	if (!JSON_STRING) {
		fclose(fp);
		fprintf(stderr, "out of memory reading json\n");
		return 1;
	}
	fread(JSON_STRING, size, 1, fp);
	JSON_STRING[size] = '\0';
	fclose(fp);

	/* token walk lives in the unit-tested pure helper (src/cli/parse.c) */
	gd_json j;
	int rc = parse_json_buffer(JSON_STRING, fs->drive, &j);
	free(JSON_STRING);
	if (rc != 0) {
		fprintf(stderr, "Failed to parse JSON\n");
		return 1;
	}

	if (j.has_logfile)  fs->logfile  = strdup(j.logfile);
	if (j.has_usageurl) fs->usageurl = strdup(j.usageurl);
	if (j.has_pkey)     fs->pkey     = strdup(j.pkey);
	if (j.has_args)     fs->args     = strdup(j.args);
	return 0;
}

void gd_log(const char* fmt, ...)
{
	AcquireSRWLockExclusive(&g_log_lock);
	char message[1000];
	memset(message, 0, 1000);
	va_list args;
	va_start(args, fmt);
	vsnprintf(message, sizeof(message), fmt, args);
	va_end(args);
	printf("%s", message);

	if (g_logfile) {
		FILE* f = fopen(g_logfile, "a");
		if (f != NULL) {
			char time_s[TIME_SIZE];
			time_str(time_mu(), time_s);
			fprintf(f, "%s: CLI: %d: %s", time_s, GetCurrentThreadId(), message);
			fclose(f);
		}
	}
	ReleaseSRWLockExclusive(&g_log_lock);
}

int _post(const char* url, const char* data)
{
	wchar_t wurl[MAX_PATH];
	mbstowcs_s(0, wurl, strlen(url) + 1, url, MAX_PATH);
	URL_COMPONENTS urlComp;
	ZeroMemory(&urlComp, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(urlComp);
	urlComp.dwSchemeLength = (DWORD)-1;
	urlComp.dwHostNameLength = (DWORD)-1;
	urlComp.dwUrlPathLength = (DWORD)-1;
	urlComp.dwExtraInfoLength = (DWORD)-1;

	BOOL success = WinHttpCrackUrl(wurl, (DWORD)wcslen(wurl), 0, &urlComp);
	if (!success) {
		return 1;
	}
	wchar_t wschema[6];
	wchar_t whost[100];
	wchar_t wpath[100];
	ZeroMemory(wschema, sizeof(wschema));
	ZeroMemory(whost, sizeof(whost));
	ZeroMemory(wpath, sizeof(wpath));
	wcsncpy_s(wschema, 6,
		urlComp.lpszScheme, (rsize_t)urlComp.dwSchemeLength);
	wcsncpy_s(whost, 100,
		urlComp.lpszHostName, (rsize_t)urlComp.dwHostNameLength);
	wcsncpy_s(wpath, 100,
		urlComp.lpszUrlPath, (rsize_t)urlComp.dwUrlPathLength);
	INTERNET_PORT port = urlComp.nPort;

	DWORD datalen = (DWORD)strlen(data);

	LPWSTR phost = whost;
	LPWSTR ppath = wpath;
	LPCWSTR additionalHeaders = L"Content-Type: application/x-www-form-urlencoded\r\n";
	BOOL  bResults = FALSE;
	HINTERNET  hSession = NULL;
	HINTERNET  hConnect = NULL;
	HINTERNET  hRequest = NULL;

	hSession = WinHttpOpen(L"Golddrive-WinHttp/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);

	if (hSession) {
		if (!WinHttpSetTimeouts(hSession, 2000, 2000, 1000, 10)) {
			printf("Error %u in WinHttpSetTimeouts.\n", GetLastError());
			return 1;
		}
		hConnect = WinHttpConnect(hSession, phost, port, 0);
	}
	int secflag = !wcscmp(wschema, L"https") ? WINHTTP_FLAG_SECURE : 0;
	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, L"POST", ppath,
			NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			secflag);
	if (!hRequest) {
		if (hConnect) WinHttpCloseHandle(hConnect);
		if (hSession) WinHttpCloseHandle(hSession);
		return 1;
	}
	DWORD headersLength = (DWORD)-1;

	int retry = 0;
	int retries = 0;
	int maxretries = 1;
	do {
		retry = 0;

		bResults = WinHttpSendRequest(hRequest,
			additionalHeaders, headersLength, (LPVOID)data,
			datalen, datalen, 0);

		if (bResults == FALSE) {
			int result = GetLastError();

			if (result == ERROR_WINHTTP_SECURE_FAILURE) {
				/* do not bypass certificate validation */
				gd_log("SSL certificate validation failed for usage URL\n");
				break;
			}
			else if (result == ERROR_WINHTTP_RESEND_REQUEST) {
				retry = 1;
				retries++;
				if (retries > maxretries)
					break;
			}
		}
	} while (retry);

	if (!bResults) {
		int err = GetLastError();
		printf("Error %d has occurred.\n", err);
		return 1;
	}
	if (hRequest)
		WinHttpCloseHandle(hRequest);
	if (hConnect)
		WinHttpCloseHandle(hConnect);
	if (hSession)
		WinHttpCloseHandle(hSession);
	return 0;
}

DWORD WINAPI _post_background(LPVOID data)
{
	usagedata* d = (usagedata*)data;
	int rc = 0;
	rc = _post(d->url, d->data);
	free(d);
	return rc;
}

HANDLE* gd_usage(const char* action, const char* data)
{
	if (!g_conf.usageurl)
		return 0;
	/* validate URL starts with http:// or https:// */
	if (strncmp(g_conf.usageurl, "http://", 7) != 0 &&
		strncmp(g_conf.usageurl, "https://", 8) != 0)
		return 0;
	char hostname[256];
	gethostname(hostname, sizeof(hostname));
	char exepath[MAX_PATH];
	char version[100] = { 0 };
	GetModuleFileNameA(NULL, exepath, MAX_PATH);
	get_file_version(exepath, version);

	usagedata* d = malloc(sizeof(usagedata));
	if (!d)
		return 0;
	strcpy_s(d->url, MAX_PATH, g_conf.usageurl);
	strcpy_s(d->data, 1024, "application=GOLDDRIVE");
	strcat_s(d->data, 1024, "&version=");
	strcat_s(d->data, 1024, version);
	strcat_s(d->data, 1024, "&user=");
	strcat_s(d->data, 1024, g_conf.user);
	strcat_s(d->data, 1024, "&action=");
	strcat_s(d->data, 1024, action);
	strcat_s(d->data, 1024, "&client=");
	strcat_s(d->data, 1024, hostname);
	strcat_s(d->data, 1024, "&data=");
	strcat_s(d->data, 1024, data);

	HANDLE* thread = CreateThread(NULL, 0, _post_background, d, 0, NULL);
	return thread;
}
