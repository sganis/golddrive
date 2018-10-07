#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
//#include "winposix.h"
//#include "fake.h"
#include "pyutil.h"


//CRITICAL_SECTION CriticalSection;
//HANDLE gMutex;
//PyThreadState * mainThreadState = NULL;
//PyObject *mainModule;

#define debug(...)  printf("%s: ", __func__); printf(__VA_ARGS__); printf("\n");

#define PROGNAME                        "golddrive-fuse"

#define concat_path(ptfs, fn, fp)       (sizeof fp > (unsigned)snprintf(fp, sizeof fp, "%s%s", ptfs->rootdir, fn))

#define fi_dirbit                       (0x8000000000000000ULL)
#define fi_fh(fi, MASK)                 ((fi)->fh & (MASK))
#define fi_setfh(fi, FH, MASK)          ((fi)->fh = (size_t)(FH) | (MASK))
#define fi_fd(fi)                       (fi_fh(fi, fi_dirbit) ? \
    dirfd((DIR *)(size_t)fi_fh(fi, ~fi_dirbit)) : (int)fi_fh(fi, ~fi_dirbit))
#define fi_dirp(fi)                     ((DIR *)(size_t)fi_fh(fi, ~fi_dirbit))
#define fi_setfd(fi, fd)                (fi_setfh(fi, fd, 0))
#define fi_setdirp(fi, dirp)            (fi_setfh(fi, dirp, fi_dirbit))

#define ptfs_impl_fullpath(n)           \
    char full ## n[PATH_MAX];           \
    if (!concat_path(((PTFS *)fuse_get_context()->private_data), n, full ## n))\
        return -ENAMETOOLONG;           \
    n = full ## n

typedef struct
{
    const char *rootdir;
} PTFS;

static int ptfs_getattr(const char *path, struct fuse_stat *stbuf, struct fuse_file_info *fi)
{
	ptfs_impl_fullpath(path);
	return -1 != lstat(path, stbuf) ? 0 : -errno;
}

static int ptfs_mkdir(const char *path, fuse_mode_t mode)
{
	debug("mkdir %s\n", path);

    ptfs_impl_fullpath(path);
    return -1 != mkdir(path, mode) ? 0 : -errno;
}

static int ptfs_unlink(const char *path)
{
    ptfs_impl_fullpath(path);

    return -1 != unlink(path) ? 0 : -errno;
}

static int ptfs_rmdir(const char *path)
{
    ptfs_impl_fullpath(path);

    return -1 != rmdir(path) ? 0 : -errno;
}

static int ptfs_rename(const char *oldpath, const char *newpath, unsigned int flags)
{
    ptfs_impl_fullpath(newpath);
    ptfs_impl_fullpath(oldpath);

    return -1 != rename(oldpath, newpath) ? 0 : -errno;
}

static int ptfs_chmod(const char *path, fuse_mode_t mode, struct fuse_file_info *fi)
{
    ptfs_impl_fullpath(path);

    return -1 != chmod(path, mode) ? 0 : -errno;
}

static int ptfs_chown(const char *path, fuse_uid_t uid, fuse_gid_t gid, struct fuse_file_info *fi)
{
    ptfs_impl_fullpath(path);

    return -1 != lchown(path, uid, gid) ? 0 : -errno;
}

static int ptfs_truncate(const char *path, fuse_off_t size, struct fuse_file_info *fi)
{
    if (0 == fi)
    {
        ptfs_impl_fullpath(path);

        return -1 != truncate(path, size) ? 0 : -errno;
    }
    else
    {
        int fd = fi_fd(fi);

        return -1 != ftruncate(fd, size) ? 0 : -errno;
    }
}

static int ptfs_open(const char *path, struct fuse_file_info *fi)
{
    ptfs_impl_fullpath(path);
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

static int ptfs_read(const char *path, char *buf, size_t size, fuse_off_t off,
    struct fuse_file_info *fi)
{
    int fd = fi_fd(fi);

    int nb;
    return -1 != (nb = pread(fd, buf, size, off)) ? nb : -errno;
}

static int ptfs_write(const char *path, const char *buf, size_t size, fuse_off_t off,
    struct fuse_file_info *fi)
{
    int fd = fi_fd(fi);

    int nb;
    return -1 != (nb = pwrite(fd, buf, size, off)) ? nb : -errno;
}

static int ptfs_statfs(const char *path, struct fuse_statvfs *stbuf)
{
	ptfs_impl_fullpath(path);
	return -1 != statvfs(path, stbuf) ? 0 : -errno;
}

static int ptfs_release(const char *path, struct fuse_file_info *fi)
{
    int fd = fi_fd(fi);

    close(fd);
    return 0;
}

static int ptfs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    int fd = fi_fd(fi);

    return -1 != fsync(fd) ? 0 : -errno;
}

static int ptfs_opendir(const char *path, struct fuse_file_info *fi)
{
    ptfs_impl_fullpath(path);

    DIR *dirp;
    return 0 != (dirp = opendir(path)) ? (fi_setdirp(fi, dirp), 0) : -errno;
}

static int ptfs_readdir(const char *path, void *buf, 
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

static int ptfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    DIR *dirp = fi_dirp(fi);

    return -1 != closedir(dirp) ? 0 : -errno;
}

static void *ptfs_init(struct fuse_conn_info *conn, struct fuse_config *conf)
{
    conn->want |= (conn->capable & FUSE_CAP_READDIRPLUS);

#if defined(FSP_FUSE_CAP_CASE_INSENSITIVE)
    conn->want |= (conn->capable & FSP_FUSE_CAP_CASE_INSENSITIVE);
#endif

    return fuse_get_context()->private_data;
}

static int ptfs_create(const char *path, fuse_mode_t mode, struct fuse_file_info *fi)
{
    ptfs_impl_fullpath(path);

    int fd;
    return -1 != (fd = open(path, fi->flags, mode)) ? (fi_setfd(fi, fd), 0) : -errno;
}

static int ptfs_utimens(const char *path, const struct fuse_timespec tv[2], struct fuse_file_info *fi)
{
    ptfs_impl_fullpath(path);

    return -1 != utimensat(AT_FDCWD, path, tv, AT_SYMLINK_NOFOLLOW) ? 0 : -errno;
}

static struct fuse_operations ptfs_ops =
{
    .getattr = ptfs_getattr,
    .mkdir = ptfs_mkdir,
    .unlink = ptfs_unlink,
    .rmdir = ptfs_rmdir,
    .rename = ptfs_rename,
    .chmod = ptfs_chmod,
    .chown = ptfs_chown,
    .truncate = ptfs_truncate,
    .open = ptfs_open,
    .read = ptfs_read,
    .write = ptfs_write,
    .statfs = ptfs_statfs,
    .release = ptfs_release,
    .fsync = ptfs_fsync,
    .opendir = ptfs_opendir,
    .readdir = ptfs_readdir,
    .releasedir = ptfs_releasedir,
    .init = ptfs_init,
    .create = ptfs_create,
    .utimens = ptfs_utimens,
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

#if defined(_WIN64) || defined(_WIN32)
    /*
     * When building for Windows (rather than Cygwin or POSIX OS)
     * allow the path to be specified using the --VolumePrefix
     * switch using the syntax \\passthrough-fuse\C$\Path. This
     * allows us to run the file system under the WinFsp.Launcher
     * and start it using commands like:
     *
     *     net use z: \\passthrough-fuse\C$\Path
     */
    if (0 == ptfs.rootdir)
        for (int argi = 1; argc > argi; argi++)
        {
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
#endif

    if (0 == ptfs.rootdir)
        usage();


	printf("mian thread: %d\n", GetCurrentThreadId());

	// Initialize the critical section one time only.
	//if (!InitializeCriticalSectionAndSpinCount(&CriticalSection,
	//	0x00000400))
	//	return;

	//gMutex = CreateMutex(
	//	NULL,              // default security attributes
	//	FALSE,             // initially not owned
	//	NULL);             // unnamed mutex
	//if (gMutex == NULL)
	//{
	//	printf("CreateMutex error: %d\n", GetLastError());
	//	return 1;
	//}


	// initialize Python
	wchar_t *program = Py_DecodeLocale(argv[0], NULL);
	if (program == NULL) {
		fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
		exit(1);
	}
	Py_SetProgramName(program);  /* optional but recommended */
	Py_Initialize();
	if (!Py_IsInitialized()) {
		debug("Unable to initialize Python interpreter.");
		return 1;
	}

	
	// save a pointer to the main PyThreadState object
	//mainThreadState = PyThreadState_Get();

	//PyEval_InitThreads();
	//PyThreadState* main_thread = PyEval_SaveThread();
	
	//PyRun_SimpleString("import sftp as s; sftp = s.SFTP();print('connected:', sftp.connected)\n");
	init_sftp();

	//mainModule = PyImport_AddModule("__main__");

	//PyObject *var = PyObject_GetAttrString(mainModule, "sftp");
	//PyObject *result = pyCall(var, "hello", NULL, NULL);
	//;
	//printf("hello: %s\n", PyUnicode_AsUTF8(result));

	// run fuse main
    int rc = fuse_main(argc, argv, &ptfs_ops, &ptfs);

	//PyEval_RestoreThread(main_thread);

	if (Py_FinalizeEx() < 0) {
		exit(120);
	}
	PyMem_RawFree(program);

	// Release resources used by the critical section object.
	//DeleteCriticalSection(&CriticalSection);
	//CloseHandle(gMutex);
	return rc;
}
