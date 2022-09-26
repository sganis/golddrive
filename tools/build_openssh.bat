:: Golddrive
:: 04/04/2020, San
:: Build openssh

@echo off
setlocal

set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%

set VERSION=8.9.1.0

cd C:\temp
set OPENSSH=openssh-portable-%VERSION%
rd /s /q %OPENSSH%
curl -L -O https://github.com/PowerShell/openssh-portable/archive/refs/tags/V%VERSION%.zip
tar xf V%VERSION%.zip

:: apply patch 
cd %OPENSSH%
python %DIR%\patch_openssh.py

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

:: build
msbuild %OPENSSH%\contrib\win32\openssh\config.vcxproj ^
	-p:Configuration=Release -m -v:minimal -t:rebuild /p:PlatformToolset=v142
msbuild %OPENSSH%\contrib\win32\openssh\win32iocompat.vcxproj ^
	-p:Configuration=Release -m -v:minimal /p:PlatformToolset=v142
msbuild %OPENSSH%\contrib\win32\openssh\openbsd_compat.vcxproj ^
	-p:Configuration=Release -m -v:minimal /p:PlatformToolset=v142
msbuild %OPENSSH%\contrib\win32\openssh\libssh.vcxproj ^
	-p:Configuration=Release -m -v:minimal /p:PlatformToolset=v142
msbuild %OPENSSH%\contrib\win32\openssh\keygen.vcxproj ^
	-p:Configuration=Release -m -v:minimal /p:PlatformToolset=v142
msbuild %OPENSSH%\contrib\win32\openssh\ssh.vcxproj ^
	-p:Configuration=Release -m -v:minimal /p:PlatformToolset=v142

cd %CWD%
