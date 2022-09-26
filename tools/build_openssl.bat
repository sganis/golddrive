:: Golddrive
:: 09/08/2018, San
:: Build openssl

@echo off
setlocal

set VERSION=3.0.5

set "PATH=C:\Program Files\NASM;C:\Strawberry\perl\bin;C:\Windows\System32;C:\Windows"
set DIR=%~dp0
set DIR=%DIR:~0,-1%

rd /s /q C:\openssl-x64 2>nul
rd /s /q C:\openssl-x86 2>nul

cd C:\temp
rd /s /q openssl-openssl-%VERSION% 2>nul

curl -L -O https://github.com/openssl/openssl/archive/refs/tags/openssl-%VERSION%.zip
tar xf openssl-%VERSION%.zip
xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-x64 /s /e /i /y /q
xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-x86 /s /e /i /y /q

cd openssl-openssl-%VERSION%-x64
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
perl Configure 			    ^
	VC-WIN64A 				^
	no-shared               ^
	--api=1.1.0 			^
	no-deprecated			^
	--prefix=C:\openssl-x64	^
	--openssldir=C:\openssl-x64

nmake build_generated
nmake libcrypto.lib
nmake install_dev
xcopy C:\openssl-x64\lib\libcrypto.lib ^
	%DIR%\..\vendor\openssl\lib\x64\libcrypto.lib* /y /s /i
xcopy C:\openssl-x64\include %DIR%\..\vendor\openssl\include /y /s /i

cd ..\openssl-openssl-%VERSION%-x86
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
perl Configure 			    ^
	VC-WIN32 				^
	no-shared               ^
	--api=1.1.0 			^
	no-deprecated			^
	--prefix=C:\openssl-x86	^
	--openssldir=C:\openssl-x86

nmake build_generated
nmake libcrypto.lib
nmake install_dev
xcopy C:\openssl-x86\lib\libcrypto.lib ^
	%DIR%\..\vendor\openssl\lib\x86\libcrypto.lib* /y /s /i

cd ..
rd /s /q openssl-* 2>nul

cd %DIR%