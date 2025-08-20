@echo off
REM SystemMonitor Build Script using CMake and vcpkg
REM This script can be run locally or in a CI environment.

setlocal

REM --- Locate vcpkg ---
IF DEFINED VCPKG_ROOT (
    echo "Using VCPKG_ROOT from environment: %VCPKG_ROOT%"
) ELSE (
    echo "VCPKG_ROOT not set. Searching for vcpkg directory at the project root..."
    IF EXIST "%~dp0vcpkg" (
        set "VCPKG_ROOT=%~dp0vcpkg"
        echo "Found vcpkg at: %VCPKG_ROOT%"
    ) ELSE (
        echo "Error: Could not find the vcpkg directory."
        echo "Please do one of the following:"
        echo "1. Set the VCPKG_ROOT environment variable to your vcpkg installation path."
        echo "2. Clone vcpkg into the project's root directory: git clone https://github.com/Microsoft/vcpkg.git"
        exit /b 1
    )
)

REM --- Bootstrap vcpkg if needed ---
IF NOT EXIST "%VCPKG_ROOT%\vcpkg.exe" (
    echo "vcpkg executable not found, running bootstrap script..."
    call "%VCPKG_ROOT%\bootstrap-vcpkg.bat" -disableMetrics
    IF NOT EXIST "%VCPKG_ROOT%\vcpkg.exe" (
        echo "Error: vcpkg bootstrap failed."
        exit /b 1
    )
)

REM --- Install dependencies ---
echo "Installing dependencies using vcpkg..."
"%VCPKG_ROOT%\vcpkg" install --triplet x64-windows-static

REM --- Configure and Build ---
set "TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
echo "Using Toolchain File: %TOOLCHAIN_FILE%"

echo "Configuring project with CMake..."
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN_FILE%" -DCMAKE_BUILD_TYPE=Release -DVCPKG_TARGET_TRIPLET=x64-windows-static

echo "Building project..."
cmake --build build --config Release

endlocal

echo "Build finished. The executable should be in the bin/ directory."
