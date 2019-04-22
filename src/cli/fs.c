#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "util.h"
#include "fs.h"
#include "gd.h"

void *f_init(struct fuse_conn_info *conn, struct fuse_config *conf)
{
	//conn->want |= (conn->capable & FUSE_CAP_READDIRPLUS);
#if defined(FUSE_CAP_READDIRPLUS)
	conn->want |= (conn->capable & FUSE_CAP_READDIRPLUS);
#endif

#if defined(FSP_FUSE_USE_STAT_EX) && defined(FSP_FUSE_CAP_STAT_EX)
	conn->want |= (conn->capable & FSP_FUSE_CAP_STAT_EX);
#endif
	return fuse_get_context()->private_data;
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
	if (rc) {
		// debug
		int err = -errno;
	}
	return rc;
}

int f_mkdir(const char * path, fuse_mode_t  mode)
{
	realpath(path);
	int rc = -1 != gd_mkdir(path, mode) ? 0 : -errno;
	if (rc) {
		int err = -errno;
	}
	return rc;
}

int f_unlink(const char *path)
{
	realpath(path);
	int rc = -1 != gd_unlink(path) ? 0 : -errno;
	if (rc) {
		int err = -errno;
	}
	return rc;
}

int f_rmdir(const char * path)
{
	realpath(path);
	//int rc;
	// rmdir fails if hidden files are not shown
	//if (!g_fs.hidden)
	//	rc = gd_rm_hidden(path);
	
	int rc = -1 != gd_rmdir(path) ? 0 : -errno;
	if (rc) {
		int err = -errno;
	}
	//ShowLastError();
	return rc;
}

int f_rename(const char *oldpath, const char *newpath, unsigned int flags)
{
	realpath(newpath);
	realpath(oldpath);
	int rc = -1 != gd_rename(oldpath, newpath) ? 0 : -errno;
	if (rc) {
		int err = -errno;
	}
	return rc;
}

int f_chmod(const char *path, fuse_mode_t mode, struct fuse_file_info *fi)
{
	realpath(path);
	//char cmd[1000], out[1000], err[1000];
	//sprintf_s(cmd, sizeof cmd, "chmod 777 \"%s\"\n", path);
	//run_command(cmd, out, err);
	return -1 != gd_chmod(path, mode) ? 0 : -errno;
}

int f_chown(const char *path, fuse_uid_t uid, fuse_gid_t gid, struct fuse_file_info *fi)
{
	realpath(path);
	return -1 != gd_chown(path, uid, gid) ? 0 : -errno;
}

int f_truncate(const char *path, fuse_off_t size, struct fuse_file_info *fi)
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
	realpath(path);
	intptr_t fd;
	return -1 != (fd = gd_open(path, fi->flags, 0)) ? (fi_setfd(fi, fd), 0) : -errno;
}

int f_read(const char *path, char *buf, size_t size, fuse_off_t off, struct fuse_file_info *fi)
{
	intptr_t fd = fi_fd(fi);
	int nb;
	return -1 != (nb = gd_read(fd, buf, size, off)) ? nb : -errno;
}

int f_write(const char *path, const char *buf, size_t size, fuse_off_t off,	struct fuse_file_info *fi)
{
	intptr_t fd = fi_fd(fi);
	int nb;
	return -1 != (nb = gd_write(fd, buf, size, off)) ? nb : -errno;
}

int f_statfs(const char *path, struct fuse_statvfs *stbuf)
{
	realpath(path);
	return -1 != gd_statvfs(path, stbuf) ? 0 : -errno;
}

int f_release(const char *path, struct fuse_file_info *fi)
{
	intptr_t fd = fi_fd(fi);
	gd_close(fd);
	return 0;
}

int f_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	intptr_t fd = fi_fd(fi);
	return -1 != gd_fsync(fd) ? 0 : -errno;
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
		if (!g_fs.hidden && de->hidden) {
			//printf("skipping hidden file: %s\n", fname);
			continue;
		}
		if (0 != filler(buf, de->d_name, &de->d_stat, 0, FUSE_FILL_DIR_PLUS))
			return -ENOMEM;
	}

	return -errno;
}
//int f_readdir(const char *path, void *buf, fuse_fill_dir_t filler, fuse_off_t off,
//	struct fuse_file_info *fi, enum fuse_readdir_flags flags)
//{
//	log_info("%s\n", path);
//	int rc = 0;
//	gd_dir_t *dirp = fi_dirp(fi);
//	gd_handle_t *sh = dirp->handle;
//	LIBSSH2_SFTP_HANDLE* handle = sh->handle;
//	LIBSSH2_SFTP_ATTRIBUTES attrs;
//	char fname[FILENAME_MAX];
//	//char longentry[512];
//	//struct dirent *de;
//	//struct fuse_stat *stbuf;
//
//	gd_lock();
//	libssh2_sftp_rewind(handle);
//	g_sftp_calls++;
//
//	for (;;) {
//		//de = san_readdir_entry(dirp);
//
//		rc = libssh2_sftp_readdir(handle, fname, FILENAME_MAX, &attrs);
//		g_sftp_calls++;
//		//int a = strcmp(path, "/");
//		//int b = strlen(fname);
//		//int c = fname[1] == ':';
//
//		//if (a == 0 &&  b == 2 && c)
//		//{
//		//	// it is a drive
//		//	fname[1] = '\0';
//		//}
//
//
//		/* skip hidden files */
//		if (rc > 1							/* file name length 2 or more	*/
//			&& !g_fs.hidden				/* user parameter not set		*/
//			&& strncmp(fname, ".", 1) == 0  /* file name starts with .		*/
//			&& strncmp(fname, "..", 2)) {	/* file name is not ..			*/
//			//printf("skipping hidden file: %s\n", fname);
//			continue;
//		}
//
//		if (rc < 0) {
//			gd_error(sh->path);
//			break; // or continue?
//		}
//		if (rc == 0) {
//			// no more files
//			rc = 0;
//			break;
//		}
//
//		//if (longentry[0] != '\0') {
//		//	printf("%s\n", longentry);
//		//}
//		// resolve symbolic links
//		if (attrs.permissions & LIBSSH2_SFTP_S_IFLNK) {
//			char fullpath[MAX_PATH];
//			strcpy_s(fullpath, MAX_PATH, dirp->path);
//			int pathlen = (int)strlen(dirp->path);
//			if (!(pathlen > 0 && dirp->path[pathlen - 1] == '/'))
//				strcat_s(fullpath, 2, "/");
//			strcat_s(fullpath, FILENAME_MAX, fname);
//			memset(&attrs, 0, sizeof attrs);
//			rc = libssh2_sftp_stat_ex(g_ssh->sftp, fullpath,
//				(int)strlen(fullpath), LIBSSH2_SFTP_STAT, &attrs);
//			g_sftp_calls++;
//
//			if (rc) {
//				gd_error(fullpath);
//			}
//		}
//
//		//stbuf = &dirp->de.d_stat;
//		copy_attributes(&dirp->de.d_stat, &attrs);
//		strcpy_s(dirp->de.d_name, FILENAME_MAX, fname);
//
//#if STATS
//		// stats	
//		//debug("sftp calls cached/total: %zd/%zd (%.1f%% cached)\n",
//		//		g_sftp_cached_calls, g_sftp_calls, 
//		//		(g_sftp_cached_calls*100/(double)g_sftp_calls));
//#endif
//
//		//de = &dirp->de;
//
//		//if (!de) {
//		//	break;
//		//}
//		if (0 != filler(buf, dirp->de.d_name, &dirp->de.d_stat, 0, FUSE_FILL_DIR_PLUS)) {
//			rc = -ENOMEM;
//			break;
//		}
//	}
//	gd_unlock();
//	return rc;
//
//}
//

int f_releasedir(const char *path, struct fuse_file_info *fi)
{
	gd_dir_t *dirp = fi_dirp(fi);
	return gd_closedir(dirp);
}

int f_create(const char *path, fuse_mode_t mode, struct fuse_file_info *fi)
{
	realpath(path);
	intptr_t fd;
	// remove execution bit in files
	fuse_mode_t mod = mode & 0666; // int 438 
	int rc = -1 != (fd = gd_open(path, fi->flags, mod)) ? (fi_setfd(fi, fd), 0) : -errno;
	return rc;
}

int f_utimens(const char *path, const struct fuse_timespec tv[2], struct fuse_file_info *fi)
{
	realpath(path);
	return -1 != gd_utimensat(AT_FDCWD, path, tv, AT_SYMLINK_NOFOLLOW) ? 0 : -errno;
}


