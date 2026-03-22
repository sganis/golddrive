# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Golddrive is an SSHFS implementation for Windows that maps network drives to remote filesystems using SSH. It uses WinFsp (Windows File System Proxy) as the FUSE layer.

## Prerequisites

- **Visual Studio 2022** with C++ desktop workload (for MSVC compiler)
- **.NET Framework 4.8** (included with Windows 10; SDK-style projects using `net48` target)
- **WinFsp** installed from https://github.com/winfsp/winfsp/releases (provides FUSE headers and runtime)
- **Inno Setup 6** (optional, for building the installer) from https://jrsoftware.org/isinfo.php

## Building Locally

### 1. CLI (C native, requires MSVC)

Open a **Developer Command Prompt for VS 2022** (or run `vcvars64.bat` first), then:
```cmd
msbuild src\cli\cli.vcxproj /t:rebuild /p:Configuration=Release /p:Platform=x64 /p:SolutionDir=%CD%\src\ /v:minimal
```

Or use the helper script (sets up vcvars automatically):
```cmd
tools\build_cli.bat
```

### 2. WPF App (.NET Framework 4.8)

```cmd
dotnet build src\app\app.csproj -c Release -o build\Release\x64
```

### 3. Tests (.NET Framework 4.8)

Build:
```cmd
dotnet build src\test\test.csproj -c Release -o build\Release\x64
```

Run (requires SSH server, see Testing section):
```cmd
dotnet test src\test\test.csproj -c Release --no-build -v normal
```

### 4. Installer (Inno Setup 6)

After building CLI and App:
```cmd
tools\build_installer.bat 2.6
```

Or call ISCC directly:
```cmd
"%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" /DMyAppVersion=2.6 /DMyPlatform=x64 /DMyConfiguration=Release installer\setup.iss
```

### Full build (all components)

From a Developer Command Prompt:
```cmd
tools\build.bat
tools\build_installer.bat 2.6
```

### Register CLI for development (admin terminal, one-time)
```cmd
tools\register.bat Release x64
```

## Testing

Tests require environment variables for SSH connectivity:
- `GOLDDRIVE_HOST` - SSH server hostname
- `GOLDDRIVE_USER` - SSH username
- `GOLDDRIVE_PASS` - SSH password (for initial key setup tests)

Or use the test script (has defaults for local dev):
```cmd
tools\test.bat
```

## Build Output

All build artifacts go to `build/{Configuration}/{Platform}/`:
- `golddrive.exe` - CLI filesystem driver
- `golddrive-app.exe` - GUI application
- `golddrive-test.dll` - Test assembly
- `golddrive-{version}-{platform}-setup.exe` - Installer

## Architecture

### Components

- **cli** (`src/cli/`) - Native C FUSE filesystem implementation using libssh2. Produces `golddrive.exe` which handles all SFTP operations. Key files:
  - `gd.c` / `gd.h` - Core SFTP operations (stat, read, write, mkdir, etc.)
  - `main.c` - FUSE entry point and filesystem callbacks
  - `cache.c` / `cache.h` - Inode and stat caching

- **app** (`src/app/`) - WPF GUI application (.NET Framework 4.8) with Material Design UI for managing drive connections. Uses MVVM pattern:
  - `Service/MountService.cs` - Core mounting logic, SSH key management, drive status
  - `ViewModel/MainWindowViewModel.cs` - Main UI logic
  - `Common/Drive.cs` - Drive model with mount point parsing
  - `Controls/` - WPF UserControls (DriveControl, HostControl, PasswordControl, SettingsControl, AboutControl)

- **test** (`src/test/`) - MSTest unit tests for the app layer

- **installer** (`installer/`) - Inno Setup script producing a standalone installer EXE

### Dependencies

External libraries in `vendor/`:
- libssh2 1.11.1 - SSH2 protocol library
- OpenSSL 3.6.0 - Cryptographic library
- OpenSSH v10 - SSH client tools (ssh.exe, ssh-keygen.exe)
- WinFsp - Windows FUSE implementation

NuGet packages (app):
- MaterialDesignThemes - Material Design UI
- Newtonsoft.Json - JSON serialization
- NLog - Logging
- SSH.NET - SSH client for key setup and testing
- System.ServiceProcess - WinFsp service detection (framework assembly)

### Drive Mounting

Drives are mounted using Windows `net use` command with UNC path format:
```
net use Z: \\golddrive\[user@]hostname[!port] /persistent:yes
```

The CLI registers with WinFsp as a network provider service. Configuration is stored in `%LOCALAPPDATA%\golddrive\config.json`.

## CI

AppVeyor builds x64 Release on Windows (Visual Studio 2022 image). The CLI is built with `msbuild`, the .NET app and tests with `dotnet build`, and the installer with Inno Setup 6. The build runs the full test suite including filesystem stress tests (fsx, fsbench, iozone).
