:: run this from admin terminal

::"C:\Program Files (x86)\WinFsp\bin\fsreg.bat" ^
::	golddrive ^
::	C:\Users\sant\Documents\golddrive\golddrive\bin\golddrive.exe ^
::	"%%2 %%1" "D:P(A;;RPWPLC;;;WD)"

@echo off
setlocal
setlocal EnableDelayedExpansion

set RegKey=HKLM\Software\WOW6432Node\WinFsp\Services\golddrive
set exe=C:\Users\san\Desktop\golddrive\src\build\Debug\golddrive.exe
set arg="%%2 %%1"
set sec="D:P(A;;RPWPLC;;;WD)"

reg add !RegKey! /v Executable /t REG_SZ /d !exe! /f /reg:32
reg add !RegKey! /v CommandLine /t REG_SZ /d !arg! /f /reg:32
reg add !RegKey! /v JobControl /t REG_DWORD /d 1 /f /reg:32
reg add !RegKey! /v Security /t REG_SZ /d !sec! /f /reg:32
reg add !RegKey! /v RunAs /t REG_SZ /d "." /f /reg:32

