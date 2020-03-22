::mkdir build && cd build
perl Configure no-shared VC-WIN64A --prefix=C:\Openssl
nmake
nmake install

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
