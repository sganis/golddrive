:: Download http://zlib.net/zlib1211.zip
mkdir build && cd build

set "MSVC=Visual Studio 16 2019"

ECHO "Building with platform %MSVC%"
cmake ..                                         ^
    -G"%MSVC%"                                   ^
    -DCMAKE_INSTALL_PREFIX="C:\zlib"             ^
    -DCMAKE_BUILD_TYPE=Release                   ^
    -DBUILD_SHARED_LIBS=OFF

cmake --build . --config Release --target install
::copy C:/zlib/lib/zlibstatic.lib ../lib/
::cd ..
::rd /s zlib-1.2.11
