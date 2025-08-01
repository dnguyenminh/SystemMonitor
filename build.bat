@echo off
echo Building SystemMonitor...
cl /EHsc /std:c++17 /I. main.cpp src\*.cpp /Fe:SystemMonitor.exe advapi32.lib kernel32.lib psapi.lib
if %ERRORLEVEL% == 0 (
    echo Build successful!
    echo SystemMonitor.exe created
) else (
    echo Build failed!
    exit /b 1
)
