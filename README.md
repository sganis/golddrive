# Gold Drive 1.0.0

Date: 08/15/2018

Map a network drive to a remote file systems using SSH.
Source code available at http://github.com/sganis/golddrive.

# Dependencies

- WinFsp: https://github.com/billziss-gh/winfsp/releases/download/v1.3/winfsp-1.3.18160.msi
- Microsoft C++ 2015 Runtime:
  https://www.microsoft.com/en-us/download/details.aspx?id=48145


# Known issues

- Error: Cannot create WinFsp-FUSE file system: unspecified error.
  It is missing this Windows Update: Security Update for Windows 7 for x64-based Systems (KB3033929), available at https://technet.microsoft.com/en-us/library/security/3033929.aspx

# Benchmarks

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

