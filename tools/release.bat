:: SSHFS
:: Make zip release
:: 08/05/2018, sganis

echo off
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set /p VERSION=<%DIR%\..\golddrive\version.txt
md %DIR%\..\release 2>nul

%DIR%\7za.exe a -tzip -mx9 %DIR%\..\release\golddrive-%VERSION%-x64.zip %DIR%\..\golddrive\*