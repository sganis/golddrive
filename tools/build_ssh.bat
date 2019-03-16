:: build libssh 
:: go to libssh source code directory
mkdir build && cd build
set OPENSSL_DIR="C:\openssl-static"
set "MSVC=Visual Studio 14 2015 Win64"
::https://indy.fulgan.com/SSL/openssl-%OPENSSL_VER%-%ARCH%-win%PYTHON_ARCH%.zip
::https://indy.fulgan.com/SSL/openssl-1.0.2o-x64_86-win64.zip
:: static library
::http://p-nand-q.com/programming/windows/openssl-1.0.2j-64bit-release-static-vs2015.7z

ECHO "Building with platform %MSVC%"
cmake .. -G "NMake Makefiles"   		^
 -DCMAKE_BUILD_TYPE=Release             ^
 -DCRYPTO_BACKEND=OpenSSL               ^
 -G"%MSVC%"                             ^
 -DBUILD_SHARED_LIBS=OFF                ^
 -DOPENSSL_ROOT_DIR=%OPENSSL_DIR% 		^
 -DOPENSSL_MSVC_STATIC_RT=TRUE 			^
 -DOPENSSL_USE_STATIC_LIBS=TRUE			^
 -DWITH_ZLIB=OFF

cmake --build . --config Release


