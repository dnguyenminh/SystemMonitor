# SystemMonitor

[![Build Status](https://github.com/dnguyenminh/SystemMonitor/workflows/SystemMonitor%20CI%2FCD/badge.svg)](https://github.com/dnguyenminh/SystemMonitor/actions)
[![Build Status](https://github.com/dnguyenminh/SystemMonitor/workflows/Build%20Status/badge.svg)](https://github.com/dnguyenminh/SystemMonitor/actions)
[![Release](https://img.shields.io/github/v/release/dnguyenminh/SystemMonitor)](https://github.com/dnguyenminh/SystemMonitor/releases)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Windows](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows)
[![TLS](https://img.shields.io/badge/Email-TLS%20Encrypted-green.svg)](https://en.wikipedia.org/wiki/Transport_Layer_Security)

A professional C++ Windows system monitoring application with real-time process tracking, intelligent email alerting, and multiple display modes. Features secure TLS email notifications, comprehensive logging, and enterprise-grade configuration management.

...

## How to Run

The main executable is `SystemMonitor.exe.bin`. To run it, you should use the provided `SystemMonitor.bat` script. This script handles renaming the executable and launching it.

1.  Download the release package from the [releases page](https://github.com/dnguyenminh/SystemMonitor/releases).
2.  Extract the contents of the ZIP file to a folder on your computer.
3.  Navigate to the extracted folder.
4.  Run `SystemMonitor.bat`.

    **Note:** If Windows Defender or other antivirus software flags the downloaded ZIP or its contents, this is likely a false positive due to the executable being unsigned and downloaded from the internet. The `.bat` file workaround is designed to help with this. You may still need to manually "unblock" the ZIP file in its properties after downloading, or add an exclusion to your antivirus.

## How to Build

To build SystemMonitor from source, you will need:
*   Visual Studio 2022 (or compatible C++ build tools)
*   CMake (version 3.15 or higher)
*   vcpkg (for dependency management)

For detailed build instructions and configuration options, please refer to the [Build Configuration Guide](docs/BUILD_CONFIGURATION.md).

### Application Configuration

#### Configuration File: `config/SystemMonitor.cfg`

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
# For DATE_BASED or COMBINED strategies only:
# LOG_DATE_FREQUENCY=DAILY
# LOG_DATE_FORMAT=%Y%m%d
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

...

> **Note:**  
> `LOG_DATE_FREQUENCY` and `LOG_DATE_FORMAT` are only relevant for `LOG_ROTATION_STRATEGY=DATE_BASED` or `COMBINED`.  
> For `SIZE_BASED` rotation, these options are ignored.

...
