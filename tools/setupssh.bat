@echo off
:: Setup SSH RSA key for golddrive
:: Usage: tools\setupssh.bat [user] [host] [port]
:: Example: tools\setupssh.bat support 192.168.100.250 22

setlocal EnableDelayedExpansion

if [%3]==[] goto :usage
if "%1"=="-h" goto :usage

set USER=%1
set HOST=%2
set PORT=%3
set PASS=%4
if "%PASS%"=="" set PASS=support

set SKEY=%USERPROFILE%\.ssh\id_rsa
set PKEY=%USERPROFILE%\.ssh\id_rsa.pub

if not exist "%USERPROFILE%\.ssh" mkdir "%USERPROFILE%\.ssh"

:: generate RSA key if not exists
if not exist "%SKEY%" (
    echo Generating RSA key...
    ssh-keygen -t rsa -b 4096 -m PEM -q -N "" -f %SKEY%
)

:: read public key
set /p PUBKEY=<%PKEY%

:: transfer key using paramiko (handles password and keyboard-interactive)
python -c "import paramiko;c=paramiko.SSHClient();c.set_missing_host_key_policy(paramiko.AutoAddPolicy());c.connect('%HOST%',username='%USER%',password='%PASS%',port=%PORT%,timeout=10,allow_agent=False,look_for_keys=False);c.exec_command('umask 077;mkdir -p .ssh;echo \"%PUBKEY%\" >> .ssh/authorized_keys;chmod 700 .ssh;chmod 644 .ssh/authorized_keys');c.close();print('Key transferred')"
if %errorlevel% neq 0 goto fail

:: test key auth
ssh -i %SKEY% -p %PORT% ^
    -oPasswordAuthentication=no ^
    -oStrictHostKeyChecking=no ^
    -oUserKnownHostsFile=/dev/null ^
    -oBatchMode=yes ^
    %USER%@%HOST% echo OK
if %errorlevel% neq 0 goto fail
echo SSH setup complete
goto :eof

:fail
echo FAILED
goto :eof

:usage
echo usage: %0 [user] [host] [port]
exit /B 1
