:: build libssh 
:: go to libssh source code directory
:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

mkdir build && cd build
set "MSVC=Visual Studio 16 2019"

ECHO "Building with platform %MSVC%"
cmake .. 								^
 -DCMAKE_BUILD_TYPE=Release             ^
 -DCRYPTO_BACKEND=OpenSSL               ^
 -G"%MSVC%"                             ^
 -DOPENSSL_ROOT_DIR="C:\openssl"        ^
 -DZLIB_LIBRARY="C:\zlib\lib"              ^
 -DZLIB_INCLUDE_DIR="C:\zlib\include"              ^
 -DOPENSSL_MSVC_STATIC_RT=TRUE 			^
 -DOPENSSL_USE_STATIC_LIBS=TRUE			^
 -DBUILD_SHARED_LIBS=OFF                
:: -DWITH_ZLIB=OFF
 
cmake --build . --config Release

cd %DIR%
