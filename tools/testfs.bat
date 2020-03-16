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

%DIR%\fstools\fsx.exe -N 10000 test xxxxxx
%DIR%\fstools\fsx.exe -N 10000 -L test xxxxxx
del test test.*
if !ERRORLEVEL! neq 0 goto fail

%DIR%\fstools\fsbench-x64.exe -rdwr_cc_* -mmap_* ^
 -file_attr* -file_list_single* -file_list_none*
:: -rdwr_nc_*
if !ERRORLEVEL! neq 0 goto fail

%DIR%\iozone\iozone.exe -i0 -i1 -i2 -s 1m -s1g -r1m
if !ERRORLEVEL! neq 0 goto fail

echo PASSED
exit /b 0

:fail
echo FAILED
exit /b 1
