:: Golddrive
:: Set development environment
:: 08/05/2018, sganis

@ECHO OFF
SET DIR=%~dp0
SET DIR=%DIR:~0,-1%
DOSKEY ll=dir

SET "PATH=C:\Windows\System32;C:\Windows;C:\Windows\System32\wbem"
echo Setting development environment...
::CALL "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

SET "PATH=C:\Python37;C:\Python37\Scripts;%PATH%"
SET "PATH=C:\Program Files (x86)\WinFsp\bin;%PATH%"
SET "PATH=C:\Program Files\Git\bin;%PATH%"
SET "PATH=C:\Program Files\Git\usr\bin;%PATH%"
SET "PATH=C:\Program Files (x86)\WiX Toolset v3.11\bin;%PATH%"

CD /D %USERPROFILE%\Documents\golddrive
CALL test\setenv.bat

CALL %COMSPEC%
