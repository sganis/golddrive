#pragma once
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <winfsp/winfsp.h>
#include <fuse.h>
#include <fcntl.h>
#include "global.h"

// fuse3 fs operations
void *f_init(struct fuse_conn_info *conn, struct fuse_config *conf);
int f_getattr(const char *path, struct fuse_stat *stbuf, struct fuse_file_info *fi);
int f_readlink(const char* path, char* buf, size_t size);
int f_statfs(const char *path, struct fuse_statvfs *st);
int f_opendir(const char *path, struct fuse_file_info *fi);
int f_readdir(const char *path, void *buf, fuse_fill_dir_t filler, fuse_off_t off, struct fuse_file_info *fi, enum fuse_readdir_flags flags);
int f_releasedir(const char *path, struct fuse_file_info *fi);
int f_mkdir(const char *path, fuse_mode_t mode);
int f_rmdir(const char *path);
int f_read(const char *path, char *buf, size_t size, fuse_off_t offset, struct fuse_file_info *fi);
int f_write(const char *path, const char *buf, size_t size, fuse_off_t offset, struct fuse_file_info *fi);
int f_release(const char *path, struct fuse_file_info *fi);
int f_rename(const char *oldpath, const char *newpath, unsigned int flags);
int f_unlink(const char *path);
int f_create(const char *path, fuse_mode_t mode, struct fuse_file_info *fi);
int f_open(const char *path, struct fuse_file_info *fi);
int f_utimens(const char *path, const struct fuse_timespec tv[2], struct fuse_file_info *fi);
int f_truncate(const char *path, fuse_off_t size, struct fuse_file_info *fi);
int f_fsync(const char *path, int datasync, struct fuse_file_info *fi);
int f_flush(const char* path, struct fuse3_file_info* fi);
int f_chmod(const char *path, fuse_mode_t mode, struct fuse_file_info *fi);
int f_chown(const char *path, fuse_uid_t uid, fuse_gid_t gid, struct fuse_file_info *fi);
int f_mknod(const char* path, fuse_mode_t mode, fuse_dev_t dev);
int f_setxattr(const char* path, const char* name, const char* value, size_t size, int flags);
int f_getxattr(const char* path, const char* name, char* value, size_t size);
int f_listxattr(const char* path, char* namebuf, size_t size);
int f_removexattr(const char* path, const char* name);





