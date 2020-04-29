[![Build status](https://ci.appveyor.com/api/projects/status/x6cc6xew8amyv3s6?svg=true)](https://ci.appveyor.com/project/sganis/golddrive)

# Golddrive

SSHFS for Windows - Map drives to remote filesystems using SSH.

Installation
------------

Install WinFsp from https://github.com/billziss-gh/winfsp/releases, then install Golddrive from https://github.com/sganis/golddrive/releases. The installation requires admin priviledges.

How to use
----------

Run the Golddrive app available in the start menu, choose a free drive and connect to your remote ssh server. User and port are optional, they default to the current windows user and port 22. Hostname can be a machine name, domain name or IP address.
The first time after a successful login, the app will generate ssh keys to mount drives. Golddrive only supports key authentication at this moment.

Once ssh keys are generated, it is also possible to mount drives using Windows `net use` command or Windows Explorer Map Drive feature. The command line must be like this:

    > net use Z: \\golddrive\[user@]hostname[!port]

If something goes wrong, test the ssh authentication with an ssh client. Go to Help -> Open Terminal, and run:

    > ssh [-p port] user@hostname

A successful login to the remote hostname without password should be the result.


