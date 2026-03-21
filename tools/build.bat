@echo off
:: Build all components: CLI + App + Tests
:: Usage: tools\build.bat [platform]
:: Example: tools\build.bat x64

setlocal enabledelayedexpansion

set PLATFORM=%~1
if "%PLATFORM%"=="" set PLATFORM=x64

:: Setup MSVC environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Visual Studio 2022 C++ tools not found
    exit /b 1
)

set SOLUTIONDIR=%~dp0..\src\
set OUTDIR=%~dp0..\build\Release\%PLATFORM%

echo === Building CLI ===
msbuild %SOLUTIONDIR%cli\cli.vcxproj /t:rebuild /p:Configuration=Release /p:Platform=%PLATFORM% /p:SolutionDir=%SOLUTIONDIR% /v:minimal
if !errorlevel! neq 0 (
    echo CLI BUILD FAILED
    exit /b 1
)

echo === Building App ===
dotnet build %SOLUTIONDIR%app\app.csproj -c Release -o %OUTDIR%
if !errorlevel! neq 0 (
    echo APP BUILD FAILED
    exit /b 1
)

echo === Building Tests ===
dotnet build %SOLUTIONDIR%test\test.csproj -c Release -o %OUTDIR%
if !errorlevel! neq 0 (
    echo TEST BUILD FAILED
    exit /b 1
)

echo BUILD SUCCESS
