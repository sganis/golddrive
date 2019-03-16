:: SSHFS
:: Make zip release
:: 08/05/2018, sganis

@echo off
setlocal
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set PATH=C:\cygwin64\bin;%PATH%
set CWD="%cd%"
cd %DIR%\..

:: cleanup
git clean -dfx

:: get version
set /p VERSION=<golddrive\version.txt

:: prepare folder
md release 2>nul
cd golddrive

:: zip
%DIR%\7za.exe a -tzip -mx9 ..\release\golddrive-%VERSION%-x64.zip *

cd "%CWD%"