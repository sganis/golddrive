:: Golddrive
:: 04/04/2020, San
:: Build openssh x64
:: Usage: build_openssh.bat
@echo off
setlocal
set VERSION=10.0.0.0
set WORKDIR=C:\Temp
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%
set OPENSSH=openssh-portable-%VERSION%

:: Visual Studio 2022 configuration
set VCVARSALL="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

echo ========================================
echo Building OpenSSH %VERSION% for x64
echo ========================================

:: Download
cd %WORKDIR%

if exist "v%VERSION%.zip" (
    echo OpenSSH %VERSION% zip file already exists, skipping download...
) else (
    echo Downloading OpenSSH %VERSION%...
    curl -L -O https://github.com/PowerShell/openssh-portable/archive/refs/tags/v%VERSION%.zip
    if errorlevel 1 (
        echo Error: Failed to download OpenSSH
        cd %CWD%
        exit /b 1
    )
)

:: Extract
rd /s /q %OPENSSH% 2>nul
echo Extracting OpenSSH %VERSION%...
tar xf v%VERSION%.zip
if errorlevel 1 (
    echo Error: Failed to extract OpenSSH
    cd %CWD%
    exit /b 1
)

:: Apply patches
cd %OPENSSH%
echo Applying patches...
python %DIR%\patch_openssh.py
if errorlevel 1 (
    echo Error: Failed to apply patches
    cd %CWD%
    exit /b 1
)

:: Setup MSVC
call %VCVARSALL% x64
if errorlevel 1 (
    echo Error: Failed to initialize Visual Studio environment
    cd %CWD%
    exit /b 1
)

:: Build
echo Building config...
msbuild contrib\win32\openssh\config.vcxproj ^
    -p:Configuration=Release -p:Platform=x64 ^
    -m -v:minimal -t:rebuild /p:PlatformToolset=v143
if errorlevel 1 (
    echo Error: config build failed
    cd %CWD%
    exit /b 1
)

echo Building win32iocompat...
msbuild contrib\win32\openssh\win32iocompat.vcxproj ^
    -p:Configuration=Release -p:Platform=x64 ^
    -m -v:minimal /p:PlatformToolset=v143
if errorlevel 1 (
    echo Error: win32iocompat build failed
    cd %CWD%
    exit /b 1
)

echo Building openbsd_compat...
msbuild contrib\win32\openssh\openbsd_compat.vcxproj ^
    -p:Configuration=Release -p:Platform=x64 ^
    -m -v:minimal /p:PlatformToolset=v143
if errorlevel 1 (
    echo Error: openbsd_compat build failed
    cd %CWD%
    exit /b 1
)

echo Building libssh...
msbuild contrib\win32\openssh\libssh.vcxproj ^
    -p:Configuration=Release -p:Platform=x64 ^
    -m -v:minimal /p:PlatformToolset=v143
if errorlevel 1 (
    echo Error: libssh build failed
    cd %CWD%
    exit /b 1
)

echo Building ssh-keygen...
msbuild contrib\win32\openssh\keygen.vcxproj ^
    -p:Configuration=Release -p:Platform=x64 ^
    -m -v:minimal /p:PlatformToolset=v143
if errorlevel 1 (
    echo Error: keygen build failed
    cd %CWD%
    exit /b 1
)

echo Building ssh...
msbuild contrib\win32\openssh\ssh.vcxproj ^
    -p:Configuration=Release -p:Platform=x64 ^
    -m -v:minimal /p:PlatformToolset=v143
if errorlevel 1 (
    echo Error: ssh build failed
    cd %CWD%
    exit /b 1
)

echo Building sftp...
msbuild contrib\win32\openssh\sftp.vcxproj ^
    -p:Configuration=Release -p:Platform=x64 ^
    -m -v:minimal /p:PlatformToolset=v143
if errorlevel 1 (
    echo Error: sftp build failed
    cd %CWD%
    exit /b 1
)

:: Copy outputs to vendor
echo Copying binaries to vendor...
xcopy bin\x64\Release\ssh.exe %DIR%\..\vendor\openssh\x64\ssh.exe* /y /i
xcopy bin\x64\Release\ssh-keygen.exe %DIR%\..\vendor\openssh\x64\ssh-keygen.exe* /y /i
xcopy bin\x64\Release\sftp.exe %DIR%\..\vendor\openssh\x64\sftp.exe* /y /i

:: Cleanup
cd %WORKDIR%
echo.
echo Cleaning up temporary files...
rd /s /q %OPENSSH% 2>nul
cd %CWD%

echo.
echo ========================================
echo Build complete
echo ========================================
echo Binaries: %DIR%\..\vendor\openssh\x64\
echo ========================================
