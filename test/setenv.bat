:: SSHFS
:: Set development environment
:: 08/05/2018, sganis

@echo off
set DIR=%~dp0
set DIR=%DIR:~0,-1%
doskey ll=dir

::set PATH=C:\Python37;C:\Python37\Scripts;%PATH%
::set GOLDDRIVE=%DIR%\..

:: needed for winfsp-tests tool
set PATH=C:\Program Files (x86)\WinFsp\bin;%PATH%

ECHO 1. localhost
ECHO 2. 192.168.100.201
ECHO.
CHOICE /C 12 /M "Enter ssh server for testing: "
:: Note - list ERRORLEVELS in decreasing order
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

