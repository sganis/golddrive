:: SSHFS
:: Set development environment
:: 08/05/2018, sganis

@echo off
set DIR=%~dp0
set DIR=%DIR:~0,-1%
doskey ll=dir
set PATH=%DIR%\..\sshfs\bin;%PATH%
echo on
set GOLDDRIVE_USER=support
set GOLDDRIVE_PASS=support
set GOLDDRIVE_HOST=192.168.100.201
set GOLDDRIVE_PORT=2222

