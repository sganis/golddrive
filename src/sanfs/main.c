#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include "sanssh2.h"
#include "winposix.h"

CRITICAL_SECTION gCriticalSection;
//HANDLE gMutex;

#define debug(...)						printf("%s: ", __func__); printf(__VA_ARGS__); printf("\n");
#define PROGNAME                        "sanfs"
#define concat_path(ptfs, fn, fp)       (sizeof fp > (unsigned)snprintf(fp, sizeof fp, "%s%s", ptfs->rootdir, fn))
#define fi_dirbit                       (0x8000000000000000ULL)
#define fi_fh(fi, MASK)                 ((fi)->fh & (MASK))
#define fi_setfh(fi, FH, MASK)          ((fi)->fh = (size_t)(FH) | (MASK))
#define fi_fd(fi)                       (fi_fh(fi, fi_dirbit) ? \
										dirfd((DIR *)(size_t)fi_fh(fi, ~fi_dirbit)) : \
										(int)fi_fh(fi, ~fi_dirbit))
#define fi_dirp(fi)                     ((DIR *)(size_t)fi_fh(fi, ~fi_dirbit))
#define fi_setfd(fi, fd)                (fi_setfh(fi, fd, 0))
#define fi_setdirp(fi, dirp)            (fi_setfh(fi, dirp, fi_dirbit))
#define fs_fullpath(n)					\
    char full ## n[PATH_MAX];           \
    if (!concat_path(((PTFS *)fuse_get_context()->private_data), n, full ## n))\
        return -ENAMETOOLONG;           \
    n = full ## n

typedef struct {
    const char *rootdir;
} PTFS;

static int fs_getattr(
	const char *path, 
	struct fuse_stat *stbuf, 
	struct fuse_file_info *fi)
{
	fs_fullpath(path);
	return -1 != lstat(path, stbuf) ? 0 : -errno;
}

static int fs_mkdir(const char *path, fuse_mode_t mode)
{
	fs_fullpath(path);
    return -1 != mkdir(path, mode) ? 0 : -errno;
}

static int fs_unlink(const char *path)
{
    fs_fullpath(path);
    return -1 != unlink(path) ? 0 : -errno;
}

static int fs_rmdir(const char *path)
{
    fs_fullpath(path);
    return -1 != rmdir(path) ? 0 : -errno;
}

static int fs_rename(const char *oldpath, const char *newpath, unsigned int flags)
{
    fs_fullpath(newpath);
    fs_fullpath(oldpath);
    return -1 != rename(oldpath, newpath) ? 0 : -errno;
}

static int fs_truncate(const char *path, fuse_off_t size, struct fuse_file_info *fi)
{
    if (0 == fi)
    {
        fs_fullpath(path);
        return -1 != truncate(path, size) ? 0 : -errno;
    }
    else
    {
        int fd = fi_fd(fi);
        return -1 != ftruncate(fd, size) ? 0 : -errno;
    }
}

static int fs_open(const char *path, struct fuse_file_info *fi)
{
    fs_fullpath(path);
	int fd;
	//return -1 != (fd = open(path, fi->flags)) ? (fi_setfd(fi, fd), 0) : -errno;
	fd = popen(path, fi->flags);
	if (-1 != fd) 
	{
		int rc = (fi_setfd(fi, fd), 0);
		return rc;
	}
	else 
	{
		return -errno;
	}
}

static int fs_read(const char *path, char *buf, size_t size, fuse_off_t off,
    struct fuse_file_info *fi)
{
    int fd = fi_fd(fi);
    int nb;
    return -1 != (nb = pread(fd, buf, size, off)) ? nb : -errno;
}

static int fs_write(const char *path, const char *buf, size_t size, fuse_off_t off,
    struct fuse_file_info *fi)
{
    int fd = fi_fd(fi);
    int nb;
    return -1 != (nb = pwrite(fd, buf, size, off)) ? nb : -errno;
}

static int fs_statfs(const char *path, struct fuse_statvfs *stbuf)
{
	fs_fullpath(path);
	return -1 != statvfs(path, stbuf) ? 0 : -errno;
}

static int fs_release(const char *path, struct fuse_file_info *fi)
{
    int fd = fi_fd(fi);
    close(fd);
    return 0;
}

static int fs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    int fd = fi_fd(fi);
    return -1 != fsync(fd) ? 0 : -errno;
}

static int fs_opendir(const char *path, struct fuse_file_info *fi)
{
    fs_fullpath(path);
    DIR *dirp;
    return 0 != (dirp = opendir(path)) ? (fi_setdirp(fi, dirp), 0) : -errno;
}

static int fs_readdir(const char *path, void *buf, 
	fuse_fill_dir_t filler, fuse_off_t off,
    struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
	debug("readdir: %s\n", path);
	DIR *dirp = fi_dirp(fi);
	struct dirent *de;

	rewinddir(dirp);
	// fake it
	de = readdir(dirp);
	if (0 != filler(buf, de->d_name, &de->d_stat, 0, FUSE_FILL_DIR_PLUS))
		return -ENOMEM;
	return 0;


//	for (;;)
//	{
//		errno = 0;
//		if (0 == (de = readdir(dirp)))
//			break;
//#if defined(_WIN64) || defined(_WIN32)
//		if (0 != filler(buf, de->d_name, &de->d_stat, 0, FUSE_FILL_DIR_PLUS))
//#else
//		if (0 != filler(buf, de->d_name, 0, 0, 0))
//#endif
//			return -ENOMEM;
//	}
//	return -errno;
    
}

static int fs_releasedir(const char *path, struct fuse_file_info *fi)
{
    DIR *dirp = fi_dirp(fi);
    return -1 != closedir(dirp) ? 0 : -errno;
}



static int fs_create(const char *path, fuse_mode_t mode, struct fuse_file_info *fi)
{
    fs_fullpath(path);
    int fd;
    return -1 != (fd = open(path, fi->flags, mode)) ? (fi_setfd(fi, fd), 0) : -errno;
}

static int fs_utimens(const char *path, const struct fuse_timespec tv[2], struct fuse_file_info *fi)
{
    fs_fullpath(path);
    return -1 != utimensat(AT_FDCWD, path, tv, AT_SYMLINK_NOFOLLOW) ? 0 : -errno;
}

static void *fs_init(struct fuse_conn_info *conn, struct fuse_config *conf)
{
	conn->want |= (conn->capable & FUSE_CAP_READDIRPLUS);

#if defined(FSP_FUSE_CAP_CASE_INSENSITIVE)
	conn->want |= (conn->capable & FSP_FUSE_CAP_CASE_INSENSITIVE);
#endif

	return fuse_get_context()->private_data;
}

static struct fuse_operations fs_ops =
{
    .getattr = fs_getattr,
    .mkdir = fs_mkdir,
    .unlink = fs_unlink,
    .rmdir = fs_rmdir,
    .rename = fs_rename,
    //.chmod = fs_chmod,
    //.chown = fs_chown,
    .truncate = fs_truncate,
    .open = fs_open,
    .read = fs_read,
    .write = fs_write,
    .statfs = fs_statfs,
    .release = fs_release,
    .fsync = fs_fsync,
    .opendir = fs_opendir,
    .readdir = fs_readdir,
    .releasedir = fs_releasedir,
    .init = fs_init,
    .create = fs_create,
    .utimens = fs_utimens,
};

static void usage(void)
{
    fprintf(stderr, "usage: " PROGNAME " [FUSE options] rootdir mountpoint\n");
    exit(2);
}

int main(int argc, char *argv[])
{
    PTFS ptfs = { 0 };

    if (3 <= argc && '-' != argv[argc - 2][0] && '-' != argv[argc - 1][0])
    {
        ptfs.rootdir = realpath(argv[argc - 2], 0); /* memory freed at process end */

		// test 
		char name[] = "";
		ptfs.rootdir = malloc(strlen(name) + 1);
		strcpy(ptfs.rootdir, name);

        argv[argc - 2] = argv[argc - 1];
        argc--;
    }

	if (0 == ptfs.rootdir) {
		for (int argi = 1; argc > argi; argi++)	{
			int strncmp(const char *a, const char *b, size_t length);
			char *strchr(const char *s, int c);
			char *p = 0;

			if (0 == strncmp("--UNC=", argv[argi], sizeof "--UNC=" - 1))
				p = argv[argi] + sizeof "--UNC=" - 1;
			else if (0 == strncmp("--VolumePrefix=", argv[argi], sizeof "--VolumePrefix=" - 1))
				p = argv[argi] + sizeof "--VolumePrefix=" - 1;

			if (0 != p && '\\' != p[1])
			{
				p = strchr(p + 1, '\\');
				if (0 != p &&
					(
					('A' <= p[1] && p[1] <= 'Z') ||
						('a' <= p[1] && p[1] <= 'z')
						) &&
					'$' == p[2])
				{
					p[2] = ':';
					ptfs.rootdir = realpath(p + 1, 0); /* memory freed at process end */
					p[2] = '$';
					break;
				}
			}
		}
	}

    if (0 == ptfs.rootdir)
        usage();



	//printf("main thread: %d\n", GetCurrentThreadId());

	// Initialize the critical section one time only.
	if (!InitializeCriticalSectionAndSpinCount(&gCriticalSection, 0x00000400))
		return 1;

	//gMutex = CreateMutex(
	//	NULL,              // default security attributes
	//	FALSE,             // initially not owned
	//	NULL);             // unnamed mutex
	//if (gMutex == NULL)
	//{
	//	printf("CreateMutex error: %d\n", GetLastError());
	//	return 1;


	// run fuse main
    int rc = fuse_main(argc, argv, &fs_ops, &ptfs);

	// Release resources used by the critical section object.
	DeleteCriticalSection(&gCriticalSection);
	//CloseHandle(gMutex);
	return rc;
}
