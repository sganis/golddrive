:: Golddrive
:: 10/30/2018, sganis
::
:: Build dependencies
:: 1. OpenSSL
:: 2. Zlib
:: 3. LibSSH
:: 4. LibSSH2


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

set download=0
set build_zlib=1
set build_ossl=1
set build_ssh1=1
set build_ssh2=1

:: run vsvars[64|32].bat and set platform
::set PLATFORM=x64
set CONFIGURATION=Release
set "MSVC=Visual Studio 16 2019"

set CURDIR=%CD%
set TARGET=%CD%\lib

set ZLIB=zlib1211
set ZLIBF=zlib-1.2.11
set OPENSSL=OpenSSL_1_1_1e
set LIBSSH=libssh-0.9.3
set LIBSSH2=libssh2-1.9.0

:: openssl : 	https://github.com/openssl/openssl/archive/OpenSSL_1_1_1e.zip 
:: zlib: 		http://zlib.net/zlib1211.zip
:: libssh: 		https://www.libssh.org/files/0.9/libssh-0.9.3.tar.xz
:: libssh2: 	https://github.com/libssh2/libssh2/releases/download/libssh2-1.9.0/libssh2-1.9.0.tar.gz

set OPENSSL_URL=https://github.com/openssl/openssl/archive/%OPENSSL%.zip
set ZLIB_URL=http://zlib.net/%ZLIB%.zip
set LIBSSH_URL=https://www.libssh.org/files/0.9/%LIBSSH%.tar.xz
set LIBSSH2_URL=https://github.com/libssh2/libssh2/archive/%LIBSSH2%.zip

if %download% equ 1 (
	powershell -Command "Invoke-WebRequest %OPENSSL_URL% -OutFile openssl-%OPENSSL%.zip"
	powershell -Command "Invoke-WebRequest %ZLIB_URL% -OutFile %ZLIB%.zip"
	powershell -Command "Invoke-WebRequest %LIBSSH_URL% -OutFile %LIBSSH%.tar.xz"
	powershell -Command "Invoke-WebRequest %LIBSSH2_URL% -OutFile libssh2-%LIBSSH2%.zip"
)

if %PLATFORM%==x86 (
	set ARCH=Win32
	set OARCH=WIN32
) else (
	set ARCH=x64
	set OARCH=WIN64A
)

:: openssl
if %build_ossl% equ 1 (
if exist openssl-%OPENSSL% rd /s /q openssl-%OPENSSL%
%DIR%\7za.exe x openssl-%OPENSSL%.zip
cd openssl-%OPENSSL%
perl Configure no-shared no-asm no-stdio no-sock 		^
	VC-%OARCH% --prefix=C:\openssl-%PLATFORM% 			^
	--openssldir=C:\openssl-%PLATFORM%
nmake
nmake install 
mkdir %TARGET%\openssl
mkdir %TARGET%\openssl\lib
mkdir %TARGET%\openssl\lib
mkdir %TARGET%\openssl\lib\%PLATFORM%
robocopy C:\openssl-%PLATFORM%\lib 						^
	%TARGET%\openssl\lib\%PLATFORM% 	^
	libcrypto.lib
robocopy C:\openssl-%PLATFORM%\include 					^
	%TARGET%\openssl\include /e

cd %CURDIR%
)

:: zlib
if %build_zlib% equ 1 (
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
mkdir %TARGET%\zlib\lib
mkdir %TARGET%\zlib\lib\%PLATFORM%
robocopy C:\zlib-%PLATFORM%\lib 						^
	%TARGET%\zlib\lib\%PLATFORM% 		^
	zlibstatic.lib
robocopy C:\zlib-%PLATFORM%\include 					^
	%TARGET%\zlib\include /e

cd %CURDIR%
)

:: libssh
if %build_ssh1% equ 1 (
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
mkdir %TARGET%\libssh\lib
mkdir %TARGET%\libssh\lib\%PLATFORM%
robocopy C:\libssh-%PLATFORM%\lib 						^
	%TARGET%\libssh\lib\%PLATFORM% 		^
	ssh.lib
robocopy C:\libssh-%PLATFORM%\bin 						^
	%TARGET%\libssh\lib\%PLATFORM% 		^
	ssh.dll
robocopy C:\libssh-%PLATFORM%\include 					^
	%TARGET%\libssh\include /e

cd %CURDIR%
)

:: libssh2
if %build_ssh2% equ 1 (
if exist %LIBSSH2% rd /s /q %LIBSSH2%
%DIR%\7za.exe e %LIBSSH2%.tar.gz -y 					^
	&& %DIR%\7za.exe x %LIBSSH2%.tar -y
cd %LIBSSH2%
mkdir build && cd build
cmake .. 												^
	-A %ARCH%  											^
	-G"%MSVC%"                             				^
	-DBUILD_SHARED_LIBS=OFF 							^
	-DCMAKE_INSTALL_PREFIX="C:/libssh2-%PLATFORM%"      	^
 	-DCRYPTO_BACKEND=OpenSSL               				^
	-DOPENSSL_ROOT_DIR="C:/openssl-%PLATFORM%"        	^
	-DENABLE_ZLIB_COMPRESSION=ON ^
	-DZLIB_LIBRARY="C:/zlib-%PLATFORM%/lib/zlibstatic.lib" ^
	-DZLIB_INCLUDE_DIR="C:/zlib-%PLATFORM%/include"     ^
	-DOPENSSL_MSVC_STATIC_RT=TRUE 						^
	-DOPENSSL_USE_STATIC_LIBS=TRUE						^
	-DBUILD_TESTING=OFF 								^
	-DBUILD_EXAMPLES=OFF

cmake --build . --config Release --target install
mkdir %TARGET%
mkdir %TARGET%\libssh2
mkdir %TARGET%\libssh2\lib
mkdir %TARGET%\libssh2\lib
mkdir %TARGET%\libssh2\lib\%PLATFORM%
robocopy C:\libssh2-%PLATFORM%\lib 						^
	%TARGET%\libssh2\lib\%PLATFORM% 	^
	libssh2.lib
robocopy C:\libssh2-%PLATFORM%\include 					^
	%TARGET%\libssh2\include /e

cd %CURDIR%
)

echo PASSED
exit /b 0

:fail
echo FAILED
exit /b 1

