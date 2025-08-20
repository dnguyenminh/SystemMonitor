@echo off
REM SystemMonitor Static Build Script (CMake + vcpkg manifest mode)

REM Step 1: Install static dependencies
vcpkg install --triplet x64-windows-static

REM Step 2: Configure CMake with static toolchain
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static

REM Step 3: Build release executable
cmake --build build --config Release --clean-first

REM Output: bin\SystemMonitor.exe
