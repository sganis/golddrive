:: Golddrive
:: 04/04/2020, San
:: Build openssh

:: @echo off
setlocal

set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%

set VERSION=10.0.0.0
rem set PATH=C:\Program Files\NASM;C:\Strawberry\perl\bin;C:\Windows\System32;C:\Windows

cd C:\temp
set OPENSSH=openssh-portable-%VERSION%
rd /s /q %OPENSSH% 2>nul
curl -L -O https://github.com/PowerShell/openssh-portable/archive/refs/tags/v%VERSION%.zip
tar xf v%VERSION%.zip

:: apply patch
cd %OPENSSH%
python %DIR%\patch_openssh.py

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

:: build
msbuild contrib\win32\openssh\config.vcxproj ^
	-p:Configuration=Release -m -v:minimal -t:rebuild /p:PlatformToolset=v143
msbuild contrib\win32\openssh\win32iocompat.vcxproj ^
	-p:Configuration=Release -m -v:minimal /p:PlatformToolset=v143
msbuild contrib\win32\openssh\openbsd_compat.vcxproj ^
	-p:Configuration=Release -m -v:minimal /p:PlatformToolset=v143
msbuild contrib\win32\openssh\libssh.vcxproj ^
	-p:Configuration=Release -m -v:minimal /p:PlatformToolset=v143
msbuild contrib\win32\openssh\keygen.vcxproj ^
	-p:Configuration=Release -m -v:minimal /p:PlatformToolset=v143
msbuild contrib\win32\openssh\ssh.vcxproj ^
	-p:Configuration=Release -m -v:minimal /p:PlatformToolset=v143

cd %CWD%
