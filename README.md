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
