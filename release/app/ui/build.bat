:: Golddrive
:: Make ui
:: Sep 8, 2018, sganis

@echo off
setlocal

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%

cd %DIR%
pyuic5 -o app_ui.py --import-from ui app.ui
pyuic5 -o login_ui.py --import-from ui login.ui
pyuic5 -o about_ui.py --import-from ui about.ui
pyrcc5 -o assets_rc.py assets.qrc
cd %CWD%
