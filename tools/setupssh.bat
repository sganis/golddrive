:: Golddrive
:: 04/01/2020, San
@echo off
setlocal
setlocal EnableDelayedExpansion

if [%3]==[] goto :usage
if "%1"=="-h" goto :usage

set USER=%1
set HOST=%2
set PORT=%3

set DIR=%~dp0
set DIR=%DIR:~0,-1%
set NOW=%DATE:/=%%TIME::=%
set NOW=%NOW: =%
set NOW=%NOW:~0,-3%
set USERHOST=%USER%@%HOST%
set SKEY=%USERPROFILE%\.ssh\id_golddrive_%USER%
set PKEY=%USERPROFILE%\.ssh\id_golddrive_%USER%.pub
if exist %SKEY% rename %SKEY% id_golddrive_%USER%.%NOW%.bak 
if exist %PKEY% rename %PKEY% id_golddrive_%USER%.pub.%NOW%.bak 

:: generate
ssh-keygen -q -N "" -f %SKEY%

:: transfer
type %PKEY% | ssh %USER%@%HOST% -p %PORT% "umask 077; mkdir -p .ssh; cat >> .ssh/authorized_keys; chmod 700 .ssh; chmod 644 .ssh/authorized_keys"
if %errorlevel% neq 0 goto fail

:: test
ssh -i %SKEY% -p %PORT% 			^
	-oPasswordAuthentication=no 	^
	-oStrictHostKeyChecking=no 		^
	-oUserKnownHostsFile=/dev/null 	^
	-oBatchMode=yes 				^
	%USER%@%HOST% echo OK
if %errorlevel% neq 0 goto fail
goto :eof

:fail
echo FAILED
goto :eof


:usage
echo usage: %0 [user] [host] [port]
exit /B 1
