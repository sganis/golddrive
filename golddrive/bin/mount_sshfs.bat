:: SSHFS
:: Mount linux filesystem in windows drive
:: Usage: sshfs.bat [host] [drive]
:: 10/30/2018, sganis
::
:: golddrive keys must be setup

@echo off
setlocal
 
if [%3]==[] goto :usage
if "%1"=="-h" goto :usage

set USER=%1
set HOST=%2
set DRIVE=%3
 
:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

set PORT=2222
set USERHOST=%USER%@%HOST%
set PATH=%DIR%\sshfs\bin;%PATH%

sshfs.exe %USERHOST%:/ %DRIVE%  ^
   -o port=%PORT% ^
   -o IdentityFile=%USERPROFILE:\=/%/.ssh/id_rsa-%USER%-golddrive ^
   -o VolumePrefix=/sshfs/%USERHOST% ^
   -o uid=-1,gid=-1,create_umask=007,mask=007 ^
   -o PasswordAuthentication=no ^
   -o StrictHostKeyChecking=no ^
   -o UserKnownHostsFile=/dev/null ^
   -o compression=no ^
   -o rellinks ^
   -o reconnect ^
   -f -o ssh_command='ssh -vvv'

goto :eof
 
:usage
echo usage: %0 [user] [host] [drive]
exit /B 1
