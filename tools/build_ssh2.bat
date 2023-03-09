:: Golddrive
:: 09/08/2018, San
:: Build libssh2

@echo off
setlocal

set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
mkdir build_x64
cd build_x64
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
    %DIR%\..\vendor\libssh2\lib\x64\libssh2.lib* /y /s /i
xcopy C:\libssh2-x64\include ^
    %DIR%\..\vendor\libssh2\include /y /s /i
cd ..

rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"

rem mkdir build_x86
rem cd build_x86
rem cmake .. ^
rem  -A Win32 ^
rem  -DCMAKE_INSTALL_PREFIX="C:/libssh2-x86"        ^
rem  -DCMAKE_BUILD_TYPE=Release                     ^
rem  -DCRYPTO_BACKEND=OpenSSL                       ^
rem  -DBUILD_SHARED_LIBS=OFF                        ^
rem  -DOPENSSL_ROOT_DIR=C:/openssl-x86              ^
rem  -DENABLE_ZLIB_COMPRESSION=OFF                  ^
rem  -DBUILD_TESTING=OFF                            ^
rem  -DBUILD_EXAMPLES=OFF                           ^
rem  -DENABLE_CRYPT_NONE=ON                         ^
rem  -DCLEAR_MEMORY=OFF
rem cmake --build . --config Release --target install

rem xcopy C:\libssh2-x86\lib\libssh2.lib ^
rem     %DIR%\..\vendor\libssh2\lib\x86\libssh2.lib* /y /s /i

rem cd %CWD%