:: Golddrive
:: Make ui
:: 09/08/2018, sganis

@echo off
setlocal

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%

cd %DIR%\..\golddrive
pyuic5 -o app\app_ui.py ..\assets\ui\app.ui
pyuic5 -o app\host_ui.py ..\assets\ui\host.ui
pyuic5 -o app\login_ui.py ..\assets\ui\login.ui
pyuic5 -o app\about_ui.py ..\assets\ui\about.ui
pyrcc5 -o app\resources_rc.py ..\assets\resources.qrc
cd %CWD%
