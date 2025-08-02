@echo off
REM SystemMonitor Version Management
REM Quick commands for version control and releases

if "%1"=="" goto :show_help

if "%1"=="status" goto :git_status
if "%1"=="prepare" goto :prepare_release
if "%1"=="build" goto :build_only
if "%1"=="clean" goto :clean_all
if "%1"=="tag" goto :create_tag
if "%1"=="push" goto :push_release
goto :show_help

:git_status
echo Current Git Status:
git status
git log --oneline -5
goto :end

:prepare_release
if "%2"=="" (
    echo ERROR: Version required for prepare command
    echo Usage: release.bat prepare v1.0.0
    goto :end
)
echo Preparing release %2...
call scripts\prepare-release.bat %2
goto :end

:build_only
echo Building SystemMonitor...
call build.bat
if %ERRORLEVEL% EQU 0 (
    echo ✅ Build successful: bin\SystemMonitor.exe
) else (
    echo ❌ Build failed
)
goto :end

:clean_all
echo Cleaning all build artifacts...
if exist bin\*.exe del /q bin\*.exe
if exist *.obj del /q *.obj
if exist *.pdb del /q *.pdb
if exist *.ilk del /q *.ilk
echo ✅ Clean completed
goto :end

:create_tag
if "%2"=="" (
    echo ERROR: Version required for tag command
    echo Usage: release.bat tag v1.0.0 "Release message"
    goto :end
)
set "TAG_MSG=%3"
if "%TAG_MSG%"=="" set "TAG_MSG=SystemMonitor %2"
echo Creating git tag %2...
git tag -a "%2" -m "%TAG_MSG%"
echo ✅ Tag %2 created
goto :end

:push_release
if "%2"=="" (
    echo ERROR: Version required for push command
    echo Usage: release.bat push v1.0.0
    goto :end
)
echo Pushing release %2 to origin...
git push origin main
git push origin %2
echo ✅ Release %2 pushed to origin
goto :end

:show_help
echo ====================================================
echo  SystemMonitor Release Management
echo ====================================================
echo.
echo Commands:
echo   status                    Show git status and recent commits
echo   build                     Build SystemMonitor only
echo   clean                     Clean all build artifacts
echo   prepare ^<version^>         Prepare complete release package
echo   tag ^<version^> [message]   Create git tag
echo   push ^<version^>            Push release to origin
echo.
echo Examples:
echo   release.bat status
echo   release.bat build
echo   release.bat prepare v1.0.0
echo   release.bat tag v1.0.0 "First stable release"
echo   release.bat push v1.0.0
echo.
echo Full Release Workflow:
echo   1. release.bat prepare v1.0.0
echo   2. release.bat push v1.0.0
echo   3. Create GitHub release with generated ZIP
echo.
goto :end

:end
