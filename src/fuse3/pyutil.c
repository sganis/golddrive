#include "pyutil.h"
#include <winfsp/winfsp.h>
#include <fcntl.h>
#include <fuse.h>


struct _DIR
{
	size_t h, fh;
	struct dirent de;
	char path[];
};

static int maperror(int winerrno);

static inline void *error0(void)
{
	errno = maperror(GetLastError());
	return 0;
}

static inline int error(void)
{
	errno = maperror(GetLastError());
	return -1;
}

char *realpath(const char *path, char *resolved)
{
	char *result;

	if (0 == resolved)
	{
		result = malloc(PATH_MAX); /* sets errno */
		if (0 == result)
			return 0;
	}
	else
		result = resolved;

	int err = 0;
	DWORD len = GetFullPathNameA(path, PATH_MAX, result, 0);
	if (0 == len)
		err = GetLastError();
	else if (PATH_MAX < len)
		err = ERROR_INVALID_PARAMETER;

	if (0 == err)
	{
		HANDLE h = CreateFileA(result,
			FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			0,
			OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
		if (INVALID_HANDLE_VALUE != h)
			CloseHandle(h);
		else
			err = GetLastError();
	}

	if (0 != err)
	{
		if (result != resolved)
			free(result);

		errno = maperror(err);
		result = 0;
	}

	return result;
}

int statvfs(const char *path, struct fuse_statvfs *stbuf)
{
	memset(stbuf, 0, sizeof *stbuf);
	//stbuf->f_bsize = 4096;
	//stbuf->f_frsize = 4096;
	//stbuf->f_blocks = 108105942;
	//stbuf->f_bfree = 46763607;
	//stbuf->f_bavail = 41266379;
	//stbuf->f_fsid = 1716932112;
	//stbuf->f_namemax = 255;
	//return 0;

	PyThreadState *state;
	state = PyThreadState_New(gMainThreadState->interp);
	PyThreadState_Swap(state);
	PyObject *presult;
	presult = PyObject_CallMethod(sftp, "statfs", "(s)", path);
	int rc = -1;

	if (PyTuple_Check(presult)) {
		PyObject *item;
		memset(stbuf, 0, sizeof *stbuf);
		item = PyTuple_GetItem(presult, 0);
		stbuf->f_bsize = PyLong_AsSsize_t(item);
		item = PyTuple_GetItem(presult, 1);
		stbuf->f_frsize = PyLong_AsSsize_t(item);
		item = PyTuple_GetItem(presult, 2);
		stbuf->f_blocks = PyLong_AsSsize_t(item);
		item = PyTuple_GetItem(presult, 3);
		stbuf->f_bfree = PyLong_AsSsize_t(item);
		item = PyTuple_GetItem(presult, 4);
		stbuf->f_bavail = PyLong_AsSsize_t(item);
		item = PyTuple_GetItem(presult, 5);
		stbuf->f_fsid = PyLong_AsSsize_t(item);
		item = PyTuple_GetItem(presult, 6);
		stbuf->f_namemax = PyLong_AsSsize_t(item);
		Py_CLEAR(item);	
		rc = 0;
	}

	Py_CLEAR(presult);
	PyThreadState_Swap(gMainThreadState);
	PyThreadState_Clear(state);
	PyThreadState_Delete(state);
	//PyGILState_Release(gstate);
	return rc;
}

unsigned long hash(unsigned char *str)
{
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}
size_t popen(const char *path, int oflag, ...)
{
	// create hash of path to fake file handler number
	return hash(path);
}

int ftruncate(int fd, fuse_off_t size)
{
	HANDLE h = (HANDLE)(intptr_t)fd;
	FILE_END_OF_FILE_INFO EndOfFileInfo;

	EndOfFileInfo.EndOfFile.QuadPart = size;

	if (!SetFileInformationByHandle(h, FileEndOfFileInfo, &EndOfFileInfo, sizeof EndOfFileInfo))
		return error();

	return 0;
}

int pread(int fd, void *buf, size_t nbyte, fuse_off_t offset)
{
	//sptrintf(buf, )


	return 1;
}

int pwrite(int fd, const void *buf, size_t nbyte, fuse_off_t offset)
{
	// not supported
	return 0;
	/*HANDLE h = (HANDLE)(intptr_t)fd;
	OVERLAPPED Overlapped = { 0 };
	DWORD BytesTransferred;

	Overlapped.Offset = (DWORD)offset;
	Overlapped.OffsetHigh = (DWORD)(offset >> 32);

	if (!WriteFile(h, buf, (DWORD)nbyte, &BytesTransferred, &Overlapped))
	return error();*/

	//return BytesTransferred;
}

int fsync(int fd)
{
	HANDLE h = (HANDLE)(intptr_t)fd;

	if (!FlushFileBuffers(h))
		return error();

	return 0;
}

int close(int fd)
{
	return 0;
}

int lstat(const char *path, struct fuse_stat *stbuf)
{
	PyThreadState *state;
	state = PyThreadState_New(gMainThreadState->interp);
	PyThreadState_Swap(state);
	//PyGILState_STATE gstate = PyGILState_Ensure();
	
	PyObject *presult;
	presult = PyObject_CallMethod(sftp, "fstat", "(s)", path);
	int rc = 0;

	//presult = pyCall(sftp, "fstat", pargs, NULL, presult);

	if (PyTuple_Check(presult)) {
		PyObject *item;
		memset(stbuf, 0, sizeof *stbuf);
		item = PyTuple_GetItem(presult, 0);
		stbuf->st_uid = PyLong_AsSsize_t(item); 
		item = PyTuple_GetItem(presult, 1);
		stbuf->st_gid = PyLong_AsSsize_t(item); 
		item = PyTuple_GetItem(presult, 2);
		struct fuse_timespec atime;
		long at = PyLong_AsSsize_t(item); 
		atime.tv_sec = at;
		atime.tv_nsec = 0;
		stbuf->st_atim = atime; 
		item = PyTuple_GetItem(presult, 3);
		struct fuse_timespec mtime;
		long mt = PyLong_AsSsize_t(item); 
		mtime.tv_sec = mt;
		mtime.tv_nsec = 0;
		stbuf->st_mtim = mtime;			
		item = PyTuple_GetItem(presult, 4);
		stbuf->st_mode = PyLong_AsSsize_t(item);
		item = PyTuple_GetItem(presult, 5);
		stbuf->st_size = PyLong_AsSsize_t(item);
		//item = PyTuple_GetItem(presult, 6);
		//stbuf->st_flags = PyLong_AsSsize_t(item);
		stbuf->st_nlink = 1;
		
		Py_CLEAR(item);
	}
	else {
		rc = PyLong_AsSsize_t(presult);
	}
	Py_DECREF(presult);
	PyThreadState_Swap(gMainThreadState);
	PyThreadState_Clear(state);
	PyThreadState_Delete(state);
	//PyGILState_Release(gstate);
	return rc;


	//if (strncmp(path, "/", sizeof path) == 0)
	//{
	//	memset(stbuf, 0, sizeof *stbuf);
	//	stbuf->st_uid = 197609;
	//	stbuf->st_gid = 197121;
	//	stbuf->st_mode = 0777 | 040000;
	//	stbuf->st_nlink = 1;
	//	stbuf->st_size = 4096;
	//	struct fuse_timespec atime;
	//	atime.tv_sec = 1538576756;
	//	atime.tv_nsec = 0;
	//	stbuf->st_atim = atime;
	//	struct fuse_timespec mtime;
	//	mtime.tv_sec = 1538576756;
	//	mtime.tv_nsec = 0;
	//	stbuf->st_atim = mtime;
	//	return 0;
	//}

	//else if (strncmp(path, "/hello-world", sizeof path) == 0)
	//{
	//	memset(stbuf, 0, sizeof *stbuf);
	//	stbuf->st_uid = 197609;
	//	stbuf->st_gid = 197121;
	//	stbuf->st_mode = 0777 | 0100000;
	//	stbuf->st_nlink = 1;
	//	stbuf->st_size = 4096;
	//	struct fuse_timespec atime;
	//	atime.tv_sec = 1538576756;
	//	atime.tv_nsec = 0;
	//	stbuf->st_atim = atime;
	//	struct fuse_timespec mtime;
	//	mtime.tv_sec = 1538576756;
	//	mtime.tv_nsec = 0;
	//	stbuf->st_atim = mtime;
	//	return 0;
	//}
	//else if (strncmp(path, "/New Rich Text Document.rtf", sizeof path) == 0)
	//{
	//	memset(stbuf, 0, sizeof *stbuf);
	//	stbuf->st_uid = 197609;
	//	stbuf->st_gid = 197121;
	//	stbuf->st_mode = 0777 | 0100000;
	//	stbuf->st_nlink = 1;
	//	stbuf->st_size = 4096;
	//	struct fuse_timespec atime;
	//	atime.tv_sec = 1538576756;
	//	atime.tv_nsec = 0;
	//	stbuf->st_atim = atime;
	//	struct fuse_timespec mtime;
	//	mtime.tv_sec = 1538576756;
	//	mtime.tv_nsec = 0;
	//	stbuf->st_atim = mtime;
	//	return 0;
	//}
	//return -1;

	//S_ISTXT - undefined
	//S_IFWHT - undefined
	//S_IFSOCK - 140000
	//S_IFLNK - 120000
	//S_IFREG - 100000
	//S_IFBLK - 60000
	//S_IFDIR - 40000
	//S_IFCHR - 20000
	//S_IFIFO - 10000
	//S_ISUID - 4000
	//S_ISGID - 2000
	//S_ISVTX - 1000
	//S_IRWXU - 700
	//S_IRUSR - 400
	//S_IWUSR - 200
	//S_IXUSR - 100
	//S_IRWXG - 70
	//S_IRGRP - 40
	//S_IWGRP - 20
	//S_IXGRP - 10
	//S_IRWXO - 7
	//S_IROTH - 4
	//S_IWOTH - 2
	//S_IXOTH - 1
}

int chmod(const char *path, fuse_mode_t mode)
{
	/* we do not support file security */
	return 0;
}

int lchown(const char *path, fuse_uid_t uid, fuse_gid_t gid)
{
	/* we do not support file security */
	return 0;
}

int lchflags(const char *path, uint32_t flags)
{
#if defined(FSP_FUSE_USE_STAT_EX)
	UINT32 FileAttributes = MapFlagsToFileAttributes(flags);

	if (0 == FileAttributes)
		FileAttributes = FILE_ATTRIBUTE_NORMAL;

	if (!SetFileAttributesA(path, FileAttributes))
		return error();
#endif

	return 0;
}

int truncate(const char *path, fuse_off_t size)
{
	HANDLE h = CreateFileA(path,
		FILE_WRITE_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		0,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (INVALID_HANDLE_VALUE == h)
		return error();

	int res = ftruncate((int)(intptr_t)h, size);

	CloseHandle(h);

	return res;
}

int utime(const char *path, const struct fuse_utimbuf *timbuf)
{
	if (0 == timbuf)
		return utimensat(AT_FDCWD, path, 0, AT_SYMLINK_NOFOLLOW);
	else
	{
		struct fuse_timespec times[2];
		times[0].tv_sec = timbuf->actime;
		times[0].tv_nsec = 0;
		times[1].tv_sec = timbuf->modtime;
		times[1].tv_nsec = 0;
		return utimensat(AT_FDCWD, path, times, AT_SYMLINK_NOFOLLOW);
	}
}

int utimensat(int dirfd, const char *path, const struct fuse_timespec times[2], int flag)
{
	/* ignore dirfd and assume that it is always AT_FDCWD */
	/* ignore flag and assume that it is always AT_SYMLINK_NOFOLLOW */

	HANDLE h = CreateFileA(path,
		FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		0,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (INVALID_HANDLE_VALUE == h)
		return error();

	UINT64 LastAccessTime, LastWriteTime;
	if (0 == times)
	{
		FILETIME FileTime;
		GetSystemTimeAsFileTime(&FileTime);
		LastAccessTime = LastWriteTime = *(PUINT64)&FileTime;
	}
	else
	{
		FspPosixUnixTimeToFileTime((void *)&times[0], &LastAccessTime);
		FspPosixUnixTimeToFileTime((void *)&times[1], &LastWriteTime);
	}

	int res = SetFileTime(h,
		0, (PFILETIME)&LastAccessTime, (PFILETIME)&LastWriteTime) ? 0 : error();

	CloseHandle(h);

	return res;
}

int setcrtime(const char *path, const struct fuse_timespec *tv)
{
	HANDLE h = CreateFileA(path,
		FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		0,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (INVALID_HANDLE_VALUE == h)
		return error();

	UINT64 CreationTime;
	FspPosixUnixTimeToFileTime((void *)tv, &CreationTime);

	int res = SetFileTime(h,
		(PFILETIME)&CreationTime, 0, 0) ? 0 : error();

	CloseHandle(h);

	return res;
}

int unlink(const char *path)
{
	if (!DeleteFileA(path))
		return error();

	return 0;
}

int rename(const char *oldpath, const char *newpath)
{
	if (!MoveFileExA(oldpath, newpath, MOVEFILE_REPLACE_EXISTING))
		return error();

	return 0;
}

int mkdir(const char *path, fuse_mode_t mode)
{
	if (!CreateDirectoryA(path, 0/* default security */))
		return error();

	return 0;
}

int rmdir(const char *path)
{
	if (!RemoveDirectoryA(path))
		return error();

	return 0;
}

DIR *opendir(const char *path)
{
	size_t pathlen = strlen(path);
	if (0 < pathlen && '/' == path[pathlen - 1])
		pathlen--;

	DIR *dirp = malloc(sizeof *dirp + pathlen + 3); /* sets errno */
	if (0 == dirp)
	{
		return 0;
	}

	memset(dirp, 0, sizeof *dirp);
	dirp->h = hash(path);
	dirp->fh = INVALID_HANDLE_VALUE;
	memcpy(dirp->path, path, pathlen);
	dirp->path[pathlen + 0] = '/';
	dirp->path[pathlen + 1] = '*';
	dirp->path[pathlen + 2] = '\0';

	return dirp;
}

int dirfd(DIR *dirp)
{
	return (int)(intptr_t)dirp->h;
}

void rewinddir(DIR *dirp)
{
	dirp->fh = INVALID_HANDLE_VALUE;
}

struct dirent *readdir(DIR *dirp)
{
	if (INVALID_HANDLE_VALUE == dirp->fh)
	{
		dirp->fh = hash(dirp->path);
	}
	strcpy(dirp->de.d_name, "hello-world.txt");
	struct fuse_stat *stbuf = &dirp->de.d_stat;
	memset(stbuf, 0, sizeof *stbuf);
	stbuf->st_size = 10000;
	stbuf->st_mode = 0777 | 0100000; /* regular file */
	stbuf->st_nlink = 1;
	struct fuse_timespec atime;
	atime.tv_sec = 1538576756;
	atime.tv_nsec = 0;
	stbuf->st_atim = atime;
	struct fuse_timespec mtime;
	mtime.tv_sec = 1538576756;
	mtime.tv_nsec = 0;
	stbuf->st_mtim = mtime;

	return &dirp->de;
}

int closedir(DIR *dirp)
{
	return 0;
}

static int maperror(int winerrno)
{
	switch (winerrno)
	{
	case ERROR_INVALID_FUNCTION:
		return EINVAL;
	case ERROR_FILE_NOT_FOUND:
		return ENOENT;
	case ERROR_PATH_NOT_FOUND:
		return ENOENT;
	case ERROR_TOO_MANY_OPEN_FILES:
		return EMFILE;
	case ERROR_ACCESS_DENIED:
		return EACCES;
	case ERROR_INVALID_HANDLE:
		return EBADF;
	case ERROR_ARENA_TRASHED:
		return ENOMEM;
	case ERROR_NOT_ENOUGH_MEMORY:
		return ENOMEM;
	case ERROR_INVALID_BLOCK:
		return ENOMEM;
	case ERROR_BAD_ENVIRONMENT:
		return E2BIG;
	case ERROR_BAD_FORMAT:
		return ENOEXEC;
	case ERROR_INVALID_ACCESS:
		return EINVAL;
	case ERROR_INVALID_DATA:
		return EINVAL;
	case ERROR_INVALID_DRIVE:
		return ENOENT;
	case ERROR_CURRENT_DIRECTORY:
		return EACCES;
	case ERROR_NOT_SAME_DEVICE:
		return EXDEV;
	case ERROR_NO_MORE_FILES:
		return ENOENT;
	case ERROR_LOCK_VIOLATION:
		return EACCES;
	case ERROR_BAD_NETPATH:
		return ENOENT;
	case ERROR_NETWORK_ACCESS_DENIED:
		return EACCES;
	case ERROR_BAD_NET_NAME:
		return ENOENT;
	case ERROR_FILE_EXISTS:
		return EEXIST;
	case ERROR_CANNOT_MAKE:
		return EACCES;
	case ERROR_FAIL_I24:
		return EACCES;
	case ERROR_INVALID_PARAMETER:
		return EINVAL;
	case ERROR_NO_PROC_SLOTS:
		return EAGAIN;
	case ERROR_DRIVE_LOCKED:
		return EACCES;
	case ERROR_BROKEN_PIPE:
		return EPIPE;
	case ERROR_DISK_FULL:
		return ENOSPC;
	case ERROR_INVALID_TARGET_HANDLE:
		return EBADF;
	case ERROR_WAIT_NO_CHILDREN:
		return ECHILD;
	case ERROR_CHILD_NOT_COMPLETE:
		return ECHILD;
	case ERROR_DIRECT_ACCESS_HANDLE:
		return EBADF;
	case ERROR_NEGATIVE_SEEK:
		return EINVAL;
	case ERROR_SEEK_ON_DEVICE:
		return EACCES;
	case ERROR_DIR_NOT_EMPTY:
		return ENOTEMPTY;
	case ERROR_NOT_LOCKED:
		return EACCES;
	case ERROR_BAD_PATHNAME:
		return ENOENT;
	case ERROR_MAX_THRDS_REACHED:
		return EAGAIN;
	case ERROR_LOCK_FAILED:
		return EACCES;
	case ERROR_ALREADY_EXISTS:
		return EEXIST;
	case ERROR_FILENAME_EXCED_RANGE:
		return ENOENT;
	case ERROR_NESTING_NOT_ALLOWED:
		return EAGAIN;
	case ERROR_NOT_ENOUGH_QUOTA:
		return ENOMEM;
	default:
		if (ERROR_WRITE_PROTECT <= winerrno && winerrno <= ERROR_SHARING_BUFFER_EXCEEDED)
			return EACCES;
		else if (ERROR_INVALID_STARTING_CODESEG <= winerrno && winerrno <= ERROR_INFLOOP_IN_RELOC_CHAIN)
			return ENOEXEC;
		else
			return EINVAL;
	}
}

long WinFspLoad(void)
{
	return FspLoad(0);
}
