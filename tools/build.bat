:: Golddrive
:: 09/08/2018, San

@echo off
setlocal

msbuild %~dp0..\src\golddrive.sln -t:rebuild -p:Configuration=Release -m:4 -v:minimal
