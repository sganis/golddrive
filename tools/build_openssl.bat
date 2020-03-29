:: Golddrive
:: 09/08/2018, San
@echo off
setlocal
setlocal EnableDelayedExpansion

:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

perl Configure no-shared no-comp VC-WIN64A --prefix=C:\Openssl --openssldir=C:\Openssl
call ms\do_win64a
nmake -f ms\nt.mak clean
nmake -f ms\nt.mak
nmake -f ms\nt.mak install

::perl Configure VC-WIN32 --prefix=C:\Openssl32

:: nasm and this command not need in v1.1+, just run nmake 
::ms\do_win64a
::ms\do_nasm.bat
::nmake -f ms\nt.mak clean
::nmake -f ms\nt.mak 
::nmake -f ms\nt.mak test
::nmake -f ms\nt.mak install
::copy inc32/include C:\Openssl\include
::copy out32/libeay32.lib C:\Openssl\lib
::copy out32/ssleay32.lib C:\Openssl\lib
