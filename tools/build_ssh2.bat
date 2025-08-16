:: Golddrive
:: 09/08/2018, San
:: Build libssh2

@echo off
setlocal

set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%
set VERSION=1.11.1

curl -L -O https://github.com/libssh2/libssh2/archive/refs/tags/libssh2-%VERSION%.zip
tar xf libssh2-%VERSION%.zip
cd libssh2-libssh2-%VERSION%

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

mkdir build_x64
cd build_x64
cmake .. ^
 -DCMAKE_INSTALL_PREFIX="C:/libssh2-x64"		^
 -DCMAKE_BUILD_TYPE=Release						^
 -DCRYPTO_BACKEND=OpenSSL               		^
 -DBUILD_SHARED_LIBS=OFF                 		^
 -DOPENSSL_ROOT_DIR=C:/openssl-x64 				^
 -DENABLE_ZLIB_COMPRESSION=OFF 					^
 -DBUILD_TESTING=OFF 							^
 -DBUILD_EXAMPLES=OFF 							^
 -DENABLE_CRYPT_NONE=ON							^
 -DCLEAR_MEMORY=OFF
cmake --build . --config Release --target install

xcopy C:\libssh2-x64\lib\libssh2.lib %DIR%\..\vendor\libssh2\lib\x64\libssh2.lib* /y /s /i
xcopy C:\libssh2-x64\include %DIR%\..\vendor\libssh2\include /y /s /i
cd ..


call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"

mkdir build_x86
cd build_x86
cmake .. ^
 -DCMAKE_INSTALL_PREFIX="C:/libssh2-x86"        ^
 -DCMAKE_BUILD_TYPE=Release                     ^
 -DCRYPTO_BACKEND=OpenSSL                       ^
 -DBUILD_SHARED_LIBS=OFF                        ^
 -DOPENSSL_ROOT_DIR=C:/openssl-x86              ^
 -DENABLE_ZLIB_COMPRESSION=OFF                  ^
 -DBUILD_TESTING=OFF                            ^
 -DBUILD_EXAMPLES=OFF                           ^
 -DENABLE_CRYPT_NONE=ON                         ^
 -DCLEAR_MEMORY=OFF
cmake --build . --config Release --target install

xcopy C:\libssh2-x86\lib\libssh2.lib %DIR%\..\vendor\libssh2\lib\x86\libssh2.lib* /y /s /i

cd %CWD%