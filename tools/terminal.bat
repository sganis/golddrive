:: Golddrive
:: Set development environment
:: 08/05/2018, sganis

@ECHO OFF
SET DIR=%~dp0
SET DIR=%DIR:~0,-1%

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
rem set PATH=C:\Qt\6.8.0\msvc2022_64\bin;%PATH%

set JAVA_HOME=C:\Program Files\Android\Android Studio\jbr
set ANDROID_HOME=C:\Users\San\AppData\Local\Android\Sdk
set NDK_HOME=C:\Users\San\AppData\Local\Android\Sdk\ndk\28.0.12916984

set PATH=C:\Python312;C:\Python312\Scripts;%PATH%

cd c:\Dev
call cmd 


