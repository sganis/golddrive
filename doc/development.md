# Development notes

## Dependencies

- WinFsp: https://github.com/billziss-gh/winfsp/releases

- In case of missing dll like: api-ms-win-crt-runtime-l1-1-0.dll, 
  update the Universal C Runtime in Windows (KB2999226): 
  https://support.microsoft.com/en-us/help/2999226/update-for-universal-c-runtime-in-windows

- In case of error: Cannot create WinFsp-FUSE file system: unspecified error.
  It is missing this Windows Update: Availability of SHA-2 Code Signing Support (KB3033929):
  https://technet.microsoft.com/en-us/library/security/3033929.aspx

## Cygwin to build sshfs-win

Steps here: https://github.com/billziss-gh/sshfs-win/issues/41

## Visual Studio

There are scripts in the tools directory to build dependencies:

- Build OpenSSL
- Build libssh
- Build libssh2
- Build OpenSSH

OpenSSH portable is available for windows using LibreSSL. Unfortunatelly, it is much slower in Golddrive.
I replaced LibreSSL with the following steps. Only works with OpenSSL 1.0.x, not 1.1.x for far.

## Build OpenSSH with Visual Studio 2019

1. Clone openssh-portable from microsoft repository powershell: https://github.com/PowerShell/openssh-portable.git
2. Remove config project
3. Retarget solution to VS 2019
4. Add these lines in the define section of posix_compat/inc/unistd.h:
	```
	#pragma warning(disable: 4005 4030)
	#define _CRT_INTERNAL_NONSTDC_NAMES 0
	```
5. Change contrib/win32/openssh/paths.targets, replacing these xml elements by openssl and zlib path and the openssl lib name:
    ```
    <LibreSSL-Path>c:\openssl-x64\</LibreSSL-Path>
    <LibreSSL-x64-Path>c:\openssl-x64\lib\</LibreSSL-x64-Path>
    <ZLib-Path>c:\zlib-x64\include\</ZLib-Path>
    <ZLib-x64-Path>c:\zlib-x64\lib\</ZLib-x64-Path>
    <SSLLib>libeay32.lib;</SSLLib>
    ```

## Alternatives and Benchmarks

  - NetDrive 3.8
    * Commercial
    * Performance not tested
    * Qt app
    * Requires cloud account, not clear if sftp is supported without account.

  - WebDrive 19
    * Commercial
    * Good speed, popup of upload/download progress while windows progress
    * C++ app
    * Unstable command line on intensive i/o
    * No public key authentication
    
  - SFTPNetDrive v2:
    * Commercial, free for personal use
    * 25 MB/s, much better than previous v1
    * .Net app using cbfsconnet from callback-tech., nsofware spin-off
    * Nsoftware, ipworks ssh .net company.

  - ExpanDrive 7
    * Commercial, 50 USD
    * 25 MB/s
    * Electron app using cbfsconnet
    * Unstable command line on intensive i/o

  - Montain Duck 3.3.6
    * Commercial
    * 50 MB/s
    * Mono/.Net app using cbfsconnect
    * Unstable command line on intensive i/o
  
  - Raidrive 2020.2.12
    * Free and Commercial options
    * 50 MB/s
    * .Net app with all static
    * Unstable command line on intensive i/o
  
  - SSHFS-Win
    * Open Source, Windows build of sshfs from libfuse
    * 50 MB/s
    * Cygwin app using winfsp
    * Unstable on intensive i/o
    * Problem with antivirus when copying large files

## Ciphers

- check server cipher support: 
  ```
  nmap --script ssh2-enum-algos -sV -p <port> <host>
  ````
- client: 
  ```
  ssh -Q cipher
  ```


