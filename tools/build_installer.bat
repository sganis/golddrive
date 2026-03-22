@echo off
:: Build installer only (requires CLI and App to be built first)
:: Usage: tools\build_installer.bat [version]
:: Example: tools\build_installer.bat 2.6

setlocal

set VERSION=%~1
if "%VERSION%"=="" set VERSION=2.6

set ISCC="%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
set SCRIPT=%~dp0..\installer\setup.iss

if not exist %ISCC% (
    echo ERROR: Inno Setup 6 not found at %ISCC%
    exit /b 1
)

echo Building installer: Golddrive %VERSION% x64
%ISCC% /DMyAppVersion=%VERSION% /DMyPlatform=x64 /DMyConfiguration=Release %SCRIPT%

if %ERRORLEVEL% neq 0 (
    echo INSTALLER BUILD FAILED
    exit /b 1
)

echo INSTALLER BUILD SUCCESS
