#include <assert.h>
#include "config.h"
#include "util.h"
#include "gd.h"
#include "cache.h"
#include <Winhttp.h>


GDSSH* gd_init_ssh(void)
{
	int rc;
	char* errmsg = 0;
	int errlen;
	SOCKADDR_IN sin;
	HOSTENT* he;
	SOCKET sock;
	LIBSSH2_SESSION* ssh = NULL;
	LIBSSH2_SFTP* sftp = NULL;
	LIBSSH2_CHANNEL* channel = NULL;
	int thread = GetCurrentThreadId();

	// initialize windows socket
	WSADATA wsadata;
	rc = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (rc != 0) {
		gd_log("%zd: %d :ERROR: %s: %d: WSAStartup failed, rc=%d\n",
			time_mu(), thread, __func__, __LINE__, rc);
		return 0;
	}


	// init ssh
	rc = libssh2_init(0);
	if (rc) {
		gd_log("%zd: %d :ERROR: %s: %d: "
			"failed to initialize crypto library, rc=%d\n",
			time_mu(), thread, __func__, __LINE__, rc);
		return 0;
	}

	/* Create a session instance */
	ssh = libssh2_session_init();
	if (!ssh) {
		gd_log("%zd: %d :ERROR: %s: %d: "
			"failed allocate memory for ssh session\n",
			time_mu(), thread, __func__, __LINE__);
		return 0;
	}


	/* supported symetric algorithms */
	//const char** algorithms;
	//rc = libssh2_session_supported_algs(ssh, LIBSSH2_METHOD_CRYPT_CS, &algorithms);
	//if (rc > 0) {
	//	gd_log("Supported symmetric encryption:\n");
	//	for (int i = 0; i < rc; i++)
	//		gd_log("\t%s\n", algorithms[i]);
	//	libssh2_free(ssh, algorithms);
	//}
	//rc = libssh2_session_supported_algs(ssh, LIBSSH2_METHOD_KEX, &algorithms);
	//if (rc > 0) {
	//	gd_log("Supported key exchange:\n");
	//	for (int i = 0; i < rc; i++)
	//		gd_log("\t%s\n", algorithms[i]);
	//	libssh2_free(ssh, algorithms);
	//}
	//rc = libssh2_session_supported_algs(ssh, LIBSSH2_METHOD_HOSTKEY, &algorithms);
	//if (rc > 0) {
	//	gd_log("Supported host keys:\n");
	//	for (int i = 0; i < rc; i++)
	//		gd_log("\t%s\n", algorithms[i]);
	//	libssh2_free(ssh, algorithms);
	//}
	//rc = libssh2_session_supported_algs(ssh, LIBSSH2_METHOD_MAC_CS, &algorithms);
	//if (rc > 0) {
	//	gd_log("Supported MAC:\n");
	//	for (int i = 0; i < rc; i++)
	//		gd_log("\t%s\n", algorithms[i]);
	//	libssh2_free(ssh, algorithms);
	//}
	//// set compression to get info
	//libssh2_session_flag(ssh, LIBSSH2_FLAG_COMPRESS, 1);
	//rc = libssh2_session_supported_algs(ssh, LIBSSH2_METHOD_COMP_CS, &algorithms);
	//if (rc > 0) {
	//	gd_log("Supported compression:\n");
	//	for (int i = 0; i < rc; i++)
	//		gd_log("\t%s\n", algorithms[i]);
	//	libssh2_free(ssh, algorithms);
	//}

	/* debug, need to build with tracing */
	//libssh2_trace(ssh, 
	//	//LIBSSH2_TRACE_SFTP | LIBSSH2_TRACE_ERROR | LIBSSH2_TRACE_CONN
	//	//LIBSSH2_TRACE_SFTP | LIBSSH2_TRACE_ERROR
	//		//LIBSSH2_TRACE_TRANS 
	//		
	//	LIBSSH2_TRACE_KEX   
	//		| LIBSSH2_TRACE_AUTH  
	//		| LIBSSH2_TRACE_CONN  
	//		| LIBSSH2_TRACE_SCP   
	//		| LIBSSH2_TRACE_SFTP  
	//		| LIBSSH2_TRACE_ERROR 
	//		| LIBSSH2_TRACE_PUBLICKEY 
	//		| LIBSSH2_TRACE_SOCKET
	//);
	//libssh2_trace_sethandler(ssh, 0, libssh2_logger);

	// compression
	//libssh2_session_flag(ssh, LIBSSH2_FLAG_COMPRESS, g_conf.compress);

	// encryption
	if (g_conf.cipher) {
		rc = libssh2_session_method_pref(ssh, 
			LIBSSH2_METHOD_CRYPT_CS, g_conf.cipher);
		rc = libssh2_session_method_pref(ssh, 
			LIBSSH2_METHOD_CRYPT_SC, g_conf.cipher);
			//while (libssh2_session_method_pref(ssh, LIBSSH2_METHOD_CRYPT_CS,
		//	g_conf.cipher) == LIBSSH2_ERROR_EAGAIN);
		//while (libssh2_session_method_pref(ssh, LIBSSH2_METHOD_MAC_CS,
		//	g_conf.cipher) == LIBSSH2_ERROR_EAGAIN);
		//while (libssh2_session_method_pref(ssh, LIBSSH2_METHOD_MAC_SC,
		//	g_conf.cipher) == LIBSSH2_ERROR_EAGAIN);

		if (rc) {
			rc = libssh2_session_last_error(ssh, &errmsg, &errlen, 0);
			gd_log("%zd: %d :ERROR: %s: %d: "
				"failed to set cipher [rc=%d, %s]\n",
				time_mu(), thread, __func__, __LINE__, rc, errmsg);
			//LIBSSH2_ERROR_METHOD_NOT_SUPPORTED
			return 0;
		}
	}

	// non-blocking mode by 
	// g_conf.block = 1;
	libssh2_session_set_blocking(ssh, g_conf.block);

	/* create socket  */
	he = gethostbyname(g_conf.host);
	if (!he) {
		gd_log("%zd: %d :ERROR: %s: %d: host not found: %s\n",
			time_mu(), thread, __func__, __LINE__, g_conf.host);
		return 0;
	}
	sin.sin_addr.s_addr = **(int**)he->h_addr_list;
	sin.sin_family = AF_INET;
	sin.sin_port = htons((u_short)g_conf.port);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	rc = connect(sock, (SOCKADDR*)(&sin), sizeof(SOCKADDR_IN));
	if (rc) {
		gd_log("%zd: %d :ERROR: %s: %d: "
			"failed to open socket, host not found or port not open, rc=%d\n",
			time_mu(), thread, __func__, __LINE__, rc);
		return 0;
	}

	/* ... start it up. This will trade welcome banners, exchange keys,
	* and setup crypto, compression, and MAC layers	*/
	while ((rc = libssh2_session_handshake(ssh, sock)) ==
		LIBSSH2_ERROR_EAGAIN)
		Sleep(10);

	if (rc) {
		rc = libssh2_session_last_error(ssh, &errmsg, &errlen, 0);
		gd_log("%zd: %d :ERROR: %s: %d: "
			"failed to complete ssh handshake [rc=%d, %s]\n",
			time_mu(), thread, __func__, __LINE__, rc, errmsg);
		return 0;
	}

	//gd_log("Session symmetric encryption:\n\t%s\n", 
	//	libssh2_session_methods(ssh, LIBSSH2_METHOD_CRYPT_CS));
	//gd_log("Session key exchange:\n\t%s\n",
	//	libssh2_session_methods(ssh, LIBSSH2_METHOD_KEX));
	//gd_log("Session host keys:\n\t%s\n",
	//	libssh2_session_methods(ssh, LIBSSH2_METHOD_HOSTKEY));
	//gd_log("Session MAC:\n\t%s\n",
	//	libssh2_session_methods(ssh, LIBSSH2_METHOD_MAC_CS));
	
	/* At this point we havn't yet authenticated.  The first thing to do
	 * is check the hostkey's fingerprint against our known hosts Your app
	 * may have it hard coded, may go to a file, may present it to the
	 * user, that's your call
	 */
	 //char* fingerprint = libssh2_hostkey_hash(ssh, LIBSSH2_HOSTKEY_HASH_SHA1);
	 //gd_log("Fingerprint: ");
	 //for (int i = 0; i < 20; i++) {
	 //	gd_log("%02X ", (unsigned char)fingerprint[i]);
	 //}
	 //gd_log("\n");

	 /* check what authentication methods are available */
	//char* userauthlist = NULL;
	//do {
	//	userauthlist = libssh2_userauth_list(
	//		ssh, g_conf.user, (unsigned int)strlen(g_conf.user));
	//} while (!userauthlist && libssh2_session_last_errno(ssh) == 
	//	LIBSSH2_ERROR_EAGAIN);
	//
	//if (strstr(userauthlist, "publickey") == NULL) {
	//	gd_log("Publick key authentication not available in server.\n");
	//	gd_log("Authentication methods: %s\n", userauthlist);
	//	return 0;
	//}


	 // authenticate with keys
	char pubkey[1000];
	strcpy(pubkey, g_conf.pkey);
	strcat(pubkey, ".pub");
	while ((rc = libssh2_userauth_publickey_fromfile(
		ssh, g_conf.user, pubkey, g_conf.pkey, NULL)) ==
		LIBSSH2_ERROR_EAGAIN)
		Sleep(10);
	
	// or password
	//while ((rc = libssh2_userauth_password(
	//	ssh, g_conf.user, "support")) ==
	//	LIBSSH2_ERROR_EAGAIN);

	if (rc) {
		rc = libssh2_session_last_error(ssh, &errmsg, &errlen, 0);
		gd_log("%zd: %d :ERROR: %s: %d: "
			"authentication by public key failed [rc=%d, %s]\n",
			time_mu(), thread, __func__, __LINE__, rc, errmsg);
		return 0;
	}

	// users was autheticated

	// init sftp channel
	do {
		sftp = libssh2_sftp_init(ssh);
		if ((!sftp) && (libssh2_session_last_errno(ssh) !=
			LIBSSH2_ERROR_EAGAIN))
		{
			gd_log("%zd: %d :ERROR: %s: %d: "
				"failed to start sftp session [rc=%d, %s]\n",
				time_mu(), thread, __func__, __LINE__, rc, errmsg);
			return 0;
		}
	} while (!sftp);

	do {
		channel = libssh2_channel_open_session(ssh);
		if ((!channel) && (libssh2_session_last_errno(ssh) !=
			LIBSSH2_ERROR_EAGAIN))
			break;
	} while (!channel);
	if (!channel) {
		rc = libssh2_session_last_error(ssh, &errmsg, NULL, 0);
		log_error("ERROR: invalid channel to run commands, rc=%d, %s\n", rc, errmsg);
		return 0;
	}
	else {
		while ((rc = libssh2_channel_shell(channel)) ==
			LIBSSH2_ERROR_EAGAIN)
			Sleep(10);
		if (rc) {
			rc = libssh2_session_last_error(ssh, &errmsg, NULL, 0);
			gd_log("cannot request shell: [rc=%d, %s]\n", rc, errmsg);
			return 0;
		}
	}
	


	g_ssh = malloc(sizeof(GDSSH));
	if (g_ssh) {
		g_ssh->socket = sock;
		g_ssh->ssh = ssh;
		g_ssh->sftp = sftp;
		g_ssh->channel = channel;
		g_ssh->thread = GetCurrentThreadId();
	}
	return g_ssh;
}

int gd_finalize(void)
{
	log_info("FINALIZE\n");

	while (libssh2_channel_close(g_ssh->channel) ==
		LIBSSH2_ERROR_EAGAIN);
	while (libssh2_channel_free(g_ssh->channel) ==
		LIBSSH2_ERROR_EAGAIN);
	while (libssh2_sftp_shutdown(g_ssh->sftp) ==
		LIBSSH2_ERROR_EAGAIN);
	while (libssh2_session_disconnect(g_ssh->ssh, "ssh session disconnected") ==
		LIBSSH2_ERROR_EAGAIN);
	while (libssh2_session_free(g_ssh->ssh) ==
		LIBSSH2_ERROR_EAGAIN);


	libssh2_exit();
	closesocket(g_ssh->socket);

	free(g_ssh);
	printf("sftp calls: %zu\n", g_sftp_calls);
	printf("cache hits: %zu\n", g_cache_calls);

	return 0;
}

int gd_stat(const char* path, struct fuse_stat* stbuf)
{
	log_info("%s\n", path);
	int rc = 0;

#ifdef USE_CACHE
	CACHE_STAT* cstat = cache_stat_find(path);
	if (cstat) {
		copy_attributes(stbuf, &cstat->attrs);
		cache_stat_lock();
		g_cache_calls++;
		cache_stat_unlock();
		return 0;
	} 
#endif
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
	}
	copy_attributes(stbuf, &attrs);

#ifdef USE_CACHE
	cstat = malloc(sizeof * cstat);
	cstat->attrs = attrs;
	strcpy_s(cstat->path, MAX_PATH, path);
	cache_stat_add(cstat);
#endif 
	return rc;

}

int gd_fstat(intptr_t fd, struct fuse_stat* stbuf)
{

	int rc = 0;
	GDHANDLE* sh = (GDHANDLE*)fd;

	return gd_stat(sh->path, stbuf);

	//LIBSSH2_SFTP_HANDLE* handle = sh->file_handle;
	//assert(handle);

	//LIBSSH2_SFTP_ATTRIBUTES attrs;

	//gd_lock();
	//while ((rc = libssh2_sftp_fstat_ex(handle, &attrs, 0)) ==
	//	LIBSSH2_ERROR_EAGAIN) {
	//	waitsocket(g_ssh);
	//	g_sftp_calls++;
	//}
	//gd_unlock();

	//log_debug("rc=%d, %s\n", rc, sh->path);
	//if (rc < 0) {
	//	gd_error(sh->path);
	//	rc = error();
	//}
	//// debugging libssh2 write issue
	////sh->size = attrs.filesize;
	//copy_attributes(stbuf, &attrs);
	////log_info("%zu, size=%zu\n", (size_t)handle, attrs.filesize);
	//return rc;
}

int gd_readlink(const char* path, char* buf, size_t size)
{
	log_info("%s, size=%zu\n", path, size);
	log_debug("%s, size=%zu, buf=%s\n", path, size, buf);
	int rc;
	assert(size > 0);

	char* target = malloc(MAX_PATH);
	// rc is number of bytes in target
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
	assert(rc < size);

	// replace double slashes
	char* output = malloc(MAX_PATH);	
	char c = 0;
	char* t = target;
	char* o = output;
	for (int i = 0; *t != '\0'; i++) {
		if (t[0] == '/' && c == '/') {
			rc--;
		}
		else {
			c = t[0];
			*o++ = c;
		}
		t++;	
	}


	strncpy(buf, output, rc);
	buf[rc] = '\0';
	free(output);
	free(target);	
	return 0;
}

int gd_mkdir(const char* path, fuse_mode_t mode)
{
	int rc = 0;
	log_info("%s, mode=%u\n", path, mode);
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

	return rc;
}

int gd_rmdir(const char* path)
{
	// rm path/* and rmdir path
	int rc = 0;
	log_info("%s\n", path);

	if (g_conf.fastrm) {
		// rename dir/folder -> dir/.deleted_folder
		// call rm -rf dir/.deleted_folder
		//char deleted[MAX_PATH];
		//char drive[_MAX_DRIVE];
		//char dir[_MAX_DIR];
		//char fname[_MAX_FNAME];
		//char ext[_MAX_EXT];
		//rc = _splitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
		//if (rc != 0) {
		//	gd_error(path);
		//	rc = error();
		//	return rc;
		//}
		//// rename		
		//rc = sprintf_s(deleted, sizeof deleted, "%s.deleted_%s_%s", dir, fname, ext);
		//rc = gd_rename(path, deleted);

		//if (rc) {
		//	gd_error(path);
		//	rc = error();
		//}

		//char cmd[COMMAND_SIZE];
		//char out[COMMAND_SIZE];
		//// FIXME: do not hard-code stat path, find it using which cmd after login
		//sprintf_s(cmd, sizeof cmd, "/bin/rm -rf \"%s.deleted_%s_%s\"", dir, fname, ext);
		//gd_lock();
		//rc = run_command_channel_exec(cmd, out, 0);
		//gd_unlock();
	}
	else {
		gd_lock();
		while ((rc = libssh2_sftp_rmdir_ex(
			g_ssh->sftp, path, (int)strlen(path))) ==
			LIBSSH2_ERROR_EAGAIN) {
			waitsocket(g_ssh);
			g_sftp_calls++;
		}
		gd_unlock();
	}

	if (rc < 0) {
		gd_error(path);
		rc = error();
	}

	//if (g_conf.audit) {
	//	gd_log("%s: RMDIR: %s\n", g_conf.user, path);
	//}

	return rc;
}

static int _gd_rename(const char* from, const char* to)
{
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
			strcpy(totmp, to);
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
	return rc ? -1 : 0;

}

int gd_truncate(const char* path, fuse_off_t size)
{
	int rc = 0;
	//log_error("%s, size=%zu\n", path, size);
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

	//struct fuse_stat stbuf;
	//gd_stat(path, &stbuf);
	//log_error("new size: %zu\n", stbuf.st_size);
	return rc;
}

int gd_ftruncate(intptr_t fd, fuse_off_t size)
{
	int rc = 0;
	GDHANDLE* sh = (GDHANDLE*)fd;
	rc = gd_truncate(sh->path, size);
	return rc;
}

intptr_t gd_open(const char* path, int flags, unsigned int mode)
{
	int rc;
	GDHANDLE* sh = malloc(sizeof(GDHANDLE));
	assert(sh);
	sh->file_handle = 0;
	sh->dir_handle = 0;
	strcpy_s(sh->path, MAX_PATH, path);
	// sh->mode = 0777 | ((LIBSSH2_SFTP_S_ISDIR(mode)) ? S_IFDIR : 0);
	//sh->mode = mode & 0777;
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
		return -EINVAL;;
	}

	if (flags & O_CREAT)
		pflags |= GD_CREAT;

	if (flags & O_EXCL)
		pflags |= GD_EXCL;

	if (flags & O_TRUNC)
		pflags |= GD_TRUNC;

	if (flags & O_APPEND)
		pflags |= GD_APPEND;

	//log_error("%s, mode=%u, flags=%d\n", path, mode, pflags);

	//error("%s, pflags: %d\n", path, pflags);
	sh->flags = pflags;

	// check if file has hard links
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
			g_ssh->sftp, sh->path, (int)strlen(sh->path),
			sh->flags, sh->mode, LIBSSH2_SFTP_OPENFILE);
		g_sftp_calls++;
		if (!handle && libssh2_session_last_errno(g_ssh->ssh) !=
			LIBSSH2_ERROR_EAGAIN)
			break;
	} while (!handle);

	if (!handle) {
		gd_error(sh->path);
		gd_unlock();
		return error();
	}
	gd_unlock();

	sh->file_handle = handle;

	log_info("OPEN HANDLE : %zu:%zu: %s, flags=%d, mode=%d\n",
		(size_t)sh, (size_t)handle, sh->path, sh->flags, sh->mode);

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
		// log read event
		gd_log("%s: READ: %s\n", g_conf.user, sh->path);
	}

	LIBSSH2_SFTP_HANDLE* handle = sh->file_handle;

	log_info("READING HANDLE: %zu size=%zu, offset=%zu\n",
		(size_t)handle, size, offset);

	gd_lock();
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
		gd_error("ERROR: Unable to read chuck of file\n");
		rc = error();
		if (rc)
			total = -1;
	}
	gd_unlock();


	//if (rc == 0 && total > -1 && size != total)
	//{
	//	//log_warn("**************** WARNING ****************  need=%zu, actual=%zu, probably EOF\n",
	//	//	size, total);
	//	//rc = -1;
	//	//errno = 0; // EOF
	//	//total = -1;
	//}
	log_debug("FINISH READING HANDLE %zu, bytes: %zu\n", (size_t)handle, total);

	return total;// >= 0 ? (int)total : rc;
}

int gd_write(intptr_t fd, const void* buf, size_t size, fuse_off_t offset)
{
	GDHANDLE* sh = (GDHANDLE*)fd;
	int rc;
	int total = 0;
	size_t chunk = size;
	const char* pos = buf;
	//log_error("WRITING HANDLE: %zu size: %zu\n", (size_t)sh, size);

	if (g_conf.audit && offset == 0 && size > 0) {
		// log read event
		gd_log("%s: WRITE: %s\n", g_conf.user, sh->path);
	}


	LIBSSH2_SFTP_HANDLE* handle = sh->file_handle;
	log_info("WRITING HANDLE: %zu size: %zu\n", (size_t)handle, size);

	gd_lock();
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
		gd_error("ERROR: Unable to write chuck of data\n");
		rc = error();
		if (rc)
			total = -1;
	}
	gd_unlock();

	log_debug("FINISH WRITING %zu, bytes: %zu\n", (size_t)handle, total);
	return total;// >= 0 ? (int)total : rc;
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
	stbuf->f_bsize = stvfs.f_bsize;			/* file system block size */
	stbuf->f_frsize = stvfs.f_frsize;		/* fragment size */
	stbuf->f_blocks = stvfs.f_blocks;		/* size of fs in f_frsize units */
	stbuf->f_bfree = stvfs.f_bfree;			/* # free blocks */
	stbuf->f_bavail = stvfs.f_bavail;		/* # free blocks for non-root */
	stbuf->f_files = stvfs.f_files;			/* # inodes */
	stbuf->f_ffree = stvfs.f_ffree;			/* # free inodes */
	stbuf->f_favail = stvfs.f_favail;		/* # free inodes for non-root */
	//stbuf->f_fsid = stvfs.f_fsid;			/* file system ID */
	//stbuf->f_flag = stvfs.f_flag;			/* mount flags */
	stbuf->f_namemax = stvfs.f_namemax;		/* maximum filename length */


	// mac
	//stbuf->f_namemax = 255;
	//stbuf->f_bsize = stvfs.f_bsize;
	///*
	// * df seems to use f_bsize instead of f_frsize, so make them
	// * the same
	// */
	//stbuf->f_frsize = stbuf->f_bsize;
	//stbuf->f_blocks = stbuf->f_bfree = stbuf->f_bavail =
	//	1000ULL * 1024 * 1024 * 1024 / stbuf->f_frsize;
	//stbuf->f_files = stbuf->f_ffree = 1000000000;
	//return 0;
	return rc;
}

int gd_close(intptr_t fd)
{
	int rc = 0;
	GDHANDLE* sh = (GDHANDLE*)fd;
	LIBSSH2_SFTP_HANDLE* handle;
	handle = sh->file_handle;
	assert(handle);
	gd_lock();
	while ((rc = libssh2_sftp_close_handle(handle)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	if (rc < 0) {
		gd_error(sh->path);
		rc = error();
	}
	gd_unlock();
	log_info("CLOSE HANDLE: %zu:%zu\n", (size_t)sh, (size_t)handle);
	free(sh);
	sh = NULL;
	return rc;
}

GDDIR* gd_opendir(const char* path)
{
	log_info("%s\n", path);
	int rc = 0;
	GDDIR* dirp = 0;
	GDHANDLE* sh = malloc(sizeof(GDHANDLE));
	assert(sh);
	sh->file_handle = 0;
	sh->dir_handle = 0;
	unsigned int mode = 0;
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
		gd_unlock();
		rc = error();
		return 0;
	}
	gd_unlock();

	size_t pathlen = strlen(path);
	if (0 < pathlen && '/' == path[pathlen - 1])
		pathlen--;

	dirp = malloc(sizeof * dirp + pathlen + 2); /* sets errno */
	if (0 == dirp) {
		// fixme:
		// close handle
		return 0;
	}

	strcpy_s(sh->path, MAX_PATH, path);
	sh->dir_handle = handle;
	sh->dir = 1;
	memset(dirp, 0, sizeof * dirp);
	dirp->handle = sh;
	memcpy(dirp->path, path, pathlen);
	dirp->path[pathlen + 0] = '/';
	dirp->path[pathlen + 1] = '\0';
	return dirp;
}
void gd_rewinddir(GDDIR* dirp)
{
	GDHANDLE* sh = dirp->handle;
	LIBSSH2_SFTP_HANDLE* handle = sh->dir_handle;
	gd_lock();
	libssh2_sftp_seek64(handle, 0);
	g_sftp_calls++;
	gd_unlock();
}
struct GDDIRENT* gd_readdir(GDDIR* dirp)
{
	int rc;
	GDHANDLE* sh = dirp->handle;

	LIBSSH2_SFTP_HANDLE* handle = sh->dir_handle;
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	memset(&attrs, 0, sizeof attrs);
	char fname[FILENAME_MAX];

	gd_lock();
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
		// no more files
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
	gd_lock();
	while ((rc = libssh2_sftp_close_handle(handle)) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}
	if (rc < 0) {
		gd_error(dirfh->path);
		rc = error();
	}
	gd_unlock();
	//log_info("CLOSE HANDLE: %zu:%zu\n", (size_t)dirfh, (size_t)handle);
	free(dirfh);
	dirfh = NULL;
	free(dirp);
	dirp = NULL;
	return rc;
}

intptr_t gd_dirfd(GDDIR* dirp)
{
	return (intptr_t)dirp->handle;
}

int gd_check_hlink(const char* path)
{
	// check for hard link
	int rc = 0;
	char cmd[COMMAND_SIZE];
	char out[COMMAND_SIZE];

	// FIXME: use stat cmd until we get hlinks from sftp v6
	// FIXME: do not hard-code stat path, find it using which cmd after login
	sprintf_s(cmd, sizeof cmd, "/usr/bin/stat -c%%h \"%s\"", path);
	gd_lock();
	rc = run_command_channel_exec(cmd, out, 0);
	gd_unlock();
	int hlinks = 0;
	if (!rc) {
		hlinks = atoi(out);
	}
	if (hlinks > 1) {
		//error("opening for writing hard linked file: %s\n"
		//	"number of links: %d\n", path, hlinks);
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
		//printf("Path extracted with _splitpath_s:\n");
		//printf("   Drive: %s\n", drive);
		//printf("   Dir: %s\n", dir);
		//printf("   Filename: %s\n", fname);
		//printf("   Ext: %s\n", ext);

		// backup file		
		rc = sprintf_s(backup, sizeof backup, "%s.%s_%s_%zu.hlink",	dir, fname, ext, time_mu());

		rc = gd_rename(path, backup);

		if (rc) {
			gd_error(path);
			rc = error();
		}
		else {
			gd_lock();
			LIBSSH2_SFTP_HANDLE* handle;
			// fixme: AND mode with -o create_umask arg
			unsigned int mode = 432; // oct 660
			unsigned flags = LIBSSH2_FXF_READ | LIBSSH2_FXF_WRITE
				| LIBSSH2_FXF_CREAT | LIBSSH2_FXF_EXCL; // int 43

			do {
				handle = libssh2_sftp_open_ex(
					g_ssh->sftp, path, (int)strlen(path),
					flags, mode, LIBSSH2_SFTP_OPENFILE);
				g_sftp_calls++;
				if (!handle && libssh2_session_last_errno(g_ssh->ssh) !=
					LIBSSH2_ERROR_EAGAIN)
					break;

			} while (!handle);
			while ((rc = libssh2_sftp_close_handle(handle)) ==
				LIBSSH2_ERROR_EAGAIN) {
				waitsocket(g_ssh);
				g_sftp_calls++;
			}
			gd_unlock();
		}
	}
	return rc;
}

int gd_utimens(const char* path, const struct fuse_timespec tv[2], struct fuse_file_info* fi)
{
	log_info("%s\n", path);
	int rc = 0;

	UINT64 LastAccessTime, LastWriteTime;
	if (0 == tv) {
		FILETIME FileTime;
		GetSystemTimeAsFileTime(&FileTime);
		LastAccessTime = LastWriteTime = *(PUINT64)&FileTime;
	}
	else {
		FspPosixUnixTimeToFileTime((void*)&tv[0], &LastAccessTime);
		FspPosixUnixTimeToFileTime((void*)&tv[1], &LastWriteTime);
	}
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	attrs.flags = LIBSSH2_SFTP_ATTR_ACMODTIME;
	attrs.atime = (unsigned long)LastAccessTime;
	attrs.mtime = (unsigned long)LastWriteTime;
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
	return rc;
}

int gd_flush(intptr_t fd)
{
	return gd_fsync(fd);
	//return 0;
}
int gd_fsync(intptr_t fd)
{
	int rc = 0;
	GDHANDLE* sh = (GDHANDLE*)fd;
	LIBSSH2_SFTP_HANDLE* handle = sh->file_handle;
	assert(handle);
	log_info("%s\n", sh->path);
	// flush file ?
	gd_lock();
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
	return rc;
}


int run_command(const char* cmd, char* out, char* err)
{
	int rc = 0;
	size_t offset = 0;
	char buffer[0x4000];

	LIBSSH2_CHANNEL* channel;
	char* errmsg;

	do {
		channel = libssh2_channel_open_session(g_ssh->ssh);
		g_sftp_calls++;
		if (!channel && libssh2_session_last_errno(g_ssh->ssh) !=
			LIBSSH2_ERROR_EAGAIN)
			break;
	} while (!channel);

	if (!channel) {
		rc = libssh2_session_last_error(g_ssh->ssh, &errmsg, NULL, 0);
		log_debug("ERROR: unable to init ssh chanel, rc=%d, %s\n", rc, errmsg);
		return 1;
	}

	/*libssh2_channel_set_blocking(channel, g_conf.block);*/
	//g_sftp_calls++;

	while ((rc = libssh2_channel_exec(channel, cmd))
		== LIBSSH2_ERROR_EAGAIN) {
		waitsocket(g_ssh);
		g_sftp_calls++;
	}

	if (rc != 0) {
		rc = libssh2_session_last_error(g_ssh->ssh, &errmsg, NULL, 0);
		log_debug("ERROR: unable to execute command, rc=%d, %s\n", rc, errmsg);
		goto finish;
	}

	/* read stdout */
	out[0] = '\0';
	for (;;) {
		do {
			//char buffer[0x4000];

			rc = (int)libssh2_channel_read(channel, buffer, sizeof(buffer));

			if (rc > 0) {
				//strncat(out, buffer, bytesread);
				memcpy(out + offset, buffer, rc);
				offset += rc;
			}
		} while (rc > 0);

		if (rc == LIBSSH2_ERROR_EAGAIN)
			waitsocket(g_ssh);
		else
			break;
	}
	/* read stderr */
	if (err) {
		err[0] = '\0';
		offset = 0;
		for (;;) {
			do {
				//char buffer[0x4000];

				rc = (int)libssh2_channel_read_stderr(channel, buffer, sizeof(buffer));

				if (rc > 0) {
					//strncat(err, buffer, bytesread);
					memcpy(out + offset, buffer, rc);
					offset += rc;
				}
			} while (rc > 0);

			if (rc == LIBSSH2_ERROR_EAGAIN)
				waitsocket(g_ssh);
			else
				break;
		}
	}
	/* get exit code */
	while ((rc = libssh2_channel_close(channel)) ==
		LIBSSH2_ERROR_EAGAIN)
		waitsocket(g_ssh);

	//rc = libssh2_channel_close(channel);
	if (rc == 0)
		rc = libssh2_channel_get_exit_status(channel);
	else
		rc = 127;

finish:
	while ((rc = libssh2_channel_free(channel)) ==
		LIBSSH2_ERROR_EAGAIN)
		waitsocket(g_ssh);

	//log_error("command executed: %s\n", cmd);
	return (int)rc;
				}

int run_command_channel_exec(const char* cmd, char* out, char* err)
{
	int rc = 0;
	int rcode = 0;
	size_t offset = 0;

	memset(out, 0, COMMAND_SIZE);
	if (err)
		memset(err, 0, COMMAND_SIZE);



	//rcode = run_command(cmd, out, err);


	// TODO
	LIBSSH2_CHANNEL* channel = g_ssh->channel;
	char* errmsg;
	char buffer[COMMAND_SIZE];
	memset(buffer, 0, COMMAND_SIZE);

	if (!channel) {
		rc = libssh2_session_last_error(g_ssh->ssh, &errmsg, NULL, 0);
		log_error("ERROR: invalid channel to run commands, rc=%d, %s\n", rc, errmsg);
		return rc;
	}
	
	char newcmd[COMMAND_SIZE];
	strcpy(newcmd, cmd);
	strcat(newcmd, ";echo RCODE=$?\n");
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
			memcpy(out + offset, buffer, rc);
			offset += rc;
			if (libssh2_channel_eof(channel))
				break;
			//if (!libssh2_poll_channel_read(channel, 0))
			//	break;
			if (strstr(out, "RCODE="))
				break;
		}

		char* ptr = strstr(out, "RCODE=");
		if (ptr) {
			char* rcs = str_ndup(ptr + 6, 10);
			rcs[min(strcspn(rcs, "\n"), 10)] = '\0';
			rcode = atoi(rcs);
			free(rcs);
			*ptr = '\0';
			out[min(strcspn(out, "\n"), COMMAND_SIZE)] = '\0';
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
				memcpy(err + offset, buffer, rc);
				offset += rc;
				if (libssh2_channel_eof(channel))
					break;
				//if (!libssh2_poll_channel_read(channel,1))
				//	break;
				if(strstr(err, "\n"))
					break;
			}
			err[min(strcspn(err, "\n"), COMMAND_SIZE)] = '\0';
			//while ((rc = libssh2_channel_flush_stderr(channel)) ==
			//	LIBSSH2_ERROR_EAGAIN)
			//	Sleep(10);
		}

		/*if (err && libssh2_poll_channel_read(channel, 1)) {
			offset = 0;
			while ((rc = (int)libssh2_channel_read_ex(
				channel, 1, buffer, sizeof(buffer))) ==
				LIBSSH2_ERROR_EAGAIN) {
				waitsocket(g_ssh);
				g_sftp_calls++;
				if (libssh2_channel_eof(channel))
					break;
			}
			memcpy(err, buffer, rc);
			offset += rc;
			err[min(strcspn(err, "\r\n"), COMMAND_SIZE)] = '\0';
		}*/
	}
	
	//log_error("command executed: %s\n", cmd);
	return rcode;
}


//int gd_chmod(const char* path, fuse_mode_t mode)
//{
//	log_info("%s, mode=%u\n", path, mode);
//	/* we do not support file security */
//	return 0;
//}
//
//int gd_chown(const char* path, fuse_uid_t uid, fuse_gid_t gid)
//{
//	log_info("%s, uid=%d, gid=%d\n", path, uid, gid);
//	/* we do not support file security */
//	return 0;
//}
//
//int gd_setxattr(const char* path, const char* name, const char* value, size_t size, int flags)
//{
//	return 0;
//}
//
//int gd_getxattr(const char* path, const char* name, char* value, size_t size)
//{
//	return 0;
//}
//
//int gd_listxattr(const char* path, char* namebuf, size_t size)
//{
//	return 0;
//}
//
//int gd_removexattr(const char* path, const char* name)
//{
//	return 0;
//}


void copy_attributes(struct fuse_stat* stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs)
{
	//int isdir = LIBSSH2_SFTP_S_ISDIR(attrs->permissions);
	if (!attrs)
		return;
	memset(stbuf, 0, sizeof * stbuf);
	stbuf->st_uid = attrs->uid;
	stbuf->st_gid = attrs->gid;
	stbuf->st_mode = attrs->permissions;
	//stbuf->st_mode = 0777 | ((LIBSSH2_SFTP_S_ISDIR(attrs->permissions)) ? S_IFDIR : 0);
	stbuf->st_size = attrs->filesize;
	stbuf->st_birthtim.tv_sec = attrs->mtime;
	stbuf->st_atim.tv_sec = attrs->atime;
	stbuf->st_mtim.tv_sec = attrs->mtime;
	stbuf->st_ctim.tv_sec = attrs->mtime;
	stbuf->st_nlink = 1;
	/*if (LIBSSH2_SFTP_S_ISLNK(attrs->permissions)) {
		int a = attrs->permissions;
	}*/
#if defined(FSP_FUSE_USE_STAT_EX)
	//	if (hidden) {
	//		stbuf->st_flags |= FSP_FUSE_UF_READONLY;
	//	}
#endif
}




void get_filetype(unsigned long mode, char* filetype)
{
	if (LIBSSH2_SFTP_S_ISREG(mode))			strcpy_s(filetype, 4, "REG");
	else if (LIBSSH2_SFTP_S_ISDIR(mode))	strcpy_s(filetype, 4, "DIR");
	else if (LIBSSH2_SFTP_S_ISLNK(mode))	strcpy_s(filetype, 4, "LNK");
	else if (LIBSSH2_SFTP_S_ISCHR(mode))	strcpy_s(filetype, 4, "CHR");
	else if (LIBSSH2_SFTP_S_ISBLK(mode))	strcpy_s(filetype, 4, "BLK");
	else if (LIBSSH2_SFTP_S_ISFIFO(mode))	strcpy_s(filetype, 4, "FIF");
	else if (LIBSSH2_SFTP_S_ISSOCK(mode))	strcpy_s(filetype, 4, "SOC");
	else									strcpy_s(filetype, 4, "NAN");
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



//void print_permissions(const char* path, LIBSSH2_SFTP_ATTRIBUTES* attrs)
//{
//	char perm[10];
//	char ftype[4];
//	mode_human(attrs->permissions, perm);
//	get_filetype(attrs->permissions, ftype);
//	printf("%s %s %d %d %s\n", perm, ftype, attrs->uid, attrs->gid, path);
//}

//void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES* attrs)
//{
//	printf("path:  %s\n", path);
//	printf("flags: %ld\n", attrs->flags);
//	printf("size:  %zd\n", attrs->filesize);
//	printf("uid:   %ld\n", attrs->uid);
//	printf("gid:   %ld\n", attrs->gid);
//	printf("mode:  %ld\n", attrs->permissions);
//	printf("atime: %ld\n", attrs->atime);
//	printf("mtime: %ld\n", attrs->mtime);
//}

//void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS* st)
//{
//	printf("path:    %s\n", path);
//	printf("bsize:   %zd\n", st->f_bsize);    	/* file system block size */
//	printf("frsize:  %zd\n", st->f_frsize);   	/* fragment size */
//	printf("blocks:  %zd\n", st->f_blocks);   	/* size of fs in f_frsize units */
//	printf("bfree:   %zd\n", st->f_bfree);    	/* # free blocks */
//	printf("bavail:  %zd\n", st->f_bavail);   	/* # free blocks for non-root */
//	printf("files:   %zd\n", st->f_files);    	/* # inodes */
//	printf("ffree:   %zd\n", st->f_ffree);    	/* # free inodes */
//	printf("favail:  %zd\n", st->f_favail);   	/* # free inodes for non-root */
//	printf("fsid:    %zd\n", st->f_fsid);     	/* file system ID */
//	printf("flag:    %zd\n", st->f_flag);     	/* mount flags */
//	printf("namemax: %zd\n", st->f_namemax);  	/* maximum filename length */
//
//}


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
	// rc < 0: ssh error
	// rc > 0: sftp errot
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
	printf("libssh2: %s\n", data);
}


int gd_threads(int n, int c)
{
	// guess number of threads in this app
	// n: ThreadCount arg
	// c: number of cores
	// w: winfsp threads = n < 1 ? c : max(2, n) + 1
	// t: total = w + c + main thread
	return (n < 1 ? c : max(2, n)) + c + 2;
}

int jsoneq(const char* json, jsmntok_t* tok, const char* s)
{
	return (tok->type == JSMN_STRING
		&& (int)strlen(s) == tok->end - tok->start
		&& strncmp(json + tok->start, s, tok->end - tok->start) == 0) ? 0 : -1;
}

int load_json(GDCONFIG* fs)
{
	// fill json with json->drive parameters
	if (!file_exists(fs->json)) {
		fprintf(stderr, "cannot read json file: %s\n", fs->json);
		return 1;
	}
	char* JSON_STRING = 0;
	size_t size = 0;
	FILE* fp = fopen(fs->json, "r");
	fseek(fp, 0, SEEK_END); /* Go to end of file */
	size = ftell(fp); /* How many bytes did we pass ? */
	rewind(fp);
	JSON_STRING = calloc(size + 1, sizeof(char*)); /* size + 1 byte for the \0 */
	fread(JSON_STRING, size, 1, fp); /* Read 1 chunk of size bytes from fp into buffer */
	JSON_STRING[size] = '\0';
	fclose(fp);

	int i, r;
	char* val;
	jsmn_parser p;
	jsmntok_t* tok;
	/* We expect no more than 1024 tokens */
	//jsmntok_t t[1000];
	//int num_tokens = sizeof(t) / sizeof(t[0]);
	int num_tokens = 1024;
	jsmntok_t* t = malloc(num_tokens * sizeof(jsmntok_t)); 

	jsmn_init(&p);
	r = jsmn_parse(&p, JSON_STRING, strlen(JSON_STRING), 
		t, num_tokens);
	if (r < 0) {
		fprintf(stderr, "Failed to parse JSON: %d\n", r);
		free(t);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		fprintf(stderr, "Object expected, json type=%d\n", t[0].type);
		free(t);
		return 1;
	}

	/* Loop over all keys of the root object */
	for (i = 1; i < r; i++) {
		if (jsoneq(JSON_STRING, &t[i], "LogFile") == 0) {
			tok = &t[i + 1];
			val = str_ndup(JSON_STRING + tok->start, tok->end - tok->start);
			fs->logfile = strdup(val);
			free(val);
			i++;
		}
		else if (jsoneq(JSON_STRING, &t[i], "UsageUrl") == 0) {
			tok = &t[i + 1];
			val = str_ndup(JSON_STRING + tok->start, tok->end - tok->start);
			fs->usageurl = strdup(val);
			free(val);
			i++;
		}
		else if (jsoneq(JSON_STRING, &t[i], "Drives") == 0) {
			int size = t[i + 1].size;
			i++;
			for (int j = 0; j < size; j++) {
				tok = &t[i + 1];
				char* key = str_ndup(JSON_STRING + tok->start, tok->end - tok->start);
				jsmntok_t* v = &t[i + 2];

				if (strcmp(key, fs->drive) == 0) {
					i = i + 3;
					for (int k = 0; k < v->size; k++) {
						tok = &t[i + 1];
						if (tok->type == JSMN_STRING) {
							char* k = str_ndup(JSON_STRING + tok->start, tok->end - tok->start);
							if (strcmp(k, "Args") == 0) {
								tok = &t[i + 2];
								fs->args = str_ndup(JSON_STRING + tok->start, tok->end - tok->start);
							}
							free(k);
							i = i + 2;
						}
						else if (tok->type == JSMN_ARRAY) {
							i = i + tok->size + 1;
						}
					}
				}
				else {
					i = i + 3;
					for (int k = 0; k < v->size; k++) {
						tok = &t[i + 1];
						if (tok->type == JSMN_STRING) {
							i = i + 2;
						}
						else if (tok->type == JSMN_ARRAY) {
							i = i + tok->size + 1;
						}
					}
				}
				free(key);

				//else if (v->type == JSMN_ARRAY) {
				//	//fs->hostcount = v->size;
				//	//fs->hostlist = malloc(v->size);
				//	//for (int u = 0; u < v->size; u++) {
				//	//	jsmntok_t *h = &t[i+j+u+4];
				//	//	int ssize = h->end - h->start;
				//	//	//printf("  * %.*s\n", h->end - h->start, JSON_STRING + h->start); 
				//	//	fs->hostlist[u] = strndup(JSON_STRING + h->start, ssize);
				//	//	fs->hostlist[u][ssize] = '\0';
				//	//	//printf("host %d: %s\n", u+1, fs->hostlist[u]);
				//	//	
				//	//}
				//	//i += t[i + 1].size + 1;

				//	i = i + v->size + 1;
				//}
				//free(key);

			}
		}
		else {
			// assume key with 1 value, skip it
			i++;
		}
	}
	free(JSON_STRING);
	free(t);
	//printf("Hosts:\n");
	//i = 0;
	//while (json->hosts[i])
	//	printf("  - %s\n", json->hosts[i++]);
	//printf("Port: %s\n", json->port);
	//printf("Drive: %s\n", json->drive);
	//printf("Path: %s\n", json->path);
	return 0;
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
		int error = GetLastError();
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
	//wcsncpy_s(wpath, sizeof(wpath), urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
	int port = urlComp.nPort;


	DWORD datalen = (DWORD)strlen(data);
	//wchar_t* wpath = L"/";
	// convert char* to LPWSTR
	//wchar_t whost[MAX_PATH];
	//wchar_t wpath[MAX_PATH];
	//mbstowcs_s(0, whost, strlen(host) + 1, host, MAX_PATH);
	//mbstowcs_s(0, wpath, strlen(page), page, sizeof(wchar_t) * 100);
	//wpath[strlen(page)] = 0;
	//printf("schema: %ls\n", wschema);
	//printf("host: %ls\n", whost);
	//printf("port: %d\n", port);
	//printf("path: %ls\n", wpath);

	LPWSTR phost = whost;
	LPWSTR ppath = wpath;
	LPCWSTR additionalHeaders = L"Content-Type: application/x-www-form-urlencoded\r\n";
	BOOL  bResults = FALSE;
	HINTERNET  hSession = NULL;
	HINTERNET  hConnect = NULL;
	HINTERNET  hRequest = NULL;

	hSession = WinHttpOpen(L"HTTP Logger/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);

	if (hSession) {
		if (!WinHttpSetTimeouts(hSession, 4000, 4000, 2000, 10)) {
			printf("Error %u in WinHttpSetTimeouts.\n", GetLastError());
			return 1;
		}
		hConnect = WinHttpConnect(hSession, phost, port, 0);
	}
	//int secflag = https ? WINHTTP_FLAG_SECURE : 0;
	int secflag = WINHTTP_FLAG_SECURE;
	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, L"POST", ppath,
			NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,	secflag); 
    
    // to be calculated if additionalHeaders is a null terminated string
	DWORD headersLength = -1; 

	//bResults = WinHttpSendRequest(hRequest,
	//     additionalHeaders, headersLength, (LPVOID)data,
	//     datalen, datalen, 0);

	int retry = 0;
	int optionset = 0;
	int retries = 0;
	int maxretries = 1;
	do {
		retry = 0;

		int result = NO_ERROR;
		// no retry on success, possible retry on failure
		bResults = WinHttpSendRequest(hRequest,
			additionalHeaders, headersLength, (LPVOID)data,
			datalen, datalen, 0);

		if (bResults == FALSE) {


			result = GetLastError();

			// (1) If you want to allow SSL certificate errors and continue
			// with the connection, you must allow and initial failure and then
			// reset the security flags. From: "HOWTO: Handle Invalid Certificate
			// Authority Error with WinInet"
			// http://support.microsoft.com/default.aspx?scid=kb;EN-US;182888
			if (result == ERROR_WINHTTP_SECURE_FAILURE) {
				DWORD dwFlags =
					SECURITY_FLAG_IGNORE_UNKNOWN_CA |
					SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
					SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
					SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

				if (optionset)
					break;

				if (WinHttpSetOption(
					hRequest,
					WINHTTP_OPTION_SECURITY_FLAGS,
					&dwFlags,
					sizeof(dwFlags))) {
					retry = 1;
					optionset = 1;
				}
			}
			// (2) Negotiate authorization handshakes may return this error
			// and require multiple attempts
			// http://msdn.microsoft.com/en-us/library/windows/desktop/aa383144%28v=vs.85%29.aspx
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
		gd_log("Usage: error %d has occurred.\n", err);
		return 1;
	}

	gd_log("Usage: log sent.\n");

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
HANDLE* gd_usage(const char* message)
{
	if (!g_conf.usageurl)
		return 0;
	
	gd_log("Usage: sending %s %s %s\n", g_conf.user, g_conf.host, message);

	usagedata* d = malloc(sizeof(usagedata));
	strcpy_s(d->url, MAX_PATH, g_conf.usageurl);
	strcpy_s(d->data, 1024, "user=");
	strcat_s(d->data, 1024, g_conf.user);
	strcat_s(d->data, 1024, "&host=");
	strcat_s(d->data, 1024, g_conf.host);
	strcat_s(d->data, 1024, "&message=");
	strcat_s(d->data, 1024, message);

	HANDLE* thread = CreateThread(NULL, 0, _post_background, d, 0, NULL);
	return thread;
	// this must be at the end of main
	//WaitForSingleObject(thread, 5000);
	//CloseHandle(thread);
}

void gd_log(const char* fmt, ...)
{
	AcquireSRWLockExclusive(&g_log_lock);
	char message[1000];
	memset(message, 0, 1000);
	va_list args;
	va_start(args, fmt);
	vsprintf(message, fmt, args);
	va_end(args);
	printf("%s", message);

	if (g_logfile) {
		
		FILE* f = fopen(g_logfile, "a");
		if (f != NULL) {
			char time_s[TIME_SIZE];
			int ok = time_str(time_mu(), time_s);
			fprintf(f, "%s: CLI: %d: %s", time_s, GetCurrentThreadId(), message);
		}
		fclose(f);
	}
	ReleaseSRWLockExclusive(&g_log_lock);
}

// function to create a queue of given capacity. 
// It initializes size of queue as 0 
GDQUEUE* gd_create_queue(unsigned capacity)
{
	GDQUEUE* queue = (GDQUEUE*) malloc(sizeof(GDQUEUE));
	queue->capacity = capacity;
	queue->front = queue->size = 0;
	queue->rear = capacity - 1; // This is important, see the enqueue 
	queue->data = (char**)malloc(queue->capacity);
	return queue;
}

// Queue is full when size becomes equal to the capacity 
int gd_queue_is_full(GDQUEUE* queue)
{
	return (queue->size == queue->capacity);
}

// Queue is empty when size is 0 
int gd_queue_is_empty(GDQUEUE* queue)
{
	return (queue->size == 0);
}

// Function to add an item to the queue. 
// It changes rear and size 
void gd_enqueue(GDQUEUE* queue, char* item)
{
	if (gd_queue_is_full(queue))
		return;
	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->data[queue->rear] = item;
	queue->size = queue->size + 1;
	printf("data enqueued: %s\n", item);
}

// Function to remove an item from queue. 
// It changes front and size 
char* gd_dequeue(GDQUEUE* queue)
{
	if (gd_queue_is_empty(queue))
		return 0;
	char* item = queue->data[queue->front];
	queue->front = (queue->front + 1) % queue->capacity;
	queue->size = queue->size - 1;
	return item;
}

// Function to get front of queue 
//int gd_queue_front(struct Queue* queue)
//{
//	if (gd_queue_is_empty(queue))
//		return INT_MIN;
//	return queue->array[queue->front];
//}
//
//// Function to get rear of queue 
//int gd_queue_rear(struct Queue* queue)
//{
//	if (gd_queue_is_empty(queue))
//		return INT_MIN;
//	return queue->array[queue->rear];
//}
//
//// Driver program to test above functions./ 
//int main()
//{
//	struct Queue* queue = createQueue(1000);
//
//	enqueue(queue, 10);
//	enqueue(queue, 20);
//	enqueue(queue, 30);
//	enqueue(queue, 40);
//
//	printf("%d dequeued from queue\n\n", dequeue(queue));
//
//	printf("Front item is %d\n", front(queue));
//	printf("Rear item is %d\n", rear(queue));
//
//	return 0;
//}



//#if defined(FSP_FUSE_USE_STAT_EX)
//static inline uint32_t MapFileAttributesToFlags(UINT32 FileAttributes)
//{
//	uint32_t flags = 0;
//
//	if (FileAttributes & FILE_ATTRIBUTE_READONLY)
//		flags |= FSP_FUSE_UF_READONLY;
//	if (FileAttributes & FILE_ATTRIBUTE_HIDDEN)
//		flags |= FSP_FUSE_UF_HIDDEN;
//	if (FileAttributes & FILE_ATTRIBUTE_SYSTEM)
//		flags |= FSP_FUSE_UF_SYSTEM;
//	if (FileAttributes & FILE_ATTRIBUTE_ARCHIVE)
//		flags |= FSP_FUSE_UF_ARCHIVE;
//
//	return flags;
//}
//
//static inline UINT32 MapFlagsToFileAttributes(uint32_t flags)
//{
//	UINT32 FileAttributes = 0;
//
//	if (flags & FSP_FUSE_UF_READONLY)
//		FileAttributes |= FILE_ATTRIBUTE_READONLY;
//	if (flags & FSP_FUSE_UF_HIDDEN)
//		FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
//	if (flags & FSP_FUSE_UF_SYSTEM)
//		FileAttributes |= FILE_ATTRIBUTE_SYSTEM;
//	if (flags & FSP_FUSE_UF_ARCHIVE)
//		FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
//
//	return FileAttributes;
//}
//#endif


//#include <windows.h>
//#include <stdio.h>
//
//#define THREADCOUNT 4 
//
//HANDLE ghWriteEvent;
//HANDLE ghThreads[THREADCOUNT];
//
//DWORD WINAPI ThreadProc(LPVOID);
//
//void CreateEventsAndThreads(void)
//{
//	int i;
//	DWORD dwThreadID;
//
//	// Create a manual-reset event object. The write thread sets this
//	// object to the signaled state when it finishes writing to a 
//	// shared buffer. 
//
//	ghWriteEvent = CreateEvent(
//		NULL,               // default security attributes
//		TRUE,               // manual-reset event
//		FALSE,              // initial state is nonsignaled
//		TEXT("WriteEvent")  // object name
//	);
//
//	if (ghWriteEvent == NULL) {
//		printf("CreateEvent failed (%d)\n", GetLastError());
//		return;
//	}
//
//	// Create multiple threads to read from the buffer.
//
//	for (i = 0; i < THREADCOUNT; i++) {
//		// TODO: More complex scenarios may require use of a parameter
//		//   to the thread procedure, such as an event per thread to  
//		//   be used for synchronization.
//		ghThreads[i] = CreateThread(
//			NULL,              // default security
//			0,                 // default stack size
//			ThreadProc,        // name of the thread function
//			NULL,              // no thread parameters
//			0,                 // default startup flags
//			&dwThreadID);
//
//		if (ghThreads[i] == NULL) {
//			printf("CreateThread failed (%d)\n", GetLastError());
//			return;
//		}
//	}
//}
//
//void WriteToBuffer(VOID)
//{
//	// TODO: Write to the shared buffer.
//
//	printf("Main thread writing to the shared buffer...\n");
//
//	// Set ghWriteEvent to signaled
//
//	if (!SetEvent(ghWriteEvent)) {
//		printf("SetEvent failed (%d)\n", GetLastError());
//		return;
//	}
//}
//
//void CloseEvents()
//{
//	// Close all event handles (currently, only one global handle).
//
//	CloseHandle(ghWriteEvent);
//}
//
//int main(void)
//{
//	DWORD dwWaitResult;
//
//	// TODO: Create the shared buffer
//
//	// Create events and THREADCOUNT threads to read from the buffer
//
//	CreateEventsAndThreads();
//
//	// At this point, the reader threads have started and are most
//	// likely waiting for the global event to be signaled. However, 
//	// it is safe to write to the buffer because the event is a 
//	// manual-reset event.
//
//	WriteToBuffer();
//
//	printf("Main thread waiting for threads to exit...\n");
//
//	// The handle for each thread is signaled when the thread is
//	// terminated.
//	dwWaitResult = WaitForMultipleObjects(
//		THREADCOUNT,   // number of handles in array
//		ghThreads,     // array of thread handles
//		TRUE,          // wait until all are signaled
//		INFINITE);
//
//	switch (dwWaitResult) {
//		// All thread objects were signaled
//	case WAIT_OBJECT_0:
//		printf("All threads ended, cleaning up for application exit...\n");
//		break;
//
//		// An error occurred
//	default:
//		printf("WaitForMultipleObjects failed (%d)\n", GetLastError());
//		return 1;
//	}
//
//	// Close the events to clean up
//
//	CloseEvents();
//
//	return 0;
//}
//
//DWORD WINAPI ThreadProc(LPVOID lpParam)
//{
//	// lpParam not used in this example.
//	UNREFERENCED_PARAMETER(lpParam);
//
//	DWORD dwWaitResult;
//
//	printf("Thread %d waiting for write event...\n", GetCurrentThreadId());
//
//	dwWaitResult = WaitForSingleObject(
//		ghWriteEvent, // event handle
//		INFINITE);    // indefinite wait
//
//	switch (dwWaitResult) {
//		// Event object was signaled
//	case WAIT_OBJECT_0:
//		//
//		// TODO: Read from the shared buffer
//		//
//		printf("Thread %d reading from buffer\n",
//			GetCurrentThreadId());
//		break;
//
//		// An error occurred
//	default:
//		printf("Wait error (%d)\n", GetLastError());
//		return 0;
//	}
//
//	// Now that we are done reading the buffer, we could use another
//	// event to signal that this thread is no longer reading. This
//	// example simply uses the thread handle for synchronization (the
//	// handle is signaled when the thread terminates.)
//
//	printf("Thread %d exiting\n", GetCurrentThreadId());
//	return 1;
//}