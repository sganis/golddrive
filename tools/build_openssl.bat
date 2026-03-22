:: Golddrive
:: 09/08/2018, San
:: Build openssl x64
:: Usage: build_openssl.bat
@echo off
setlocal
set VERSION=3.6.0
set TEMP=C:\Temp
set DIR=%~dp0
set DIR=%DIR:~0,-1%
set CWD=%CD%

:: Visual Studio 2022 configuration
set VCVARSALL="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

echo ========================================
echo Building OpenSSL %VERSION% for x64
echo ========================================

:: Clean up previous build
rd /s /q C:\openssl-x64 2>nul

:: Download and extract OpenSSL
cd %TEMP%

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

cd openssl-openssl-%VERSION%
set PATH=C:\Program Files\NASM;C:\Strawberry\perl\bin;C:\Windows\System32;C:\Windows
call %VCVARSALL% x64
if errorlevel 1 (
    echo Error: Failed to initialize Visual Studio environment
    cd %CWD%
    exit /b 1
)
perl Configure                  ^
    VC-WIN64A                   ^
    no-shared                   ^
    --prefix=C:\openssl-x64     ^
    --openssldir=C:\openssl-x64
if errorlevel 1 (
    echo Error: OpenSSL Configure failed
    cd %CWD%
    exit /b 1
)
nmake build_generated
if errorlevel 1 (
    echo Error: nmake build_generated failed
    cd %CWD%
    exit /b 1
)
nmake libcrypto.lib
if errorlevel 1 (
    echo Error: nmake libcrypto.lib failed
    cd %CWD%
    exit /b 1
)
nmake install_dev
if errorlevel 1 (
    echo Error: nmake install_dev failed
    cd %CWD%
    exit /b 1
)
xcopy C:\openssl-x64\lib\libcrypto.lib %DIR%\..\vendor\openssl\lib\x64\libcrypto.lib* /y /s /i
xcopy C:\openssl-x64\include %DIR%\..\vendor\openssl\include /y /s /i
cd ..

:: Cleanup
echo.
echo Cleaning up temporary files...
rd /s /q openssl-openssl-%VERSION% 2>nul
cd %CWD%

echo.
echo ========================================
echo Build complete
echo ========================================
echo x64 libraries: %DIR%\..\vendor\openssl\lib\x64\
echo Headers:       %DIR%\..\vendor\openssl\include\
echo ========================================
