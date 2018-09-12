# golddrive 0.1

Date: 08/15/2018

Map a network drive to a remote file systems using SSH.
Source code available at http://github.com/sganis/golddrive.

# Dependencies

- WinFsp: https://github.com/billziss-gh/winfsp/releases/download/v1.3/winfsp-1.3.18160.msi
- Microsoft C++ 2015 Runtime:
  https://www.microsoft.com/en-us/download/details.aspx?id=48145

# Project folders

```
- app: 				main app
- lib: 				python embedded
- sshfs: 			sshfs and ssh
- config.json: 		configuration file
- golddrive.exe: 	application executable
- readme.md: 		this file
```

# Known issues

- Error: Cannot create WinFsp-FUSE file system: unspecified error.
  It is missing this Windows Update: Security Update for Windows 7 for x64-based Systems (KB3033929), available at https://technet.microsoft.com/en-us/library/security/3033929.aspx

# TODO

- Login first time with host and password form
- Connect using key instead of asking password if ssh key is setup already 
- Create installer
- Check for WinFSP and prompt to install
- About software versions: ssh, sshfs, winfsp, and golddrive
