:: SSHFS
:: Setup ssh keys for authentication
:: 07/31/2018, sganis
:: Dependencies: ssh, ssh-keygen,
 
::@echo off
 
:: check for parameters
if [%1]==[] goto usage
 
set HOST=%1
 
set HOME=%USERPROFILE%
set PATH=C:\Program Files\SSHFS-Win\bin;%PATH%
set RESULT=1
	
:: generate ssh keys if not present already
if not exist %HOME%\.ssh\id_rsa (
	echo Generating ssh key pair...
	mkdir %HOME%\.ssh 2>nul
	echo -n 'y/n' | ssh-keygen -f %HOME%\.ssh\id_rsa -q -N ""
) else (
	echo Private key already exists
	call :testssh RESULT
	if %RESULT% equ 0 (
		echo SSH is already working.
		goto :eof
	) 
)

set PUBKEY=""
echo Saving public key to %HOST%...
::for /f "tokens=*" %%i in (%USERPROFILE%\.ssh\id_rsa.pub) do set PUBKEY=%%i
more %USERPROFILE%\.ssh\id_rsa.pub ^
	| ssh %HOST% -o StrictHostKeyChecking=no "cat >> .ssh/authorized_keys && sed -i 's/\r//g' .ssh/authorized_keys && chmod 440 .ssh/authorized_keys"

set RESULT=1
call :testssh RESULT
if %RESULT% neq 0 (
	echo SSH setup failed.
	echo exit code: %RESULT%
) else (
	echo Done.
)
goto :eof



:testssh
echo Testing ssh in %HOST%...
ssh -q -o BatchMode=yes %HOST% exit
set %~1=%ERRORLEVEL%
exit /b 0


:usage
echo usage: %0 ^<ssh-server^>
exit /b 1