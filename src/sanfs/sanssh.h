#pragma once
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <fuse.h>

#define BUFFER_SIZE 32767
#define ERROR_LEN MAXERRORLENGTH

/* SSH error codes */
static const char * ssh_errors[] = {
	"OK"
	"SOCKET_NONE",
	"BANNER_RECV",
	"BANNER_SEND",
	"INVALID_MAC",
	"KEX_FAILURE",
	"ALLOC",
	"SOCKET_SEND",
	"KEY_EXCHANGE_FAILURE",
	"TIMEOUT",
	"HOSTKEY_INIT",
	"HOSTKEY_SIGN",
	"DECRYPT",
	"SOCKET_DISCONNECT",
	"PROTO",
	"PASSWORD_EXPIRED",
	"FILE",
	"METHOD_NONE",
	"AUTHENTICATION_FAILED",
	"PUBLICKEY_UNVERIFIED",
	"CHANNEL_OUTOFORDER",
	"CHANNEL_FAILURE",
	"CHANNEL_REQUEST_DENIED",
	"CHANNEL_UNKNOWN",
	"CHANNEL_WINDOW_EXCEEDED",
	"CHANNEL_PACKET_EXCEEDED",
	"CHANNEL_CLOSED",
	"CHANNEL_EOF_SENT",
	"SCP_PROTOCOL",
	"ZLIB",
	"SOCKET_TIMEOUT",
	"SFTP_PROTOCOL",
	"REQUEST_DENIED",
	"METHOD_NOT_SUPPORTED",
	"INVAL",
	"INVALID_POLL_TYPE",
	"PUBLICKEY_PROTOCOL",
	"EAGAIN",
	"BUFFER_TOO_SMALL",
	"BAD_USE",
	"COMPRESS",
	"OUT_OF_BOUNDARY",
	"AGENT_PROTOCOL",
	"SOCKET_RECV",
	"ENCRYPT",
	"BAD_SOCKET",
	"KNOWN_HOSTS",
	"CHANNEL_WINDOW_FULL",
	"UNKNOWN"
};


/* SFTP Status Codes (returned by libssh2_sftp_last_error() ) */
static const char *sftp_errors[] = {
	"OK",
	"EOF",
	"NO_SUCH_FILE",
	"PERMISSION_DENIED",
	"FAILURE",
	"BAD_MESSAGE",
	"NO_CONNECTION",
	"CONNECTION_LOST",
	"OP_UNSUPPORTED",
	"INVALID_HANDLE",
	"NO_SUCH_PATH",
	"FILE_ALREADY_EXISTS",
	"WRITE_PROTECT",
	"NO_MEDIA",
	"NO_SPACE_ON_FILESYSTEM",
	"QUOTA_EXCEEDED",
	"UNKNOWN_PRINCIPAL",
	"LOCK_CONFLICT",
	"DIR_NOT_EMPTY",
	"NOT_A_DIRECTORY",
	"INVALID_FILENAME",
	"LINK_LOOP",
	"UNKNOWN"
};
#define san_error(path) {						\
	int thread_id = GetCurrentThreadId();				\
	int err = libssh2_session_last_errno(g_sanssh);		\
	if (err < 0 || err > 47) err = 48;					\
	char* msg = ssh_errors[err];						\
	if (err == LIBSSH2_ERROR_SFTP_PROTOCOL) {			\
		err = libssh2_sftp_last_error(g_sanssh->sftp);	\
		if (err <0 || err>21)	err = 22;				\
		msg = sftp_errors[err];							\
	}													\
	fprintf(stderr, "%d :ERROR: %s: %d: [sftp %ld: %s], path: %s\n", \
			thread_id, __func__,__LINE__, err, msg, path); \
}

#define INVALID_HANDLE ((LIBSSH2_SFTP_HANDLE*)(LONG_PTR)-1)

typedef struct _SANSSH {
	SOCKET socket;
	LIBSSH2_SESSION *ssh;
	LIBSSH2_SFTP *sftp;
	int rc;
	char error[ERROR_LEN];
} SANSSH;

//typedef struct _DIR DIR;
struct dirent
{
	struct fuse_stat d_stat;
	char d_name[255];
};
typedef struct _DIR {
	LIBSSH2_SFTP_HANDLE *handle;
	struct dirent de;
	char path[];
} DIR;

int file_exists(const char* path);
int waitsocket();
void copy_attributes(struct fuse_stat *stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs);
SANSSH *san_init(const char *hostname, int port, const char *username, 
	const char *pkey);
int san_finalize();
int san_stat(const char *path, struct fuse_stat *stbuf);
int san_fstat(size_t fd, struct fuse_stat *stbuf);
int san_statvfs(const char *path, struct fuse_statvfs *st);
DIR *san_opendir(const char *path);
size_t san_dirfd(DIR *dirp);
void san_rewinddir(DIR *dirp);
struct dirent *san_readdir(DIR *dirp);
int san_closedir(DIR *dirp);
int san_truncate(const char *path, fuse_off_t size);
int san_ftruncate(int fd, fuse_off_t size);
int san_mkdir(const char *path);
int san_rmdir(const char *path);
int san_close_handle(LIBSSH2_SFTP_HANDLE *handle);
int san_rename(const char *source, const char *destination);
int san_unlink(const char *path);
int san_fsync(size_t fd);
//int san_realpath(const char *path, char *target);
//int san_read(const char * remotefile, const char * localfile);
int san_read(size_t handle, void *buf, size_t nbyte, fuse_off_t offset);
int san_read_async(const char * remotefile, const char * localfile);
LIBSSH2_SFTP_HANDLE * san_open(const char *path, long mode);
void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st);
void get_filetype(unsigned long perm, char* filetype);
int run_command(const char *cmd, char *out, char *err);

