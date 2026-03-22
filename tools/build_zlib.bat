:: Golddrive
:: 09/08/2018, San
:: Build zlib x64

@echo off
setlocal

set PATH=C:\Program Files\NASM;C:\Strawberry\perl\bin;C:\Windows\System32;C:\Windows
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%

curl -L -O https://zlib.net/zlib132.zip
tar xf zlib132.zip
cd zlib-1.3.2

md build_x64
cd build_x64

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cmake ..                                         ^
    -A x64 									     ^
    -G"Visual Studio 17 2022"                    ^
    -DCMAKE_INSTALL_PREFIX="C:/zlib-x64"         ^
    -DZLIB_BUILD_SHARED=OFF                      ^
    -DZLIB_BUILD_STATIC=ON                       ^
    -DZLIB_BUILD_TESTING=OFF

cmake --build . --config Release --target install
xcopy C:\zlib-x64\include ^
    %DIR%\..\vendor\zlib\include /y /s /i
:: zlib 1.3.2 names static lib "zs.lib", copy as both names for compatibility
xcopy C:\zlib-x64\lib\zs.lib ^
    %DIR%\..\vendor\zlib\lib\x64\zs.lib* /y /s /i
copy /y C:\zlib-x64\lib\zs.lib ^
    %DIR%\..\vendor\zlib\lib\x64\zlib.lib

cd ..\..
rd /s /q zlib-1.3.2
del zlib132.zip
cd %CWD%
