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

%DIR%\..\vendor\fstools\fsx.exe -N 5000 test xxxxxx
%DIR%\..\vendor\fstools\fsx.exe -N 5000 -L test xxxxxx
del test test.*
echo error: !ERRORLEVEL!
if !ERRORLEVEL! neq 0 goto fail

%DIR%\..\vendor\fstools\fsbench-x64.exe -rdwr_cc_* -mmap_* ^
 -file_attr* -file_list_single* -file_list_none* -rdwr_nc_*
if !ERRORLEVEL! neq 0 goto fail

rem %DIR%\..\vendor\iozone\iozone.exe -i0 -i1 -i2 -s 1m -s10m -s100m -r1m
%DIR%\..\vendor\iozone\iozone.exe -a -i0 -i1 -s100m -s1g -r2m -r4m -r8m
if !ERRORLEVEL! neq 0 goto fail


echo PASSED
exit /b 0

:fail
echo FAILED
exit /b 1
