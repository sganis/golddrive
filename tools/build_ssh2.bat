:: Golddrive
:: 09/08/2018, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

mkdir build && cd build

::set OPENSSL_DIR=%DIR%\..\src\lib\openssl
set OPENSSL_DIR=C:\openssl2

cmake .. ^
 -DCRYPTO_BACKEND=OpenSSL               ^
 -DBUILD_SHARED_LIBS=OFF                ^
 -DOPENSSL_ROOT_DIR=%OPENSSL_DIR% 		^
 -DOPENSSL_MSVC_STATIC_RT=TRUE 			^
 -DOPENSSL_USE_STATIC_LIBS=TRUE 		^
 -DENABLE_ZLIB_COMPRESSION=OFF
cmake --build . --config Release


