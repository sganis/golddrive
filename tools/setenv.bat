:: Golddrive
:: Set development environment
:: 08/05/2018, sganis

@echo off
set DIR=%~dp0
set DIR=%DIR:~0,-1%
doskey ll=dir

SET "PATH=C:\Windows\System32;C:\Windows;C:\Windows\System32\wbem;%PATH%"
SET "PATH=C:\Program Files (x86)\WinFsp\bin;%PATH%"
SET "PATH=C:\Python37;C:\Python37\Scripts;%PATH%"
SET "PATH=C:\Program Files (x86)\WinFsp\bin;%PATH%"
SET "PATH=C:\Program Files\Git\bin;%PATH%"
SET "PATH=C:\Program Files\Git\usr\bin;%PATH%"
SET "PATH=C:\Program Files (x86)\WiX Toolset v3.11\bin;%PATH%"

echo Setting development environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"


::set GOLDDRIVE=%DIR%\..
ECHO Select remote test host:
ECHO 1. localhost
ECHO 2. 192.168.100.201
ECHO 3. 192.168.99.100
ECHO 4. mac
ECHO.
CHOICE /C 1234 /M "Enter ssh server for testing: "
:: Note - list ERRORLEVELS in decreasing order
IF ERRORLEVEL 4 (
	SET HOST=mac
	goto hostdone 
)
IF ERRORLEVEL 3 (
	SET HOST=192.168.99.101
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
set "user=sant"
set /p user="Enter user [%user%]: "
set /p pass="Enter pass: "

echo on
set GOLDDRIVE_USER=%user%
set GOLDDRIVE_PASS=%pass%
set GOLDDRIVE_HOST=%HOST%
set GOLDDRIVE_PORT=22

