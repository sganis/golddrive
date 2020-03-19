:: Golddrive
:: 09/08/2018, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

echo current dir:
cd

%DIR%\fstools\fsx.exe -N 1000 test xxxxxx
rem %DIR%\fstools\fsx.exe -N 1000 -L test xxxxxx
rem del test test.*
if !ERRORLEVEL! neq 0 goto fail

rem %DIR%\fstools\fsbench-x64.exe --files=100 -rdwr_cc_* -mmap_* ^
rem  -file_attr* -file_list_single* -file_list_none* -rdwr_nc_*
rem if !ERRORLEVEL! neq 0 goto fail

rem %DIR%\iozone\iozone.exe -i0 -i1 -i2 -s 1m -s10m -r1m
rem if !ERRORLEVEL! neq 0 goto fail

echo PASSED
exit /b 0

:fail
echo FAILED
exit /b 1
