:: Golddrive
:: 09/08/2018, San
:: Build libssh2 with architecture selection
:: Usage: build_libssh2.bat [x64|x86|arm64|all]
@echo off
setlocal
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%
set VERSION=1.11.0
set TEMP=C:\Temp

:: Visual Studio 2022 configuration
set VCVARSALL="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
set CMAKE_GENERATOR="Visual Studio 17 2022"

:: Parse architecture argument (default to "all")
set ARCH=%1
if "%ARCH%"=="" set ARCH=all

:: Validate architecture argument
if /i not "%ARCH%"=="x64" if /i not "%ARCH%"=="x86" if /i not "%ARCH%"=="arm64" if /i not "%ARCH%"=="all" (
    echo Invalid architecture: %ARCH%
    echo Usage: %~nx0 [x64^|x86^|arm64^|all]
    echo   x64   - Build 64-bit Intel/AMD only
    echo   x86   - Build 32-bit Intel/AMD only
    echo   arm64 - Build ARM64 only
    echo   all   - Build all architectures ^(default^)
    exit /b 1
)

echo ========================================
echo Building libssh2 %VERSION% for: %ARCH%
echo ========================================

:: Clean up previous builds
if /i "%ARCH%"=="x64" rd /s /q C:\libssh2-x64 2>nul
if /i "%ARCH%"=="x86" rd /s /q C:\libssh2-x86 2>nul
if /i "%ARCH%"=="arm64" rd /s /q C:\libssh2-arm64 2>nul
if /i "%ARCH%"=="all" (
    rd /s /q C:\libssh2-x64 2>nul
    rd /s /q C:\libssh2-x86 2>nul
    rd /s /q C:\libssh2-arm64 2>nul
)

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

:: ====================================
:: Build x64 (64-bit Intel/AMD)
:: ====================================
if /i "%ARCH%"=="x64" goto build_x64
if /i "%ARCH%"=="all" goto build_x64
goto skip_x64

:build_x64
echo.
echo ========================================
echo Building libssh2 for x64...
echo ========================================
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
    echo Error: CMake configuration failed for x64
    cd %CWD%
    exit /b 1
)
cmake --build . --config Release --target install
if errorlevel 1 (
    echo Error: CMake build failed for x64
    cd %CWD%
    exit /b 1
)
xcopy C:\libssh2-x64\lib\libssh2.lib %DIR%\..\vendor\libssh2\lib\x64\libssh2.lib* /y /s /i
xcopy C:\libssh2-x64\include %DIR%\..\vendor\libssh2\include /y /s /i
cd ..

:skip_x64

:: ====================================
:: Build x86 (32-bit Intel/AMD)
:: ====================================
if /i "%ARCH%"=="x86" goto build_x86
if /i "%ARCH%"=="all" goto build_x86
goto skip_x86

:build_x86
echo.
echo ========================================
echo Building libssh2 for x86...
echo ========================================
call %VCVARSALL% x86
if errorlevel 1 (
    echo Error: Failed to initialize Visual Studio environment for x86
    cd %CWD%
    exit /b 1
)
mkdir build_x86
cd build_x86
cmake .. ^
 -G %CMAKE_GENERATOR% ^
 -A Win32 ^
 -DCMAKE_INSTALL_PREFIX="C:/libssh2-x86"        ^
 -DCMAKE_BUILD_TYPE=Release                     ^
 -DCRYPTO_BACKEND=OpenSSL                       ^
 -DBUILD_SHARED_LIBS=OFF                        ^
 -DOPENSSL_ROOT_DIR=C:/openssl-x86              ^
 -DENABLE_ZLIB_COMPRESSION=OFF                  ^
 -DBUILD_TESTING=OFF                            ^
 -DBUILD_EXAMPLES=OFF                           ^
 -DENABLE_CRYPT_NONE=ON                         ^
 -DCLEAR_MEMORY=OFF
if errorlevel 1 (
    echo Error: CMake configuration failed for x86
    cd %CWD%
    exit /b 1
)
cmake --build . --config Release --target install
if errorlevel 1 (
    echo Error: CMake build failed for x86
    cd %CWD%
    exit /b 1
)
xcopy C:\libssh2-x86\lib\libssh2.lib %DIR%\..\vendor\libssh2\lib\x86\libssh2.lib* /y /s /i
xcopy C:\libssh2-x86\include %DIR%\..\vendor\libssh2\include /y /s /i
cd ..

:skip_x86

:: ====================================
:: Build ARM64
:: ====================================
if /i "%ARCH%"=="arm64" goto build_arm64
if /i "%ARCH%"=="all" goto build_arm64
goto skip_arm64

:build_arm64
echo.
echo ========================================
echo Building libssh2 for ARM64...
echo ========================================
call %VCVARSALL% amd64_arm64
if errorlevel 1 (
    echo Error: Failed to initialize Visual Studio environment for ARM64
    echo Make sure you have "C++ ARM64 build tools" installed in Visual Studio
    cd %CWD%
    exit /b 1
)
mkdir build_arm64
cd build_arm64
cmake .. ^
 -G %CMAKE_GENERATOR% ^
 -A ARM64 ^
 -DCMAKE_INSTALL_PREFIX="C:/libssh2-arm64"      ^
 -DCMAKE_BUILD_TYPE=Release                     ^
 -DCRYPTO_BACKEND=OpenSSL                       ^
 -DBUILD_SHARED_LIBS=OFF                        ^
 -DOPENSSL_ROOT_DIR=C:/openssl-arm64            ^
 -DENABLE_ZLIB_COMPRESSION=OFF                  ^
 -DBUILD_TESTING=OFF                            ^
 -DBUILD_EXAMPLES=OFF                           ^
 -DENABLE_CRYPT_NONE=ON                         ^
 -DCLEAR_MEMORY=OFF
if errorlevel 1 (
    echo Error: CMake configuration failed for ARM64
    cd %CWD%
    exit /b 1
)
cmake --build . --config Release --target install
if errorlevel 1 (
    echo Error: CMake build failed for ARM64
    cd %CWD%
    exit /b 1
)
xcopy C:\libssh2-arm64\lib\libssh2.lib %DIR%\..\vendor\libssh2\lib\arm64\libssh2.lib* /y /s /i
xcopy C:\libssh2-arm64\include %DIR%\..\vendor\libssh2\include /y /s /i
cd ..

:skip_arm64

:: Cleanup
echo.
echo Cleaning up temporary files...
cd %TEMP%
rd /s /q libssh2-libssh2-%VERSION% 2>nul
cd %CWD%

echo.
echo ========================================
echo Build complete for: %ARCH%
echo ========================================
if /i "%ARCH%"=="x64" echo x64 libraries:   %DIR%\..\vendor\libssh2\lib\x64\
if /i "%ARCH%"=="x86" echo x86 libraries:   %DIR%\..\vendor\libssh2\lib\x86\
if /i "%ARCH%"=="arm64" echo ARM64 libraries: %DIR%\..\vendor\libssh2\lib\arm64\
if /i "%ARCH%"=="all" (
    echo x64 libraries:   %DIR%\..\vendor\libssh2\lib\x64\
    echo x86 libraries:   %DIR%\..\vendor\libssh2\lib\x86\
    echo ARM64 libraries: %DIR%\..\vendor\libssh2\lib\arm64\
)
echo Headers:         %DIR%\..\vendor\libssh2\include\
echo ========================================