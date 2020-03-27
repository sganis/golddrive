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

//#define LIBSSH_STATIC 1
#include <libssh/libssh.h> 
#include <libssh/sftp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

 //#define _CRT_SECURE_NO_WARNINGS
#define BUFFER_SIZE 65535

#define S_IRWXU 0000700    /* RWX mask for owner */
#define S_IRUSR 0000400    /* R for owner */
#define S_IWUSR 0000200    /* W for owner */
#define S_IXUSR 0000100    /* X for owner */

#define S_IRWXG 0000070    /* RWX mask for group */
#define S_IRGRP 0000040    /* R for group */
#define S_IWGRP 0000020    /* W for group */
#define S_IXGRP 0000010    /* X for group */

#define S_IRWXO 0000007    /* RWX mask for other */
#define S_IROTH 0000004    /* R for other */
#define S_IWOTH 0000002    /* W for other */
#define S_IXOTH 0000001    /* X for other */

#define S_ISUID 0004000    /* set user id on execution */
#define S_ISGID 0002000    /* set group id on execution */
#define S_ISVTX 0001000    /* save swapped text even after use */

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
int verify_knownhost(ssh_session session)
{
	enum ssh_known_hosts_e state;
	unsigned char *hash = NULL;
	ssh_key srv_pubkey = NULL;
	size_t hlen;
	char buf[10];
	char *hexa;
	char *p;
	int cmp;
	int rc;
	rc = ssh_get_server_publickey(session, &srv_pubkey);
	if (rc < 0) {
		return -1;
	}
	rc = ssh_get_publickey_hash(srv_pubkey,
		SSH_PUBLICKEY_HASH_SHA1,
		&hash,
		&hlen);
	ssh_key_free(srv_pubkey);
	if (rc < 0) {
		return -1;
	}
	state = ssh_session_is_known_server(session);
	switch (state) {
	case SSH_KNOWN_HOSTS_OK:
		/* OK */
		break;
	case SSH_KNOWN_HOSTS_CHANGED:
		fprintf(stderr, "Host key for server changed: it is now:\n");
		ssh_print_hexa("Public key hash", hash, hlen);
		fprintf(stderr, "For security reasons, connection will be stopped\n");
		ssh_clean_pubkey_hash(&hash);
		return -1;
	case SSH_KNOWN_HOSTS_OTHER:
		fprintf(stderr, "The host key for this server was not found but an other"
			"type of key exists.\n");
		fprintf(stderr, "An attacker might change the default server key to"
			"confuse your client into thinking the key does not exist\n");
		ssh_clean_pubkey_hash(&hash);
		return -1;
	case SSH_KNOWN_HOSTS_NOT_FOUND:
		fprintf(stderr, "Could not find known host file.\n");
		fprintf(stderr, "If you accept the host key here, the file will be"
			"automatically created.\n");
		/* FALL THROUGH to SSH_SERVER_NOT_KNOWN behavior */
	case SSH_KNOWN_HOSTS_UNKNOWN:
		hexa = ssh_get_hexa(hash, hlen);
		fprintf(stderr, "The server is unknown. Do you trust the host key?\n");
		fprintf(stderr, "Public key hash: %s\n", hexa);
		ssh_string_free_char(hexa);
		ssh_clean_pubkey_hash(&hash);
		p = fgets(buf, sizeof(buf), stdin);
		if (p == NULL) {
			return -1;
		}
		cmp = strncmp(buf, "yes", 3);
		if (cmp != 0) {
			return -1;
		}
		rc = ssh_session_update_known_hosts(session);
		if (rc < 0) {
			fprintf(stderr, "Error %s\n", strerror(errno));
			return -1;
		}
		break;
	case SSH_KNOWN_HOSTS_ERROR:
		fprintf(stderr, "Error %s", ssh_get_error(session));
		ssh_clean_pubkey_hash(&hash);
		return -1;
	}
	ssh_clean_pubkey_hash(&hash);
	return 0;
}

int show_remote_processes(ssh_session session)
{
	ssh_channel channel;
	int rc;
	char buffer[256];
	int nbytes;
	channel = ssh_channel_new(session);
	if (channel == NULL)
		return SSH_ERROR;
	rc = ssh_channel_open_session(channel);
	if (rc != SSH_OK)
	{
		ssh_channel_free(channel);
		return rc;
	}
	rc = ssh_channel_request_exec(channel, "ps aux");
	if (rc != SSH_OK)
	{
		ssh_channel_close(channel);
		ssh_channel_free(channel);
		return rc;
	}
	nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
	while (nbytes > 0)
	{
		if (write(1, buffer, nbytes) != (unsigned int)nbytes)
		{
			ssh_channel_close(channel);
			ssh_channel_free(channel);
			return SSH_ERROR;
		}
		nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
	}

	if (nbytes < 0)
	{
		ssh_channel_close(channel);
		ssh_channel_free(channel);
		return SSH_ERROR;
	}
	ssh_channel_send_eof(channel);
	ssh_channel_close(channel);
	ssh_channel_free(channel);
	return SSH_OK;
}

sftp_attributes san_stat(ssh_session session, sftp_session sftp, char * path)
{
	sftp_attributes attrs = sftp_lstat(sftp, path);
	if (!attrs) {
		fprintf(stderr, "Can't rename file: %s\n",
			ssh_get_error(session));
		return SSH_ERROR;
	}
	return attrs;
}

int san_readdir(ssh_session session, sftp_session sftp)
{
	sftp_dir dir;
	sftp_attributes attributes;
	int rc;
	dir = sftp_opendir(sftp, "/tmp");
	if (!dir)
	{
		fprintf(stderr, "Directory not opened: %s\n",
			ssh_get_error(session));
		return SSH_ERROR;
	}
	printf("Name                       Size Perms    Owner\tGroup\n");
	while ((attributes = sftp_readdir(sftp, dir)) != NULL)
	{
		printf("%-20s %10llu %.8o %s(%d)\t%s(%d)\n",
			attributes->name,
			attributes->size,
			attributes->permissions,
			attributes->owner,
			attributes->uid,
			attributes->group,
			attributes->gid);
		sftp_attributes_free(attributes);
	}
	if (!sftp_dir_eof(dir))
	{
		fprintf(stderr, "Can't list directory: %s\n",
			ssh_get_error(session));
		sftp_closedir(dir);
		return SSH_ERROR;
	}
	rc = sftp_closedir(dir);
	if (rc != SSH_OK)
	{
		fprintf(stderr, "Can't close directory: %s\n",
			ssh_get_error(session));
		return rc;
	}
	return rc;
}

int san_mkdir(ssh_session session, sftp_session sftp)
{
	int rc;
	rc = sftp_mkdir(sftp, "/tmp/helloworld", S_IRWXU | S_IRWXG | S_IRWXO);
	if (rc != SSH_OK)
	{
		if (sftp_get_error(sftp) != SSH_FX_FILE_ALREADY_EXISTS)
		{
			fprintf(stderr, "Can't create directory: %s\n",
				ssh_get_error(session));
			return rc;
		}
		else {
			fprintf(stderr, "Can't make directory: %s\n",
				ssh_get_error(session));
			return SSH_ERROR;
		}
	}
	return SSH_OK;
}

int san_write(ssh_session session, sftp_session sftp)
{
	int access_type = O_WRONLY | O_CREAT | O_TRUNC;
	sftp_file file;
	const char *helloworld = "Hello, World!\n";
	int length = strlen(helloworld);
	int rc, nwritten;
	file = sftp_open(sftp, "/tmp/helloworld/helloworld.txt",
		access_type, 0777);
	if (file == NULL) {
		fprintf(stderr, "Can't open file for writing: %s\n",
			ssh_get_error(session));
		return SSH_ERROR;
	}
	nwritten = sftp_write(file, helloworld, length);
	if (nwritten != length) {
		fprintf(stderr, "Can't write data to file: %s\n",
			ssh_get_error(session));
		sftp_close(file);
		return SSH_ERROR;
	}
	rc = sftp_close(file);
	if (rc != SSH_OK) {
		fprintf(stderr, "Can't close the written file: %s\n",
			ssh_get_error(session));
		return rc;
	}
	return SSH_OK;
}

int san_rename(ssh_session session, sftp_session sftp)
{
	int rc = sftp_rename(sftp,
		"/tmp/helloworld/helloworld.txt",
		"/tmp/helloworld/renamed.txt");
	if (rc != SSH_OK) {
		fprintf(stderr, "Can't rename file: %s\n",
			ssh_get_error(session));
		return SSH_ERROR;
	}
	return SSH_OK;
}

int san_read(ssh_session session, sftp_session sftp,
	const char* remotefile, const char* localfile)
{
	sftp_file file;
	
	int nbytes, nwritten, rc;
	file = sftp_open(sftp, remotefile, O_RDONLY, 0);
	if (file == NULL) {
		fprintf(stderr, "Can't open file for reading: %s\n", ssh_get_error(session));
		return SSH_ERROR;
	}

	FILE *fd;
	if (fopen_s(&fd, localfile, "wb")) {
		fprintf(stderr, "Can't open file %s for writing: %s\n",
		 localfile, strerror(errno));
		return SSH_ERROR;
	}	
	
	size_t bytesize = sizeof(char);
	size_t total = 0;
	int duration;
	//int bufsize = 64 * 1024;	
	int start;

	fprintf(stderr, "donwloading %s -> %s...\n", remotefile, localfile);
	
	start = time(NULL);
	//char *buffer = (char*)malloc(bufsize);
	char buffer[BUFFER_SIZE];
	for (;;) {
		nbytes = sftp_read(file, buffer, BUFFER_SIZE);
		if (nbytes == 0) 
			break; // EOF
		fwrite(buffer, bytesize, nbytes, fd);
		total += nbytes;
	}
	//free(buffer);
	duration = time(NULL) - start;

	rc = sftp_close(file);
	if (rc != SSH_OK) {
		fprintf(stderr, "Can't close the read file: %s\n", ssh_get_error(session));
		return rc;
	}
	printf("bytes     : %ld\n", total);
	printf("elapsed   : %ld secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)(duration)));

	return SSH_OK;
}

int san_read_async(ssh_session session, sftp_session sftp, 
	const char* remotefile, const char* localfile)
{
	sftp_file file;
	int async_request;
	int nbytes;
	long counter;
	int rc;
	file = sftp_open(sftp, remotefile, O_RDONLY, 0);
	if (file == NULL) {
		fprintf(stderr, "Can't open file for reading: %s\n", ssh_get_error(session));
		return SSH_ERROR;
	}
	FILE *fd;
	if (fopen_s(&fd, localfile, "wb")) {
		fprintf(stderr, "Can't open file %s for writing: %s\n",
			localfile, strerror(errno));
		return SSH_ERROR;
	}	

	sftp_file_set_nonblocking(file);

	size_t bytesize = sizeof(char);
	size_t total = 0;
	int duration;
	//const int bufsize = 64 * 1024;
	int start;

	fprintf(stderr, "donwloading %s -> %s...\n", remotefile, localfile);

	start = time(NULL);
	//char *buffer = (char*)malloc(bufsize);
	char buffer[BUFFER_SIZE];

	async_request = sftp_async_read_begin(file, BUFFER_SIZE);
	counter = 0L;
	//usleep(10000);
	if (async_request >= 0) {
		nbytes = sftp_async_read(file, buffer, BUFFER_SIZE, async_request);
	} else {
		nbytes = -1;
	}
	while (nbytes > 0 || nbytes == SSH_AGAIN) {
		if (nbytes > 0) {
			fwrite(buffer, bytesize, nbytes, fd);
			total += nbytes;
			async_request = sftp_async_read_begin(file, BUFFER_SIZE);
		} else {
			counter++;
		}
		//usleep(10000);
		if (async_request >= 0) {
			nbytes = sftp_async_read(file, buffer, BUFFER_SIZE,	async_request);
		} else {
			nbytes = -1;
		}
	}
	//free(buffer);
	duration = time(NULL) - start;

	if (nbytes < 0) {
		fprintf(stderr, "Error while reading file: %s\n", ssh_get_error(session));
		sftp_close(file);
		return SSH_ERROR;
	}
	//printf("The counter has reached value: %ld\n", counter);
	rc = sftp_close(file);
	if (rc != SSH_OK) {
		fprintf(stderr, "Can't close the read file: %s\n", ssh_get_error(session));
		return rc;
	}

	printf("bytes     : %ld\n", total);
	printf("elapsed   : %ld secs.\n", duration);
	printf("speed     : %d MB/s.\n", (int)(total / 1024.0 / 1024.0 / (double)(duration)));


	return SSH_OK;
}

int main(int argc, char *argv[])
{
	const char *hostname = "";
	const char *username = "";
	const char *password = "";
	const char *remotefile = "";
	const char *localfile = "";
	int port = 22;
	char pkeypath[MAX_PATH];
	int rc;
	char *errmsg;
	//SOCKET sock;
	//SOCKADDR_IN sin;

	if (argc == 2 && strcmp(argv[1], "-V") == 0) {
		printf("sanssh 1.0.1\n");
		return 0;
	}

	if (argc < 5) {
		usage(argv[0]);
		return 1;
	}

	hostname = argv[1];
	username = argv[2];
	remotefile = argv[3];
	localfile = argv[4];

	// get public key
	if (argc > 5) {
		strcpy_s(pkeypath, MAX_PATH, argv[5]);
	}
	else {
		char profile[BUFFER_SIZE];
		ExpandEnvironmentStringsA("%USERPROFILE%", profile, BUFFER_SIZE);
		strcpy_s(pkeypath, MAX_PATH, profile);
		strcat_s(pkeypath, MAX_PATH, "\\.ssh\\id_rsa");
	}

	if (!file_exists(pkeypath)) {
		printf("error: cannot read private key: %s\n", pkeypath);
		return 1;
	}

	printf("username   : %s\n", username);
	printf("private key: %s\n", pkeypath);
	printf("connecting...\n");


	printf("testing...\n");

	sftp_attributes attrs = san_stat(ssh, sftp, "/tmp/test.link");

	// mkdir
	//san_mkdir(ssh, sftp);

	// write
	//san_write(ssh, sftp);

	// rename
	//san_rename(ssh, sftp);

	// readddir
	//san_readdir(ssh, sftp);

	// read
	rc = san_read(ssh, sftp, remotefile, localfile);
	if (rc != SSH_OK) {
		fprintf(stderr, "Error transfering file\n");
		return SSH_ERROR;
	}
	
	
	//show_remote_processes(ssh);

shutdown:
	printf("shuting down...\n");

	ssh_disconnect(ssh);
	ssh_free(ssh);
	WSACleanup();
	
	return 0;
}
