:: Golddrive
:: 09/08/2018, San
:: Build zlib

@echo off
setlocal

set PATH=C:\Program Files\NASM;C:\Strawberry\perl\bin;C:\Windows\System32;C:\Windows
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%

curl -L -O https://zlib.net/zlib1212.zip
tar xf zlib1212.zip
cd zlib-1.2.12
set "MSVC=Visual Studio 16 2019"

md build_x64
cd build_x64

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cmake ..                                         ^
    -A x64 									     ^
    -G"%MSVC%"                                   ^
    -DCMAKE_INSTALL_PREFIX="C:/zlib-x64"         ^
    -DBUILD_SHARED_LIBS=OFF

cmake --build . --config Release --target install
xcopy C:\zlib-x64\include ^
    %DIR%\..\vendor\zlib\include /y /s /i
xcopy C:\zlib-x64\lib\zlibstatic.lib ^
    %DIR%\..\vendor\zlib\lib\x64\zlibstatic.lib* /y /s /i
cd ..

md build_x86
cd build_x86

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"

cmake ..                                         ^
    -A Win32                                     ^
    -G"%MSVC%"                                   ^
    -DCMAKE_INSTALL_PREFIX="C:/zlib-x86"         ^
    -DBUILD_SHARED_LIBS=OFF

cmake --build . --config Release --target install
xcopy C:\zlib-x86\include ^
    %DIR%\..\vendor\zlib\include /y /s /i
xcopy C:\zlib-x86\lib\zlibstatic.lib ^
    %DIR%\..\vendor\zlib\lib\x86\zlibstatic.lib* /y /s /i

cd ..\..
rd /s /q zlib-1.2.12
del zlib1212.zip
cd %CWD%