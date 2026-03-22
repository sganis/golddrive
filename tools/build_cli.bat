@echo off
:: Build CLI only
:: Usage: tools\build_cli.bat

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

set SOLUTIONDIR=%~dp0..\src\
msbuild %SOLUTIONDIR%cli\cli.vcxproj /t:rebuild /p:Configuration=Release /p:Platform=x64 /p:SolutionDir=%SOLUTIONDIR% /v:minimal
if !errorlevel! neq 0 (
    echo BUILD FAILED
    exit /b 1
)
echo BUILD SUCCESS
