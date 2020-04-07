:: Golddrive
:: 04/04/2020, San

@echo off
setlocal
:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%


:: https://github.com/PowerShell/openssh-portable/archive/v8.1.0.0.zip

:: winlibs is cloned in same folder as golddrive
:: openssh-portable is cloned in same folder as golddrive
set OPENSSH=%DIR%\..\..\openssh-portable

:: apply patch 
cd %OPENSSH%
python %DIR%\..\..\winlibs\patch_openssh.py

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
