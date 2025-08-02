@echo off
REM SystemMonitor Release Preparation Script
REM This script prepares a new release with proper versioning and build

echo ====================================================
echo  SystemMonitor Release Preparation
echo ====================================================

REM Check if version parameter is provided
if "%1"=="" (
    echo ERROR: Version number required
    echo Usage: prepare-release.bat ^<version^>
    echo Example: prepare-release.bat v1.0.0
    echo.
    exit /b 1
)

set "VERSION=%1"
set "RELEASE_DIR=releases\%VERSION%"
set "BUILD_DIR=build\release"

echo Preparing release %VERSION%...
echo.

REM Create release directories
if not exist releases mkdir releases
if not exist "%RELEASE_DIR%" mkdir "%RELEASE_DIR%"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo Step 1: Clean previous builds...
if exist bin\*.exe del /q bin\*.exe
if exist *.obj del /q *.obj

echo Step 2: Building SystemMonitor for release...
call build.bat
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    exit /b 1
)

echo Step 3: Creating release package...
REM Copy executable
copy "bin\SystemMonitor.exe" "%RELEASE_DIR%\SystemMonitor.exe"

REM Copy configuration template (without sensitive data)
if not exist "%RELEASE_DIR%\config" mkdir "%RELEASE_DIR%\config"
copy "config\SystemMonitor.cfg" "%RELEASE_DIR%\config\SystemMonitor.cfg.template"

REM Copy documentation
copy "README.md" "%RELEASE_DIR%\README.md"
copy "docs\*.md" "%RELEASE_DIR%\docs\" 2>nul

REM Create deployment readme
call :create_deployment_readme "%RELEASE_DIR%"

echo Step 4: Creating archive...
powershell -Command "Compress-Archive -Path '%RELEASE_DIR%\*' -DestinationPath '%RELEASE_DIR%.zip' -Force"

echo Step 5: Git operations...
git add .
git commit -m "Release %VERSION%: Production build with static linking"
git tag -a "%VERSION%" -m "SystemMonitor %VERSION% - Enterprise monitoring with TLS email alerts"

echo.
echo ✅ Release %VERSION% prepared successfully!
echo.
echo Release package: %RELEASE_DIR%.zip
echo Git tag: %VERSION%
echo.
echo Next steps:
echo 1. git push origin main
echo 2. git push origin %VERSION%
echo 3. Create GitHub release with %RELEASE_DIR%.zip
echo.
pause
exit /b 0

:create_deployment_readme
set "DEPLOY_DIR=%1"
echo Creating deployment readme...
(
echo # SystemMonitor %VERSION% - Deployment Guide
echo.
echo ## Quick Start
echo.
echo 1. Extract all files to your target directory
echo 2. Rename `SystemMonitor.cfg.template` to `SystemMonitor.cfg`
echo 3. Configure email settings in `SystemMonitor.cfg`
echo 4. Run: `SystemMonitor.exe --display silence`
echo.
echo ## Features
echo - Real-time CPU, RAM, and Disk monitoring
echo - TLS-encrypted email alerts for threshold violations
echo - Multiple display modes ^(line, top, compact, silence^)
echo - Static linking - no external dependencies required
echo.
echo ## Requirements
echo - Windows 10/11 or Windows Server 2016+
echo - Visual C++ Redistributable 2022 ^(usually pre-installed^)
echo.
echo ## Configuration
echo Edit `SystemMonitor.cfg` to customize:
echo - Monitoring thresholds
echo - Email SMTP settings
echo - Alert timing and cooldown periods
echo.
echo ## Support
echo For issues and documentation: https://github.com/dnguyenminh/SystemMonitor
) > "%DEPLOY_DIR%\DEPLOYMENT_README.md"
exit /b 0
