:: build libssh 
:: go to libssh source code directory
:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

mkdir build && cd build
set "MSVC=Visual Studio 16 2019"
set OPENSSL_DIR=C:/Users/Sant/Documents/golddrive/src/lib/openssl/lib/x64

ECHO "Building with platform %MSVC%"
cmake .. 								^
 -A x64 								^
 -G"%MSVC%"                             ^
 -DCMAKE_INSTALL_PREFIX="C:/libssh-x64"		^
 -DBUILD_SHARED_LIBS=ON          					^
 -DOPENSSL_ROOT_DIR=%OPENSSL_DIR%       ^
 -DOPENSSL_MSVC_STATIC_RT=TRUE 			^
 -DOPENSSL_USE_STATIC_LIBS=TRUE			^
 -DBUILD_SHARED_LIBS=ON         		^
 -DWITH_ZLIB=OFF ^
 -DWITH_SERVER=OFF ^
 -DWITH_EXAMPLES=OFF ^
 -DCMAKE_BUILD_TYPE=Release
:: -DWITH_DEBUG_CALLTRACE=OFF ^
:: -DWITH_SYMBOL_VERSIONING=OFF ^ 
:: -DZLIB_LIBRARY="C:\zlib\lib"              
:: -DZLIB_INCLUDE_DIR="C:\zlib\include"              

cmake --build . --config Release --target install 
::-- /clp:ErrorsOnly

cd ..

