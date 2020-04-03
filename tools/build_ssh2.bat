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
rem set OPENSSL_DIR=C:/Users/Sant/Documents/golddrive/src/lib/openssl/lib/x64
rem set OPENSSL_DIR=C:/Users/Sant/Documents/winlibs/prefix/openssl-x64
set OPENSSL_DIR=C:/Openssl
set ZLIB_DIR="C:/zlib-x64"

cmake .. ^
 -DCMAKE_INSTALL_PREFIX="C:/libssh2-x64"		^
 -DCMAKE_BUILD_TYPE=Release						^
 -DCRYPTO_BACKEND=OpenSSL               		^
 -DBUILD_SHARED_LIBS=OFF                 		^
 -DOPENSSL_ROOT_DIR=%OPENSSL_DIR% 				^
 -DENABLE_ZLIB_COMPRESSION=OFF 					^
 -DBUILD_TESTING=OFF 							^
 -DBUILD_EXAMPLES=OFF 							^
 -DENABLE_CRYPT_NONE=ON							
 
 rem -DOPENSSL_MSVC_STATIC_RT=TRUE 					^
 rem -DOPENSSL_USE_STATIC_LIBS=TRUE 
 rem ::-DZLIB_LIBRARY=%ZLIB_DIR%/lib/zlibstatic.lib   ^
 rem ::-DZLIB_INCLUDE_DIR=%ZLIB_DIR%/include 			

cmake --build . --config Release --target install
:: -- /clp:ErrorsOnly

cd ..
