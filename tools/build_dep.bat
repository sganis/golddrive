:: Golddrive
:: 10/30/2018, sganis
::
:: Build dependencies

@echo off
setlocal
 
::if [%4]==[] goto :usage
::if "%1"=="-h" goto :usage
::
::set USER=%1
::set HOST=%2
::set PORT=%3
::set DRIVE=%4
 
:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

set PLATFORM=x64
set CONFIGURATION=Release

set CURDIR=%CD%
set TARGET=%CD%\lib

if %PLATFORM%==x86 (
	set ARCH=Win32
	set OARCH=WIN32
) else (
	set ARCH=x64
	set OARCH=WIN64A
)

set "MSVC=Visual Studio 16 2019"

:: openssl : 	https://github.com/openssl/openssl/archive/OpenSSL_1_1_1e.zip 
:: zlib: 		http://zlib.net/zlib1211.zip
:: libssh: 		https://www.libssh.org/files/0.9/libssh-0.9.3.tar.xz
set ZLIB=zlib1211
set ZLIBF=zlib-1.2.11
set OPENSSL=openssl-OpenSSL_1_1_1e
set LIBSSH=libssh-0.9.3

:: openssl
if exist %OPENSSL% rd /s /q %OPENSSL%
%DIR%\7za.exe x %OPENSSL%.zip
cd %OPENSSL%
perl Configure no-shared no-asm no-stdio no-sock 		^
	VC-%OARCH% --prefix=C:\openssl-%PLATFORM% 			^
	--openssldir=C:\openssl-%PLATFORM%
nmake
nmake install 
mkdir %TARGET%\openssl
mkdir %TARGET%\openssl\lib
mkdir %TARGET%\openssl\lib\%CONFIGURATION%
mkdir %TARGET%\openssl\lib\%CONFIGURATION%\%PLATFORM%
robocopy C:\openssl-%PLATFORM%\lib 						^
	%TARGET%\openssl\lib\%CONFIGURATION%\%PLATFORM% 	^
	libcrypto.lib
robocopy C:\openssl-%PLATFORM%\include 					^
	%TARGET%\openssl\include /e 
cd %CURDIR%

:: zlib
if exist %ZLIBF% rd /s /q %ZLIBF%
%DIR%\7za.exe x %ZLIB%.zip
cd %ZLIBF%
mkdir build && cd build
cmake ..                                         		^
	-A %ARCH% 									 		^
	-G"%MSVC%"                                   		^
	-DCMAKE_INSTALL_PREFIX="C:\zlib-%PLATFORM%"  		^
	-DBUILD_SHARED_LIBS=OFF
cmake --build . --config Release --target install
mkdir %TARGET%
mkdir %TARGET%\zlib
mkdir %TARGET%\zlib\lib
mkdir %TARGET%\zlib\lib\%CONFIGURATION%
mkdir %TARGET%\zlib\lib\%CONFIGURATION%\%PLATFORM%
robocopy C:\zlib-%PLATFORM%\lib 						^
	%TARGET%\zlib\lib\%CONFIGURATION%\%PLATFORM% 		^
	zlibstatic.lib
robocopy C:\zlib-%PLATFORM%\include 					^
	%TARGET%\zlib\include /e 
cd %CURDIR%

:: libssh
if exist %LIBSSH% rd /s /q %LIBSSH%
%DIR%\7za.exe e %LIBSSH%.tar.xz -y 						^
	&& %DIR%\7za.exe x %LIBSSH%.tar -y
cd %LIBSSH%
mkdir build && cd build
cmake .. 												^
	-A %ARCH%  											^
	-G"%MSVC%"                             				^
	-DCMAKE_INSTALL_PREFIX="C:\libssh-%PLATFORM%"      	^
	-DOPENSSL_ROOT_DIR="C:\openssl-%PLATFORM%"        	^
	-DZLIB_LIBRARY="C:\zlib-%PLATFORM%\lib\zlibstatic.lib" ^
	-DZLIB_INCLUDE_DIR="C:\zlib-%PLATFORM%\include"     ^
	-DOPENSSL_MSVC_STATIC_RT=TRUE 						^
	-DOPENSSL_USE_STATIC_LIBS=TRUE						^
	-DBUILD_SHARED_LIBS=ON
cmake --build . --config Release --target install
mkdir %TARGET%
mkdir %TARGET%\libssh
mkdir %TARGET%\libssh\lib
mkdir %TARGET%\libssh\lib\%CONFIGURATION%
mkdir %TARGET%\libssh\lib\%CONFIGURATION%\%PLATFORM%
robocopy C:\libssh-%PLATFORM%\lib 						^
	%TARGET%\libssh\lib\%CONFIGURATION%\%PLATFORM% 		^
	ssh.lib
robocopy C:\libssh-%PLATFORM%\bin 						^
	%TARGET%\libssh\bin\%CONFIGURATION%\%PLATFORM% 		^
	ssh.dll
robocopy C:\libssh-%PLATFORM%\include 					^
	%TARGET%\libssh\include /e 
cd %CURDIR%

echo PASSED
exit /b 0

:fail
echo FAILED
exit /b 1

