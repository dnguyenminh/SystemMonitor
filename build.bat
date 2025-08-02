@echo off
REM SystemMonitor Build Script with libcurl TLS Integration
REM This script compiles SystemMonitor with secure email capabilities

echo ====================================================
echo  SystemMonitor Build Script - libcurl TLS Edition
echo ====================================================

REM =================================================================
REM Configuration Variables - Modify these paths for your environment
REM =================================================================
REM Visual Studio BuildTools installation path
if not defined VS_BUILD_TOOLS_PATH (
    set "VS_BUILD_TOOLS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
)

REM vcpkg installation path - normalize forward slashes to backslashes for Windows
if not defined VCPKG_ROOT (
    set "VCPKG_ROOT=c:\vcpkg"
) else (
    REM Replace forward slashes with backslashes for Windows path compatibility
    set "VCPKG_ROOT=%VCPKG_ROOT:/=\%"
)

REM Target platform for vcpkg
if not defined VCPKG_TARGET (
    set "VCPKG_TARGET=x64-windows-static"
)

REM Output directory for compiled executable
if not defined OUTPUT_DIR (
    set "OUTPUT_DIR=bin"
)

REM Executable name
if not defined EXE_NAME (
    set "EXE_NAME=SystemMonitor.exe"
)

echo Configuration:
echo   Visual Studio: "%VS_BUILD_TOOLS_PATH%"
echo   vcpkg Root:    "%VCPKG_ROOT%"
echo   Target:        "%VCPKG_TARGET%"
echo   Output Dir:    "%OUTPUT_DIR%"
echo.

REM Debug: Show the exact include path that will be used
echo Debug: Include path will be "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include"
echo Debug: Checking if curl.h exists at "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include\curl\curl.h"
if exist "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include\curl\curl.h" (
    echo Debug: ✅ curl.h found!
) else (
    echo Debug: ❌ curl.h NOT found!
    echo Debug: Listing contents of include directory:
    if exist "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include" (
        dir "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include" /B
    ) else (
        echo Debug: Include directory does not exist
    )
)
echo.

REM Check if vcpkg libcurl is available
if not exist "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include\curl\curl.h" (
    echo ERROR: libcurl not found at "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include\curl\curl.h"
    echo Please install with: vcpkg install curl[ssl]:%VCPKG_TARGET%
    echo.
    echo Make sure VCPKG_ROOT is set correctly: "%VCPKG_ROOT%"
    pause
    exit /b 1
)

REM Check if Visual Studio BuildTools exists
if not exist "%VS_BUILD_TOOLS_PATH%\VC\Auxiliary\Build\vcvars64.bat" (
    echo ERROR: Visual Studio BuildTools not found at "%VS_BUILD_TOOLS_PATH%"
    echo Please set VS_BUILD_TOOLS_PATH to your Visual Studio installation directory
    pause
    exit /b 1
)

REM Check if Visual Studio environment is already configured (for CI/CD)
if defined VCINSTALLDIR (
    echo Visual Studio x64 environment already configured
    echo VCINSTALLDIR: %VCINSTALLDIR%
) else (
    echo Setting up Visual Studio x64 environment...
    call "%VS_BUILD_TOOLS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
)

REM Create output directory if it doesn't exist
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo.
echo Compiling SystemMonitor with libcurl TLS integration...
echo.

REM Debug: Show compiler environment and paths
echo Debug: Compiler executable: 
where cl 2>nul || echo Debug: cl.exe not found in PATH
echo Debug: Original INCLUDE environment variable:
echo INCLUDE=%INCLUDE%
echo Debug: Original LIB environment variable:
echo LIB=%LIB%

REM Add vcpkg paths to environment variables for additional safety
set "VCPKG_INCLUDE_PATH=%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include"
set "VCPKG_LIB_PATH=%VCPKG_ROOT%\installed\%VCPKG_TARGET%\lib"

if defined INCLUDE (
    set "INCLUDE=%INCLUDE%;%VCPKG_INCLUDE_PATH%"
) else (
    set "INCLUDE=%VCPKG_INCLUDE_PATH%"
)

if defined LIB (
    set "LIB=%LIB%;%VCPKG_LIB_PATH%"
) else (
    set "LIB=%VCPKG_LIB_PATH%"
)

echo Debug: Updated INCLUDE environment variable:
echo INCLUDE=%INCLUDE%
echo Debug: Updated LIB environment variable:
echo LIB=%LIB%
echo.

REM Compile with libcurl TLS support (STATIC LINKING)
cl /EHsc /std:c++17 /DWIN32_LEAN_AND_MEAN /DCURL_STATICLIB ^
   main.cpp ^
   src/SystemMetrics.cpp ^
   src/ProcessManager.cpp ^
   src/Logger.cpp ^
   src/EmailNotifier.cpp ^
   src/Configuration.cpp ^
   src/ConsoleDisplay.cpp ^
   src/SystemMonitor.cpp ^
   /I"%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include" ^
   /link /LIBPATH:"%VCPKG_ROOT%\installed\%VCPKG_TARGET%\lib" ^
   libcurl.lib zlib.lib ^
   ws2_32.lib wldap32.lib advapi32.lib crypt32.lib normaliz.lib user32.lib kernel32.lib ^
   iphlpapi.lib secur32.lib ^
   /OUT:"%OUTPUT_DIR%\%EXE_NAME%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ Build failed! Check for missing dependencies or compilation errors.
    echo Note: Full SystemMonitor compilation requires all factory implementations.
    echo.
    pause
    exit /b 1
)

REM Clean up object files after successful compilation
echo Cleaning up object files...
del /q *.obj 2>nul

echo.
echo ✅ SystemMonitor with libcurl TLS integration compiled successfully!
echo Executable: "%OUTPUT_DIR%\%EXE_NAME%"
echo.
echo Features included:
echo   - Real-time system monitoring
echo   - Email alerts with libcurl TLS encryption
echo   - Gmail SMTP with App Password authentication
echo   - Professional alert and recovery notifications
echo.
echo To run: ".\%OUTPUT_DIR%\%EXE_NAME%"
echo Config: config\SystemMonitor.cfg
echo.
pause
