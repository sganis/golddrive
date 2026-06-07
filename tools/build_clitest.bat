@echo off
:: Build and run the native CLI unit tests (src/clitest).
:: Hermetic: compiles parse.c only, no WinFsp / no libssh2.lib link, so it runs
:: on any machine with VS2022 + the vendored libssh2 headers.
:: Usage: tools\build_clitest.bat

setlocal enabledelayedexpansion

set "VCVARS="
for %%E in (Enterprise Professional Community BuildTools) do (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" (
        set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat"
    )
)
if "%VCVARS%"=="" (
    echo ERROR: Visual Studio 2022 C++ tools not found
    exit /b 1
)
call "%VCVARS%" >nul 2>&1

set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\test"
if not exist "%OUT%" mkdir "%OUT%"

:: build from the output dir so the compiler PDB (vc*.pdb) stays under build\
pushd "%OUT%"
cl /nologo /W4 /wd4996 /MT /Zi ^
   /I "%ROOT%\vendor\libssh2\include" ^
   "%ROOT%\src\clitest\clitest.c" ^
   "%ROOT%\src\clitest\fuzz.c" ^
   "%ROOT%\src\clitest\nettest.c" ^
   "%ROOT%\src\cli\parse.c" ^
   "%ROOT%\src\cli\util.c" ^
   "%ROOT%\src\cli\jsmn.c" ^
   "%ROOT%\src\cli\net.c" ^
   /Fe:"%OUT%\clitest.exe" /Fo:"%OUT%\\" /Fd:"%OUT%\clitest.pdb" ^
   /link version.lib advapi32.lib ws2_32.lib
set "RC=!errorlevel!"
popd
if !RC! neq 0 (
    echo BUILD FAILED
    exit /b 1
)

echo.
"%OUT%\clitest.exe"
exit /b !errorlevel!
