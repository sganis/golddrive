:: Golddrive
:: 09/08/2018, San
:: Build openssl

@echo off
setlocal

set VERSION=3.0.5
set TEMP=C:\Temp
set PATH=C:\Program Files\NASM;C:\Strawberry\perl\bin;C:\Windows\System32;C:\Windows
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%

rd /s /q C:\openssl-1.1.1q-x64 2>nul
rd /s /q C:\openssl-x86 2>nul

cd %TEMP%
rd /s /q openssl-openssl-%VERSION% 2>nul

rem curl -L -O https://github.com/openssl/openssl/archive/refs/tags/openssl-%VERSION%.zip
rem tar xf openssl-%VERSION%.zip
rem xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-x64 /s /e /i /y /q
rem xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-x86 /s /e /i /y /q
rem cd openssl-openssl-%VERSION%-x64

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
cd C:\Temp\openssl-OpenSSL_1_1_1q
perl Configure 			    ^
	VC-WIN64A 				^
	no-shared               ^
	--prefix=C:\openssl-1.1.1q-x64	^
	--openssldir=C:\openssl-1.1.1q-x64

nmake build_generated
nmake libcrypto.lib
nmake install_dev
xcopy C:\openssl-1.1.1q-x64\lib\libcrypto.lib ^
	%DIR%\..\vendor\openssl\lib\x64\libcrypto.lib* /y /s /i
xcopy C:\openssl-1.1.1q-x64\include %DIR%\..\vendor\openssl\include /y /s /i
cd ..

rem cd openssl-openssl-%VERSION%-x86
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
rem perl Configure 			    ^
rem 	VC-WIN32 				^
rem 	no-shared               ^
rem 	--api=1.1.0 			^
rem 	no-deprecated			^
rem 	--prefix=C:\openssl-x86	^
rem 	--openssldir=C:\openssl-x86

rem nmake build_generated
rem nmake libcrypto.lib
rem nmake install_dev
rem xcopy C:\openssl-x86\lib\libcrypto.lib ^
rem 	%DIR%\..\vendor\openssl\lib\x86\libcrypto.lib* /y /s /i
rem cd ..

rem rd /s /q openssl-* 2>nul

cd %CWD%