#pragma once
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <winfsp/winfsp.h>
#include <fuse.h>
#include <fcntl.h>


#define BUFFER_SIZE 32767
#define ERROR_LEN MAXERRORLENGTH
#define O_ACCMODE						0x0003
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

#define gd_error(path) {											\
	int thread = GetCurrentThreadId();								\
	rc = libssh2_session_last_errno(g_ssh->ssh);					\
	/*printf("libssh2_session_last_errno: %d\n", rc);*/				\
	if (rc > 0 || rc < -47)											\
		rc = -48;													\
	const char* msg = ssh_errors[-rc];								\
	int skip = 0;													\
	if (rc == LIBSSH2_ERROR_SFTP_PROTOCOL) {						\
		rc = libssh2_sftp_last_error(g_ssh->sftp);					\
		/*printf("libssh2_sftp_last_error: %d\n", rc);*/			\
		if (rc <0 || rc>21)											\
			rc = 22;												\
		/* skip some common errors	*/								\
		/* 2: no such file			*/								\
		/* 3: access denied			*/								\
		skip = (rc == 2 || rc == 3) ? 1 : 0;						\
		msg = sftp_errors[rc];										\
	} 																\
	if (!skip) {													\
		fprintf(stderr, "%zd: %d :ERROR: %s: %d: [rc=%d: %s], path: %s\n", \
			time_mu(), thread, __func__, __LINE__, rc, msg, path);	\
		fflush(stderr);												\
		fflush(stdout);												\
	}																\
}


/* macros */
#define fi_dirbit                       (0x8000000000000000ULL)
#define fi_fh(fi, MASK)                 ((fi)->fh & (MASK))
#define fi_setfh(fi, FH, MASK)          ((fi)->fh = (size_t)(FH) | (MASK))
#define fi_fd(fi)                       (fi_fh(fi, fi_dirbit) ? \
										san_dirfd((gd_dir_t *)(size_t)fi_fh(fi, ~fi_dirbit)) : \
										(size_t)fi_fh(fi, ~fi_dirbit))
#define fi_setfd(fi, fd)                (fi_setfh(fi, fd, 0))
#define fi_dirp(fi)                     ((gd_dir_t *)(size_t)fi_fh(fi, ~fi_dirbit))
#define fi_setdirp(fi, dirp)            (fi_setfh(fi, dirp, fi_dirbit))

#define concat_path(s1, s2, s)			(sizeof s > (unsigned)snprintf(s, sizeof s, "%s%s", s1, s2))
#define realpath(n)						\
    char real ## n[PATH_MAX];			\
	/* expand home directory */			\
	if (strncmp(n, "/~", 2) == 0) {		\
		if (!concat_path(g_fs.home, n+2, real ## n))	\
			return -ENAMETOOLONG;		\
		n = real ## n;					\
	}										


/* count the number of threads in this app */
/* n is the -o ThreadCount=n arg, c is number of cores*/
int san_threads(int n, int c);

typedef struct gdssh_t {
	int thread;						/* key, thread id that owns this struct */
	SOCKET socket;					/* sockey id */
	LIBSSH2_SESSION *ssh;			/* ssh session struct */
	LIBSSH2_SFTP *sftp;				/* sftp session struct */
	int rc;							/* return code from the last ssh/sftp call */
} gdssh_t;

typedef struct gdhandle_t {
	LIBSSH2_SFTP_HANDLE *handle;	/* key, remote file handler				*/
	int is_dir;						/* is directory							*/
	int flags;						/* open flags							*/
	int mode;						/* open mode							*/
	char path[PATH_MAX];			/* file full path						*/
} gd_handle_t;

struct gd_dirent_t {
	struct fuse_stat d_stat;		/* file stats */
	char d_name[FILENAME_MAX];		/* file name  */
};

typedef struct gd_dir_t {
	gd_handle_t *handle;			/* file handle			*/
	struct gd_dirent_t de;			/* file item entry		*/
	char path[PATH_MAX];			/* directory full path	*/
} gd_dir_t;

enum _FILE_TYPE {
	FILE_ISREG = 0,
	FILE_ISDIR = 1,
} FILE_TYPE;

void mode_human(unsigned long mode, char* human);
void print_permissions(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st);
void get_filetype(unsigned long perm, char* filetype);
int run_command(const char *cmd, char *out, char *err);
//char * realpath(const char * path);
int waitsocket(gdssh_t* sanssh);
void copy_attributes(struct fuse_stat *stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs);

// fs operations
void *f_init(struct fuse_conn_info *conn, struct fuse_config *conf);
int f_getattr(const char *path, struct fuse_stat *stbuf, struct fuse_file_info *fi);
int f_statfs(const char *path, struct fuse_statvfs *st);
int f_opendir(const char *path, struct fuse_file_info *fi);
int f_readdir(const char *path, void *buf, fuse_fill_dir_t filler, fuse_off_t off, struct fuse_file_info *fi, enum fuse_readdir_flags flags);
int f_releasedir(const char *path, struct fuse_file_info *fi);
int f_mkdir(const char *path, fuse_mode_t mode);
int f_rmdir(const char *path);
int f_read(const char *path, char *buf, size_t size, fuse_off_t offset, struct fuse_file_info *fi);
int f_write(const char *path, const char *buf, size_t size, fuse_off_t offset, struct fuse_file_info *fi);
int f_release(const char *path, struct fuse_file_info *fi);
int f_flush(const char *path, struct fuse_file_info *fi);
int f_rename(const char *oldpath, const char *newpath, unsigned int flags);
int f_unlink(const char *path);
int f_create(const char *path, fuse_mode_t mode, struct fuse_file_info *fi);
int f_open(const char *path, struct fuse_file_info *fi);
int f_utimens(const char *path, const struct fuse_timespec tv[2], struct fuse_file_info *fi);
int f_truncate(const char *path, fuse_off_t size, struct fuse_file_info *fi);
int f_fsync(const char *path, int datasync, struct fuse_file_info *fi);

gd_handle_t * gd_openfile(const char *path, int is_dir, unsigned int mode, struct fuse_file_info *fi);
int gd_stat(const char *path, struct fuse_stat *stbuf);
int gd_close(gd_handle_t* sh);
int gd_rename(const char *oldpath, const char *newpath);
int utimensat(int dirfd, const char *path, const struct fuse_timespec times[2], int flag);
size_t san_dirfd(gd_dir_t *dirp);
gdssh_t *gd_init_ssh(const char *host, int port, const char *user, const char *pkey);
int gd_finalize(void);
int gd_check_hlink(const char *path);

extern gdssh_t *g_ssh;
extern SRWLOCK g_ssh_lock;


inline void gd_lock() { AcquireSRWLockExclusive(&g_ssh_lock); }
inline void gd_unlock() { ReleaseSRWLockExclusive(&g_ssh_lock); }


