:: Golddrive
:: Mount linux filesystem in windows drive
:: Usage: golddrive [host] [drive]
:: 10/30/2018, sganis
::
:: golddrive keys must be setup
 
@echo off
setlocal
 
if [%1]==[] goto :usage
if "%1"=="-h" goto :usage
 
set DRIVE=%1
:: echo Drive: %DRIVE%
set DIR=%~dp0
set DIR=%DIR:~0,-1%
 
:: mount
%DIR%\golddrive.exe %DRIVE% 
goto :eof
 
:usage
echo usage: %0 [drive]
exit /B 1
