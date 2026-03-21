@echo off
:: Register golddrive with WinFsp (run from admin terminal)
:: Usage: tools\register.bat [configuration] [platform]
:: Example: tools\register.bat Release x64

setlocal EnableDelayedExpansion

set CONFIG=%~1
set PLAT=%~2
if "%CONFIG%"=="" set CONFIG=Release
if "%PLAT%"=="" set PLAT=x64

set RegKey=HKLM\Software\WOW6432Node\WinFsp\Services\golddrive
set exe=%~dp0..\build\%CONFIG%\%PLAT%\golddrive.exe
set arg="%%2 %%1"
set sec="D:P(A;;RPWPLC;;;WD)"

if not exist "!exe!" (
    echo ERROR: golddrive.exe not found at !exe!
    echo Build the CLI first: tools\build_cli.bat
    exit /b 1
)

echo Registering: !exe!
reg add !RegKey! /v Executable /t REG_SZ /d !exe! /f /reg:32
reg add !RegKey! /v CommandLine /t REG_SZ /d !arg! /f /reg:32
reg add !RegKey! /v JobControl /t REG_DWORD /d 1 /f /reg:32
reg add !RegKey! /v Security /t REG_SZ /d !sec! /f /reg:32
reg add !RegKey! /v RunAs /t REG_SZ /d "." /f /reg:32

echo Registration complete
