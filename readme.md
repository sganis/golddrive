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


Build
-----

See development.md document.





