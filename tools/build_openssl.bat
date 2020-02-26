::mkdir build && cd build
perl Configure VC-WIN64A --prefix=C:\Openssl2
ms\do_win64a
::nmake -f ms\nt.mak clean
nmake -f ms\nt.mak 
::nmake -f ms\nt.mak test
::nmake -f ms\nt.mak install
copy inc32/include C:\Openssl\include
copy out32/libeay32.lib C:\Openssl\lib
copy out32/ssleay32.lib C:\Openssl\lib
