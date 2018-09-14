# Gold Drive 1.0.0

Date: 08/15/2018

Map a network drive to a remote file systems using SSH.
Source code available at http://github.com/sganis/golddrive.

# Dependencies

- WinFsp: https://github.com/billziss-gh/winfsp/releases/download/v1.3/winfsp-1.3.18160.msi
- Microsoft C++ 2015 Runtime:
  https://www.microsoft.com/en-us/download/details.aspx?id=48145

# Project folders

```
- app:           main app
- lib:           python embedded
- sshfs:         sshfs and ssh
- config.json:   configuration file
- golddrive.exe: application executable
- readme.md:     this file
```

# Known issues

- Error: Cannot create WinFsp-FUSE file system: unspecified error.
  It is missing this Windows Update: Security Update for Windows 7 for x64-based Systems (KB3033929), available at https://technet.microsoft.com/en-us/library/security/3033929.aspx

# Benchmarks

  - SFTPNetDrive:
  	* Commercial, free for personal use. 
    * unstable, drive is disconnected on intensive i/o.
    * Nsoftware, ipworks ssh .net company.
    * C++ app.
  - ExpandDrive
  	* Drive mounted as exfs with 10 TB free
    * Commercial
    * stable, 10x slower, 3 MB/s, no cmd line, cloud options.
    * electron app, 50 USD.
    * Apparently deletes files without permissions.
    * overall: good.
  - NetDrive
  	* Commercial
  	* Qt app, requires cloud account
  - Montain Duck
    * Commercial
    * Drive mounted as ntfs without permissions
    * .Net app
    * Cannot mount root folder, 2x slower than sshfs-win.
    * Error messages on intensive i/o, need to reconnect manually.

  
# TODO

- Login first time with host and password form
- Connect using key instead of asking password if ssh key is setup already 
- Create installer
- Check for WinFSP and prompt to install
- About software versions: ssh, sshfs, winfsp, and golddrive
