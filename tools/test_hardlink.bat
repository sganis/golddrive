:: Golddrive
:: 09/08/2018, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

ssh support@localhost "cd /tmp && rm -f hlink.* && echo hello > hlink.txt; ln hlink.txt hlink.2.txt; ln -s hlink.txt slink.txt; ls -li /tmp"
if !ERRORLEVEL! neq 0 goto fail

echo done
exit /b 0

:fail
echo failed
exit /b 1
