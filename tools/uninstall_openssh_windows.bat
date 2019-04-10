:: install openssh in windows
:: Download version 7.7.p1
:: https://github.com/PowerShell/Win32-OpenSSH/releases/download/v7.7.2.0p1-Beta/OpenSSH-Win64.zip
::
:: installation instructions
:: https://github.com/PowerShell/Win32-OpenSSH/wiki/Install-Win32-OpenSSH

set "OPENSSH=C:\Program Files\OpenSSH-Win64"
powershell.exe -ExecutionPolicy Bypass -File "%OPENSSH%\uninstall-sshd.ps1"