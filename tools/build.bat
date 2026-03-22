@echo off
:: Build all components: CLI + App + Tests
:: Usage: tools\build.bat

setlocal enabledelayedexpansion

:: Setup MSVC environment - detect VS edition
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
set OUTDIR=%~dp0..\build\Release\x64

echo === Building CLI ===
msbuild %SOLUTIONDIR%cli\cli.vcxproj /t:rebuild /p:Configuration=Release /p:Platform=x64 /p:SolutionDir=%SOLUTIONDIR% /v:minimal
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
