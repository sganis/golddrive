[![Build status](https://ci.appveyor.com/api/projects/status/x6cc6xew8amyv3s6?svg=true)](https://ci.appveyor.com/project/sganis/golddrive)

# Golddrive

Map Windows network drives to remote filesystems using SSH.


Installation
------------

Install the latest release of WinFsp from https://github.com/billziss-gh/winfsp/releases.

Download the msi installer from https://github.com/sganis/golddrive/releases.

The installation requires adminstatator priviledges.


How to use
----------

Run the app Golddrive available after installation, choose a free drive and set a mount point as [user@]hostname[!port]. User and port are optional, with the default as the current windows user and port 22.
The first time after a successful login, which could be using password or ssh keys, the all will generate its own ssh keys to mount drives. Golddrive only supports ssh keys authentication at the moment.

If mounting drives using the app is working, then it is also possible to mount using windows `net use` command or Windows Explorer Map Drive feature. The command line must be like this:

    > net use drive \\golddrive\[user@]hostname[!port]

Again, ssh key authentication must be available. There will no be any prompt for password

It is useful to test the ssh authentication with and ssh client. Windows 10 has OpenSSH as a feature, and Golddrive includes the ssh client. To use it, go to Help -> Open Terminal, and run:

    > ssh [-p port] user@hostname

A successful login to the remote hostname without password should be the result.


Screenshots
-----------

TODO

Build
-----

See development.md document.





