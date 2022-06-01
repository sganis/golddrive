:: Golddrive
:: 09/08/2018, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

rem cd C:\openssl-openssl-3.0.3-x64
rem perl Configure 			    ^
rem 	VC-WIN64A 				^
rem 	no-shared               ^
rem 	--prefix=C:\openssl-x64	^
rem 	--openssldir=C:\openssl-x64

rem nmake build_generated
rem nmake libcrypto.lib
rem nmake install_dev
xcopy C:\openssl-x64\lib\libcrypto.lib* %DIR%\..\vendor\openssl\x64\lib /y /s /i
xcopy C:\openssl-x64\include %DIR%\..\vendor\openssl\x64\include /y /s /i

rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
rem cd C:\openssl-openssl-3.0.3-x86
rem perl Configure 			    ^
rem 	VC-WIN32 				^
rem 	no-shared               ^
rem 	--prefix=C:\openssl-x86	^
rem 	--openssldir=C:\openssl-x86

rem nmake build_generated
rem nmake libcrypto.lib
rem nmake install_dev
xcopy C:\openssl-x86\lib\libcrypto.lib* %DIR%\..\vendor\openssl\x86\lib /y /s /i
xcopy C:\openssl-x86\include %DIR%\..\vendor\openssl\x64\include /y /s /i

