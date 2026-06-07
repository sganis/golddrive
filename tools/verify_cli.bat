@echo off
:: Compile-verify the native CLI without a full WinFsp dev install.
:: Bootstraps the WinFsp SDK headers/libs from the vendored MSI (administrative
:: extract -- no elevation, no driver install), then compiles all CLI sources
:: (/c) against the real WinFsp/libssh2/OpenSSL headers. Catches build breakage
:: locally before CI. Usage: tools\verify_cli.bat
setlocal enabledelayedexpansion

set "VCVARS="
for %%E in (Enterprise Professional Community BuildTools) do (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat"
)
if "%VCVARS%"=="" ( echo ERROR: VS2022 C++ tools not found & exit /b 1 )
call "%VCVARS%" >nul 2>&1

pushd "%~dp0.."
set "ROOT=%CD%"
popd
set "SDK=%ROOT%\build\winfsp-sdk\DYNAMIC"
set "MSI=%ROOT%\vendor\winfsp\winfsp-2.1.25156.msi"
set "OUT=%ROOT%\build\verify"

:: extract the WinFsp SDK once
if not exist "%SDK%\inc\winfsp\winfsp.h" (
    echo Extracting WinFsp SDK from vendored MSI...
    msiexec /a "%MSI%" /qn TARGETDIR="%ROOT%\build\winfsp-sdk"
    if not exist "%SDK%\inc\winfsp\winfsp.h" ( echo ERROR: SDK extract failed & exit /b 1 )
)

if not exist "%OUT%" mkdir "%OUT%"
pushd "%OUT%"
cl /nologo /c /W4 /wd4996 /MT ^
  /D_WIN64 /DNDEBUG /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS ^
  /DNTDDI_VERSION=0x0A000000 /D_WIN32_WINNT=0x0A00 ^
  /I "%SDK%\inc\fuse3" /I "%SDK%\inc" ^
  /I "%ROOT%\vendor\libssh2\include" /I "%ROOT%\vendor\openssl\include" ^
  "%ROOT%\src\cli\cache.c" "%ROOT%\src\cli\gd.c" "%ROOT%\src\cli\jsmn.c" ^
  "%ROOT%\src\cli\main.c" "%ROOT%\src\cli\net.c" "%ROOT%\src\cli\pool.c" ^
  "%ROOT%\src\cli\parse.c" "%ROOT%\src\cli\util.c"
set "RC=!errorlevel!"
popd
if !RC! neq 0 ( echo COMPILE FAILED & exit /b 1 )
echo COMPILE OK
