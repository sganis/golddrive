:: SSHFS
:: Set development environment
:: 08/05/2018, sganis

@echo off
set DIR=%~dp0
set DIR=%DIR:~0,-1%
doskey ll=dir
set PATH=C:\Python37;C:\Python37\Scripts;%PATH%
set PATH=%DIR%\..\sshfs\bin;%PATH%
echo on
set GOLDDRIVE_USER=support
set GOLDDRIVE_PASS=support
set GOLDDRIVE_HOST=localhost
set GOLDDRIVE_PORT=2222

