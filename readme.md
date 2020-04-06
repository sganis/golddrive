[![Build status](https://ci.appveyor.com/api/projects/status/x6cc6xew8amyv3s6?svg=true)](https://ci.appveyor.com/project/sganis/golddrive)

# Golddrive

Map Windows drives to remote filesystems using SSH.

Installation
------------

Install the latest release of WinFsp from https://github.com/billziss-gh/winfsp/releases. Then, download and install the latest released msi installer from https://github.com/sganis/golddrive/releases. The latest development version is available at https://ci.appveyor.com/project/sganis/golddrive/build/artifacts. 
The installation requires adminstatator priviledges.

How to use
----------

Run the app, choose a free drive and set the mount point as [user@]hostname[!port]. User and port are optional. The current windows user and port 22 are the defaults.
The first time after a successful login, the app will generate ssh keys to mount drives. Golddrive only supports ssh keys authentication at the moment.

Once ssh keys are generated, it is also possible to mount drives using Windows `net use` command or Windows Explorer Map Drive feature. The command line must be like this:

    > net use drive \\golddrive\[user@]hostname[!port]

If something goes wrong, test the ssh authentication with an ssh client. Go to Help -> Open Terminal, and run:

    > ssh [-p port] user@hostname

A successful login to the remote hostname without password should be the result.


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

  - NetDrive
    * Commercial
    * Performance not tested
    * Qt app
    * Requires cloud account

  - ExpandDrive
    * Commercial, 50 USD
    * 3 MB/s.
    * Electron app
    * Drive mounted as exfs with 10 TB free
    * Stable, no cmd line, cloud options.
    * Apparently deletes files without permissions.

  - SFTPNetDrive:
    * Commercial, free for personal use
    * 3 MB/s
    * C++ app.
    * Unstable, drive is disconnected on intensive i/o.
    * Nsoftware, ipworks ssh .net company.

  - Montain Duck
    * Commercial
    * 15 MB/s
    * Mono/.Net app
    * Drive mounted as ntfs without permissions
    * Cannot mount root folder
    * Unstable on intensive i/o, need to reconnect manually
  
  - SSHFS-Win
    * Open Source, Windows build of sshfs from libfuse
    * 50 MB/s
    * Cygwin app
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


