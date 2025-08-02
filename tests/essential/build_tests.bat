@echo off
REM Essential Tests Build Script for SystemMonitor
REM Builds only the core technology validation tests

echo ====================================================
echo  SystemMonitor Essential Tests - Build Script
echo ====================================================

REM Use the same environment variables as main build
if not defined VS_BUILD_TOOLS_PATH (
    set "VS_BUILD_TOOLS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
)

if not defined VCPKG_ROOT (
    set "VCPKG_ROOT=c:\vcpkg"
)

if not defined VCPKG_TARGET (
    set "VCPKG_TARGET=x64-windows"
)

echo Configuration:
echo   Visual Studio: %VS_BUILD_TOOLS_PATH%
echo   vcpkg Root:    %VCPKG_ROOT%
echo   Target:        %VCPKG_TARGET%
echo.

REM Check dependencies
if not exist "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include\curl\curl.h" (
    echo ERROR: libcurl not found. Essential tests require libcurl.
    echo Please install with: vcpkg install curl[ssl]:%VCPKG_TARGET%
    pause
    exit /b 1
)

if not exist "%VS_BUILD_TOOLS_PATH%\VC\Auxiliary\Build\vcvars64.bat" (
    echo ERROR: Visual Studio BuildTools not found at %VS_BUILD_TOOLS_PATH%
    pause
    exit /b 1
)

echo Setting up Visual Studio x64 environment...
call "%VS_BUILD_TOOLS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

echo.
echo Building Essential Tests...
echo.

REM Build libcurl email test (requires libcurl)
echo [1/3] Building libcurl email test...
cl /EHsc /std:c++17 libcurl_email_test.cpp ^
   /I"%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include" ^
   /link /LIBPATH:"%VCPKG_ROOT%\installed\%VCPKG_TARGET%\lib" ^
   libcurl.lib ws2_32.lib wldap32.lib advapi32.lib crypt32.lib normaliz.lib

if %ERRORLEVEL% NEQ 0 (
    echo ❌ libcurl email test build failed!
    goto :cleanup
)

REM Build integration status test (no external deps)
echo [2/3] Building integration status test...
cl /EHsc /std:c++17 integration_status.cpp

if %ERRORLEVEL% NEQ 0 (
    echo ❌ Integration status test build failed!
    goto :cleanup
)

REM Build configuration test (no external deps)
echo [3/3] Building configuration test...
cl /EHsc /std:c++17 config_email_test.cpp

if %ERRORLEVEL% NEQ 0 (
    echo ❌ Configuration test build failed!
    goto :cleanup
)

echo.
echo ✅ All essential tests built successfully!
echo.
echo Available test executables:
echo   - libcurl_email_test.exe    (TLS Email Integration)
echo   - integration_status.exe    (System Integration Status)
echo   - config_email_test.exe     (Configuration Validation)
echo.
echo To run all tests: run_essential_tests.bat
echo To run individual test: [test_name].exe
echo.

:cleanup
echo Cleaning up object files...
del /q *.obj 2>nul

if %ERRORLEVEL% EQU 0 (
    echo.
    echo 🎯 Essential tests are ready!
    echo These tests validate the core SystemMonitor technologies.
) else (
    echo.
    echo ❌ Some tests failed to build. Check error messages above.
)

echo.
pause
