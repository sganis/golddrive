@echo off
:: Build installer only (requires CLI and App to be built first)
:: Usage: tools\build_installer.bat [version] [platform]
:: Example: tools\build_installer.bat 2.5 x64

setlocal

set VERSION=%~1
set PLATFORM=%~2

if "%VERSION%"=="" set VERSION=2.5
if "%PLATFORM%"=="" set PLATFORM=x64

set ISCC="%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
set SCRIPT=%~dp0..\installer\setup.iss

if not exist %ISCC% (
    echo ERROR: Inno Setup 6 not found at %ISCC%
    exit /b 1
)

echo Building installer: Golddrive %VERSION% %PLATFORM%
%ISCC% /DMyAppVersion=%VERSION% /DMyPlatform=%PLATFORM% /DMyConfiguration=Release %SCRIPT%

if %ERRORLEVEL% neq 0 (
    echo INSTALLER BUILD FAILED
    exit /b 1
)

echo INSTALLER BUILD SUCCESS
