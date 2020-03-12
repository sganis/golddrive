#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "util.h"
#include "fs.h"
#include "gd.h"

void *f_init(struct fuse_conn_info *conn, struct fuse_config *conf)
{
#if defined(FUSE_CAP_READDIRPLUS)
	conn->want |= (conn->capable & FUSE_CAP_READDIRPLUS);
#endif
	//conn->want |= (conn->capable & FSP_FUSE_CAP_STAT_EX);
#if defined(FSP_FUSE_USE_STAT_EX) && defined(FSP_FUSE_CAP_STAT_EX)
	conn->want |= (conn->capable & FSP_FUSE_CAP_STAT_EX);
#endif
	return fuse_get_context()->private_data;
}

int f_statfs(const char* path, struct fuse_statvfs* stbuf)
{
	realpath(path);
	return -1 != gd_statvfs(path, stbuf) ? 0 : -errno;
}

int f_getattr(const char *path, struct fuse_stat *stbuf, struct fuse_file_info *fi)
{
	int rc;
	if (0 == fi)
	{
		realpath(path);
		rc = -1 != gd_stat(path, stbuf) ? 0 : -errno;
	}
	else
	{
		intptr_t fd = fi_fd(fi);
		rc = -1 != gd_fstat(fd, stbuf) ? 0 : -errno;
	}
	//if (rc) {
	//	// debug
	//	int err = -errno;
	//}

	//printf("f_getattr: %s, rc=%d\n", path, rc);

	return rc;
}

int f_readlink(const char* path, char* buf, size_t size)
{
	realpath(path);
	int rc = -1 != gd_readlink(path, buf, size) ? 0 : -errno;
	/*if (rc) {
		int err = -errno;
	}*/
	return rc;
}


int f_unlink(const char *path)
{
	realpath(path);
	int rc = -1 != gd_unlink(path) ? 0 : -errno;
	/*if (rc) {
		int err = -errno;
	}*/
	return rc;
}



int f_create(const char* path, fuse_mode_t mode, struct fuse_file_info* fi)
{
	// printf("f_create: %s, mode=%d, flags=%d\n", path, mode, fi->flags);

	realpath(path);
	intptr_t fd;
	// remove execution bit in files
	// fuse_mode_t mod = mode & 0666; // int 438 
	fuse_mode_t mod = mode;
	int rc = -1 != (fd = gd_open(path, fi->flags, mod)) ? (fi_setfd(fi, fd), 0) : -errno;
	return rc;
}

int f_truncate(const char* path, fuse_off_t size, struct fuse_file_info* fi)
{
	if (0 == fi)
	{
		realpath(path);
		return -1 != gd_truncate(path, size) ? 0 : -errno;
	}
	else
	{
		intptr_t fd = fi_fd(fi);
		return -1 != gd_ftruncate(fd, size) ? 0 : -errno;
	}
}

int f_open(const char *path, struct fuse_file_info *fi)
{
	// printf("f_open: %s, flags=%d\n", path, fi->flags);

	realpath(path);
	intptr_t fd;
	int rc = -1 != (fd = gd_open(path, fi->flags, 0)) ? (fi_setfd(fi, fd), 0) : -errno;
	/*if (rc != 0) {
		printf("error: f_open: %s, flags=%d\n", path, fi->flags);
	}*/
	return rc;
}

int f_read(const char *path, char *buf, size_t size, fuse_off_t off, struct fuse_file_info *fi)
{
	intptr_t fd = fi_fd(fi);
	int nb;
	int rc = -1 != (nb = gd_read(fd, buf, size, off)) ? nb : -errno;
	
	return rc;
	
}

int f_write(const char *path, const char *buf, size_t size, fuse_off_t off,	struct fuse_file_info *fi)
{
	int rc = 0;
	intptr_t fd = fi_fd(fi);
	int nb;
	rc = -1 != (nb = gd_write(fd, buf, size, off)) ? nb : -errno;
	
	//if(size != nb)
	//	printf("f_write error: %s, flags=%d, size=%lld, rc=%d\n", path, fi->flags, size, rc);

	return rc;
}


int f_release(const char *path, struct fuse_file_info *fi)
{
	intptr_t fd = fi_fd(fi);
	return gd_close(fd);
}

int f_rename(const char* oldpath, const char* newpath, unsigned int flags)
{
	realpath(newpath);
	realpath(oldpath);
	int rc = -1 != gd_rename(oldpath, newpath) ? 0 : -errno;
	/*if (rc) {
		int err = -errno;
	}*/
	return rc;
}

int f_opendir(const char *path, struct fuse_file_info *fi)
{
	realpath(path);
	gd_dir_t *dirp;
	return 0 != (dirp = gd_opendir(path)) ? (fi_setdirp(fi, dirp), 0) : -errno;
}

int f_readdir(const char *path, void *buf, fuse_fill_dir_t filler, fuse_off_t off,
	struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
	gd_dir_t *dirp = fi_dirp(fi);	
	struct gd_dirent_t *de;

	gd_rewinddir(dirp);

	for (;;)
	{
		errno = 0;
		if (0 == (de = gd_readdir(dirp)))
			break;
		
		/* skip hidden files */
		//if (!g_fs.hidden && de->hidden) {
		//	//printf("skipping hidden file: %s\n", fname);
		//	continue;
		//}
		if (0 != filler(buf, de->d_name, &de->d_stat, 0, FUSE_FILL_DIR_PLUS))
			return -ENOMEM;
	}

	return -errno;
}

int f_releasedir(const char *path, struct fuse_file_info *fi)
{
	gd_dir_t *dirp = fi_dirp(fi);
	return gd_closedir(dirp);
}

int f_mkdir(const char* path, fuse_mode_t  mode)
{
	realpath(path);
	int rc = -1 != gd_mkdir(path, mode) ? 0 : -errno;
	/*if (rc) {
		int err = -errno;
	}*/
	return rc;
}

int f_rmdir(const char* path)
{
	realpath(path);
	//int rc;
	// rmdir fails if hidden files are not shown
	//if (!g_fs.hidden)
	//	rc = gd_rm_hidden(path);

	int rc = -1 != gd_rmdir(path) ? 0 : -errno;
	/*if (rc) {
		int err = -errno;
	}*/
	//ShowLastError();
	return rc;
}


int f_utimens(const char *path, const struct fuse_timespec tv[2], struct fuse_file_info *fi)
{
	realpath(path);
	return -1 != gd_utimens(path, tv, fi) ? 0 : -errno;
}

int f_fsync(const char* path, int datasync, struct fuse_file_info* fi)
{
	intptr_t fd = fi_fd(fi);
	return -1 != gd_fsync(fd) ? 0 : -errno;
}

int f_flush(const char* path, struct fuse_file_info* fi)
{
	intptr_t fd = fi_fd(fi);
	return -1 != gd_flush(fd) ? 0 : -errno;
}

int f_chmod(const char* path, fuse_mode_t mode, struct fuse_file_info* fi)
{
	realpath(path);
	//char cmd[1000], out[1000], err[1000];
	//sprintf_s(cmd, sizeof cmd, "chmod 777 \"%s\"\n", path);
	//run_command(cmd, out, err);
	return -1 != gd_chmod(path, mode) ? 0 : -errno;
}

int f_chown(const char* path, fuse_uid_t uid, fuse_gid_t gid, struct fuse_file_info* fi)
{
	realpath(path);
	return -1 != gd_chown(path, uid, gid) ? 0 : -errno;
}
int f_mknod(const char* path, fuse_mode_t mode, fuse_dev_t dev)
{
	return 0;
}

int f_setxattr(const char* path, const char* name, const char* value, size_t size, int flags)
{
	realpath(path);
	return -1 != gd_setxattr(path, name, value, size, flags) ? 0 : -errno;
}

int f_getxattr(const char* path, const char* name, char* value, size_t size)
{
	realpath(path);
	int nb;
	return -1 != (nb = gd_getxattr(path, name, value, size)) ? nb : -errno;
}

int f_listxattr(const char* path, char* namebuf, size_t size)
{
	realpath(path);
	int nb;
	return -1 != (nb = gd_listxattr(path, namebuf, size)) ? nb : -errno;
}

int f_removexattr(const char* path, const char* name)
{
	realpath(path);
	return -1 != gd_removexattr(path, name) ? 0 : -errno;
}
