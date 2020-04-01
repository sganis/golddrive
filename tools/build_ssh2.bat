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
set OPENSSL_DIR=C:/Users/Sant/Documents/golddrive/src/lib/openssl
set ZLIB_DIR="C:/zlib-x64"

cmake .. ^
 -DCMAKE_INSTALL_PREFIX="C:/libssh2-x64"		^
 -DCRYPTO_BACKEND=OpenSSL               		^
 -DBUILD_SHARED_LIBS=OFF                		^
 -DOPENSSL_ROOT_DIR=%OPENSSL_DIR% 				^
 -DOPENSSL_MSVC_STATIC_RT=TRUE 					^
 -DOPENSSL_USE_STATIC_LIBS=TRUE 				^
 -DENABLE_ZLIB_COMPRESSION=ON 					^
 -DBUILD_TESTING=OFF 							^
 -DBUILD_EXAMPLES=OFF %DOPEN_SSL_STATIC%		^
 -DENABLE_CRYPT_NONE=ON							^
 -DENABLE_MAC_NONE=ON ^
 -DZLIB_LIBRARY=%ZLIB_DIR%/lib/zlibstatic.lib   ^
 -DZLIB_INCLUDE_DIR=%ZLIB_DIR%/include 			

cmake --build . --config Debug  --target install -- /clp:ErrorsOnly


