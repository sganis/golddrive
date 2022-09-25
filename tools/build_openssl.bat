:: Golddrive
:: 09/08/2018, San
::@echo off
setlocal
setlocal EnableDelayedExpansion

set VERSION=3.0.5

set "PATH=C:\Program Files\NASM;%PATH%"
set "PATH=C:\Strawberry\perl\bin;%PATH%"

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

rd /s /q C:\openssl-x64
rd /s /q C:\openssl-x86

cd C:\temp
rd /s /q openssl-openssl-%VERSION%

curl -L -O https://github.com/openssl/openssl/archive/refs/tags/openssl-%VERSION%.zip
tar xf openssl-%VERSION%.zip
xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-x64 /s /e /i /y /q
xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-x86 /s /e /i /y /q

cd openssl-openssl-%VERSION%-x64
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
perl Configure 			    ^
	VC-WIN64A 				^
	no-shared               ^
	--prefix=C:\openssl-x64	^
	--openssldir=C:\openssl-x64

nmake build_generated
nmake libcrypto.lib
nmake install_dev
xcopy C:\openssl-x64\lib\libcrypto.lib ^
	%DIR%\..\vendor\openssl\lib\libcrypto-x64.lib* /y /s /i
xcopy C:\openssl-x64\include ^
	%DIR%\..\vendor\openssl\include /y /s /i

cd ..\openssl-openssl-%VERSION%-x86
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
perl Configure 			    ^
	VC-WIN32 				^
	no-shared               ^
	--prefix=C:\openssl-x86	^
	--openssldir=C:\openssl-x86

nmake build_generated
nmake libcrypto.lib
nmake install_dev
xcopy C:\openssl-x86\lib\libcrypto.lib ^
	%DIR%\..\vendor\openssl\lib\libcrypto-x86.lib* /y /s /i

