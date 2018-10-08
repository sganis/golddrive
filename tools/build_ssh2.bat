mkdir build && cd build

set OPENSSL_VER=1.0.2p
set OPENSSL_DIR="C:\OpenSSL"
set ARCH=x64_86
set PYTHON_ARCH=win64
set "MSVC=Visual Studio 14 2015 Win64"
::https://indy.fulgan.com/SSL/openssl-%OPENSSL_VER%-%ARCH%-win%PYTHON_ARCH%.zip
::https://indy.fulgan.com/SSL/openssl-1.0.2o-x64_86-win64.zip

ECHO "Building with platform %MSVC%"
cmake .. -G "NMake Makefiles"   		^
 -DCMAKE_BUILD_TYPE=Release             ^
 -DCRYPTO_BACKEND=OpenSSL               ^
 -G"%MSVC%"                             ^
 -DBUILD_SHARED_LIBS=ON                 ^
 -DOPENSSL_ROOT_DIR=%OPENSSL_DIR%
REM -DENABLE_ZLIB_COMPRESSION=OFF          ^
REM -DENABLE_CRYPT_NONE=ON                 ^
REM -DENABLE_MAC_NONE=ON                   ^
REM -DZLIB_LIBRARY=C:/zlib/lib/zlib.lib    ^
REM -DZLIB_INCLUDE_DIR=C:/zlib/include     ^
REM	 -DOPENSSL_MSVC_STATIC_RT=TRUE
REM	 -DOPENSSL_USE_STATIC_LIBS=TRUE


REM cp %OPENSSL_DIR%\lib\VC\libeay32MD.lib %APPVEYOR_BUILD_FOLDER%
REM cp %OPENSSL_DIR%\lib\VC\ssleay32MD.lib %APPVEYOR_BUILD_FOLDER%
REM cp %OPENSSL_DIR%\libeay32.dll %APPVEYOR_BUILD_FOLDER%\ssh2\
REM cp %OPENSSL_DIR%\ssleay32.dll %APPVEYOR_BUILD_FOLDER%\ssh2\

cmake --build . --config Release


