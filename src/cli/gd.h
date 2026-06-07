// src/cli/gd.h
#pragma once
#include "config.h"
#include "jsmn.h"

GDSSH *gd_init_ssh(void);
int gd_reconnect(void);
int gd_finalize(int);
int gd_stat(const char *path, struct fuse_stat *stbuf);
int gd_fstat(intptr_t fd, struct fuse_stat *stbuf);
int gd_readlink(const char* path, char* buf, size_t size);
int gd_mkdir(const char *path, fuse_mode_t mode);
int gd_unlink(const char *path);
int gd_rmdir(const char * path);
int gd_rename(const char *from, const char *to);
int gd_truncate(const char *path, fuse_off_t size);
int gd_ftruncate(intptr_t fd, fuse_off_t size);
intptr_t gd_open(const char *path, int flags, unsigned int mode);
GDDIR* gd_opendir(const char* path);
int gd_read(intptr_t fd, void *buf, size_t size, fuse_off_t offset);
int gd_write(intptr_t fd, const void *buf, size_t size, fuse_off_t offset);
int gd_statvfs(const char * path, struct fuse_statvfs *stbuf);
int gd_close(intptr_t fd);
void gd_rewinddir(GDDIR* dirp);
struct GDDIRENT * gd_readdir(GDDIR *dirp);
int gd_closedir(GDDIR *dirp);
intptr_t gd_dirfd(GDDIR *dirp);
int gd_check_hlink(const char *path);
int gd_utimens(const char* path, const struct fuse_timespec tv[2], struct fuse_file_info* fi);
int gd_fsync(intptr_t fd);
int gd_flush(intptr_t fd);
void gd_log(const char* fmt, ...);
int load_json(GDCONFIG* conf);

/* Send message to UsageUrl in config.json */
HANDLE* gd_usage(const char* action, const char* data);

DWORD WINAPI _post_background(LPVOID data);
int _post(const char* url, const char* data);
int get_ssh_error(GDSSH* ssh);
int map_error(int rc);
int run_command_channel_exec(const char* cmd, char* out, char* err);
int waitsocket(GDSSH* ssh);

void libssh2_logger(LIBSSH2_SESSION* session, void* context,
	const char* data, size_t length);
void copy_attributes(struct fuse_stat* stbuf, LIBSSH2_SFTP_ATTRIBUTES* attrs);
