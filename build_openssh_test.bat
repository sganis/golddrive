@echo off
setlocal enabledelayedexpansion
cd C:\Temp\openssh-portable-10.0.0.0
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

echo === Building config ===
msbuild contrib\win32\openssh\config.vcxproj -p:Configuration=Release -p:Platform=x64 -m -v:minimal -t:rebuild /p:PlatformToolset=v143
if !errorlevel! neq 0 (echo CONFIG FAILED & exit /b 1)

echo === Building win32iocompat ===
msbuild contrib\win32\openssh\win32iocompat.vcxproj -p:Configuration=Release -p:Platform=x64 -m -v:minimal /p:PlatformToolset=v143
if !errorlevel! neq 0 (echo WIN32IOCOMPAT FAILED & exit /b 1)

echo === Building openbsd_compat ===
msbuild contrib\win32\openssh\openbsd_compat.vcxproj -p:Configuration=Release -p:Platform=x64 -m -v:minimal /p:PlatformToolset=v143
if !errorlevel! neq 0 (echo OPENBSD_COMPAT FAILED & exit /b 1)

echo === Building libssh ===
msbuild contrib\win32\openssh\libssh.vcxproj -p:Configuration=Release -p:Platform=x64 -m -v:minimal /p:PlatformToolset=v143
if !errorlevel! neq 0 (echo LIBSSH FAILED & exit /b 1)

echo === Building keygen ===
msbuild contrib\win32\openssh\keygen.vcxproj -p:Configuration=Release -p:Platform=x64 -m -v:minimal /p:PlatformToolset=v143
if !errorlevel! neq 0 (echo KEYGEN FAILED & exit /b 1)

echo === Building ssh ===
msbuild contrib\win32\openssh\ssh.vcxproj -p:Configuration=Release -p:Platform=x64 -m -v:minimal /p:PlatformToolset=v143
if !errorlevel! neq 0 (echo SSH FAILED & exit /b 1)

echo === ALL BUILDS SUCCEEDED ===
