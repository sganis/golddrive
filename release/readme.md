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

- After successfull connection, show link to open in explorer
- Check if drive is in use
- Check if drive is operational (connected)
- Login first time with host and password form
- Check if drive is disconnected
- Check if drive is in error state (invalid)
- Connect using key instead of asking password if ssh key is setup already 
- Explore fsptool-x64.exe to query drives and user ids
- Create installer
- Add about info
- Show link to open explorer again after restarting