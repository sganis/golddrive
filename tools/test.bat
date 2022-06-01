:: Golddrive
:: 09/08/2018, San

@echo off
setlocal

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

::appveyor test script
::set GOLDDRIVE_HOST=%my_ip%
::set GOLDDRIVE_PASS=%my_variable%
::set GOLDDRIVE_USER=support
::set GOLDDRIVE_PORT=%my_port%
::vstest.console /logger:Appveyor src\test\bin\Debug\golddrive-test.dll


vstest.console ^
	%DIR%\..\src\.build\Release\%PLATFORM%\golddrive-test.dll ^
	/Settings:%DIR%\..\src\test\test.runsettings