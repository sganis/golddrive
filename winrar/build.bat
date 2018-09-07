:: Golddrive installer
:: 08/05/2018, sganis

@echo off

set VERSION=1.0

set DIR=%~dp0
set DIR=%DIR:~0,-1%
set ROOT=%DIR%\..
set ICON=%ROOT%\release\app\assets\golddrive.ico

echo Creating installer for golddrive..
"%DIR%\winrar.exe" a -r -m5 -ibck -sfx ^
	-z"%DIR%\config.txt" ^
	-iicon"%ICON%" ^
	-d%USERPROFILE%\golddrive ^
	"%DIR%\golddrive-%VERSION%-x64.exe" %DIR%\..\release
echo Done.
