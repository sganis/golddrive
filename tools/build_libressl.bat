:: Golddrive
:: 04/08/2020, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%

:: https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-3.0.2.tar.gz
set PREFIX=C:\libressl
set CONFIGURATION=Release

mkdir build
cd build 

cmake .. ^
  -G "Visual Studio 16 2019" ^
  -A x64 ^
  -DLIBRESSL_APPS=OFF 	^
  -DLIBRESSL_TESTS=OFF 	^
  -DBUILD_SHARED_LIBS=ON 	^
  -DENABLE_ASM=ON 	^
  -DOPENSSLDIR=%PREFIX% 	^
  -DCMAKE_INSTALL_PREFIX=%PREFIX%
  
cmake --build . --config %CONFIGURATION% 
::-- /clp:ErrorsOnly 
cd %CWD%
