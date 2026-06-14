# Golddrive Help

Golddrive is an SSHFS network drive for Windows. It maps a drive letter to a
remote filesystem over SSH using WinFsp as the FUSE layer. No additional
software is needed on the server — any machine running an SSH server will work.

## Prerequisites

- **WinFsp** (the FUSE layer) is bundled with the installer and installed
  automatically. If you already have it, Golddrive uses your existing copy.
- The remote server must have an SSH server running (OpenSSH, etc.)
- You need a user account on the remote server

## Quick Start

1. Open the Golddrive app from the Start Menu
2. Select a free drive letter (G: through Z:)
3. Enter the hostname and your password
4. Click Connect

SSH keys are generated automatically on first connection. Subsequent
connections use key-based authentication and do not require a password.

---

## Method 1: Golddrive App (GUI)

The graphical application is the easiest way to manage drives.

### Connecting a Drive

1. Launch **Golddrive** from the Start Menu or `C:\Program Files\Golddrive`
2. Select a drive letter from the dropdown (letters G: through Z:)
3. Enter the SSH server hostname or IP address
4. Optionally set a custom port (default: 22), username, or remote path
5. Click **Connect**
6. On first connection, enter your SSH password to generate keys
7. The drive appears in Windows Explorer once connected

### Disconnecting a Drive

1. Select the connected drive from the dropdown
2. Click **Disconnect**

### Settings

- **Label**: custom display name for the drive in Explorer
- **User**: SSH username (defaults to your Windows username)
- **Port**: SSH port (defaults to 22)
- **SSH Key**: path to private key (defaults to `%USERPROFILE%\.ssh\id_rsa`)
- **Args**: extra CLI arguments passed to the filesystem driver

### Configuration File

Settings are stored in: `%LOCALAPPDATA%\golddrive\config.json`

---

## Method 2: Command Line (net use)

Mount drives from any Command Prompt or PowerShell using `net use`.

### UNC Path Format

    \\golddrive\[user@]hostname[!port][\path]

### Examples

Mount remote home directory of current user:

    net use Z: \\golddrive\myserver /persistent:yes

Mount with explicit username:

    net use Z: \\golddrive\john@myserver /persistent:yes

Mount with custom port:

    net use Z: \\golddrive\john@myserver!2222 /persistent:yes

Mount a specific remote directory:

    net use Z: \\golddrive\john@myserver!2222\projects /persistent:yes

Mount using IP address:

    net use Z: \\golddrive\admin@192.168.1.100 /persistent:yes

### Disconnecting

    net use Z: /delete

Disconnect all golddrive mounts:

    net use * /delete /yes

### Notes

- The `/persistent:yes` flag makes the drive reconnect on login
- SSH key authentication must be set up before using `net use`
  (connect once through the app, or manually place your key)
- The private key must be at `%USERPROFILE%\.ssh\id_rsa` (or configure
  a custom key path through the app)

---

## Method 3: Windows Explorer (Map Network Drive)

You can also mount drives through the Windows Explorer GUI.

### Steps

1. Open **Windows Explorer** (Win+E)
2. Right-click **This PC** in the left panel
3. Select **Map network drive...**
4. Choose a drive letter (G: through Z:)
5. In the **Folder** field, enter the UNC path:

       \\golddrive\user@hostname

6. Check **Reconnect at sign-in** if you want the drive to persist
7. Click **Finish**

### Path Examples

    \\golddrive\myserver
    \\golddrive\john@myserver
    \\golddrive\john@myserver!2222
    \\golddrive\john@myserver\projects

### Disconnecting

Right-click the mapped drive in Explorer and select **Disconnect**.

---

## CLI Reference

The `golddrive.exe` command-line tool is the filesystem driver. It is normally
invoked by WinFsp automatically, but can also be run manually for debugging.

### Usage

    golddrive drive [remote] [options]

### Options

    -h HOST             SSH server name or IP
    -u USER             SSH username (default: current Windows user)
    -k PKEY             private key path (default: %USERPROFILE%\.ssh\id_rsa)
    -p PORT             SSH port (default: 22)
    -o buffer=BYTES     read/write block size (default: 65535)
    -o create_umask=MASK  file creation permission mask
    -o keeplink         do not remove hard links before overwriting
    -o audit            log read and write events
    -o cipher=LIST      symmetric encryption cipher(s)
    -o FileInfoTimeout=N    metadata cache timeout in milliseconds
    -o DirInfoTimeout=N     directory cache timeout in milliseconds
    -o VolumeInfoTimeout=N  volume info cache timeout in milliseconds
    -o KeepFileCache    do not discard cache when files are closed
    -o ThreadCount=N    number of dispatcher threads
    -d                  enable debug output
    -s                  single-threaded mode
    --version           show version
    --help              show help

### Extra Arguments via the App

In the Golddrive app, use the **Args** field to pass options to the driver.
For example, to increase the buffer size and enable caching:

    -o buffer=131072 -o FileInfoTimeout=5000

---

## SSH Key Setup

Golddrive uses SSH key authentication. Keys are generated automatically
when you first connect through the app with a password.

### Manual Key Setup

If you prefer to set up keys manually:

1. Generate a key pair (if you don't have one):

       ssh-keygen -t rsa -b 4096 -f %USERPROFILE%\.ssh\id_rsa

2. Copy the public key to the server:

       ssh-copy-id user@hostname

   Or manually append the contents of `id_rsa.pub` to
   `~/.ssh/authorized_keys` on the server.

3. Test the connection:

       ssh -i %USERPROFILE%\.ssh\id_rsa user@hostname "echo ok"

### Custom Key Path

To use a non-default key, configure the **SSH Key** field in the app
settings for each drive.

---

## Troubleshooting

### Drive shows as disconnected or broken

- Check that WinFsp service is running: `sc query WinFsp.Launcher`
- Verify SSH connectivity: `ssh user@hostname`
- Try disconnecting and reconnecting the drive

### Permission denied

- Ensure your SSH key is in `~/.ssh/authorized_keys` on the server
- Check key file permissions on the server (`chmod 600`)
- Try connecting through the app with a password to regenerate keys

### Slow performance

- Increase buffer size: `-o buffer=131072` or larger
- Enable caching: `-o FileInfoTimeout=5000 -o DirInfoTimeout=5000`
- Check network latency to the server

### Drive letter not available

- Golddrive uses letters G: through Z:
- Ensure the letter is not used by another drive or network share
- Run `net use` to see all active network connections

### Logs

Application logs are written to: `%LOCALAPPDATA%\golddrive\logs\`
Configure logging in `NLog.config` in the installation directory.

---

## Links

- GitHub: https://github.com/sganis/golddrive
- WinFsp: https://github.com/winfsp/winfsp
