:: build libssh 
:: go to libssh source code directory
:: this script directory
set DIR=%~dp0
set DIR=%DIR:~0,-1%

mkdir build && cd build
set "MSVC=Visual Studio 16 2019"
rem set OPENSSL_DIR=C:/Users/Sant/Documents/golddrive/src/lib/openssl/lib/x64
rem set OPENSSL_DIR=C:/Openssl
rem set OPENSSL_DIR=C:/Users/Sant/Documents/winlibs/prefix/openssl-x64/lib
set OPENSSL_DIR=C:/openssl
ECHO "Building with platform %MSVC%"
cmake .. 								^
 -A x64 								^
 -G"%MSVC%"                             ^
 -DCMAKE_INSTALL_PREFIX="C:/libssh-x64"	^
 -DCMAKE_BUILD_TYPE=Release 			^
 -DBUILD_SHARED_LIBS=ON          		^
 -DOPENSSL_ROOT_DIR=%OPENSSL_DIR%       ^
 -DWITH_ZLIB=OFF 						^
 -DWITH_SERVER=OFF 						^
 -DWITH_EXAMPLES=OFF 					

:: -DCMAKE_C_STANDARD_LIBRARIES="crypt32.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib " ^
:: -DWITH_DEBUG_CALLTRACE=OFF ^
:: -DWITH_SYMBOL_VERSIONING=OFF ^ 
:: -DZLIB_LIBRARY="C:\zlib\lib"              
:: -DZLIB_INCLUDE_DIR="C:\zlib\include"              

cmake --build . --config Release --target install 
::-- /clp:ErrorsOnly

cd ..

