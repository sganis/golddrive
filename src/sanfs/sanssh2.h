#pragma once
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <fuse.h>

#define BUFFER_SIZE 32767
#define ERROR_LEN MAXERRORLENGTH
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
#define sftp_error(sanssh, path, rc) {					\
	int thread_id = GetCurrentThreadId();				\
	int sftperr = libssh2_sftp_last_error(sanssh->sftp); \
	if (sftperr <0 || sftperr>21)						\
		sftperr = 22;									\
	fprintf(stderr, "%d :ERROR: %s: %d: rc=%d, [sftp %ld: %s], path: %s\n", \
			thread_id, __func__,__LINE__, rc, sftperr, sftp_errors[sftperr], path); \
}

#define INVALID_HANDLE ((LIBSSH2_SFTP_HANDLE*)(LONG_PTR)-1)

typedef struct _SANSSH {
	SOCKET socket;
	LIBSSH2_SESSION *ssh;
	LIBSSH2_SFTP *sftp;
	int rc;
	char error[ERROR_LEN];
} SANSSH;

typedef struct _DIR DIR;
struct dirent
{
	struct fuse_stat d_stat;
	char d_name[255];
};
struct _DIR
{
	LIBSSH2_SFTP_HANDLE *h;
	struct dirent de;
	char path[];
};

int file_exists(const char* path);
int waitsocket(SANSSH *sanssh);
void copy_attributes(struct fuse_stat *stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs);
SANSSH *san_init(const char *hostname, const char *username, 
	const char *pkey, char *error);
int san_finalize(SANSSH *sanssh);
int san_stat(SANSSH *sanssh, const char *path, struct fuse_stat *stbuf);
int san_lstat(SANSSH *sanssh, const char *path, struct fuse_stat *stbuf);
int san_statvfs(SANSSH *sanssh, const char *path, struct fuse_statvfs *st);
DIR *san_opendir(SANSSH *sanssh, const char *path);
int san_dirfd(DIR *dirp);
void san_rewinddir(DIR *dirp);
struct dirent *san_readdir(DIR *dirp);
int san_closedir(DIR *dirp);

int san_mkdir(SANSSH *sanssh, const char *path);
int san_rmdir(SANSSH *sanssh, const char *path);
int san_close_handle(LIBSSH2_SFTP_HANDLE *handle);
int san_rename(SANSSH *sanssh, const char *source, const char *destination);
int san_delete(SANSSH *sanssh, const char *filename);
int san_realpath(SANSSH *sanssh, const char *path, char *target);
int san_read(SANSSH *sanssh, const char * remotefile, const char * localfile);
int san_read_async(SANSSH *sanssh, const char * remotefile, const char * localfile);
LIBSSH2_SFTP_HANDLE * san_open(SANSSH *sanssh, const char *path, long mode);
void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st);
void get_filetype(unsigned long perm, char* filetype);
int run_command(SANSSH *sanssh,	const char *cmd, char *out, char *err);

