# SystemMonitor Test Archive

This directory contains test files and development artifacts created during the libcurl TLS integration process.

## Archive Contents

### Email Integration Tests
- `*_email_test.*` - Various email testing implementations
- `smtp_*.* ` - SMTP protocol testing files  
- `libcurl_*.*` - libcurl TLS integration tests
- `simulate_*.*` - Alert simulation tests

### Integration Verification
- `integration_*.*` - Integration status and success reports
- `*.ps1` - PowerShell email testing scripts

### Purpose
These files were used to:
1. Test Gmail SMTP connectivity
2. Develop libcurl TLS integration
3. Verify real email delivery vs simulation mode
4. Debug authentication and encryption issues

## Key Achievements
✅ Gmail simulation mode eliminated  
✅ libcurl TLS integration successful  
✅ Real encrypted email delivery working  
✅ SystemMonitor email alerts functional  

## Clean Project Structure
The main project now contains only essential files:
- Core SystemMonitor source code
- libcurl TLS integrated EmailNotifier
- Clean build scripts
- Organized configuration

---
*These test files can be safely deleted after confirming production stability.*
