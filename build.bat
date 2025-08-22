REM --- Locate vcpkg ---
IF NOT DEFINED VCPKG_ROOT (
    IF EXIST "%~dp0vcpkg" (
        set "VCPKG_ROOT=%~dp0vcpkg"
        echo "Found vcpkg in project root: %VCPKG_ROOT%"
    ) ELSE IF EXIST "C:\vcpkg" (
        set "VCPKG_ROOT=C:\vcpkg"
        echo "Found vcpkg at C:\vcpkg"
    ) ELSE IF EXIST "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/vcpkg" (
        set "VCPKG_ROOT=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/vcpkg"
        echo "WARNING: VCPKG_ROOT not defined. Using Visual Studio's internal vcpkg as fallback: %VCPKG_ROOT%"
        echo "WARNING: This is generally not recommended and may cause build issues."
        echo "WARNING: Please consider using a standalone vcpkg installation."
    ) ELSE (
        echo ERROR: No vcpkg installation found.
        echo Please do one of the following:
        echo 1. Clone vcpkg into the project's root directory: git clone https://github.com/Microsoft/vcpkg.git
        echo 2. Set the VCPKG_ROOT environment variable to your standalone vcpkg installation path.
        echo 3. Install vcpkg to C:\vcpkg
        echo 4. Install Visual Studio Build Tools 2022 with C++ workload and vcpkg component.
        exit /b 1
    )
) ELSE (
    echo "Using VCPKG_ROOT from environment: %VCPKG_ROOT%"
)

REM Replace forward slashes with backslashes for Windows path compatibility
set "VCPKG_ROOT=%VCPKG_ROOT:/=\%"
echo "Final VCPKG_ROOT set to: %VCPKG_ROOT%"

REM --- Locate CMake ---
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: CMake not found in PATH. Attempting to add common installation path.
    IF EXIST "C:\Program Files\CMake\bin\cmake.exe" (
        set "PATH=%PATH%;C:\Program Files\CMake\bin"
        echo "Added C:\Program Files\CMake\bin to PATH."
    ) ELSE (
        echo ERROR: CMake not found. Please install CMake and ensure it's in your system PATH.
        echo Download from: https://cmake.org/download/
        exit /b 1
    )
)

REM Step 1: Install static dependencies
"%VCPKG_ROOT%\vcpkg" install --triplet x64-windows-static

REM Step 2: Configure CMake with static toolchain
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static

REM Step 3: Build release executable
cmake --build build --config Release --clean-first

REM Output: bin\SystemMonitor.exe