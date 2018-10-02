@echo off
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set PATH=C:\Program Files (x86)\WinFsp\bin;%PATH%

%DIR%\winfsp-tests-x64.exe --external --resilient ^
	--case-insensitive-cmp ^
	-create_allocation_test ^
	-create_fileattr_test ^
	-create_notraverse_test ^
	-create_backup_test ^
	-create_restore_test ^
	-create_namelen_test ^
	-getfileinfo_name_test ^
	-setfileinfo_test ^
	-delete_access_test ^
	-delete_mmap_test ^
	-rename_flipflop_test ^
	-rename_mmap_test ^
	-setsecurity_test ^
	-querydir_namelen_test ^
	-exec_rename_dir_test ^
	-reparse* -stream*

::delete_mmap_test, rename_flipflop_test, rename_mmap_test, exec_rename_dir_test