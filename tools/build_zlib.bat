mkdir build && cd build

set "MSVC=Visual Studio 16 2019"

set ARCH=Win32
set PLAT=x86

:: zlib
:: Download http://zlib.net/zlib1211.zip
cmake ..                                         ^
    -A %ARCH% 									 ^
    -G"%MSVC%"                                   ^
    -DCMAKE_INSTALL_PREFIX="C:\zlib-%PLAT%"      ^
    -DBUILD_SHARED_LIBS=OFF

:: cmake --build . --config Release --target install
::copy C:/zlib/lib/zlibstatic.lib ../lib/
::cd ..
::rd /s zlib-1.2.11
