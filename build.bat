@echo off
setlocal enabledelayedexpansion
REM SystemMonitor Build Script with libcurl TLS Integration
REM This script compiles SystemMonitor with secure email capabilities

echo ====================================================
echo  SystemMonitor Build Script - libcurl TLS Edition
echo ====================================================

REM =================================================================
REM Configuration Variables - Modify these paths for your environment
REM =================================================================
REM Visual Studio installation path - try multiple common locations
if not defined VS_BUILD_TOOLS_PATH (
    REM Try Enterprise first (GitHub Actions)
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_BUILD_TOOLS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_BUILD_TOOLS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_BUILD_TOOLS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_BUILD_TOOLS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
    ) else (
        set "VS_BUILD_TOOLS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
    )
)

REM vcpkg installation path - normalize forward slashes to backslashes for Windows
if not defined VCPKG_ROOT (
    REM Default fallback - prefer workspace vcpkg if it exists
    if exist "%~dp0vcpkg" (
        set "VCPKG_ROOT=%~dp0vcpkg"
        echo Using workspace vcpkg: "%~dp0vcpkg"
    ) else if exist "c:\vcpkg" (
        set "VCPKG_ROOT=c:\vcpkg"
        echo Using system vcpkg: "c:\vcpkg"
    ) else (
        echo ERROR: No vcpkg installation found. Please set VCPKG_ROOT environment variable.
        exit /b 1
    )
) else (
    REM Replace forward slashes with backslashes for Windows path compatibility
    set "VCPKG_ROOT=%VCPKG_ROOT:/=\%"
    echo Using environment vcpkg: "%VCPKG_ROOT%"
)

REM Target platform for vcpkg (force externals to static link)
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

REM Validate that we're not using VS internal vcpkg
if "%VCPKG_ROOT%" == "" (
    echo ERROR: VCPKG_ROOT is empty
    exit /b 1
)

REM Check if we're accidentally using Visual Studio's internal vcpkg
echo "%VCPKG_ROOT%" | findstr /C:"Microsoft Visual Studio" >nul
if %ERRORLEVEL% EQU 0 (
    echo ERROR: Detected Visual Studio internal vcpkg path: "%VCPKG_ROOT%"
    echo This will cause build issues. Please use a standalone vcpkg installation.
    exit /b 1
)

echo ✅ Using standalone vcpkg installation: "%VCPKG_ROOT%"
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
    exit /b 1
)

REM Check if Visual Studio BuildTools exists
if not exist "%VS_BUILD_TOOLS_PATH%\VC\Auxiliary\Build\vcvars64.bat" (
    echo ERROR: Visual Studio BuildTools not found at "%VS_BUILD_TOOLS_PATH%"
    echo Please set VS_BUILD_TOOLS_PATH to your Visual Studio installation directory
    exit /b 1
)

REM Check if Visual Studio environment is already configured (for CI/CD)
if defined VCINSTALLDIR (
    echo Visual Studio x64 environment already configured
    echo VCINSTALLDIR: "%VCINSTALLDIR%"
) else (
    echo Setting up Visual Studio x64 environment...
    REM Preserve our VCPKG_ROOT before calling vcvars64 (which may override it)
    set "ORIGINAL_VCPKG_ROOT=%VCPKG_ROOT%"
    call "%VS_BUILD_TOOLS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo Warning: Failed to set up Visual Studio environment
    )
    REM Restore our VCPKG_ROOT (vcvars64 may have overridden it with VS internal vcpkg)
    set "VCPKG_ROOT=%ORIGINAL_VCPKG_ROOT%"
    echo Restored VCPKG_ROOT to: "%VCPKG_ROOT%"
    REM After vcvars64.bat, PATH should include x64 tools
)
echo Debug: PATH after vcvars64.bat: %PATH%
echo Debug: LIB after vcvars64.bat: %LIB%
REM === Force x64 library and binary paths to avoid x86 conflicts ===
set "VC_X64_LIB=%VS_BUILD_TOOLS_PATH%\VC\Tools\MSVC\14.44.35207\lib\x64"
set "VC_X64_BIN=%VS_BUILD_TOOLS_PATH%\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
set "WINSDK_LIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64"
set "WINSDK_UCRT=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64"
set "LIB=%VC_X64_LIB%;%WINSDK_LIB%;%WINSDK_UCRT%;%VCPKG_ROOT%\installed\%VCPKG_TARGET%\lib"
set "PATH=%VC_X64_BIN%;%PATH%"
echo Debug: Forced LIB=%LIB%
echo Debug: Forced PATH=%PATH%

REM Create output directory if it doesn't exist
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo.
echo Compiling SystemMonitor with libcurl TLS integration...
echo.

REM Debug: Show compiler environment and paths
echo Debug: Compiler executable:
REM Find x64 cl.exe after vcvars64.bat
set "CL_PATH="
for /f "delims=" %%C in ('where cl') do (
    echo Found cl.exe: %%C
    echo %%C | findstr /C:"Hostx64\\x64\\cl.exe" >nul
    if !errorlevel! == 0 (
        set "CL_PATH=%%C"
        goto found_cl
    )
)
:found_cl
if not defined CL_PATH (
    REM Fallback to hardcoded path if not found
    if exist "%VS_BUILD_TOOLS_PATH%\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe" (
        set "CL_PATH=%VS_BUILD_TOOLS_PATH%\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe"
        echo Fallback: Using hardcoded x64 cl.exe path
    ) else (
        echo ERROR: x64 cl.exe not found in PATH or at expected location after vcvars64.bat
        exit /b 1
    )
)
echo Debug: Current VCPKG_ROOT after VS setup: "%VCPKG_ROOT%"
echo Debug: VCPKG paths that will be used in compilation:
echo Debug: Include flag: /I"%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include"
echo Debug: Library path: /LIBPATH:"%VCPKG_ROOT%\installed\%VCPKG_TARGET%\lib"

REM Verify the paths exist before compiling
if not exist "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include" (
    echo ERROR: Include directory missing: "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include"
    exit /b 1
)

if not exist "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\lib" (
    echo ERROR: Library directory missing: "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\lib"
    exit /b 1
)

echo Debug: ✅ All required directories exist
echo.

REM Compile with libcurl TLS support (STATIC externals, dynamic system/CRT)
"%CL_PATH%" /EHsc /std:c++17 /MD /DWIN32_LEAN_AND_MEAN /DCURL_STATICLIB ^
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
   ws2_32.lib wldap32.lib advapi32.lib crypt32.lib normaliz.lib ^
   user32.lib kernel32.lib iphlpapi.lib secur32.lib ^
   /OUT:"%OUTPUT_DIR%\%EXE_NAME%" /machine:x64

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ Build failed! Check for missing dependencies or compilation errors.
    echo Note: Full SystemMonitor compilation requires all factory implementations.
    echo.
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
REM pause - removed for CI/CD compatibility