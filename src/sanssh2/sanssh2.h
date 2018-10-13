#pragma once
#include <libssh2.h>
#include <libssh2_sftp.h>

#define BUFFER_SIZE 32767

int file_exists(const char* path);
int waitsocket(int socket_fd, LIBSSH2_SESSION *session);
int san_mkdir(LIBSSH2_SFTP *sftp, const char *path);
int san_rmdir(LIBSSH2_SFTP *sftp, const char *path);
int san_stat(LIBSSH2_SFTP *sftp, const char *path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
int san_lstat(LIBSSH2_SFTP *sftp, const char *path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
int san_statvfs(LIBSSH2_SFTP *sftp, const char *path, LIBSSH2_SFTP_STATVFS *st);
int san_close_handle(LIBSSH2_SFTP_HANDLE *handle);
int san_rename(LIBSSH2_SFTP *sftp, const char *source, const char *destination);
int san_delete(LIBSSH2_SFTP *sftp, const char *filename);
int san_realpath(LIBSSH2_SFTP *sftp, const char *path, char *target);
int san_readdir(LIBSSH2_SFTP *sftp, const char *path);
int san_read(LIBSSH2_SESSION *session, LIBSSH2_SFTP *sftp,
	const char * remotefile, const char * localfile);
int san_read_async(SOCKET sock, LIBSSH2_SESSION *session, LIBSSH2_SFTP *sftp,
	const char * remotefile, const char * localfile);
LIBSSH2_SFTP_HANDLE * san_open(LIBSSH2_SFTP *sftp, const char *path, long mode);
LIBSSH2_SFTP_HANDLE * san_opendir(LIBSSH2_SFTP *sftp, const char *path);
void print_stat(const char* path, LIBSSH2_SFTP_ATTRIBUTES *attrs);
void print_statvfs(const char* path, LIBSSH2_SFTP_STATVFS *st);
void get_filetype(unsigned long perm, char* filetype);
void usage(const char* prog);
int run_command(int sock, LIBSSH2_SESSION *session,
	const char *cmd, char *out, char *err);
