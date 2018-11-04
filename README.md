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

## Sanfs API
```
- san_init()
- san_finalize()
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
- san_mkdir()
- san_rmdir()
- san_write()
- san_rename()
- san_fsync()
- san_utimens()
- san_truncate()
- san_ftruncate()
```

## Extending Windows Explorer

https://docs.microsoft.com/en-us/windows/desktop/shell/propsheet-handlers

## uid mapping

```
uid     sid                mapped uid

-------------------------------------
0       O:S-1-5-0          perm=0
1       O:S-1-5-1          perm=1
2       O:NU               perm=2
3       O:S-1-5-3          perm=3
4       O:IU               perm=4
5       O:S-1-5-5          perm=5
6       O:SU               perm=6
7       O:AN               perm=7
8       O:S-1-5-8          perm=8
9       O:ED               perm=9
10      O:PS               perm=10
11      O:AU               perm=11
12      O:RC               perm=12
13      O:S-1-5-13         perm=13
14      O:S-1-5-14         perm=14
15      O:S-1-5-15         perm=15
16      O:S-1-5-16         perm=16
17      O:S-1-5-17         perm=17
18      O:SY               perm=18
19      O:LS               perm=19
20      O:NS               perm=20
21      O:S-1-5-21         perm=21
22      O:S-1-5-22         perm=22
...
32      O:S-1-5-32         perm=32
33      O:WR               perm=33
34      O:S-1-5-34         perm=34
...
511     O:S-1-5-511        perm=511
512     O:S-1-5-32-512     perm=512
513     O:S-1-5-32-513     perm=513
...
999     O:S-1-5-32-999     perm=999
1000    O:S-1-5-1000       perm=1000
1001    O:S-1-0-65534      perm=65534
...
2047    O:S-1-0-65534      perm=65534
2048    O:S-1-0-65534      perm=65534
2049    O:S-1-0-65534      perm=65534
...
4095    O:S-1-0-65534      perm=65534
4096    O:S-1-5-1-0        perm=4096
4097    O:S-1-5-1-1        perm=4097
...
5095    O:S-1-5-1-999      perm=5095
5096    O:S-1-5-1-1000     perm=5096
5097    O:S-1-5-1-1001     perm=5097
...
8191    O:S-1-5-1-4095     perm=8191
8192    O:S-1-5-2-0        perm=8192
8193    O:S-1-5-2-1        perm=8193
...
12287   O:S-1-5-2-4096     perm=12287
12288   O:S-1-5-3-0        perm=12288
12289   O:S-1-5-3-1        perm=12289
...
16383   O:S-1-5-3-4095     perm=16383
16384   O:S-1-5-4-0        perm=16384
16385   O:S-1-5-4-1        perm=16385
...
20479   O:S-1-5-4-4095     perm=20479
20480   O:S-1-5-5-0        perm=65534
20481   O:S-1-5-5-1        perm=65534
...
24575   O:S-1-5-5-4095     perm=65534
24576   O:S-1-5-6-0        perm=24576
24577   O:S-1-5-6-1        perm=24577
```