:: install openssh in windows
:: Download version 7.7.p1
:: https://github.com/PowerShell/Win32-OpenSSH/releases/download/v7.7.2.0p1-Beta/OpenSSH-Win64.zip
::
:: installation instructions
:: https://github.com/PowerShell/Win32-OpenSSH/wiki/Install-Win32-OpenSSH


powershell.exe -Command "(new-object System.Net.WebClient).DownloadFile('https://github.com/PowerShell/Win32-OpenSSH/releases/download/v7.7.2.0p1-Beta/OpenSSH-Win64.zip','c:\temp\OpenSSH-Win64.zip')"

::set "OPENSSH=C:\Program Files\OpenSSH-Win64"
::powershell.exe -ExecutionPolicy Bypass -File "%OPENSSH%\install-sshd.ps1"
::netsh advfirewall firewall add rule name=sshd dir=in action=allow protocol=TCP localport=22
::net start sshd

