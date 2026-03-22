:: Golddrive
:: 09/08/2018, San
:: Build libssh2 x64
:: Usage: build_ssh2.bat
@echo off
setlocal
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%
set VERSION=1.11.1
set TEMP=C:\Temp

:: Visual Studio 2022 configuration
set VCVARSALL="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
set CMAKE_GENERATOR="Visual Studio 17 2022"

echo ========================================
echo Building libssh2 %VERSION% for x64
echo ========================================

:: Clean up previous build
rd /s /q C:\libssh2-x64 2>nul

:: Download and extract libssh2
cd %TEMP%

:: Check if zip file already exists
if exist "libssh2-%VERSION%.zip" (
    echo libssh2 %VERSION% zip file already exists, skipping download...
) else (
    echo Downloading libssh2 %VERSION%...
    curl -L -O https://github.com/libssh2/libssh2/archive/refs/tags/libssh2-%VERSION%.zip
    if errorlevel 1 (
        echo Error: Failed to download libssh2
        cd %CWD%
        exit /b 1
    )
)

rd /s /q libssh2-libssh2-%VERSION% 2>nul
echo Extracting libssh2 %VERSION%...
tar xf libssh2-%VERSION%.zip
if errorlevel 1 (
    echo Error: Failed to extract libssh2
    cd %CWD%
    exit /b 1
)
cd libssh2-libssh2-%VERSION%

call %VCVARSALL% x64
if errorlevel 1 (
    echo Error: Failed to initialize Visual Studio environment for x64
    cd %CWD%
    exit /b 1
)
mkdir build_x64
cd build_x64
cmake .. ^
 -G %CMAKE_GENERATOR% ^
 -A x64 ^
 -DCMAKE_INSTALL_PREFIX="C:/libssh2-x64"        ^
 -DCMAKE_BUILD_TYPE=Release                     ^
 -DCRYPTO_BACKEND=OpenSSL                       ^
 -DBUILD_SHARED_LIBS=OFF                        ^
 -DOPENSSL_ROOT_DIR=C:/openssl-x64              ^
 -DENABLE_ZLIB_COMPRESSION=OFF                  ^
 -DBUILD_TESTING=OFF                            ^
 -DBUILD_EXAMPLES=OFF                           ^
 -DENABLE_CRYPT_NONE=ON                         ^
 -DCLEAR_MEMORY=OFF
if errorlevel 1 (
    echo Error: CMake configuration failed
    cd %CWD%
    exit /b 1
)
cmake --build . --config Release --target install
if errorlevel 1 (
    echo Error: CMake build failed
    cd %CWD%
    exit /b 1
)
xcopy C:\libssh2-x64\lib\libssh2.lib %DIR%\..\vendor\libssh2\lib\x64\libssh2.lib* /y /s /i
xcopy C:\libssh2-x64\include %DIR%\..\vendor\libssh2\include /y /s /i
cd ..\..

:: Cleanup
echo.
echo Cleaning up temporary files...
rd /s /q libssh2-libssh2-%VERSION% 2>nul
cd %CWD%

echo.
echo ========================================
echo Build complete
echo ========================================
echo x64 libraries: %DIR%\..\vendor\libssh2\lib\x64\
echo Headers:       %DIR%\..\vendor\libssh2\include\
echo ========================================
