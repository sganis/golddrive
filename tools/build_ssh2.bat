:: Golddrive
:: 09/08/2018, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

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

xcopy C:\libssh2-x64\lib\libssh2.lib ^
    %DIR%\..\vendor\libssh2\lib\libssh2-x64.lib* /y /s /i
xcopy C:\libssh2-x64\include ^
    %DIR%\..\vendor\libssh2\include /y /s /i
cd ..

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"

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

xcopy C:\libssh2-x86\lib\libssh2.lib %DIR%\..\vendor\libssh2\lib\libssh2-x86.lib* /y /s /i

cd ..
