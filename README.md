# SystemMonitor

[![Build Status](https://github.com/dnguyenminh/SystemMonitor/workflows/SystemMonitor%20CI%2FCD/badge.svg)](https://github.com/dnguyenminh/SystemMonitor/actions)
[![Build Status](https://github.com/dnguyenminh/SystemMonitor/workflows/Build%20Status/badge.svg)](https://github.com/dnguyenminh/SystemMonitor/actions)
[![Release](https://img.shields.io/github/v/release/dnguyenminh/SystemMonitor)](https://github.com/dnguyenminh/SystemMonitor/releases)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Windows](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows)
[![TLS](https://img.shields.io/badge/Email-TLS%20Encrypted-green.svg)](https://en.wikipedia.org/wiki/Transport_Layer_Security)

A professional C++ Windows system monitoring application with real-time process tracking, intelligent email alerting, and multiple display modes. Features secure TLS email notifications, comprehensive logging, and enterprise-grade configuration management.

## � Deployment

### Static Linking Benefits

SystemMonitor uses **static linking** for all third-party dependencies, which means:

✅ **Self-contained executable** - No external DLLs required
✅ **No vcpkg needed** on target machines  
✅ **Easy deployment** - Just copy the executable and config
✅ **Version consistency** - No library version conflicts

### Deployment Requirements

**Target Machine Requirements:**
- Windows 10/11 or Windows Server 2016+
- Visual C++ Redistributable 2022 (usually already installed)
- No additional dependencies required

**Deployment Package:**
```
deployment/
├── SystemMonitor.exe        # Main executable (self-contained)
├── config/
│   └── SystemMonitor.cfg    # Configuration file
└── README_DEPLOYMENT.txt    # Deployment instructions
```

### Distribution

The compiled executable can be distributed and run on any compatible Windows machine without requiring:
- vcpkg installation
- libcurl installation  
- Development tools
- Additional library packages

## 🧪 Testing

### Core Monitoring
- **Real-time System Monitoring**: CPU, RAM, and Disk usage tracking
- **Process-level Analytics**: Per-process resource utilization with PID tracking  
- **Parent Process Aggregation**: Intelligent grouping of child processes under parents
- **Threshold-based Alerting**: Configurable resource thresholds with smart detection
- **System Overhead Analysis**: Separates process usage from kernel/cache/buffer overhead

### Email Notification System
- **🔒 TLS/SSL Encryption**: Secure email delivery via libcurl with Gmail SMTP support
- **Detailed Alert Logs**: Complete system analysis in email body with process breakdowns
- **Recovery Notifications**: Automatic "all clear" emails when issues resolve
- **Smart Cooldown**: Prevents email spam with configurable cooldown periods
- **App Password Support**: Works with Gmail 2FA and App Passwords

### Display Modes
- **Line Mode**: Traditional scrolling output for logging and automation
- **Top Mode**: Interactive full-screen display like Linux `htop`
- **Compact Mode**: Space-efficient view for quick monitoring
- **Silence Mode**: Background monitoring with output only during threshold violations

### Advanced Logging
- **Structured Logging**: Detailed process logs with timestamps and resource analysis
- **Log Rotation**: Size-based and date-based rotation with configurable retention
- **Async Processing**: Non-blocking logging with configurable queue sizes
- **Debug Support**: Comprehensive debug logging for troubleshooting

## 📋 Prerequisites

- **Windows 10/11** or Windows Server 2016+
- **Visual Studio 2022** Build Tools or Visual Studio Community 2022
- **vcpkg** package manager (for libcurl TLS support during build only)
- **Administrator privileges** (recommended for complete process monitoring)

## ⚙️ Installation

### 1. Install Dependencies

**Install vcpkg and libcurl:**
```powershell
# Clone vcpkg (customize path as needed)
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# Install libcurl with TLS support
.\vcpkg install curl[ssl]:x64-windows
.\vcpkg install curl[ssl]:x86-windows

# Integrate with Visual Studio
.\vcpkg integrate install
```

**Note:** vcpkg is only required for **building** the application. The compiled executable uses **static linking** and can run on other Windows machines without vcpkg installed.

### 2. Build the Application

**Using the build script (Recommended):**
```powershell
cd C:\projects\c++\SystemMonitor
.\build.bat
```

**Customizing Build Paths:**

The build script supports configurable paths through environment variables. You can either:

1. **Set environment variables before running:**
```powershell
# Customize paths for your environment
set VS_BUILD_TOOLS_PATH=D:\Microsoft Visual Studio\2022\Professional
set VCPKG_ROOT=D:\tools\vcpkg
set VCPKG_TARGET=x64-windows
set OUTPUT_DIR=build
set EXE_NAME=SystemMonitor.exe

# Then run the build script
.\build.bat
```

2. **Edit the script directly** by modifying the default values in the configuration section

**Build Configuration Variables:**
| Variable | Description | Default Value |
|----------|-------------|---------------|
| `VS_BUILD_TOOLS_PATH` | Visual Studio installation path | `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` |
| `VCPKG_ROOT` | vcpkg installation directory | `c:\vcpkg` |
| `VCPKG_TARGET` | Target platform architecture | `x64-windows` |
| `OUTPUT_DIR` | Output directory for executable | `bin` |
| `EXE_NAME` | Name of output executable | `SystemMonitor.exe` |

**📖 For detailed build configuration options, see [docs/BUILD_CONFIGURATION.md](docs/BUILD_CONFIGURATION.md)**

*Note: The build script automatically cleans up object files (.obj) after successful compilation.*

**Manual compilation:**
```powershell
# Set up Visual Studio environment (customize path as needed)
call "%VS_BUILD_TOOLS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

# Compile with libcurl TLS support (customize vcpkg paths as needed)
cl /EHsc /std:c++17 /DWIN32_LEAN_AND_MEAN ^
   main.cpp ^
   src/SystemMetrics.cpp ^
   src/ProcessManager.cpp ^
   src/Logger.cpp ^
   src/EmailNotifier.cpp ^
   src/Configuration.cpp ^
   src/ConsoleDisplay.cpp ^
   src/SystemMonitor.cpp ^
   /I"%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include" ^
   /link /LIBPATH:"%VCPKG_ROOT%\installed\%VCPKG_TARGET%\lib" ^
   libcurl.lib ws2_32.lib wldap32.lib advapi32.lib crypt32.lib normaliz.lib user32.lib kernel32.lib ^
   /OUT:%OUTPUT_DIR%\%EXE_NAME%
```

**Alternative Installation Paths:**

If you have vcpkg or Visual Studio installed in different locations, you can set these environment variables:

```powershell
# Example for custom installations
set VCPKG_ROOT=D:\development\vcpkg
set VS_BUILD_TOOLS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community
```

## 🎮 Usage

### Command Line Interface

```bash
bin\SystemMonitor.exe [options]
```

**Core Options:**
```bash
--cpu PERCENT        CPU threshold percentage (default: 80.0)
--ram PERCENT        RAM threshold percentage (default: 80.0)  
--disk PERCENT       Disk threshold percentage (default: 80.0)
--interval MS        Monitoring interval in milliseconds (default: 5000)
--display MODE       Display mode: line, top, compact, silence (default: top)
--debug              Enable debug logging
--help, -h           Show help information
```

**Advanced Logging Options:**
```bash
--log-size MB        Maximum log file size in MB (default: 10)
--log-backups COUNT  Number of backup files to keep (default: 5)
--log-strategy TYPE  Rotation: SIZE_BASED, DATE_BASED, COMBINED
--log-frequency FREQ Date rotation: DAILY, HOURLY, WEEKLY
```

### Display Modes

| Mode | Description | Use Case | Resource Usage |
|------|-------------|----------|----------------|
| **line** | Scrolling text output | Automation, logging, minimal overhead | ⚡ Lowest |
| **top** | Full-screen interactive table | Human monitoring, detailed analysis | 🔄 Medium |
| **compact** | Space-efficient view | Quick monitoring, smaller screens | ⚡ Low |
| **silence** | Background monitoring | Server monitoring, unattended operation | ⚡ Lowest |

**Interactive Controls (all modes):**
- **'q'** - Quit application
- **'t'** - Toggle between display modes (line → top → compact → silence)

### Example Usage

```bash
# Default monitoring with top-style display
bin\SystemMonitor.exe

# High-frequency monitoring with compact display
bin\SystemMonitor.exe --interval 2000 --display compact

# Custom thresholds with debug logging
bin\SystemMonitor.exe --cpu 90 --ram 85 --disk 75 --debug

# Line mode for automation/scripting
bin\SystemMonitor.exe --display line --interval 10000

# Silence mode for background server monitoring
bin\SystemMonitor.exe --display silence --cpu 90 --ram 85
```

## 📧 Email Configuration

### Gmail Setup (Recommended)

1. **Enable 2-Factor Authentication** on your Gmail account
2. **Generate an App Password**:
   - Go to Google Account settings → Security → 2-Step Verification → App passwords
   - Generate a password for "Mail"
3. **Configure SystemMonitor**:

```ini
# config/SystemMonitor.cfg
EMAIL_ENABLED=true
EMAIL_SMTP_SERVER=smtp.gmail.com
EMAIL_SMTP_PORT=465
EMAIL_SENDER=your-email@gmail.com
EMAIL_PASSWORD=your-app-password
EMAIL_SENDER_NAME=SystemMonitor Alert System
EMAIL_RECIPIENTS=admin@company.com,ops@company.com
EMAIL_USE_TLS=true
EMAIL_USE_SSL=true
EMAIL_ALERT_DURATION_SECONDS=10
EMAIL_COOLDOWN_MINUTES=15
EMAIL_SEND_RECOVERY_ALERTS=true
EMAIL_RECOVERY_DURATION_SECONDS=30
```

### Email Alert Features

**Alert Email Contains:**
- Complete system analysis with CPU/RAM/Disk breakdown
- Individual process listings with resource usage
- System vs process overhead analysis  
- Timestamp and monitoring period information
- Actionable recommendations for operators

**Recovery Email Contains:**
- Confirmation that all thresholds are back to normal
- Recent system state showing normal operation
- Summary of original alert for reference
- Performance stabilization confirmation

## ⚙️ Configuration

### Build Configuration

The build script `build.bat` supports environment variables for flexible configuration across different development environments:

```bash
# Set these environment variables before running build.bat
set VS_BUILD_TOOLS_PATH=<path-to-visual-studio>
set VCPKG_ROOT=<path-to-vcpkg>
set VCPKG_TARGET=<target-platform>
set OUTPUT_DIR=<output-directory>
set EXE_NAME=<executable-name>
```

**Common Development Environment Examples:**

```powershell
# Visual Studio Community instead of BuildTools
set VS_BUILD_TOOLS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community

# vcpkg installed on different drive
set VCPKG_ROOT=D:\tools\vcpkg

# Building for x86 instead of x64
set VCPKG_TARGET=x86-windows

# Custom output directory
set OUTPUT_DIR=release
set EXE_NAME=SystemMonitor.exe
```

### Application Configuration

### Configuration File: `config/SystemMonitor.cfg`

```ini
# System Monitoring Thresholds
CPU_THRESHOLD=80
RAM_THRESHOLD=80
DISK_THRESHOLD=80
MONITOR_INTERVAL=5000

# Display Configuration
DISPLAY_MODE=SILENCE  # Options: LINE_BY_LINE, TOP_STYLE, COMPACT, SILENCE
DEBUG_MODE=false

# Logging Configuration
LOG_PATH=SystemMonitor.log
LOG_MAX_SIZE_MB=10
LOG_MAX_BACKUPS=5
LOG_ROTATION_ENABLED=true
LOG_ROTATION_STRATEGY=SIZE_BASED
LOG_DATE_FREQUENCY=DAILY
LOG_DATE_FORMAT=%Y%m%d
LOG_KEEP_DATE_IN_FILENAME=true

# Email Alert Configuration
EMAIL_ENABLED=true
EMAIL_SMTP_SERVER=smtp.gmail.com
EMAIL_SMTP_PORT=465
EMAIL_SENDER=your-email@gmail.com
EMAIL_PASSWORD=your-app-password
EMAIL_SENDER_NAME=SystemMonitor Alert System
EMAIL_RECIPIENTS=admin@company.com
EMAIL_USE_TLS=true
EMAIL_USE_SSL=true
EMAIL_TIMEOUT_SECONDS=30
EMAIL_ALERT_DURATION_SECONDS=10
EMAIL_COOLDOWN_MINUTES=15
EMAIL_SEND_RECOVERY_ALERTS=true
EMAIL_RECOVERY_DURATION_SECONDS=30
```

### Configuration Hierarchy

1. **Command line arguments** (highest priority)
2. **Configuration file** (`config/SystemMonitor.cfg`)
3. **Default values** (lowest priority)

## 📊 Output Formats

### Top-Style Display
```
SystemMonitor - Uptime: 1234s | Processes: 97
CPU:  6.1% | RAM: 46.2% | Disk:  4.5%
------------------------------------------------
     PID Process Name        CPU%   RAM%   Disk%
------------------------------------------------
   15344 Code.exe             3.2%  16.5%   0.1%
   23360 msedge.exe           0.7%   7.5%   0.1%
    2348 explorer.exe         0.3%   2.1%   0.0%
------------------------------------------------
Controls: [q]uit [t]oggle display mode
```

### Compact Display
```
SystemMonitor [1234s] CPU: 6.1% RAM:46.2% Disk: 4.5% Proc: 97
Top Resource Consumers:
    Code.exe[15344] C: 3.2% R:16.5% D: 0.1%
   msedge.exe[23360] C: 0.7% R: 7.5% D: 0.1%
Resource Split: Processes[C:4.3% R:29.0% D:0.2%] System[C:1.8% R:17.2% D:4.3%]
Status: Normal | Controls: [q]uit [t]oggle mode
```

### Silence Mode Output
```
SystemMonitor started in silence display mode.
Silence mode: Output will be shown only when thresholds are exceeded.
Press 'q' to quit, 't' to toggle display mode.

[13:46:04] THRESHOLD EXCEEDED - CPU: 6.9% (>5.0%) RAM: 50.5% (>5.0%) Disk: 2.7% (>5.0%)
    Top processes: Code.exe[15344] (2.2% CPU), msedge.exe[23360] (0.0% CPU), wininit.exe[1628] (0.7% CPU)
```

### Log File Format
```
===Start 02-08-2025 13:11:56 [System CPU 6.16%] [System RAM 49.61%] [System Disk 4.50%]===
SYSTEM ANALYSIS: CPU: Processes=3.60% + System/Kernel=2.56% = Total=6.16%
SYSTEM ANALYSIS: RAM: Processes=29.51% + System/Kernel=20.11% = Total=49.61%
SYSTEM ANALYSIS: DISK: Processes=0.13% + System/Kernel=4.38% = Total=4.50%
02-08-2025 13:11:56, wininit.exe, 1628, [CPU 0.06%] [RAM 2.87%] [Disk 0.00%]
02-08-2025 13:11:56, explorer.exe, 2348, [CPU 0.28%] [RAM 2.08%] [Disk 0.00%]
02-08-2025 13:11:56, Code.exe, 15344, [CPU 3.18%] [RAM 16.47%] [Disk 0.07%]
TOTALS: [Process CPU 3.60%] [Process RAM 29.51%] [Process Disk 0.13%]
SYSTEM OVERHEAD: [CPU 2.56%] [RAM 20.11%] [Disk 4.38%] (Kernel/Cache/Buffers)
===End  02-08-2025 13:11:56 [System CPU 6.16%] [System RAM 49.61%] [System Disk 4.50%]===
```

## 🏗️ Architecture

### Project Structure
```
SystemMonitor/
├── README.md                 # This file
├── main.cpp                  # Application entry point
├── build.bat            # Build script with configurable paths
├── bin/
│   └── SystemMonitor.exe # Compiled executable
├── config/
│   └── SystemMonitor.cfg    # Configuration file
├── include/                 # Header files
│   ├── Configuration.h
│   ├── EmailNotifier.h
│   ├── Logger.h
│   ├── ProcessManager.h
│   └── SystemMonitor.h
├── src/                     # Source files
│   ├── Configuration.cpp
│   ├── EmailNotifier.cpp
│   ├── Logger.cpp
│   ├── ProcessManager.cpp
│   └── SystemMonitor.cpp
├── docs/                    # Documentation
│   ├── TECHNICAL_DOCUMENTATION.md
│   └── BUILD_CONFIGURATION.md
├── log/                     # Log output directory
└── tests/essential/         # Essential technology validation tests
```

### Key Components

- **SystemMonitor**: Core system metrics collection
- **ProcessManager**: Process enumeration and resource tracking
- **EmailNotifier**: TLS-encrypted email alert system  
- **Logger**: Async logging with rotation support
- **Configuration**: File and command-line configuration management
- **ConsoleDisplay**: Multi-mode display system

## 🔧 Troubleshooting

### Common Issues

**Email notifications not working:**
```bash
# Test email configuration
bin\SystemMonitor.exe --debug
# Check log files for SSL/TLS errors
# Verify Gmail App Password is correct
# Ensure firewall allows SMTP connections
```

**High resource usage:**
```bash
# Use line mode or silence mode for minimal overhead
bin\SystemMonitor.exe --display line --interval 10000
bin\SystemMonitor.exe --display silence --interval 10000
```

**Permission errors:**
```bash
# Run as Administrator for complete process access
# Some system processes require elevated privileges
```

**Build errors:**
```bash
# Ensure vcpkg is properly installed and integrated
# Verify Visual Studio Build Tools are available
# Check that libcurl[ssl] is installed for your target architecture

# If using custom paths, verify environment variables:
echo %VCPKG_ROOT%
echo %VS_BUILD_TOOLS_PATH%

# The build script will show configuration at startup:
# Configuration:
#   Visual Studio: C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools
#   vcpkg Root:    c:\vcpkg
#   Target:        x64-windows
#   Output Dir:    bin
```

### Debug Logging

Enable debug mode for detailed troubleshooting:
```bash
bin\SystemMonitor.exe --debug
```

Debug logs are written to `SystemMonitor_debug.log` and include:
- Email SMTP connection details
- Process enumeration issues
- Configuration parsing errors
- System API call failures

## 📈 Performance

### Resource Usage
- **CPU Impact**: <1% during normal operation
- **Memory Usage**: ~10-20MB depending on process count
- **Disk I/O**: Minimal during normal operation, increases during threshold violations

### Scaling
- Tested with 500+ processes
- Handles high-frequency monitoring (1-second intervals)
- Efficient process aggregation algorithms
- Async logging prevents monitoring delays

## � Testing

### Essential Technology Tests

The project includes essential tests that validate core technologies:

```bash
# Build and run all essential tests
cd tests\essential
.\build_tests.bat
.\run_essential_tests.bat
```

**Essential Tests Include:**
- **libcurl TLS Integration** - Email functionality with encryption
- **System Integration Status** - Dependency and build environment validation  
- **Configuration Testing** - Email configuration parsing

These tests ensure the core SystemMonitor technologies are working correctly.

## �🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- **libcurl** for secure TLS email support
- **Windows Performance Toolkit** for system metrics APIs
- **vcpkg** for dependency management

---

**Note**: For production deployment, ensure proper Gmail App Password security and consider using dedicated service accounts for email notifications.
