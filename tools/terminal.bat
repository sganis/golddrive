:: Golddrive
:: Set development environment
:: 08/05/2018, sganis

@ECHO OFF
SET DIR=%~dp0
SET DIR=%DIR:~0,-1%

:: CALL %DIR%\setenv.bat
call %COMSPEC% /k c:\Dev\hotel\api\venv\Scripts\activate.bat /D c:\Dev\hotel

