#pragma once

//#define USE_LIBSSH

// windows file attributes
//#define FSP_FUSE_USE_STAT_EX

#include <stdio.h>
#include <fcntl.h>
#ifdef USE_LIBSSH
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#else
#include <libssh2.h>
#include <libssh2_sftp.h>
#endif
#include <winfsp/winfsp.h>
#include <fuse.h>

#define BUFFER_SIZE						256000
#define COMMAND_SIZE					1024

#define FSP_FUSE_CAP_STAT_EX            (1 << 23)   /* file system supports fuse_stat_ex */
/* from FreeBSD */
#define FSP_FUSE_UF_HIDDEN              0x00008000
#define FSP_FUSE_UF_READONLY            0x00001000
#define FSP_FUSE_UF_SYSTEM              0x00000080
#define FSP_FUSE_UF_ARCHIVE             0x00000800

#define ERROR_LEN MAXERRORLENGTH
#define O_ACCMODE						0x0003
#define PATH_MAX                        1024
#define AT_FDCWD                        -2
#define AT_SYMLINK_NOFOLLOW             2

extern size_t		g_sftp_calls;
extern char*		g_logfile;

#define STATS		0
#define USE_CACHE	0
#define CACHE_TTL	1000 /* millisecs */

#ifdef _WIN64
#define PLATFORM_BITS 64
#else
#define PLATFORM_BITS 32
#endif

/* logging */
#define ERROR		0
#define WARN		1
#define INFO		2
#define DEBUG		3
#define LOGLEVEL	ERROR

#define log_message(level, format, ...) {								\
	int thread = GetCurrentThreadId();									\
	printf("%zd: %zd: %-6d: %s: %-15s:%3d: ",							\
		g_sftp_calls, time_mu(), thread, level, __func__, __LINE__);	\
	printf(format, __VA_ARGS__);										\
	fflush(stdout);														\
}
#define log_debug(format, ...)  log_message("DEBUG", format, __VA_ARGS__) 
#define log_info(format, ...)   log_message("INFO ", format, __VA_ARGS__) 
#define log_warn(format, ...)   log_message("WARN ", format, __VA_ARGS__) 
#define log_error(format, ...)  log_message("ERROR", format, __VA_ARGS__) 

#if LOGLEVEL < DEBUG
#undef log_debug
#define log_debug(format, ...) {}
#endif
#if LOGLEVEL < INFO
#undef log_info
#define log_info(format, ...) {}
#endif
#if LOGLEVEL < WARN
#undef log_warn
#define log_warn(format, ...) {}
#endif

typedef struct fs_config {
	char *remote;
	char *host;
	char *locuser;
	char *user;
	char *pkey;
	char *drive;
	char *json;
	char *args;
	char *home;
	char* root;
	char* cipher;
	int has_root;
	int keeplink;
	int compress;
	int block;
	char *mountpoint;
	char letter;
	int port;
	unsigned buffer;
	unsigned local_uid;
	unsigned remote_uid;
} fs_config;

extern fs_config g_fs;

#ifndef USE_LIBSSH
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
#endif

/* macros */
#define fi_dirbit                       (0x8000000000000000ULL)
#define fi_fh(fi, MASK)                 ((fi)->fh & (MASK))
#define fi_setfh(fi, FH, MASK)          ((fi)->fh = (intptr_t)(FH) | (MASK))
#define fi_fd(fi)                       (fi_fh(fi, fi_dirbit) ? \
										gd_dirfd((gd_dir_t *)(intptr_t)fi_fh(fi, ~fi_dirbit)) : \
										(intptr_t)fi_fh(fi, ~fi_dirbit))
#define fi_setfd(fi, fd)                (fi_setfh(fi, fd, 0))
#define fi_dirp(fi)                     ((gd_dir_t *)(intptr_t)fi_fh(fi, ~fi_dirbit))
//#define fi_dirp(fi)                     ((DIR *)(intptr_t)fi_fh(fi, ~fi_dirbit))

#define fi_setdirp(fi, dirp)            (fi_setfh(fi, dirp, fi_dirbit))

#define concat_path(s1, s2, s)			(sizeof s > (unsigned)snprintf(s, sizeof s, "%s%s", s1, s2))
#define error()						    ((errno = map_error(rc)) == 0 ? 0 : -1)
#define error0()						((errno = map_error(rc)) == 0 ? 0 : 0)
#define realpath(n)						\
	if (g_fs.has_root) { 				\
		char full ## n[PATH_MAX];		\
		if (!concat_path(g_fs.root, n, full ## n)) \
			return -ENAMETOOLONG;       \
		n = full ## n;					\
	}



#ifdef USE_LIBSSH
#define gd_error(path) {															\
    rc = get_ssh_error(g_ssh);												\
	/* skip errors 2, 3 */ \
	if (2 < rc && rc > 3) { \
		const char* msg = ssh_get_error(g_ssh->ssh);				\
		gd_log("%zu: %d :ERROR: %s: %d: [rc=%d: %s], path: %s\n",					\
			time_mu(), GetCurrentThreadId(), __func__, __LINE__, rc, msg, path);	\
	} \
}
#else
#define gd_error(path) {															\
    rc = get_ssh_error(g_ssh);												\
	/* skip errors 2, 3 */ \
	if (2 < rc && rc > 3) { \
		const char* msg = rc < 0 ? ssh_errors[-rc] : sftp_errors[rc];				\
		gd_log("%zd: %d :ERROR: %s: %d: [rc=%d: %s], path: %s\n",					\
			time_mu(), GetCurrentThreadId(), __func__, __LINE__, rc, msg, path);	\
	} \
}
#endif
/* count the number of threads in this app */
/* n is the -o ThreadCount=n arg, c is number of cores*/
int gd_threads(int n, int c);

typedef struct gdssh_t {
	int rc;							/* return code from the last ssh/sftp call */
	int thread;						/* key, thread id that owns this struct */
	SOCKET socket;					/* sockey id */
#ifdef USE_LIBSSH
	ssh_session ssh;
	sftp_session sftp;
	ssh_channel channel;
#else
	LIBSSH2_SESSION *ssh;			/* ssh session struct */
	LIBSSH2_SFTP* sftp;				/* sftp session struct */
	LIBSSH2_CHANNEL* channel;		/* channel for commands */
#endif
} gdssh_t;

typedef struct gdhandle_t {
#ifdef USE_LIBSSH
	sftp_file file_handle;				/* key, remote file handler				*/
	sftp_dir dir_handle;				/* key, remote dir handler				*/
#else
	LIBSSH2_SFTP_HANDLE* file_handle;	/* key, remote file handler				*/
	LIBSSH2_SFTP_HANDLE* dir_handle;	/* key, remote file handler				*/
#endif
	int dir;						/* is directory							*/
	int flags;						/* open flags							*/
	int mode;						/* open mode							*/
	char path[PATH_MAX];			/* file full path						*/
	long size;
} gd_handle_t;

struct gd_dirent_t {
	struct fuse_stat d_stat;		/* file stats                           */
	char d_name[FILENAME_MAX];		/* file name                            */
	int dir;						/* is directory							*/
	int hidden;						/* is hidden							*/
};

typedef struct gd_dir_t {
	gd_handle_t *handle;			/* file handle			                */
	struct gd_dirent_t de;			/* file item entry		                */
	char path[PATH_MAX];			/* directory full path	                */
} gd_dir_t;

enum _FILE_TYPE {
	FILE_ISREG = 0,
	FILE_ISDIR = 1,
} FILE_TYPE;

extern gdssh_t *g_ssh;
extern SRWLOCK g_ssh_lock;
//extern CRITICAL_SECTION g_critical_section;

inline void gd_lock() 
{ 
	//printf("locking...");
	AcquireSRWLockExclusive(&g_ssh_lock);
	//EnterCriticalSection(&g_critical_section);
	//printf("locking done\n");
}
inline void gd_unlock() 
{
	//printf("unlocking...");
	ReleaseSRWLockExclusive(&g_ssh_lock);
	//LeaveCriticalSection(&g_critical_section);
	//ReleaseSRWLockShared(&g_ssh_lock);
	//printf("unlocking done\n");
}

/* file flags */
#define GD_READONLY   0x00
#define GD_READ   0x01
#define GD_WRITE  0x02
#define GD_APPEND 0x04
#define GD_CREAT  0x08
#define GD_TRUNC  0x10
#define GD_EXCL   0x20
#define GD_TEXT   0x40

/* file type flags */
#define GD_IFMT   0170000 /* type of file mask */
#define GD_IFSOCK 0140000 /* socket */
#define GD_IFLNK  0120000 /* symbolic link */
#define GD_IFREG  0100000 /* regular file */
#define GD_IFBLK  0060000 /* block special */
#define GD_IFDIR  0040000 /* directory */
#define GD_IFCHR  0020000 /* character special */
#define GD_IFIFO  0010000 /* named pipe */

/* macros to get fily type */
#define GD_ISLNK(m) (((m) & GD_IFMT) == GD_IFLNK)
#define GD_ISREG(m) (((m) & GD_IFMT) == GD_IFREG)
#define GD_ISDIR(m) (((m) & GD_IFMT) == GD_IFDIR)

