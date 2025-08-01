@echo off
echo Building SystemMonitor...
cl /EHsc /std:c++17 /I. main.cpp src\*.cpp /Fe:SystemMonitor.exe advapi32.lib kernel32.lib psapi.lib
if %ERRORLEVEL% == 0 (
    echo Build successful!
    echo SystemMonitor.exe created
    echo.
    echo Log rotation features:
    echo - Automatic log file rotation when size exceeds limit
    echo - Configurable maximum file size and backup count
    echo - Rolling backup files ^(.1, .2, .3, etc.^)
    echo - Similar to Log4J functionality
) else (
    echo Build failed!
    exit /b 1
)
