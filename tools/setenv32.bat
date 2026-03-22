:: Golddrive
:: 08/05/2018, San
:: Set development environment

@echo off
set DIR=%~dp0
set DIR=%DIR:~0,-1%

echo Setting x86 development environment...
SET PLATFORM=x86
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
