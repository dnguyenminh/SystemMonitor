@echo off
REM This batch file renames the executable and runs it.

setlocal

set "EXE_NAME=SystemMonitor.exe"
set "EXE_PATH=bin\%EXE_NAME%"
set "BIN_PATH=bin\SystemMonitor.exe.bin"

if not exist "%EXE_PATH%" (
    if exist "%BIN_PATH%" (
        echo Renaming %BIN_PATH% to %EXE_PATH%...
        ren "%BIN_PATH%" "%EXE_NAME%"
        if errorlevel 1 (
            echo Error: Could not rename %BIN_PATH%. Please check permissions.
            pause
            exit /b 1
        )
    ) else (
        echo Error: %BIN_PATH% not found.
        pause
        exit /b 1
    )
)

echo Running %EXE_PATH%...
"%EXE_PATH%" %*

endlocal