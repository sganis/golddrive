#pragma once

#define BUFFER_SIZE 32767
#define ERROR_LEN MAXERRORLENGTH

#ifdef USE_LIBSSH
#include <libssh/libssh.h>
#include <libssh/sftp.h>

typedef struct _SANSSH {
    SOCKET socket;
    ssh_session* ssh;
    sftp_session* sftp;
    int rc;
    char error[ERROR_LEN];
} SANSSH;

typedef struct _SANSSH_ATTRIBUTES {
    uint32_t flags;
    uint64_t size;
    uint32_t uid, gid, mode, atime, mtime;
} SANSSH_ATTRIBUTES;

typedef struct _SANSSH_STATVFS {
    uint64_t  f_bsize;    /* file system block size */
    uint64_t  f_frsize;   /* fragment size */
    uint64_t  f_blocks;   /* size of fs in f_frsize units */
    uint64_t  f_bfree;    /* # free blocks */
    uint64_t  f_bavail;   /* # free blocks for non-root */
    uint64_t  f_files;    /* # inodes */
    uint64_t  f_ffree;    /* # free inodes */
    uint64_t  f_favail;   /* # free inodes for non-root */
    uint64_t  f_fsid;     /* file system ID */
    uint64_t  f_flag;     /* mount flags */
    uint64_t  f_namemax;  /* maximum filename length */
} SANSSH_STATVFS;

#else // USE_LIBSSH2
#include <libssh2.h>
#include <libssh2_sftp.h>

typedef struct _SANSSH {
	SOCKET socket;
	LIBSSH2_SESSION *ssh;
	LIBSSH2_SFTP *sftp;
	int rc;
	char error[ERROR_LEN];
} SANSSH;

typedef struct _SANSSH_ATTRIBUTES {
    unsigned long flags;
    libssh2_uint64_t filesize;
    unsigned long uid, gid;
    unsigned long mode;
    unsigned long atime, mtime;
} SANSSH_ATTRIBUTES;

typedef struct _SANSSH_STATVFS {
    libssh2_uint64_t  f_bsize;    /* file system block size */
    libssh2_uint64_t  f_frsize;   /* fragment size */
    libssh2_uint64_t  f_blocks;   /* size of fs in f_frsize units */
    libssh2_uint64_t  f_bfree;    /* # free blocks */
    libssh2_uint64_t  f_bavail;   /* # free blocks for non-root */
    libssh2_uint64_t  f_files;    /* # inodes */
    libssh2_uint64_t  f_ffree;    /* # free inodes */
    libssh2_uint64_t  f_favail;   /* # free inodes for non-root */
    libssh2_uint64_t  f_fsid;     /* file system ID */
    libssh2_uint64_t  f_flag;     /* mount flags */
    libssh2_uint64_t  f_namemax;  /* maximum filename length */
} SANSSH_STATVFS;

#endif

int file_exists(const char* path);
int waitsocket(SANSSH *sanssh);
SANSSH* san_init(const char* hostname, int port, const char* username,
	const char* pkey, char* error);
int san_finalize(SANSSH *sanssh);
int san_mkdir(SANSSH *sanssh, const char *path);
int san_rmdir(SANSSH *sanssh, const char *path);
int san_stat(SANSSH *sanssh, const char *path, SANSSH_ATTRIBUTES *attrs);
int san_lstat(SANSSH *sanssh, const char *path, SANSSH_ATTRIBUTES *attrs);
int san_statvfs(SANSSH *sanssh, const char *path, SANSSH_STATVFS *st);
int san_rename(SANSSH *sanssh, const char *source, const char *destination);
int san_delete(SANSSH *sanssh, const char *filename);
int san_realpath(SANSSH *sanssh, const char *path, char *target);
int san_readdir(SANSSH *sanssh, const char *path);
int san_read(SANSSH *sanssh, const char * remotefile, const char * localfile);
int san_read_async(SANSSH *sanssh, const char * remotefile, const char * localfile);
int san_write(SANSSH* sanssh, const char* remotefile, const char* localfile);
int san_write_async(SANSSH* sanssh, const char* remotefile, const char* localfile);
void print_stat(const char* path, SANSSH_ATTRIBUTES *attrs);
void print_statvfs(const char* path, SANSSH_STATVFS *st);
void get_filetype(unsigned long perm, char* filetype);
void usage(const char* prog);
int run_command(SANSSH *sanssh,	const char *cmd, char *out, char *err);
#ifdef USE_LIBSSH
sftp_file san_open(SANSSH* sanssh, const char* path, long mode);
int san_close_file(sftp_file handle);
sftp_dir* san_opendir(SANSSH* sanssh, const char* path);
int san_close_file(sftp_file handle);
int san_close_dir(sftp_dir handle);
void copy_attributes(struct SANSSH_ATTRIBUTES* stbuf, sftp_attributes* attrs);
#else
LIBSSH2_SFTP_HANDLE* san_open(SANSSH* sanssh, const char* path, long mode);
LIBSSH2_SFTP_HANDLE* san_opendir(SANSSH* sanssh, const char* path);
int san_close_handle(LIBSSH2_SFTP_HANDLE* handle);
void copy_attributes(struct SANSSH_ATTRIBUTES* stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs);
#endif