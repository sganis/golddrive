#pragma once

#define USE_LIBSSH 1
//#define USE_LIBSSH2 1
//#define USE_WOLFSSH 1

#define BUFFER_SIZE 256000
#define ERROR_LEN MAXERRORLENGTH
#define MAX_PATH 1024
#define ERROR_LEN 1024

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


#ifdef USE_LIBSSH
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#elif USE_LIBSSH2
#include <libssh2.h>
#include <libssh2_sftp.h>
#elif USE_WOLFSSH
#include <wolfssh/ssh.h>
#include <wolfssh/wolfsftp.h>
#endif

#include <fcntl.h>

#ifdef USE_LIBSSH

#elif USE_LIBSSH2

#elif USE_WOLFSSH

#endif

typedef struct SANSSH {
#ifdef USE_LIBSSH
    ssh_session ssh;
    sftp_session sftp;
#elif USE_LIBSSH2
    LIBSSH2_SESSION* ssh;
    LIBSSH2_SFTP* sftp;
#elif USE_WOLFSSH
    WOLFSSH* ssh;
#endif	
    SOCKET socket;
	int rc;
	char error[ERROR_LEN];
} SANSSH;

typedef struct SANHANDLE {
#ifdef USE_LIBSSH
    sftp_file file_handle;				/* key, remote file handler				*/
    sftp_dir dir_handle;				/* key, remote dir handler				*/
#elif USE_LIBSSH2
    LIBSSH2_SFTP_HANDLE* file_handle;	/* key, remote file handler				*/
    LIBSSH2_SFTP_HANDLE* dir_handle;	/* key, remote file handler				*/
#elif USE_WOLFSSH
    byte* file_handle;
    byte* dir_handle;
#endif
    int dir;						/* is directory							*/
    int flags;						/* open flags							*/
    int mode;						/* open mode							*/
    char path[MAX_PATH];			/* file full path						*/
} SANHANDLE;

typedef struct SANSTAT {
    unsigned long flags;
    unsigned long filesize;
    unsigned long uid;
    unsigned long gid;
    unsigned long mode;
    unsigned long atime;
    unsigned long mtime;
} SANSTAT;

typedef struct SANSTATVFS {
    unsigned long  f_bsize;    /* file system block size */
    unsigned long  f_frsize;   /* fragment size */
    unsigned long  f_blocks;   /* size of fs in f_frsize units */
    unsigned long  f_bfree;    /* # free blocks */
    unsigned long  f_bavail;   /* # free blocks for non-root */
    unsigned long  f_files;    /* # inodes */
    unsigned long  f_ffree;    /* # free inodes */
    unsigned long  f_favail;   /* # free inodes for non-root */
    unsigned long  f_fsid;     /* file system ID */
    unsigned long  f_flag;     /* mount flags */
    unsigned long  f_namemax;  /* maximum filename length */
} SANSTATVFS;

int file_exists(const char* path);
int waitsocket(SANSSH *sanssh);
SANSSH* san_init(const char* hostname, int port, const char* username,
	const char* pkey, char* error);
int san_finalize(SANSSH *sanssh);
int san_mkdir(SANSSH *sanssh, const char *path);
int san_rmdir(SANSSH *sanssh, const char *path);
int san_stat(SANSSH *sanssh, const char *path, SANSTAT*attrs);
int san_statvfs(SANSSH *sanssh, const char *path, SANSTATVFS*st);
int san_rename(SANSSH *sanssh, const char *source, const char *destination);
int san_delete(SANSSH *sanssh, const char *filename);
int san_realpath(SANSSH *sanssh, const char *path, char *target);
int san_readdir(SANSSH *sanssh, const char *path);
int san_read(SANSSH *sanssh, const char * remotefile, const char * localfile);
int san_read_async(SANSSH *sanssh, const char * remotefile, const char * localfile);
int san_write(SANSSH* sanssh, const char* remotefile, const char* localfile);
int san_write_async(SANSSH* sanssh, const char* remotefile, const char* localfile);
void print_stat(const char* path, SANSTAT*attrs);
void print_statvfs(const char* path, SANSTATVFS*st);
//void get_filetype(unsigned long perm, char* filetype);
void usage(const char* prog);
int run_command(SANSSH *sanssh,	const char *cmd, char *out, char *err);
SANHANDLE* san_open(SANSSH* sanssh, const char* path, long mode);
SANHANDLE* san_opendir(SANSSH* sanssh, const char* path);
int san_close(SANHANDLE* handle);
void copy_attributes(struct SANSTAT* stbuf, void* attrs);
