:: Golddrive
:: 09/08/2018, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%


::set OPENSSL_DIR=%DIR%\..\src\lib\openssl
::set OPENSSL_DIR=C:/Users/Sant/Documents/golddrive/src/lib/openssl/lib/x64
rem set OPENSSL_DIR=C:/libre-x64
rem set OPENSSL_DIR=C:/openssl-3
rem set OPENSSL_DIR=C:/Users/Sant/Documents/winlibs/prefix/openssl-x64
rem set OPENSSL_DIR=C:/Openssl
::set ZLIB_DIR="C:/zlib-x64"
::set CL=/DOPENSSL_NO_ENGINE=1 %CL%

mkdir build_x64 && cd build_x64
cmake .. ^
 -A x64 ^
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

xcopy C:\libssh2-x64\lib\libssh2.lib ..\vendor\libssh\lib\x64 /y /s /i
xcopy C:\libssh2-x64\include ..\vendor\libssh\include /y /s /i
cd ..

mkdir build_x86 && cd build_x86
cmake .. ^
 -A Win32 ^
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

xcopy C:\libssh2-x86\lib\libssh2.lib ..\vendor\libssh\lib\x86 /y /s /i
rem xcopy C:\libssh2-x86\include ..\vendor\libssh\include /y /s /i

cd ..
