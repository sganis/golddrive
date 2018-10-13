#pragma once
#include <libssh2.h>
#include <libssh2_sftp.h>

#define BUFFER_SIZE 32767
#define ERROR_LEN MAXERRORLENGTH

typedef struct _SANSSH {
	SOCKET socket;
	LIBSSH2_SESSION *ssh;
	LIBSSH2_SFTP *sftp;
	int rc;
	char error[ERROR_LEN];
} SANSSH;

int file_exists(const char* path);
int waitsocket(SANSSH *sanssh);
SANSSH * san_init(const char* hostname, const char* username, 
	const char* pkey, char* error);
int san_finalize(SANSSH *sanssh);
int san_mkdir(SANSSH *sanssh, const char *path);
int san_rmdir(SANSSH *sanssh, const char *path);
int san_stat(SANSSH *sanssh, const char *path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
int san_lstat(SANSSH *sanssh, const char *path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
int san_statvfs(SANSSH *sanssh, const char *path, LIBSSH2_SFTP_STATVFS *st);
int san_close_handle(LIBSSH2_SFTP_HANDLE *handle);
int san_rename(SANSSH *sanssh, const char *source, const char *destination);
int san_delete(SANSSH *sanssh, const char *filename);
int san_realpath(SANSSH *sanssh, const char *path, char *target);
int san_readdir(SANSSH *sanssh, const char *path);
int san_read(SANSSH *sanssh, const char * remotefile, const char * localfile);
int san_read_async(SANSSH *sanssh, const char * remotefile, const char * localfile);
LIBSSH2_SFTP_HANDLE * san_open(SANSSH *sanssh, const char *path, long mode);
LIBSSH2_SFTP_HANDLE * san_opendir(SANSSH *sanssh, const char *path);
void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st);
void get_filetype(unsigned long perm, char* filetype);
void usage(const char* prog);
int run_command(SANSSH *sanssh,	const char *cmd, char *out, char *err);
