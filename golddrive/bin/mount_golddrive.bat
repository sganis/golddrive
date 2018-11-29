:: Golddrive
:: Mount linux filesystem in windows drive
:: Usage: sanfs.bat [host] [drive]
:: 10/30/2018, sganis
::
:: golddrive keys must be setup
 
@echo off
setlocal
 
if [%2]==[] goto :usage
if "%1"=="-h" goto :usage
 
set HOST=%1
set DRIVE=%2
:: echo Host : %HOST%
:: echo Drive: %DRIVE%
 
:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%
 
:: mount
%DIR%\sanfs.exe %HOST% %DRIVE% -k %USERPROFILE%\.ssh\id_rsa-%USERNAME%-golddrive
goto :eof
 
:usage
echo usage: %0 [host] [drive]
exit /B 1
