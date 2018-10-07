#ifndef PYUTIL_H
#define PYUTIL_H

#include <Python.h>
#include <winfsp/winfsp.h>
#include <fcntl.h>
#include <fuse.h>

#define debug(...)  printf("%s: ", __func__); printf(__VA_ARGS__); printf("\n");

#define O_RDONLY                        _O_RDONLY
#define O_WRONLY                        _O_WRONLY
#define O_RDWR                          _O_RDWR
#define O_APPEND                        _O_APPEND
#define O_CREAT                         _O_CREAT
#define O_EXCL                          _O_EXCL
#define O_TRUNC                         _O_TRUNC
#define PATH_MAX                        1024
#define AT_FDCWD                        -2
#define AT_SYMLINK_NOFOLLOW             2

int pyCall(PyObject *instance, const char *method,
	PyObject* args, PyObject *kwargs, PyObject *presult);


struct dirent
{
	struct fuse_stat d_stat;
	char d_name[255];
};
void init_sftp();
typedef struct _DIR DIR;
char *realpath(const char *path, char *resolved);
int statvfs(const char *path, struct fuse_statvfs *stbuf);
size_t popen(const char *path, int oflag, ...);
int ftruncate(int fd, fuse_off_t size);
int pread(int fd, void *buf, size_t nbyte, fuse_off_t offset);
int pwrite(int fd, const void *buf, size_t nbyte, fuse_off_t offset);
int fsync(int fd);
int close(int fd);
int lstat(const char *path, struct fuse_stat *stbuf);
int chmod(const char *path, fuse_mode_t mode);
int lchown(const char *path, fuse_uid_t uid, fuse_gid_t gid);
int lchflags(const char *path, uint32_t flags);
int truncate(const char *path, fuse_off_t size);
int utime(const char *path, const struct fuse_utimbuf *timbuf);
int utimensat(int dirfd, const char *path, const struct fuse_timespec times[2], int flag);
int setcrtime(const char *path, const struct fuse_timespec *tv);
int unlink(const char *path);
int rename(const char *oldpath, const char *newpath);
int mkdir(const char *path, fuse_mode_t mode);
int rmdir(const char *path);
DIR *opendir(const char *path);
int dirfd(DIR *dirp);
void rewinddir(DIR *dirp);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
long WinFspLoad(void);
#undef fuse_main
#define fuse_main(argc, argv, ops, data)\
    (WinFspLoad(), fuse_main_real(argc, argv, ops, sizeof *(ops), data))


#endif // PYUTIL_H

