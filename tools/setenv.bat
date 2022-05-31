:: Golddrive
:: 08/05/2018, San
:: Set development environment

@echo off
set DIR=%~dp0
set DIR=%DIR:~0,-1%

SET "PATH=C:\Windows\System32;C:\Windows;C:\Windows\System32\wbem;%PATH%"
SET "PATH=C:\Program Files (x86)\WinFsp\bin;%PATH%"
SET "PATH=C:\Python37;C:\Python37\Scripts;%PATH%"
SET "PATH=C:\Python38;C:\Python38\Scripts;%PATH%"
SET "PATH=C:\Program Files (x86)\WinFsp\bin;%PATH%"
SET "PATH=C:\Program Files\Git\bin;%PATH%"
SET "PATH=C:\Program Files\Git\usr\bin;%PATH%"
SET "PATH=C:\Program Files (x86)\WiX Toolset v3.11\bin;%PATH%"
SET "PATH=C:\Strawberry\perl\bin;%PATH%"
SET "PATH=C:\Program Files\NASM;%PATH%"
SET "PATH=C:\ghc\bin;%PATH%"
SET "PATH=C:\Users\San\AppData\Roaming\cabal\bin;%PATH%"

echo Setting x64 development environment...
SET PLATFORM=x64
set CONFIGURATION=Release
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
set "USER=support"
set "PASS=support"
set /p user="Enter user [%USER%]: "
set /p pass="Enter pass [%PASS%]: "

echo on
set GOLDDRIVE_USER=%USER%
set GOLDDRIVE_PASS=%PASS%
set GOLDDRIVE_HOST=%HOST%
set GOLDDRIVE_PORT=22

doskey ll=dir
doskey gc=if $1. neq . (git commit -am "$*" $T git push) else (echo "git commit and push usage: gc <message>")
doskey gs=git status
