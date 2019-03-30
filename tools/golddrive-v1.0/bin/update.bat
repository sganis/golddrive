:: GoldDrive
:: Update script
@echo off
setlocal
set DIR=%~dp0
set DIR=%DIR:~0,-1%

: restart app
taskkill /f /im golddrive.exe /t
start /b "" %DIR%\..\..\golddrive-ui.exe

