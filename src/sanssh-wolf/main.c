/*
 * Sanssh - SFTP client using libssh
 * Author: SAG
 * Date: 10/09/2018
 *
 * Download a file from remote ssh server.
 *
 * usage: sanssh hostname user /tmp/file c:\tmp\file [private key]
 * private key defaults to %USERPROFILE%\.ssh\id_rsa
 */
//#define WOLFSSH_TEST_CLIENT

#include <wolfssh/ssh.h>
#include <wolfssh/wolfsftp.h>
#include <wolfssh/test.h>
#include <wolfssh/port.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/coding.h>

//#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

byte userPassword[256];


void usleep(__int64 usec)
{
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}
static void usage(const char* prog)
{
	printf("Sanssh - sftp client using libssh\n");
	printf("usage: %s [options] <host> <user> <remote file> <local file> [public key]\n", prog);
	printf("options:\n");
	printf("    -V           show version\n");
}
static int file_exists(const char* path)
{
	DWORD attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

//sftp_attributes san_stat(ssh_session session, sftp_session sftp, char* path)
//{
//	sftp_attributes attrs = sftp_lstat(sftp, path);
//	if (!attrs) {
//		fprintf(stderr, "Can't rename file: %s\n",
//			ssh_get_error(session));
//		return SSH_ERROR;
//	}
//	return attrs;
//}
//
//int san_readdir(ssh_session session, sftp_session sftp)
//{
//	sftp_dir dir;
//	sftp_attributes attributes;
//	int rc;
//	dir = sftp_opendir(sftp, "/tmp");
//	if (!dir)
//	{
//		fprintf(stderr, "Directory not opened: %s\n",
//			ssh_get_error(session));
//		return SSH_ERROR;
//	}
//	printf("Name                       Size Perms    Owner\tGroup\n");
//	while ((attributes = sftp_readdir(sftp, dir)) != NULL)
//	{
//		printf("%-20s %10llu %.8o %s(%d)\t%s(%d)\n",
//			attributes->name,
//			attributes->size,
//			attributes->permissions,
//			attributes->owner,
//			attributes->uid,
//			attributes->group,
//			attributes->gid);
//		sftp_attributes_free(attributes);
//	}
//	if (!sftp_dir_eof(dir))
//	{
//		fprintf(stderr, "Can't list directory: %s\n",
//			ssh_get_error(session));
//		sftp_closedir(dir);
//		return SSH_ERROR;
//	}
//	rc = sftp_closedir(dir);
//	if (rc != SSH_OK)
//	{
//		fprintf(stderr, "Can't close directory: %s\n",
//			ssh_get_error(session));
//		return rc;
//	}
//	return rc;
//}
//
//int san_mkdir(ssh_session session, sftp_session sftp)
//{
//	int rc;
//	rc = sftp_mkdir(sftp, "/tmp/helloworld", S_IRWXU | S_IRWXG | S_IRWXO);
//	if (rc != SSH_OK)
//	{
//		if (sftp_get_error(sftp) != SSH_FX_FILE_ALREADY_EXISTS)
//		{
//			fprintf(stderr, "Can't create directory: %s\n",
//				ssh_get_error(session));
//			return rc;
//		}
//		else {
//			fprintf(stderr, "Can't make directory: %s\n",
//				ssh_get_error(session));
//			return SSH_ERROR;
//		}
//	}
//	return SSH_OK;
//}
//
//int san_write(ssh_session session, sftp_session sftp)
//{
//	int access_type = O_WRONLY | O_CREAT | O_TRUNC;
//	sftp_file file;
//	const char* helloworld = "Hello, World!\n";
//	int length = strlen(helloworld);
//	int rc, nwritten;
//	file = sftp_open(sftp, "/tmp/helloworld/helloworld.txt",
//		access_type, 0777);
//	if (file == NULL) {
//		fprintf(stderr, "Can't open file for writing: %s\n",
//			ssh_get_error(session));
//		return SSH_ERROR;
//	}
//	nwritten = sftp_write(file, helloworld, length);
//	if (nwritten != length) {
//		fprintf(stderr, "Can't write data to file: %s\n",
//			ssh_get_error(session));
//		sftp_close(file);
//		return SSH_ERROR;
//	}
//	rc = sftp_close(file);
//	if (rc != SSH_OK) {
//		fprintf(stderr, "Can't close the written file: %s\n",
//			ssh_get_error(session));
//		return rc;
//	}
//	return SSH_OK;
//}
//
//int san_rename(ssh_session session, sftp_session sftp)
//{
//	int rc = sftp_rename(sftp,
//		"/tmp/helloworld/helloworld.txt",
//		"/tmp/helloworld/renamed.txt");
//	if (rc != SSH_OK) {
//		fprintf(stderr, "Can't rename file: %s\n",
//			ssh_get_error(session));
//		return SSH_ERROR;
//	}
//	return SSH_OK;
//}
//
//int san_read(ssh_session session, sftp_session sftp,
//	const char* remotefile, const char* localfile)
//{
//	sftp_file file;
//
//	int nbytes, nwritten, rc;
//	file = sftp_open(sftp, remotefile, O_RDONLY, 0);
//	if (file == NULL) {
//		fprintf(stderr, "Can't open file for reading: %s\n", ssh_get_error(session));
//		return SSH_ERROR;
//	}
//
//	FILE* fd;
//	if (fopen_s(&fd, localfile, "wb")) {
//		fprintf(stderr, "Can't open file %s for writing: %s\n",
//			localfile, strerror(errno));
//		return SSH_ERROR;
//	}
//
//	size_t bytesize = sizeof(char);
//	size_t total = 0;
//	int duration;
//	//int bufsize = 64 * 1024;	
//	int start;
//
//	fprintf(stderr, "donwloading %s -> %s...\n", remotefile, localfile);
//
//	start = time(NULL);
//	//char *buffer = (char*)malloc(bufsize);
//	char buffer[BUFFER_SIZE];
//	for (;;) {
//		nbytes = sftp_read(file, buffer, BUFFER_SIZE);
//		if (nbytes == 0)
//			break; // EOF
//		fwrite(buffer, bytesize, nbytes, fd);
//		total += nbytes;
//	}
//	//free(buffer);
//	duration = time(NULL) - start;
//
//	rc = sftp_close(file);
//	if (rc != SSH_OK) {
//		fprintf(stderr, "Can't close the read file: %s\n", ssh_get_error(session));
//		return rc;
//	}
//	printf("bytes     : %ld\n", total);
//	printf("elapsed   : %ld secs.\n", duration);
//	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)(duration)));
//
//	return SSH_OK;
//}
//
//int san_read_async(ssh_session session, sftp_session sftp,
//	const char* remotefile, const char* localfile)
//{
//	sftp_file file;
//	int async_request;
//	int nbytes;
//	long counter;
//	int rc;
//	file = sftp_open(sftp, remotefile, O_RDONLY, 0);
//	if (file == NULL) {
//		fprintf(stderr, "Can't open file for reading: %s\n", ssh_get_error(session));
//		return SSH_ERROR;
//	}
//	FILE* fd;
//	if (fopen_s(&fd, localfile, "wb")) {
//		fprintf(stderr, "Can't open file %s for writing: %s\n",
//			localfile, strerror(errno));
//		return SSH_ERROR;
//	}
//
//	sftp_file_set_nonblocking(file);
//
//	size_t bytesize = sizeof(char);
//	size_t total = 0;
//	int duration;
//	//const int bufsize = 64 * 1024;
//	int start;
//
//	fprintf(stderr, "donwloading %s -> %s...\n", remotefile, localfile);
//
//	start = time(NULL);
//	//char *buffer = (char*)malloc(bufsize);
//	char buffer[BUFFER_SIZE];
//
//	async_request = sftp_async_read_begin(file, BUFFER_SIZE);
//	counter = 0L;
//	//usleep(10000);
//	if (async_request >= 0) {
//		nbytes = sftp_async_read(file, buffer, BUFFER_SIZE, async_request);
//	}
//	else {
//		nbytes = -1;
//	}
//	while (nbytes > 0 || nbytes == SSH_AGAIN) {
//		if (nbytes > 0) {
//			fwrite(buffer, bytesize, nbytes, fd);
//			total += nbytes;
//			async_request = sftp_async_read_begin(file, BUFFER_SIZE);
//		}
//		else {
//			counter++;
//		}
//		//usleep(10000);
//		if (async_request >= 0) {
//			nbytes = sftp_async_read(file, buffer, BUFFER_SIZE, async_request);
//		}
//		else {
//			nbytes = -1;
//		}
//	}
//	//free(buffer);
//	duration = time(NULL) - start;
//
//	if (nbytes < 0) {
//		fprintf(stderr, "Error while reading file: %s\n", ssh_get_error(session));
//		sftp_close(file);
//		return SSH_ERROR;
//	}
//	//printf("The counter has reached value: %ld\n", counter);
//	rc = sftp_close(file);
//	if (rc != SSH_OK) {
//		fprintf(stderr, "Can't close the read file: %s\n", ssh_get_error(session));
//		return rc;
//	}
//
//	printf("bytes     : %ld\n", total);
//	printf("elapsed   : %ld secs.\n", duration);
//	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)(duration)));
//
//
//	return SSH_OK;
//}


static int wsPublicKeyCheck(const byte* pubKey, word32 pubKeySz, void* ctx)
{
#ifdef DEBUG_WOLFSSH
	printf("Sample public key check callback\n"
		"  public key = %p\n"
		"  public key size = %u\n"
		"  ctx = %s\n", pubKey, pubKeySz, (const char*)ctx);
#else
	(void)pubKey;
	(void)pubKeySz;
	(void)ctx;
#endif
	return 0;
}


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
	if (authData->type & WOLFSSH_USERAUTH_PUBLICKEY &&
		authData->username != NULL &&
		authData->usernameSz > 0 &&
		XSTRNCMP((char*)authData->username, "hansel",
			authData->usernameSz) == 0) {
		if (authType == WOLFSSH_USERAUTH_PASSWORD) {
			printf("rejecting password type with hansel in favor of pub key\n");
			return WOLFSSH_USERAUTH_FAILURE;
		}
	}

	if (authType == WOLFSSH_USERAUTH_PASSWORD) {
		const char* defaultPassword = (const char*)ctx;
		word32 passwordSz;

		ret = WOLFSSH_USERAUTH_SUCCESS;
		if (defaultPassword != NULL) {
			passwordSz = (word32)strlen(defaultPassword);
			memcpy(userPassword, defaultPassword, passwordSz);
		}
		else {
			printf("Password: ");
			fflush(stdout);
			//SetEcho(0);
			if (WFGETS((char*)userPassword, sizeof(userPassword),
				stdin) == NULL) {
				printf("Getting password failed.\n");
				ret = WOLFSSH_USERAUTH_FAILURE;
			}
			else {
				char* c = strpbrk((char*)userPassword, "\r\n");
				if (c != NULL)
					*c = '\0';
			}
			passwordSz = (word32)strlen((const char*)userPassword);
			//SetEcho(1);
#ifdef USE_WINDOWS_API
			printf("\r\n");
#endif
		}

		if (ret == WOLFSSH_USERAUTH_SUCCESS) {
			authData->sf.password.password = userPassword;
			authData->sf.password.passwordSz = passwordSz;
		}
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
int main(int argc, char* argv[])
{
	const char* hostname = "";
	const char* username = "";
	const char* password = "";
	const char* remotefile = "";
	const char* localfile = "";
	
	char pkeypath[MAX_PATH];
	int rc;
	char* error = "";
	SOCKADDR_IN sin;
	HOSTENT* he;
	SOCKET sock;
	int port = 22;




	if (argc == 2 && strcmp(argv[1], "-V") == 0) {
		printf("sanssh wolf 1.0.0\n");
		return 0;
	}

	if (argc < 5) {
		usage(argv[0]);
		return 1;
	}

	//hostname = argv[1];
	//username = argv[2];
	//remotefile = argv[3];
	//localfile = argv[4];
	//password = argv[5];

	hostname = argv[2];
	username = argv[4];
	remotefile = argv[3];
	localfile = argv[4];
	password = argv[4];

	// get public key
	//if (argc > 6) {
	//	strcpy_s(pkeypath, MAX_PATH, argv[5]);
	//}
	//else {
	//	char profile[MAX_PATH];
	//	ExpandEnvironmentStringsA("%USERPROFILE%", profile, MAX_PATH);
	//	strcpy_s(pkeypath, MAX_PATH, profile);
	//	strcat_s(pkeypath, MAX_PATH, "\\.ssh\\id_rsa");
	//}

	//if (!file_exists(pkeypath)) {
	//	printf("error: cannot read private key: %s\n", pkeypath);
	//	return 1;
	//}

	printf("username   : %s\n", username);
	printf("private key: %s\n", pkeypath);
	printf("connecting...\n");

	// wolf
	WOLFSSH_CTX* ctx = NULL;
	static WOLFSSH* ssh = NULL;
	SOCKET_T sockFd = WOLFSSH_SOCKET_INVALID;
	SOCKADDR_IN_T clientAddr;
	socklen_t clientAddrSz = sizeof(clientAddr);
	int ret;
	
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
	
	// init 
	wolfSSH_Init();
	ctx = wolfSSH_CTX_new(WOLFSSH_ENDPOINT_CLIENT, NULL);
	if (ctx == NULL)
		err_sys("Couldn't create wolfSSH client context.");
	wolfSSH_SetUserAuth(ctx, wsUserAuth);
	ssh = wolfSSH_new(ctx);
	if (ssh == NULL)
		err_sys("Couldn't create wolfSSH session.");

	
	//wolfSSH_CTX_SetPublicKeyCheck(ctx, wsPublicKeyCheck);
	//wolfSSH_SetPublicKeyCheckCtx(ssh, (void*)"You've been sampled!");
	
	ret = wolfSSH_set_fd(ssh, (int)sock);
	if (ret != WS_SUCCESS)
		err_sys("Couldn't set the session's socket.");

	wolfSSH_SetUserAuthCtx(ssh, (void*)password);

	ret = wolfSSH_SetUsername(ssh, username);
	if (ret != WS_SUCCESS)
		err_sys("Couldn't set the username.");


	ret = wolfSSH_SFTP_connect(ssh);
	
	if (ret != WS_SUCCESS) {
		printf("%s\n", wolfSSH_ErrorToName(ret));
		err_sys("Couldn't connect SFTP");
		
	}
	
	/* get current working directory */
	WS_SFTPNAME* n = NULL;
	n = wolfSSH_SFTP_RealPath(ssh, (char*)".");
	ret = wolfSSH_get_error(ssh);
	if (n == NULL) {
		err_sys("Unable to get real path for working directory");
	}
	/* free after done with names */
	wolfSSH_SFTPNAME_list_free(n);
	n = NULL;


	//printf("testing...\n");

	WS_SFTPNAME* n = san_stat(ssh, "/tmp/file.bin");

	// mkdir
	//san_mkdir(ssh, sftp);

	// write
	//san_write(ssh, sftp);

	// rename
	//san_rename(ssh, sftp);

	// readddir
	//san_readdir(ssh, sftp);

	// read
	//rc = san_read(ssh, sftp, remotefile, localfile);
	//if (rc != SSH_OK) {
	//	fprintf(stderr, "Error transfering file\n");
	//	return SSH_ERROR;
	//}


	//show_remote_processes(ssh);

//shutdown:
	printf("shuting down...\n");

	////ssh_disconnect(ssh);
	//ssh_free(ssh);
	//WSACleanup();

	return 0;
}
