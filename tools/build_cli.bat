@echo off
:: Build CLI only
:: Usage: tools\build_cli.bat [platform]
:: Example: tools\build_cli.bat x64

setlocal enabledelayedexpansion

set PLATFORM=%~1
if "%PLATFORM%"=="" set PLATFORM=x64

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Visual Studio 2022 C++ tools not found
    exit /b 1
)

set SOLUTIONDIR=%~dp0..\src\
msbuild %SOLUTIONDIR%cli\cli.vcxproj /t:rebuild /p:Configuration=Release /p:Platform=%PLATFORM% /p:SolutionDir=%SOLUTIONDIR% /v:minimal
if !errorlevel! neq 0 (
    echo BUILD FAILED
    exit /b 1
)
echo BUILD SUCCESS
