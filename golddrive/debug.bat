:: Gold Drive
:: Run in debug mode
:: 09/05/2018, sganis

@echo off
setlocal

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%
cd %DIR%

lib\python.exe app\app.py

cd %CWD%
