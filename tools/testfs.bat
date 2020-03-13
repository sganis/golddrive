:: Golddrive
:: 09/08/2018, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

:: %DIR%\winfsp-tests-x64.exe --external --resilient --case-insensitive-cmp ^
:: 	-create_allocation_test ^
:: 	-create_fileattr_test ^
:: 	-create_notraverse_test ^
:: 	-create_backup_test ^
:: 	-create_restore_test ^
:: 	-create_namelen_test ^
:: 	-getfileinfo_name_test ^
:: 	-setfileinfo_test ^
:: 	-delete_access_test ^
:: 	-delete_mmap_test ^
:: 	-rename_flipflop_test ^
:: 	-rename_mmap_test ^
:: 	-setsecurity_test ^
:: 	-querydir_namelen_test ^
:: 	-exec_rename_dir_test ^
:: 	-reparse* -stream*
:: if !ERRORLEVEL! neq 0 goto fail

echo > test
%DIR%\fsx.exe -N 5000 -L test xxxxxx
del test test.*
if !ERRORLEVEL! neq 0 goto fail

%DIR%\fsbench-x64.exe -rdwr_cc_* -mmap_* ^
	-file_attr* -file_list_single* -file_list_none* -rdwr_nc_*
if !ERRORLEVEL! neq 0 goto fail

echo PASSED
exit /b 0

:fail
echo FAILED
exit /b 1
