:: Golddrive
:: Set development environment
:: 08/05/2018, sganis

@ECHO OFF
SET DIR=%~dp0
SET DIR=%DIR:~0,-1%

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"


set PATH=C:\Python312;C:\Python312\Scripts;%PATH%

cd c:\Dev
call cmd 


