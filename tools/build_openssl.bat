:: Golddrive
:: 09/08/2018, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

nmake /f makefile.vc clean
perl Configure 			    ^
	VC-WIN64A 				^
	no-shared               ^
	--prefix=C:\openssl-x64	^
	--openssldir=C:\openssl-x64

nmake build_libs
nmake install_dev
xcopy C:\openssl-x64\lib\libcrypto.lib ..\vendor\openssl\lib\x64 /y /s /i
xcopy C:\openssl-x64\include ..\vendor\openssl\include /y /s /i

nmake /f makefile.vc clean
perl Configure 			    ^
	VC-WIN32 				^
	no-shared               ^
	--prefix=C:\openssl-x86	^
	--openssldir=C:\openssl-x86

nmake build_libs
nmake install_dev
xcopy C:\openssl-x86\lib\libcrypto.lib ..\vendor\openssl\lib\x86 /y /s /i

