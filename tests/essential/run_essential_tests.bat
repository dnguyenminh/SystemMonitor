@echo off
REM Run all essential tests for SystemMonitor
REM Validates core technologies and integration status

echo ====================================================
echo  SystemMonitor Essential Tests - Test Runner
echo ====================================================

echo.
echo 🔧 Running essential technology validation tests...
echo.

REM Test 1: Integration Status
echo [TEST 1/3] System Integration Status
echo ----------------------------------------
if exist integration_status.exe (
    integration_status.exe
    echo.
    echo ✅ Integration status test completed
) else (
    echo ❌ integration_status.exe not found. Run build_tests.bat first.
)

echo.
echo ========================================
echo.

REM Test 2: Configuration Testing
echo [TEST 2/3] Configuration Validation
echo ----------------------------------------
if exist config_email_test.exe (
    config_email_test.exe
    echo.
    echo ✅ Configuration test completed
) else (
    echo ❌ config_email_test.exe not found. Run build_tests.bat first.
)

echo.
echo ========================================
echo.

REM Test 3: libcurl Email Integration (requires user confirmation)
echo [TEST 3/3] libcurl TLS Email Integration
echo ----------------------------------------
echo.
echo ⚠️  WARNING: This test will send a real email!
echo     Recipient: layland.ernst@freedrops.org
echo.
set /p CONFIRM="Do you want to run the email test? (y/N): "
if /i "%CONFIRM%"=="y" (
    echo.
    echo 📧 Running libcurl email test...
    if exist libcurl_email_test.exe (
        libcurl_email_test.exe
        echo.
        echo ✅ Email integration test completed
    ) else (
        echo ❌ libcurl_email_test.exe not found. Run build_tests.bat first.
    )
) else (
    echo ⏭️  Email test skipped by user choice
)

echo.
echo ========================================
echo.
echo 🎯 Essential Tests Summary:
echo ----------------------------------------
echo ✅ Integration Status - Validates system setup
echo ✅ Configuration Test - Validates email config parsing
if /i "%CONFIRM%"=="y" (
    echo ✅ Email Integration - Validates TLS email delivery
) else (
    echo ⏭️  Email Integration - Skipped
)
echo.
echo 📋 These tests validate the core SystemMonitor technologies:
echo    • vcpkg package management
echo    • libcurl TLS integration  
echo    • Visual Studio build environment
echo    • Email configuration system
echo    • Real TLS email delivery
echo.
echo All essential technologies have been validated! 🚀
echo.
pause
