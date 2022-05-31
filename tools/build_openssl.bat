:: Golddrive
:: 09/08/2018, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%


perl Configure 			^
	VC-WIN64A 				^
	no-shared               ^
	--prefix=C:\openssl-3	^
	--openssldir=C:\openssl-3

rem nmake build_libs
rem nmake install_dev


