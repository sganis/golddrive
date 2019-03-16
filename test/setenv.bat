:: SSHFS
:: Set development environment
:: 08/05/2018, sganis

@echo off
set DIR=%~dp0
set DIR=%DIR:~0,-1%
doskey ll=dir

set PATH=C:\Python37;C:\Python37\Scripts;%PATH%
set PATH=C:\Program Files (x86)\WinFsp\bin;%PATH%
::set GOLDDRIVE=%DIR%\..
ECHO Select remote test host:
ECHO 1. localhost
ECHO 2. 192.168.100.201
ECHO 3. 192.168.99.100
ECHO.
CHOICE /C 123 /M "Enter ssh server for testing: "
:: Note - list ERRORLEVELS in decreasing order
IF ERRORLEVEL 3 (
	SET HOST=192.168.99.100
	goto hostdone 
)
IF ERRORLEVEL 2 (
	SET HOST=192.168.100.201
	goto hostdone 
)
IF ERRORLEVEL 1 (
	SET HOST=localhost
	goto hostdone
)

:hostdone
echo on
set GOLDDRIVE_USER=support
set GOLDDRIVE_PASS=support
set GOLDDRIVE_HOST=%HOST%
set GOLDDRIVE_PORT=2222

