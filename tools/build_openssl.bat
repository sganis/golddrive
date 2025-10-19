:: Golddrive
:: 09/08/2018, San
:: Build openssl with architecture selection
:: Usage: build_openssl.bat [x64|x86|arm64|all]
@echo off
setlocal
set VERSION=3.6.0
set TEMP=C:\Temp
set BASE_PATH=C:\Strawberry\perl\bin;C:\Windows\System32;C:\Windows
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%

:: Visual Studio 2022 configuration
set VCVARSALL="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

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
echo Building OpenSSL %VERSION% for: %ARCH%
echo ========================================

:: Clean up previous builds
if /i "%ARCH%"=="x64" rd /s /q C:\openssl-x64 2>nul
if /i "%ARCH%"=="x86" rd /s /q C:\openssl-x86 2>nul
if /i "%ARCH%"=="arm64" rd /s /q C:\openssl-arm64 2>nul
if /i "%ARCH%"=="all" (
    rd /s /q C:\openssl-x64 2>nul
    rd /s /q C:\openssl-x86 2>nul
    rd /s /q C:\openssl-arm64 2>nul
)

:: Download and extract OpenSSL
cd %TEMP%

:: Check if zip file already exists
if exist "openssl-%VERSION%.zip" (
    echo OpenSSL %VERSION% zip file already exists, skipping download...
) else (
    echo Downloading OpenSSL %VERSION%...
    curl -L -O https://github.com/openssl/openssl/archive/refs/tags/openssl-%VERSION%.zip
    if errorlevel 1 (
        echo Error: Failed to download OpenSSL
        cd %CWD%
        exit /b 1
    )
)

rd /s /q openssl-openssl-%VERSION% 2>nul
echo Extracting OpenSSL %VERSION%...
tar xf openssl-%VERSION%.zip
if errorlevel 1 (
    echo Error: Failed to extract OpenSSL
    cd %CWD%
    exit /b 1
)

:: Create directories for requested architectures
if /i "%ARCH%"=="x64" xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-x64 /s /e /i /y /q
if /i "%ARCH%"=="x86" xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-x86 /s /e /i /y /q
if /i "%ARCH%"=="arm64" xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-arm64 /s /e /i /y /q
if /i "%ARCH%"=="all" (
    xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-x64 /s /e /i /y /q
    xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-x86 /s /e /i /y /q
    xcopy openssl-openssl-%VERSION% openssl-openssl-%VERSION%-arm64 /s /e /i /y /q
)

:: ====================================
:: Build x64 (64-bit Intel/AMD)
:: ====================================
if /i "%ARCH%"=="x64" goto build_x64
if /i "%ARCH%"=="all" goto build_x64
goto skip_x64

:build_x64
echo.
echo ========================================
echo Building OpenSSL for x64...
echo ========================================
cd openssl-openssl-%VERSION%-x64
:: Set PATH with NASM for x86/x64 builds
set PATH=C:\Program Files\NASM;%BASE_PATH%
call %VCVARSALL% x64
if errorlevel 1 (
    echo Error: Failed to initialize Visual Studio environment for x64
    cd %CWD%
    exit /b 1
)
perl Configure                  ^
    VC-WIN64A                   ^
    no-shared                   ^
    --api=1.1.0                 ^
    no-deprecated               ^
    --prefix=C:\openssl-x64     ^
    --openssldir=C:\openssl-x64
if errorlevel 1 (
    echo Error: OpenSSL Configure failed for x64
    cd %CWD%
    exit /b 1
)
nmake build_generated
if errorlevel 1 (
    echo Error: nmake build_generated failed for x64
    cd %CWD%
    exit /b 1
)
nmake libcrypto.lib
if errorlevel 1 (
    echo Error: nmake libcrypto.lib failed for x64
    cd %CWD%
    exit /b 1
)
nmake install_dev
if errorlevel 1 (
    echo Error: nmake install_dev failed for x64
    cd %CWD%
    exit /b 1
)
xcopy C:\openssl-x64\lib\libcrypto.lib %DIR%\..\vendor\openssl\lib\x64\libcrypto.lib* /y /s /i
xcopy C:\openssl-x64\include %DIR%\..\vendor\openssl\include /y /s /i
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
echo Building OpenSSL for x86...
echo ========================================
cd openssl-openssl-%VERSION%-x86
:: Set PATH with NASM for x86/x64 builds
set PATH=C:\Program Files\NASM;%BASE_PATH%
call %VCVARSALL% x86
if errorlevel 1 (
    echo Error: Failed to initialize Visual Studio environment for x86
    cd %CWD%
    exit /b 1
)
perl Configure                  ^
    VC-WIN32                    ^
    no-shared                   ^
    --api=1.1.0                 ^
    no-deprecated               ^
    --prefix=C:\openssl-x86     ^
    --openssldir=C:\openssl-x86
if errorlevel 1 (
    echo Error: OpenSSL Configure failed for x86
    cd %CWD%
    exit /b 1
)
nmake build_generated
if errorlevel 1 (
    echo Error: nmake build_generated failed for x86
    cd %CWD%
    exit /b 1
)
nmake libcrypto.lib
if errorlevel 1 (
    echo Error: nmake libcrypto.lib failed for x86
    cd %CWD%
    exit /b 1
)
nmake install_dev
if errorlevel 1 (
    echo Error: nmake install_dev failed for x86
    cd %CWD%
    exit /b 1
)
xcopy C:\openssl-x86\lib\libcrypto.lib %DIR%\..\vendor\openssl\lib\x86\libcrypto.lib* /y /s /i
xcopy C:\openssl-x86\include %DIR%\..\vendor\openssl\include /y /s /i
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
echo Building OpenSSL for ARM64...
echo ========================================
cd openssl-openssl-%VERSION%-arm64
:: Set PATH without NASM for ARM64 (NASM is x86/x64 only)
set PATH=%BASE_PATH%
call %VCVARSALL% amd64_arm64
if errorlevel 1 (
    echo Error: Failed to initialize Visual Studio environment for ARM64
    echo Make sure you have "C++ ARM64 build tools" installed in Visual Studio
    cd %CWD%
    exit /b 1
)
echo Note: NASM not needed for ARM64 build (x86/x64 only)
perl Configure                  ^
    VC-WIN64-ARM                ^
    no-shared                   ^
    --api=1.1.0                 ^
    no-deprecated               ^
    --prefix=C:\openssl-arm64   ^
    --openssldir=C:\openssl-arm64
if errorlevel 1 (
    echo Error: OpenSSL Configure failed for ARM64
    cd %CWD%
    exit /b 1
)
nmake build_generated
if errorlevel 1 (
    echo Error: nmake build_generated failed for ARM64
    cd %CWD%
    exit /b 1
)
nmake libcrypto.lib
if errorlevel 1 (
    echo Error: nmake libcrypto.lib failed for ARM64
    cd %CWD%
    exit /b 1
)
nmake install_dev
if errorlevel 1 (
    echo Error: nmake install_dev failed for ARM64
    cd %CWD%
    exit /b 1
)
xcopy C:\openssl-arm64\lib\libcrypto.lib %DIR%\..\vendor\openssl\lib\arm64\libcrypto.lib* /y /s /i
xcopy C:\openssl-arm64\include %DIR%\..\vendor\openssl\include /y /s /i
cd ..

:skip_arm64

:: Cleanup
echo.
echo Cleaning up temporary files...
rd /s /q openssl-openssl-%VERSION% 2>nul
rd /s /q openssl-openssl-%VERSION%-x64 2>nul
rd /s /q openssl-openssl-%VERSION%-x86 2>nul
rd /s /q openssl-openssl-%VERSION%-arm64 2>nul
cd %CWD%

echo.
echo ========================================
echo Build complete for: %ARCH%
echo ========================================
if /i "%ARCH%"=="x64" echo x64 libraries:   %DIR%\..\vendor\openssl\lib\x64\
if /i "%ARCH%"=="x86" echo x86 libraries:   %DIR%\..\vendor\openssl\lib\x86\
if /i "%ARCH%"=="arm64" echo ARM64 libraries: %DIR%\..\vendor\openssl\lib\arm64\
if /i "%ARCH%"=="all" (
    echo x64 libraries:   %DIR%\..\vendor\openssl\lib\x64\
    echo x86 libraries:   %DIR%\..\vendor\openssl\lib\x86\
    echo ARM64 libraries: %DIR%\..\vendor\openssl\lib\arm64\
)
echo Headers:         %DIR%\..\vendor\openssl\include\
echo ========================================