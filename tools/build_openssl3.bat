:: Golddrive
:: 09/08/2018, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%



perl Configure 				^
	VC-WIN64A 				^
	--prefix=C:\openssl-3 	^
	--openssldir=C:\openssl-3

rem nmake
rem nmake install


rem 1.1.1e
rem mkdir build
rem cd build
rem perl ..\Configure 			^
rem 	no-shared   			^
rem 	VC-WIN64A 				^
rem 	--prefix=C:\openssl 	^
rem 	--openssldir=C:\openssl
rem nmake build_libs
rem nmake install_dev
rem cd ..


::copy inc32/include C:\Openssl\include
::copy out32/libeay32.lib C:\Openssl\lib
::copy out32/ssleay32.lib C:\Openssl\lib
