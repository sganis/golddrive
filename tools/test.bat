@echo off
:: Run golddrive tests
:: Usage: tools\test.bat

setlocal

:: Default test environment
if "%GOLDDRIVE_HOST%"=="" set GOLDDRIVE_HOST=192.168.100.46
if "%GOLDDRIVE_USER%"=="" set GOLDDRIVE_USER=support
if "%GOLDDRIVE_PASS%"=="" set GOLDDRIVE_PASS=support

:: Native CLI unit tests (pure helpers in src/cli/parse.c) — must pass first
call "%~dp0build_clitest.bat"
if errorlevel 1 (
    echo NATIVE UNIT TESTS FAILED
    exit /b 1
)

dotnet test %~dp0..\src\test\test.csproj -c Release --logger "console;verbosity=detailed"
