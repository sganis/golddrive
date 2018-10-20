[![Build status](https://ci.appveyor.com/api/projects/status/x6cc6xew8amyv3s6?svg=true)](https://ci.appveyor.com/project/sganis/golddrive)

# Gold Drive

Map a network drive to a remote file systems using SSH.

## Dependencies

- WinFsp: https://github.com/billziss-gh/winfsp/releases

- In case of missing dll like: api-ms-win-crt-runtime-l1-1-0.dll, 
  update the Universal C Runtime in Windows (KB2999226): 
  https://support.microsoft.com/en-us/help/2999226/update-for-universal-c-runtime-in-windows

- In case of error: Cannot create WinFsp-FUSE file system: unspecified error.
  It is missing this Windows Update: Availability of SHA-2 Code Signing Support (KB3033929):
  https://technet.microsoft.com/en-us/library/security/3033929.aspx

- Microsoft C++ 2015 Runtime is not needed:
  https://www.microsoft.com/en-us/download/details.aspx?id=48145

- Development: Visual Studio 2015, ISO available at:
  https://go.microsoft.com/fwlink/?LinkId=615434&clcid=0x409


# Development Notes


## Alternatives and Benchmarks

  - ExpandDrive
    * Commercial, 10x slower, 3 MB/s.
    * Electron app, 50 USD.
    * Drive mounted as exfs with 10 TB free
    * stable, 10x slower, no cmd line, cloud options.
    * Apparently deletes files without permissions.

  - Montain Duck
    * Commercial, unstable, 2x slower
    * Mono/.Net app
    * Drive mounted as ntfs without permissions
    * Cannot mount root folder
    * Error messages on intensive i/o, need to reconnect manually

  - SFTPNetDrive:
  	* Commercial, unstable, slow, free for personal use. 
    * C++ app.
    * Drive is disconnected on intensive i/o.
    * Nsoftware, ipworks ssh .net company.

  - NetDrive
  	* Commercial
  	* Qt app
    * Requires cloud account

## Ciphers

- check server cipher support: `nmap --script ssh2-enum-algos -sV -p <port> <host>`
- client: `ssh -Q cipher`

## Build libssh2 with visual studio
- Requirements:
  * Cmake for windows
  * Visual studio and SDK 8.1
- git clone https://github.com/libssh2/libssh2.git
- cd libssh2
- mkdir build
- cd build
- cmake .. -G"Visual Studio 14 2015 Win64"
- cmake --build . --config Release (or open project with visual studio)

## Build libssh2 with openssl
- Download openssl binaries: http://p-nand-q.com/programming/windows/openssl-1.1.0c-64bit-release-dll-vs2015.7z
```
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
set Win64OPENSSL_ROOT_DIR=c:\openssl
set Win64OPENSSL_INCLUDE_DIR=%Win64OPENSSL_ROOT_DIR%\include
set Win64OPENSSL_LIBRARIES=%Win64OPENSSL_ROOT_DIR%\lib
cmake .. -G"Visual Studio 14 2015 Win64" -D"BUILD_SHARED_LIBS=1" -D"CMAKE_BUILD_TYPE=Release" -D"CRYPTO_BACKEND=OpenSSL" -D"OPENSSL_USE_STATIC_LIBS=TRUE" -D"OPENSSL_MSVC_STATIC_RT=TRUE"
```

## WinFsp command line options
```
usage: sanfs mountpoint [options]

    -o opt,[opt...]        mount options
    -h   --help            print help
    -V   --version         print version

FUSE options:
    -d   -o debug          enable debug output (implies -f)
    -f                     foreground operation
    -s                     disable multi-threaded operation

WinFsp-FUSE options:
    -o umask=MASK              set file permissions (octal)
    -o create_umask=MASK       set newly created file permissions (octal)
    -o uid=N                   set file owner (-1 for mounting user id)
    -o gid=N                   set file group (-1 for mounting user group)
    -o rellinks                interpret absolute symlinks as volume relative
    -o volname=NAME            set volume label
    -o VolumePrefix=UNC        set UNC prefix (/Server/Share)
        --VolumePrefix=UNC     set UNC prefix (\Server\Share)
    -o FileSystemName=NAME     set file system name
    -o DebugLog=FILE           debug log file (requires -d)

WinFsp-FUSE advanced options:
    -o FileInfoTimeout=N       metadata timeout (millis, -1 for data caching)
    -o DirInfoTimeout=N        directory info timeout (millis)
    -o VolumeInfoTimeout=N     volume info timeout (millis)
    -o KeepFileCache           do not discard cache when files are closed
    -o ThreadCount             number of file system dispatcher threads
```

## libssh2 sftp API
```
libssh2_sftp_init         - start an SFTP session
libssh2_sftp_shutdown     - shut down an SFTP session
libssh2_sftp_open_ex      - open and possibly create a file on a remote host
libssh2_sftp_read         - read from an SFTP file handle
libssh2_sftp_readdir      - read an entry from an SFTP directory
libssh2_sftp_write        - write data to an SFTP file handle
libssh2_sftp_fstat_ex     - get or set attributes on a file handle
libssh2_sftp_seek         - set the read/write position indicator to a position within a file
libssh2_sftp_tell         - get the current read/write position indicator for a file
libssh2_sftp_close_handle - close filehandle
libssh2_sftp_unlink_ex    - delete a file
libssh2_sftp_rename_ex    - rename a file
libssh2_sftp_mkdir_ex     - create a directory
libssh2_sftp_rmdir_ex     - remove a directory
libssh2_sftp_stat_ex      - get or set attributes on a file or symbolic link
libssh2_sftp_symlink_ex   - read or set a symbolic link
libssh2_sftp_realpath     - resolve a filename's path
libssh2_sftp_last_error   - return the last SFTP-specific error code
```

## Sanfs SFTP implementation
```
- san_init()
- san_shutdown()
- san_statvfs()
- san_stat()
- san_fstat()
- san_opendir()
- san_readdir()
- san_rewind()
- san_closedir()
- san_open()
- san_close()
- san_read()
- san_realpath()
- san_unlink()
- san_rename()
- san_mkdir()
- san_rmdir()
- san_fsync()
- san_write()
```