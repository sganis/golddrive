#include "sanssh.h"
#include <stdio.h>

void usage(const char* prog)
{
	printf("Sanssh - sftp client\n");
	printf("usage: %s [options] <host> <port> <user> <remote file> <local file> [public key]\n", prog);
	printf("options:\n");
	printf("    -V           show version\n");
}
int file_exists(const char* path)
{
	DWORD attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}
//void get_filetype(unsigned long perm, char* filetype)
//{
//	if (LIBSSH2_SFTP_S_ISLNK(perm))			strcpy(filetype, "LNK");
//	else if (LIBSSH2_SFTP_S_ISREG(perm))	strcpy(filetype, "REG");
//	else if (LIBSSH2_SFTP_S_ISDIR(perm))	strcpy(filetype, "DIR");
//	else if (LIBSSH2_SFTP_S_ISCHR(perm))	strcpy(filetype, "CHR");
//	else if (LIBSSH2_SFTP_S_ISBLK(perm))	strcpy(filetype, "BLK");
//	else if (LIBSSH2_SFTP_S_ISFIFO(perm))	strcpy(filetype, "FIF");
//	else if (LIBSSH2_SFTP_S_ISSOCK(perm))	strcpy(filetype, "SOC");
//	else									strcpy(filetype, "NAN");
//}
#ifdef USE_WOLFSSH
static int wsUserAuth(byte authType,
	WS_UserAuthData* authData,
	void* ctx)
{
	int ret = WOLFSSH_USERAUTH_INVALID_AUTHTYPE;

#ifdef DEBUG_WOLFSSH
	/* inspect supported types from server */
	printf("Server supports ");
	if (authData->type & WOLFSSH_USERAUTH_PASSWORD) {
		printf("password authentication");
	}
	if (authData->type & WOLFSSH_USERAUTH_PUBLICKEY) {
		printf(" and public key authentication");
	}
	printf("\n");
	printf("wolfSSH requesting to use type %d\n", authType);
#endif

	/* We know hansel has a key, wait for request of public key */
	/*if (authData->type & WOLFSSH_USERAUTH_PUBLICKEY &&
		authData->username != NULL &&
		authData->usernameSz > 0 &&
		XSTRNCMP((char*)authData->username, "hansel",
			authData->usernameSz) == 0) {
		if (authType == WOLFSSH_USERAUTH_PASSWORD) {
			printf("rejecting password type with hansel in favor of pub key\n");
			return WOLFSSH_USERAUTH_FAILURE;
		}
	}*/

	if (authType == WOLFSSH_USERAUTH_PASSWORD) {
		const char* password = (const char*)ctx;
		authData->sf.password.password = password;
		authData->sf.password.passwordSz = (word32)strlen(password);
		ret = WOLFSSH_USERAUTH_SUCCESS;
	}
	else if (authType == WOLFSSH_USERAUTH_PUBLICKEY) {
		WS_UserAuthData_PublicKey* pk = &authData->sf.publicKey;

		/* we only have hansel's key loaded */
		if (authData->username != NULL && authData->usernameSz > 0 &&
			XSTRNCMP((char*)authData->username, "hansel",
				authData->usernameSz) == 0) {
			//pk->publicKeyType = userPublicKeyType;
			//pk->publicKeyTypeSz = (word32)WSTRLEN((char*)userPublicKeyType);
			//pk->publicKey = userPublicKey;
			//pk->publicKeySz = userPublicKeySz;
			//pk->privateKey = userPrivateKey;
			//pk->privateKeySz = userPrivateKeySz;
			ret = WOLFSSH_USERAUTH_SUCCESS;
		}
	}

	return ret;
}

static int load_file(const char* fileName, byte* buf, word32 bufSz)
{
	FILE* file;
	word32 fileSz;
	word32 readSz;

	if (fileName == NULL) return 0;

	if (WFOPEN(&file, fileName, "rb") != 0)
		return 0;
	fseek(file, 0, SEEK_END);
	fileSz = (word32)ftell(file);
	rewind(file);

	if (fileSz > bufSz) {
		fclose(file);
		return 0;
	}

	readSz = (word32)fread(buf, 1, fileSz, file);
	if (readSz < fileSz) {
		fclose(file);
		return 0;
	}

	fclose(file);

	return fileSz;
}
#endif
SANSSH * san_init(const char* hostname,	int port, const char* username, 
	const char* pkey, char* error)
{
	SANSSH* sanssh = NULL;
	int rc;
#ifdef USE_LIBSSH2
	
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

	/* create socket  */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(sock, (SOCKADDR*)(&sin), sizeof(SOCKADDR_IN)) != 0) {
		sprintf(error, "failed to open socket");
		return 0;
	}

	// init ssh
	rc = libssh2_init(0);
	if (rc) {
		sprintf(error, "ssh initialization %d\n", rc);
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
	char pubkey[1000];
	strcpy(pubkey, pkey);
	strcat(pubkey, ".pub");
	rc = libssh2_userauth_publickey_fromfile(ssh, username, pubkey, pkey, NULL);
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
    sanssh = malloc(sizeof(SANSSH));
	if (sanssh) {
		sanssh->socket = sock;
		sanssh->ssh = ssh;
		sanssh->sftp = sftp;
	}
#elif USE_LIBSSH
	ssh_session ssh = ssh_new();
	if (ssh == NULL)
		exit(-1);

	ssh_options_set(ssh, SSH_OPTIONS_HOST, hostname);
	ssh_options_set(ssh, SSH_OPTIONS_USER, username);
	ssh_options_set(ssh, SSH_OPTIONS_PORT, &port);
	ssh_options_set(ssh, SSH_OPTIONS_COMPRESSION, "no");
	ssh_options_set(ssh, SSH_OPTIONS_STRICTHOSTKEYCHECK, 0);
	ssh_options_set(ssh, SSH_OPTIONS_KNOWNHOSTS, "/dev/null");


	WSADATA wsadata;
	int err;
	err = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (err != 0) {
		fprintf(stderr, "WSAStartup failed with error: %d\n", err);
		return 1;
	}

	// Connect
	rc = ssh_connect(ssh);
	if (rc != SSH_OK) {
		fprintf(stderr, "Error connecting to localhost: %s\n", ssh_get_error(ssh));
		return 0;
	}
	
	ssh_key pkeyobj;
	rc = ssh_pki_import_privkey_file(pkey, "", NULL, NULL, &pkeyobj);
	if (rc != SSH_OK) {
		fprintf(stderr, "Cannot read private key\n");
		return 0;
	}
	rc = ssh_userauth_publickey(ssh, username, pkeyobj);
	if (rc != SSH_AUTH_SUCCESS) {
		fprintf(stderr, "Key authentication wrong\n");
		return 0;
	}

	sftp_session sftp;
	sftp = sftp_new(ssh);
	if (sftp == NULL) {
		fprintf(stderr, "Error allocating SFTP session: %s\n", ssh_get_error(ssh));
		return SSH_ERROR;
	}
	rc = sftp_init(sftp);
	if (rc != SSH_OK) {
		fprintf(stderr, "Error initializing SFTP session: %d.\n", sftp_get_error(sftp));
		sftp_free(sftp);
		return rc;
	}
	sanssh = malloc(sizeof(SANSSH));
	if (sanssh) {
		//sanssh->socket = sock;
		sanssh->ssh = ssh;
		sanssh->sftp = sftp;
	}
#elif USE_WOLFSSH
	// wolf
	WOLFSSH_CTX* ctx = NULL;
	WOLFSSH* ssh = NULL;
	SOCKET sock = INVALID_SOCKET;
	SOCKADDR_IN sin;
	HOSTENT* he;
	int rc;

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

	/* create socket  */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(sock, (SOCKADDR*)(&sin), sizeof(SOCKADDR_IN)) != 0) {
		sprintf(error, "failed to open socket");
		return 0;
	}

	#ifdef DEBUG_WOLFSSH
	//wolfSSH_Debugging_ON();
	#endif
		// init 
	wolfSSH_Init();
	ctx = wolfSSH_CTX_new(WOLFSSH_ENDPOINT_CLIENT, NULL);
	if (ctx == NULL) {
		sprintf(error, "Couldn't create wolfSSH client context.");
		return 0;
	}
	wolfSSH_SetUserAuth(ctx, wsUserAuth);
	ssh = wolfSSH_new(ctx);
	if (ssh == NULL) {
		sprintf(error, "Couldn't create wolfSSH session.");
		return 0;
	}

	//wolfSSH_CTX_UsePrivateKey_buffer(ctx, pkbuffer, pkbufferSz, 0);
	//wolfSSH_CTX_SetPublicKeyCheck(ctx, wsPublicKeyCheck);
	//wolfSSH_SetPublicKeyCheckCtx(ssh, (void*)"You've been sampled!");

	// pkey is password for now
	wolfSSH_SetUserAuthCtx(ssh, (void*)pkey);

	rc = wolfSSH_SetUsername(ssh, username);
	if (rc != WS_SUCCESS) {
		sprintf(error, "Couldn't set the username.");
		return 0;
	}
	rc = wolfSSH_set_fd(ssh, (int)sock);
	if (rc != WS_SUCCESS) {
		sprintf(error, "Couldn't set the session's socket.");
		return 0;
	}

	rc = wolfSSH_SFTP_connect(ssh);

	if (rc != WS_SUCCESS) {
		sprintf(error, "Couldn't connect SFTP");
		return 0;
	}
	sanssh = malloc(sizeof(SANSSH));
	if (sanssh) {
		sanssh->socket = sock;
		sanssh->ssh = ssh;
	}
#endif
	return sanssh;
}
int san_finalize(SANSSH *sanssh)
{
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	while (libssh2_sftp_shutdown(sanssh->sftp) ==
		LIBSSH2_ERROR_EAGAIN);
	while (libssh2_session_disconnect(sanssh->ssh, "ssh session disconnected") ==
		LIBSSH2_ERROR_EAGAIN);
	while (libssh2_session_free(sanssh->ssh) ==
		LIBSSH2_ERROR_EAGAIN);
	libssh2_exit();
#elif USE_WOLFSSH

#endif

	closesocket(sanssh->socket);
	WSACleanup();
	free(sanssh);
	return 0;
}
int waitsocket(SANSSH *sanssh)
{
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	struct timeval timeout;
	int rc;
	fd_set fd;
	fd_set* writefd = NULL;
	fd_set* readfd = NULL;
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
#elif USE_WOLFSSH

#endif

}
int san_read(SANSSH *sanssh, const char * remotefile, const char * localfile)
{
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	LIBSSH2_SFTP_HANDLE* handle;

	/* Since we have set non-blocking, tell libssh2 we are blocking */


	/* Request a file via SFTP */
	handle = libssh2_sftp_open(sanssh->sftp, remotefile, LIBSSH2_FXF_READ, 0);
	if (!handle) {
		fprintf(stderr, "Unable to open file with SFTP: %ld\n",
			libssh2_sftp_last_error(sanssh->sftp));
		return 0;
	}

	FILE* file;
	unsigned bytesWritten = 0;
	if (fopen_s(&file, localfile, "wb")) {
		fprintf(stderr, "error opening %s for writing\n", localfile);
		return 0;
	}
	int bytesread;
	size_t total = 0;
	size_t byteswritten = 0;
	int start;
	int duration;

	fprintf(stderr, "donwloading %s -> %s...\n", remotefile, localfile);
	//printf("buffer size    bytes read     bytes written  total bytes\n");
	start = time(NULL);
	char* mem = (char*)malloc(BUFFER_SIZE);
	for (;;) {
		bytesread = libssh2_sftp_read(handle, mem, BUFFER_SIZE);
		if (bytesread == 0)
			break;
		byteswritten = fwrite(mem, 1, bytesread, file);
		total += bytesread;
		//printf("%-15d%-15d%-15ld%-15ld\n", bufsize, bytesread, byteswritten, total);
	}
	free(mem);
	duration = time(NULL) - start;
	if (file)
		fclose(file);
	printf("bytes     : %d\n", total);
	printf("elapsed   : %ld secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)(duration)));

	libssh2_sftp_close(handle);
	return 0;

#elif USE_WOLFSSH
	int rc = wolfSSH_SFTP_Get(sanssh->ssh, remotefile, localfile, 0, 0);
	if (rc) {
		fprintf(stderr, "read failed: rc=%d, error=%ld\n",
			rc, wolfSSH_get_error(sanssh->ssh));
	}
#endif
}
int san_write(SANSSH* sanssh, const char* remotefile, const char* localfile)
{
	int rc = 0;
	int total = 0;
	int duration;
	int start;
	fprintf(stderr, "uploading %s -> %s...\n", localfile, remotefile);
	start = time(NULL);

#ifdef USE_LIBSSH
	int access_type = O_WRONLY | O_CREAT | O_TRUNC;
	sftp_file handle;
	int bytesread;
	
	handle = sftp_open(sanssh->sftp, remotefile,
		access_type, 0777);
	if (handle == NULL) {
		fprintf(stderr, "Can't open file for writing: %s\n",
			ssh_get_error(sanssh->ssh));
		return SSH_ERROR;
	}

	FILE* file;

	if (fopen_s(&file, localfile, "rb")) {
		fprintf(stderr, "error opening %s for reading\n", localfile);
		return 0;
	}
	
	char* mem = (char*)malloc(BUFFER_SIZE);
	char* ptr;

	do {
		bytesread = fread(mem, 1, BUFFER_SIZE, file);
		if (bytesread == 0)
			break;
		ptr = mem;
		do {
			int chunk = bytesread < BUFFER_SIZE ? bytesread : BUFFER_SIZE;
			rc = sftp_write(handle, ptr, chunk);
			if (rc < 0)
				break;
			ptr += rc;
			bytesread -= rc;
			total += rc;
		} while (bytesread);
	} while (rc > 0);

	free(mem);
	if (file)
		fclose(file);
	rc = sftp_close(handle);
	

#elif USE_LIBSSH2

	LIBSSH2_SFTP_HANDLE* handle;

	/* Since we have set non-blocking, tell libssh2 we are blocking */
	//libssh2_session_set_blocking(sanssh->ssh, 1);

	/* Request a file via SFTP */
	handle = libssh2_sftp_open(sanssh->sftp, remotefile,
		LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
		LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR);
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
	//printf("buffer size    bytes read     bytes written  total bytes\n");
	
	char* mem = (char*)malloc(BUFFER_SIZE);
	char* ptr;

	do {
		bytesread = fread(mem, 1, BUFFER_SIZE, file);
		if (bytesread == 0)
			break;
		ptr = mem;
		do {
			int chunk = bytesread < BUFFER_SIZE ? bytesread : BUFFER_SIZE;
			rc = libssh2_sftp_write(handle, ptr, chunk);
			if (rc < 0)
				break;
			ptr += rc;
			bytesread -= rc;
			total += rc;
		} while (bytesread);
	} while (rc > 0);

	free(mem);
	if (file)
		fclose(file);

	libssh2_sftp_close(handle);

#elif USE_WOLFSSH
	int rc = wolfSSH_SFTP_Put(sanssh->ssh, localfile, remotefile, 0, 0);
	if (rc) {
		fprintf(stderr, "write failed: rc=%d, error=%ld\n",
			rc, wolfSSH_get_error(sanssh->ssh));
	}
#endif
	duration = time(NULL) - start;

	printf("bytes     : %d\n", total);
	printf("elapsed   : %d secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)(duration)));

	return rc;
}
int san_read_async(SANSSH *sanssh, const char * remotefile, const char * localfile)
{
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	LIBSSH2_SFTP_HANDLE* handle;
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
	char* mem = (char*)malloc(BUFFER_SIZE);
	do {
		
		while ((rc = libssh2_sftp_read(handle, mem, BUFFER_SIZE))
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
	free(mem);
	duration = time(NULL) - start;
	fclose(file);
	printf("bytes     : %d\n", total);
	printf("spin      : %d\n", spin);
	printf("duration  : %ld secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)duration));

	libssh2_sftp_close(handle);

#elif USE_WOLFSSH

#endif
	return 0;
}
int san_write_async(SANSSH* sanssh, const char* remotefile, const char* localfile)
{
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
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
	char* mem = (char*)malloc(BUFFER_SIZE);
	do {
		
		while ((rc = libssh2_sftp_read(handle, mem, BUFFER_SIZE))
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
	free(mem);
	duration = time(NULL) - start;
	fclose(file);
	printf("bytes     : %d\n", total);
	printf("spin      : %d\n", spin);
	printf("duration  : %ld secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)duration));

	libssh2_sftp_close(handle);

#elif USE_WOLFSSH

#endif
	return 0;
}

int san_mkdir(SANSSH *sanssh, const char * path)
{
	int rc = 0;
#ifdef USE_LIBSSH
	rc = sftp_mkdir(sanssh->sftp, path, S_IRWXU | S_IRWXG | S_IRWXO);
	if (rc) {
		fprintf(stderr, "mkdir failed: rc=%d, error=%ld\n",
			rc, ssh_get_error(sanssh->ssh));
	}
#elif USE_LIBSSH2
	rc = (int)libssh2_sftp_mkdir(sanssh->sftp, path,
		LIBSSH2_SFTP_S_IRWXU |
		LIBSSH2_SFTP_S_IRWXG |
		LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_mkdir failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
	return rc;
#elif USE_WOLFSSH
	rc = wolfSSH_SFTP_MKDIR(sanssh->ssh, path, 0);
	if (rc) {
		fprintf(stderr, "mkdir failed: rc=%d, error=%ld\n",
			rc, wolfSSH_get_error(sanssh->ssh));
	}
#endif
	return rc;
}
int san_rmdir(SANSSH *sanssh, const char * path)
{
	int rc = 0;
#ifdef USE_LIBSSH
	rc = sftp_rmdir(sanssh->sftp, path);
	if (rc) {
		fprintf(stderr, "mkdir failed: rc=%d, error=%ld\n",
			rc, ssh_get_error(sanssh->ssh));
	}
#elif USE_LIBSSH2
	rc = libssh2_sftp_rmdir(sanssh->sftp, path);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_rmdir failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}

#elif USE_WOLFSSH
	rc = wolfSSH_SFTP_RMDIR(sanssh->ssh, path);
	if (rc) {
		fprintf(stderr, "rmdir failed: rc=%d, error=%ld\n",
			rc, wolfSSH_get_error(sanssh->ssh));
	}
#endif
	return rc;
}
int san_stat(SANSSH *sanssh, const char * path, SANSTAT*attrs)
{
	int rc = 0;
#ifdef USE_LIBSSH
	sftp_attributes attr = sftp_lstat(sanssh->sftp, path);
	if (!attrs) {
		fprintf(stderr, "Can't rename file: %s\n",
			ssh_get_error(sanssh->ssh));
		return SSH_ERROR;
	}
	struct sftp_attributes_struct att = *attr;
	sftp_attributes_free(attr);
#elif USE_LIBSSH2
	LIBSSH2_SFTP_ATTRIBUTES att;
	rc = libssh2_sftp_stat(sanssh->sftp, path, &att);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_stat failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
#elif USE_WOLFSSH
	WS_SFTP_FILEATRB att;
	rc = wolfSSH_SFTP_LSTAT(sanssh->ssh, path, &att);
	if (rc) {
		fprintf(stderr, "stat failed: rc=%d, error=%ld\n",
			rc, wolfSSH_get_error(sanssh->ssh));
	}
#endif
	copy_attributes(attrs, (SANSTAT*)&att);
	return rc;
}

void print_stat(const char* path, SANSTAT *attrs)
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
int san_statvfs(SANSSH *sanssh, const char * path, SANSTATVFS *st)
{
	int rc = 0;
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	LIBSSH2_SFTP_STATVFS att;
	rc = libssh2_sftp_statvfs(sanssh->sftp, path, strlen(path), &att);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_statvfs failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
	memset(st, 0, sizeof(SANSTATVFS));
	st->f_bsize = att.f_bsize;			/* file system block size */
	st->f_frsize = att.f_frsize;			/* fragment size */
	st->f_blocks = att.f_blocks;			/* size of fs in f_frsize units */
	st->f_bfree = att.f_bfree;			/* # free blocks */
	st->f_bavail = att.f_bavail;			/* # free blocks for non-root */
	st->f_files = att.f_files;			/* # inodes */
	st->f_ffree = att.f_ffree;			/* # free inodes */
	st->f_favail = att.f_favail;			/* # free inodes for non-root */
	st->f_fsid = att.f_fsid;				/* file system ID */
	st->f_flag = att.f_flag;				/* mount flags */
	st->f_namemax = att.f_namemax;		/* maximum filename length */
#elif USE_WOLFSSH
	WS_SFTP_FILEATRB att;
	rc = wolfSSH_SFTP_LSTAT(sanssh->ssh, path, &att);
	if (rc) {
		fprintf(stderr, "stat failed: rc=%d, error=%ld\n",
			rc, wolfSSH_get_error(sanssh->ssh));
	}
	memset(st, 0, sizeof(SANSTATVFS));
	//st->f_bsize = att.f_bsize;			/* file system block size */
	//st->f_frsize = att.f_frsize;			/* fragment size */
	//st->f_blocks = att.f_blocks;			/* size of fs in f_frsize units */
	//st->f_bfree = att.f_bfree;			/* # free blocks */
	//st->f_bavail = att.f_bavail;			/* # free blocks for non-root */
	//st->f_files = att.f_files;			/* # inodes */
	//st->f_ffree = att.f_ffree;			/* # free inodes */
	//st->f_favail = att.f_favail;			/* # free inodes for non-root */
	//st->f_fsid = att.f_fsid;				/* file system ID */
	//st->f_flag = att.f_flag;				/* mount flags */
	//st->f_namemax = att.f_namemax;		/* maximum filename length */
#endif
	return rc;
}
void print_statvfs(const char* path, SANSTATVFS *st)
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
SANHANDLE * san_open(SANSSH *sanssh, const char *path, long mode)
{
	SANHANDLE* sh;
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	LIBSSH2_SFTP_HANDLE* handle;
	handle = libssh2_sftp_open(sanssh->sftp, path,
		LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
		LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
		LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
	if (!handle) {
		fprintf(stderr, "Unable to open file\n");
	}
	
	sh->file_handle = handle;
#elif USE_WOLFSSH

#endif
	return sh;
}
SANHANDLE * san_opendir(SANSSH *sanssh, const char *path)
{
	SANHANDLE* sh;
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
		LIBSSH2_SFTP_HANDLE* handle;
	handle = libssh2_sftp_opendir(sanssh->sftp, path);
	if (!handle) {
		fprintf(stderr, "Unable to open directory\n");
	}
	sh->file_handle = handle;
#elif USE_WOLFSSH

#endif
	return sh;
}

void copy_attributes(SANSTAT* stbuf, void* attr)
{
	if (!attr)
		return;
	memset(attr, 0, sizeof(SANSTAT));
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	LIBSSH2_SFTP_ATTRIBUTES* attrs = (LIBSSH2_SFTP_ATTRIBUTES*)attr;
	stbuf->flags = attrs->flags;
	stbuf->uid = attrs->uid;
	stbuf->gid = attrs->gid;
	stbuf->mode = attrs->permissions;
	stbuf->filesize = attrs->filesize;
	stbuf->atime = attrs->atime;
	stbuf->mtime = attrs->mtime;
#elif USE_WOLFSSH
	WS_SFTP_FILEATRB* attrs = (WS_SFTP_FILEATRB*)attr;
	stbuf->flags = attrs->flags;
	stbuf->uid = attrs->uid;
	stbuf->gid = attrs->gid;
	stbuf->mode = attrs->per;
	stbuf->filesize = attrs->sz[0];
	stbuf->atime = attrs->atime;
	stbuf->mtime = attrs->mtime;
#endif
}
int san_close(SANHANDLE *handle)
{
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	return libssh2_sftp_close_handle(handle);
#elif USE_WOLFSSH

#endif
	
}
int san_rename(SANSSH *sanssh, const char *source, const char *destination)
{
	int rc;
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	rc = libssh2_sftp_rename(sanssh->sftp, source, destination);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_rename failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
#elif USE_WOLFSSH

#endif
	

	return rc;

}
int san_delete(SANSSH *sanssh, const char *filename)
{
	int rc;
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	rc = libssh2_sftp_unlink(sanssh->sftp, filename);
	if (rc) {
		fprintf(stderr, "libssh2_sftp_unlink failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
#elif USE_WOLFSSH

#endif
	
	
	return rc;
}
int san_realpath(SANSSH *sanssh, const char *path, char *target)
{
	int rc;
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	rc = libssh2_sftp_realpath(sanssh->sftp, path, target, MAX_PATH);
	if (rc < 0) {
		fprintf(stderr, "libssh2_sftp_readlink failed: rc=%d, error=%ld\n",
			rc, libssh2_sftp_last_error(sanssh->sftp));
	}
#elif USE_WOLFSSH

#endif
	
	
	return rc;
}
int san_readdir(SANSSH *sanssh, const char *path)
{
	int rc = 0;
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	LIBSSH2_SFTP_HANDLE* handle;
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
			NULL, 0, &attrs);
		if (rc > 0) {
			/* rc is the length of the file name in the mem buffer */
			//char filetype[4];
			//get_filetype(attrs.permissions, &filetype);
			if (longentry[0] != '\0') {
				/*printf("%3s %10ld %5ld %5ld %s\n",
					filetype, attrs.filesize, attrs.uid, attrs.gid,
					longentry);*/
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

#elif USE_WOLFSSH

#endif
	
}
int run_command(SANSSH *sanssh,	const char *cmd, char *out, char *err)
{
	int rc = 0;
#ifdef USE_LIBSSH

#elif USE_LIBSSH2
	LIBSSH2_CHANNEL* channel;
	
	int bytes = 0;
	int errlen = 0;
	char* errmsg;
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

#elif USE_WOLFSSH

#endif
	return rc;

}