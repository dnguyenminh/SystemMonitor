# Essential Tests for SystemMonitor

This directory contains the essential tests that validate the key technologies used in the SystemMonitor project.

## 🔧 Key Technologies Tested

### 1. **libcurl TLS Email Integration** (`libcurl_email_test.cpp`)
**Purpose**: Validates the core email functionality with TLS encryption
- ✅ Tests libcurl initialization and TLS support
- ✅ Validates Gmail SMTP connection with SSL/TLS (port 465)
- ✅ Tests real email delivery with authentication
- ✅ Verifies email content formatting and delivery

**Key Features Tested:**
- libcurl TLS/SSL configuration
- Gmail App Password authentication
- Email payload structure and delivery
- Connection timeout and error handling

### 2. **Integration Status Validation** (`integration_status.cpp`)
**Purpose**: Comprehensive system integration status report
- ✅ Verifies vcpkg package manager installation
- ✅ Confirms libcurl[ssl] packages for both x64 and x86
- ✅ Validates Visual Studio BuildTools integration
- ✅ Reports overall project integration status

**Technical Verification:**
- Build tools availability
- Package dependencies status
- Architecture support confirmation
- Integration completion validation

### 3. **Configuration Testing** (`config_email_test.cpp`)
**Purpose**: Email configuration validation and testing
- ✅ Tests email configuration parsing
- ✅ Validates SMTP settings
- ✅ Confirms authentication parameters
- ✅ Tests configuration file integration

## 🏗️ Building and Running Tests

### Prerequisites
- Visual Studio 2022 BuildTools
- vcpkg with libcurl[ssl] installed
- SystemMonitor project dependencies

### Build Commands

**Individual Test Compilation:**
```bash
# libcurl Email Test
cl /EHsc /std:c++17 libcurl_email_test.cpp /I"%VCPKG_ROOT%\installed\x64-windows\include" /link /LIBPATH:"%VCPKG_ROOT%\installed\x64-windows\lib" libcurl.lib ws2_32.lib wldap32.lib advapi32.lib crypt32.lib normaliz.lib

# Integration Status Test  
cl /EHsc /std:c++17 integration_status.cpp

# Configuration Test
cl /EHsc /std:c++17 config_email_test.cpp
```

**Run Tests:**
```bash
.\libcurl_email_test.exe
.\integration_status.exe
.\config_email_test.exe
```

## 🎯 Test Purposes

| Test File | Technology Focus | Validation Purpose |
|-----------|------------------|-------------------|
| `libcurl_email_test.cpp` | **Email TLS Integration** | Core email functionality with encryption |
| `integration_status.cpp` | **System Integration** | Overall project setup and dependencies |
| `config_email_test.cpp` | **Configuration Management** | Email settings and configuration parsing |

## 🚀 What These Tests Validate

### Core SystemMonitor Technologies:
1. **TLS Email Delivery** - Real encrypted email sending capability
2. **vcpkg Integration** - Package management and dependency resolution
3. **Build System** - Compilation and linking with external libraries
4. **Configuration System** - Settings management and validation
5. **Cross-Platform Support** - x64/x86 architecture compatibility

### Integration Verification:
- ✅ libcurl properly linked and functional
- ✅ Gmail SMTP with App Password authentication working
- ✅ TLS/SSL encryption operational
- ✅ Build dependencies correctly installed
- ✅ Configuration parsing functional

## 📝 Test Results Interpretation

**Success Indicators:**
- All tests compile without errors
- Email test successfully sends real emails
- Integration status shows all ✅ checkmarks
- Configuration test validates settings correctly

**Failure Indicators:**
- Compilation errors indicate missing dependencies
- Email test failures suggest SMTP/TLS issues
- Integration status shows missing components
- Configuration test failures indicate parsing problems

## 🔄 Maintenance

These essential tests should be run:
- **After major system updates** - Verify continued functionality
- **Before production deployments** - Ensure all systems operational
- **When troubleshooting** - Isolate specific component issues
- **During dependency updates** - Confirm compatibility

---

*These tests represent the minimal validation suite for SystemMonitor's core technologies.*
