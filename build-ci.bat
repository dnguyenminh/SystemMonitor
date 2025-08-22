@echo off
REM SystemMonitor Build Script for CI/CD
REM Updated: Static externals, dynamic CRT/system

echo ====================================================
echo  SystemMonitor CI/CD Build Script
echo ====================================================

REM =================================================================
REM Configuration Variables - Override with environment variables
REM =================================================================
if not defined VCPKG_ROOT (
    set "VCPKG_ROOT=c:\vcpkg"
)

if not defined VCPKG_TARGET (
    set "VCPKG_TARGET=x64-windows-static"
)

if not defined OUTPUT_DIR (
    set "OUTPUT_DIR=bin"
)

if not defined EXE_NAME (
    set "EXE_NAME=SystemMonitor.exe"
)

echo Configuration:
echo   vcpkg Root:    "%VCPKG_ROOT%"
echo   Target:        "%VCPKG_TARGET%"
echo   Output Dir:    "%OUTPUT_DIR%"
echo.

REM Check if vcpkg libcurl is available
if not exist "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include\curl\curl.h" (
    echo ERROR: libcurl not found at "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include\curl\curl.h"
    echo Please install with: vcpkg install curl[ssl]:%VCPKG_TARGET%
    echo.
    echo Make sure VCPKG_ROOT is set correctly: "%VCPKG_ROOT%"
    exit /b 1
)

REM Check if Visual Studio environment is already set up (for CI)
if defined VCINSTALLDIR (
    echo Note: Using pre-configured Visual Studio environment
    echo Environment: %VCINSTALLDIR%
    goto :compile
)

REM For local testing, try to find and set up Visual Studio
echo Setting up local Visual Studio x64 environment...
if not defined VS_BUILD_TOOLS_PATH (
    set VS_BUILD_TOOLS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools
)

if exist "%VS_BUILD_TOOLS_PATH%\VC\Auxiliary\Build\vcvars64.bat" (
    echo Calling vcvars64.bat from "%VS_BUILD_TOOLS_PATH%"
    call "%VS_BUILD_TOOLS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo WARNING: Failed to set up Visual Studio environment
    )
) else (
    echo WARNING: Visual Studio environment not found. Assuming CI environment.
)

:compile
REM Create output directory if it doesn't exist
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo.
echo Compiling SystemMonitor with libcurl TLS integration...
echo.

REM Compile with libcurl TLS support
REM - /MD = dynamic CRT
REM - VCPKG_TARGET=x64-windows-static ensures externals are static
cl /EHsc /std:c++17 /MD /DWIN32_LEAN_AND_MEAN /DCURL_STATICLIB ^
   main.cpp ^
   src/SystemMetrics.cpp ^
   src/ProcessManager.cpp ^
   src/Logger.cpp ^
   src/EmailNotifier.cpp ^
   src/Configuration.cpp ^
   src/ConsoleDisplay.cpp ^
   src/SystemMonitor.cpp ^
   src/SystemInfo.cpp ^
   /I"%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include" ^
   /link /LIBPATH:"%VCPKG_ROOT%\installed\%VCPKG_TARGET%\lib" ^
   libcurl.lib zlib.lib ^
   ws2_32.lib wldap32.lib advapi32.lib crypt32.lib normaliz.lib user32.lib kernel32.lib ^
   iphlpapi.lib secur32.lib ^
   /OUT:"%OUTPUT_DIR%\%EXE_NAME%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ Build failed! Check for missing dependencies or compilation errors.
    echo.
    exit /b 1
)

REM Clean up object files after successful compilation
echo Cleaning up object files...
del /q *.obj 2>nul

echo.
echo ✅ SystemMonitor built successfully!
echo Executable: "%OUTPUT_DIR%\%EXE_NAME%"
echo.
echo Features included:
echo   - Real-time system monitoring
echo   - Email alerts with libcurl TLS encryption
echo   - Gmail SMTP with App Password authentication
echo   - Professional alert and recovery notifications
echo.