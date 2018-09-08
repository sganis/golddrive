# SSH Drive 0.1

Date: 15 Aug 2018
Contact: sganis (http://github.com/sganis)

Map a network drive to a remote file systems using SSH.

# Dependencies

- WinFsp
- Sshfs-win
- Microsoft C++ 2015 Runtime:
  https://www.microsoft.com/en-us/download/details.aspx?id=48145

# Run



# Configuration



# Known issues

- Error: Cannot create WinFsp-FUSE file system: unspecified error.
  It is missing this Windows Update: Security Update for Windows 7 for x64-based Systems (KB3033929), available at https://technet.microsoft.com/en-us/library/security/3033929.aspx

# TODO

- Login first time with host and password form
- Connect using key instead of asking password if ssh key is setup already 
- Create installer
