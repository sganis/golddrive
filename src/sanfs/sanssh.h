#pragma once
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <winfsp/winfsp.h>
#include <fuse.h>
#include "uthash.h"

#define BUFFER_SIZE 32767
#define ERROR_LEN MAXERRORLENGTH
#define O_RDONLY                        _O_RDONLY
#define O_WRONLY                        _O_WRONLY
#define O_RDWR                          _O_RDWR
#define O_APPEND                        _O_APPEND
#define O_CREAT                         _O_CREAT
#define O_EXCL                          _O_EXCL
#define O_TRUNC                         _O_TRUNC

#define PATH_MAX                        1024
#define AT_FDCWD                        -2
#define AT_SYMLINK_NOFOLLOW             2

/* SSH Status Codes (returned by libssh2_ssh_last_error() */
static const char * ssh_errors[] = {
	"OK",
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
#define san_error(path) {								\
	int thread_id = GetCurrentThreadId();				\
	SANSSH* sanssh = get_sanssh();						\
	int err = libssh2_session_last_errno(sanssh->ssh);	\
	if (err > 0 || err < -47) err = -48;					\
	const char* msg = ssh_errors[-err];					\
	if (err == LIBSSH2_ERROR_SFTP_PROTOCOL) {			\
		err = libssh2_sftp_last_error(sanssh->sftp);	\
		if (err <0 || err>21) err = 22;					\
		msg = sftp_errors[err];							\
	} 													\
	fprintf(stderr, "%zd: %d :ERROR: %s: %d: [rc=%d: %s], path: %s\n",	\
				time_ms(), thread_id, __func__,__LINE__, err, msg, path); \
}
//#define san_error(path) {												\
//	int thread_id = GetCurrentThreadId();								\
//	int err = libssh2_sftp_last_error(get_sanssh()->sftp);					\
//	if (err < 0 || err > 21) err = 22;									\
//	fprintf(stderr, "%d :ERROR: %s: %d: [sftp %ld: %s], path: %s\n",	\
//			thread_id, __func__,__LINE__, err, sftp_errors[err], path); \
//}

/* macros */
#define fi_dirbit                       (0x8000000000000000ULL)
#define fi_fh(fi, MASK)                 ((fi)->fh & (MASK))
#define fi_setfh(fi, FH, MASK)          ((fi)->fh = (intptr_t)(FH) | (MASK))
#define fi_fd(fi)                       (fi_fh(fi, fi_dirbit) ? \
										san_dirfd((DIR *)(intptr_t)fi_fh(fi, ~fi_dirbit)) : \
										(int)fi_fh(fi, ~fi_dirbit))
#define fi_setfd(fi, fd)                (fi_setfh(fi, fd, 0))
#define fi_dirp(fi)                     ((DIR *)(intptr_t)fi_fh(fi, ~fi_dirbit))
#define fi_setdirp(fi, dirp)            (fi_setfh(fi, dirp, fi_dirbit))


/* count the number of threads in this app */
/* n is the -o ThreadCount=n arg, c is number of cores*/
int san_threads(int n, int c);
long WinFspLoad(void);

#undef fuse_main
#define fuse_main(argc, argv, ops, data)\
    (WinFspLoad(), fuse_main_real(argc, argv, ops, sizeof *(ops), data))

typedef struct _CMD_ARGS {
	char host[64];
	int port;
	char user[20];
	char pkey[MAX_PATH];
} CMD_ARGS;
extern  CMD_ARGS * g_cmd_args;

typedef struct _SANSSH {
	int thread_id;				/* key */
	SOCKET socket;
	LIBSSH2_SESSION *ssh;
	LIBSSH2_SFTP *sftp;
	int rc;
	char error[ERROR_LEN];
	UT_hash_handle hh;			/* makes this structure hashable */
} SANSSH;
extern SANSSH *g_sanssh_pool;

struct dirent {
	struct fuse_stat d_stat;
	char d_name[FILENAME_MAX];
};

typedef struct _DIR {
	LIBSSH2_SFTP_HANDLE *handle;
	struct dirent de;
	char path[PATH_MAX];
} DIR;

int file_exists(const char* path);
int waitsocket(SANSSH* sanssh);
void copy_attributes(struct fuse_stat *stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs);
SANSSH *san_init(const char *host, int port, const char *user, const char *pkey);
int san_finalize();
int san_stat(const char *path, struct fuse_stat *stbuf);
int san_fstat(int fd, struct fuse_stat *stbuf);
int san_statvfs(const char *path, struct fuse_statvfs *st);
DIR *san_opendir(const char *path);
int san_dirfd(DIR *dirp);
void san_rewinddir(DIR *dirp);
struct dirent *san_readdir(DIR *dirp);
int san_closedir(DIR *dirp);
int san_truncate(const char *path, fuse_off_t size);
int san_ftruncate(int fd, fuse_off_t size);
int san_mkdir(const char *path, fuse_mode_t mode);
int san_rmdir(const char *path);
int san_close(int fd);
int san_rename(const char *source, const char *destination);
int san_unlink(const char *path);
int san_fsync(int fd);
ssize_t san_read(int fd, void *buf, size_t nbyte, fuse_off_t offset);
int san_read_async(const char * remotefile, const char * localfile);
int san_open(const char *path, long mode);
int utime(const char *path, const struct fuse_utimbuf *timbuf);
int utimensat(int dirfd, const char *path, const struct fuse_timespec times[2], int flag);
int setcrtime(const char *path, const struct fuse_timespec *tv);
void mode_human(unsigned long mode, char* human);
void print_permissions(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st);
void get_filetype(unsigned long perm, char* filetype);
int run_command(const char *cmd, char *out, char *err);

// connection pool
void sanssh_pool_add(SANSSH *value);
SANSSH * sanssh_pool_find(int thread_id);
inline SANSSH* get_sanssh(void);
