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
	"SSH_OK",
	"SSH_SOCKET_NONE",
	"SSH_BANNER_RECV",
	"SSH_BANNER_SEND",
	"SSH_INVALID_MAC",
	"SSH_KEX_FAILURE",
	"SSH_ALLOC",
	"SSH_SOCKET_SEND",
	"SSH_KEY_EXCHANGE_FAILURE",
	"SSH_TIMEOUT",
	"SSH_HOSTKEY_INIT",
	"SSH_HOSTKEY_SIGN",
	"SSH_DECRYPT",
	"SSH_SOCKET_DISCONNECT",
	"SSH_PROTO",
	"SSH_PASSWORD_EXPIRED",
	"SSH_FILE",
	"SSH_METHOD_NONE",
	"SSH_AUTHENTICATION_FAILED",
	"SSH_PUBLICKEY_UNVERIFIED",
	"SSH_CHANNEL_OUTOFORDER",
	"SSH_CHANNEL_FAILURE",
	"SSH_CHANNEL_REQUEST_DENIED",
	"SSH_CHANNEL_UNKNOWN",
	"SSH_CHANNEL_WINDOW_EXCEEDED",
	"SSH_CHANNEL_PACKET_EXCEEDED",
	"SSH_CHANNEL_CLOSED",
	"SSH_CHANNEL_EOF_SENT",
	"SSH_SCP_PROTOCOL",
	"SSH_ZLIB",
	"SSH_SOCKET_TIMEOUT",
	"SSH_SFTP_PROTOCOL",
	"SSH_REQUEST_DENIED",
	"SSH_METHOD_NOT_SUPPORTED",
	"SSH_INVAL",
	"SSH_INVALID_POLL_TYPE",
	"SSH_PUBLICKEY_PROTOCOL",
	"SSH_EAGAIN",
	"SSH_BUFFER_TOO_SMALL",
	"SSH_BAD_USE",
	"SSH_COMPRESS",
	"SSH_OUT_OF_BOUNDARY",
	"SSH_AGENT_PROTOCOL",
	"SSH_SOCKET_RECV",
	"SSH_ENCRYPT",
	"SSH_BAD_SOCKET",
	"SSH_KNOWN_HOSTS",
	"SSH_CHANNEL_WINDOW_FULL",
	"SSH_UNKNOWN"
};

/* SFTP Status Codes (returned by libssh2_sftp_last_error() ) */
static const char *sftp_errors[] = {
	"SFTP_OK",
	"SFTP_EOF",
	"SFTP_NO_SUCH_FILE",
	"SFTP_PERMISSION_DENIED",
	"SFTP_FAILURE",
	"SFTP_BAD_MESSAGE",
	"SFTP_NO_CONNECTION",
	"SFTP_CONNECTION_LOST",
	"SFTP_OP_UNSUPPORTED",
	"SFTP_INVALID_HANDLE",
	"SFTP_NO_SUCH_PATH",
	"SFTP_FILE_ALREADY_EXISTS",
	"SFTP_WRITE_PROTECT",
	"SFTP_NO_MEDIA",
	"SFTP_NO_SPACE_ON_FILESYSTEM",
	"SFTP_QUOTA_EXCEEDED",
	"SFTP_UNKNOWN_PRINCIPAL",
	"SFTP_LOCK_CONFLICT",
	"SFTP_DIR_NOT_EMPTY",
	"SFTP_NOT_A_DIRECTORY",
	"SFTP_INVALID_FILENAME",
	"SFTP_LINK_LOOP",
	"SFTP_UNKNOWN"
};
#define san_error(path) {											\
	int thread = GetCurrentThreadId();								\
	SANSSH* sanssh = get_sanssh();									\
	int rc = libssh2_session_last_errno(sanssh->ssh);				\
	if (rc > 0 || rc < -47)											\
		rc = -48;													\
	const char* msg = ssh_errors[-rc];								\
	int skip = 0;													\
	if (rc == LIBSSH2_ERROR_SFTP_PROTOCOL) {						\
		rc = libssh2_sftp_last_error(sanssh->sftp);					\
		if (rc <0 || rc>21)											\
			rc = 22;												\
		/* skip some common errors */								\
		skip = (rc == 2 || rc == 3) ? 1 : 0;						\
		msg = sftp_errors[rc];										\
	} 																\
	if (!skip) {													\
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: [rc=%d: %s], path: %s\n", \
			time_mu(), thread, __func__, __LINE__, rc, msg, path); \
	}																\
}

//#define san_error_init(errmsg, rc) {								\
//	int thread_id = GetCurrentThreadId();							\
//	if (rc > 0 || rc < -47) rc = -48;								\
//	const char* msg = ssh_errors[-rc];								\
//	if (rc == LIBSSH2_ERROR_SFTP_PROTOCOL) {						\
//		rc = libssh2_sftp_last_error(sanssh->sftp);					\
//		if (rc <0 || rc>21) rc = 22;								\
//		msg = sftp_errors[rc];										\
//	} 																\
//	fprintf(stderr, "%zd: %d :ERROR: %s: %d: %s [rc=%d: %s]\n",		\
//				time_ms(), thread_id, __func__,__LINE__, rc,		\
//				errmsg, msg);										\
//}

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
#define fi_setfh(fi, FH, MASK)          ((fi)->fh = (ssize_t)(FH) | (MASK))
#define fi_fd(fi)                       (fi_fh(fi, fi_dirbit) ? \
										san_dirfd((DIR *)(ssize_t)fi_fh(fi, ~fi_dirbit)) : \
										(ssize_t)fi_fh(fi, ~fi_dirbit))
#define fi_setfd(fi, fd)                (fi_setfh(fi, fd, 0))
#define fi_dirp(fi)                     ((DIR *)(ssize_t)fi_fh(fi, ~fi_dirbit))
#define fi_setdirp(fi, dirp)            (fi_setfh(fi, dirp, fi_dirbit))


/* count the number of threads in this app */
/* n is the -o ThreadCount=n arg, c is number of cores*/
int san_threads(int n, int c);

typedef struct _CMD_ARGS {
	char host[64];
	int port;
	char user[20];
	char pkey[MAX_PATH];
} CMD_ARGS;
extern  CMD_ARGS * g_cmd_args;

typedef struct _SANSSH {
	int thread;					/* key */
	SOCKET socket;
	LIBSSH2_SESSION *ssh;
	LIBSSH2_SFTP *sftp;
	int rc;
	char error[ERROR_LEN];
	UT_hash_handle hh;			/* makes this structure hashable */
} SANSSH;

struct dirent {
	struct fuse_stat d_stat;
	char d_name[FILENAME_MAX];
};

//typedef struct {
//	LIBSSH2_SFTP_HANDLE *handle;		/* key */
//	UT_hash_handle hh;				/* makes this structure hashable */
//} san_handke_key_t;

typedef struct _SAN_HANDLE {
	int thread;					/* key */
	LIBSSH2_SFTP_HANDLE *sftp_handle;
	UT_hash_handle hh;			/* makes this structure hashable */
} SAN_HANDLE;

typedef struct _DIR {
	SAN_HANDLE *san_handle;
	struct dirent de;
	char path[PATH_MAX];
} DIR;

void mode_human(unsigned long mode, char* human);
void print_permissions(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st);
void get_filetype(unsigned long perm, char* filetype);
int run_command(const char *cmd, char *out, char *err);

int waitsocket(SANSSH* sanssh);
void copy_attributes(struct fuse_stat *stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs);
SANSSH *san_init_ssh(const char *host, int port, const char *user, const char *pkey);
int san_finalize(void);

// fuse 
void *san_init(struct fuse_conn_info *conn, struct fuse_config *conf);
int san_getattr(const char *path, struct fuse_stat *stbuf, struct fuse_file_info *fi);
int san_stat(const char *path, struct fuse_stat *stbuf);
int san_statfs(const char *path, struct fuse_statvfs *st);
int san_opendir(const char *path, struct fuse_file_info *fi);
int san_readdir(const char *path, void *buf, fuse_fill_dir_t filler, fuse_off_t off,
	struct fuse_file_info *fi, enum fuse_readdir_flags flags);
struct dirent *san_readdir_entry(DIR *dirp);
int san_releasedir(const char *path, struct fuse_file_info *fi);
int san_truncate(const char *path, fuse_off_t size);
int san_mkdir(const char *path, fuse_mode_t mode);
int san_rmdir(const char *path);
int san_release(const char *path, struct fuse_file_info *fi);
int san_rename(const char *oldpath, const char *newpath, unsigned int flags);
int san_unlink(const char *path);
int san_create(const char *path, fuse_mode_t mode, struct fuse_file_info *fi);
int san_open(const char *path, struct fuse_file_info *fi);
int utime(const char *path, const struct fuse_utimbuf *timbuf);
int setcrtime(const char *path, const struct fuse_timespec *tv);
ssize_t san_dirfd(DIR *dirp);
int san_closedir(DIR *dirp);
int san_fstat(ssize_t fd, struct fuse_stat *stbuf);
int san_ftruncate(ssize_t fd, fuse_off_t size);
int san_close(SAN_HANDLE* sh);
int san_fsync(ssize_t fd);
ssize_t san_read(ssize_t fd, void *buf, size_t nbyte, fuse_off_t offset);
int utimensat(ssize_t dirfd, const char *path, const struct fuse_timespec times[2], int flag);

// hash table for connection pool
extern SANSSH *g_ssh_ht;
extern CRITICAL_SECTION g_ssh_lock;
void ht_ssh_add(SANSSH *value);
void ht_ssh_del(SANSSH *value);
SANSSH* ht_ssh_find(int thread);
SANSSH* get_sanssh(void);
inline void ht_ssh_lock(int lock) {
	lock ? EnterCriticalSection(&g_ssh_lock) :
		LeaveCriticalSection(&g_ssh_lock);
}

// hash table with handles to close
extern SAN_HANDLE * g_handle_close_ht;
extern CRITICAL_SECTION g_handle_close_lock;
void ht_handle_close_add(SAN_HANDLE *value);
void ht_handle_close_del(SAN_HANDLE *value);
SAN_HANDLE* ht_handle_close_find(int thread);
inline void ht_handle_close_lock(int lock) {
	lock ? EnterCriticalSection(&g_handle_close_lock) :
		LeaveCriticalSection(&g_handle_close_lock);
}
