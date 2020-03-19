mkdir build && cd build

set OPENSSL_DIR=C:\Openssl

:: static library
::http://p-nand-q.com/programming/windows/openssl-1.0.2j-64bit-release-static-vs2015.7z

cmake .. ^
 -DCRYPTO_BACKEND=OpenSSL               ^
 -DBUILD_SHARED_LIBS=OFF                ^
 -DOPENSSL_ROOT_DIR=%OPENSSL_DIR% 		^
 -DOPENSSL_MSVC_STATIC_RT=TRUE 			^
 -DOPENSSL_USE_STATIC_LIBS=TRUE ^
 -DCMAKE_BUILD_TYPE=Debug

rem cmake --build . --config Debug


