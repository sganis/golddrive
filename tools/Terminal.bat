:: Golddrive
:: Set development environment
:: 08/05/2018, sganis

@ECHO OFF
SET DIR=%~dp0
SET DIR=%DIR:~0,-1%

CD /D %USERPROFILE%\Documents\golddrive
CALL %DIR%\setenv.bat
call %COMSPEC% 
start "" /b %DIR%\..\src\golddrive.sln 
