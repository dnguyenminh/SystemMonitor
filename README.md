# SystemMonitor

[![Build Status](https://github.com/dnguyenminh/SystemMonitor/workflows/SystemMonitor%20CI%2FCD/badge.svg)](https://github.com/dnguyenminh/SystemMonitor/actions)
[![Build Status](https://github.com/dnguyenminh/SystemMonitor/workflows/Build%20Status/badge.svg)](https://github.com/dnguyenminh/SystemMonitor/actions)
[![Release](https://img.shields.io/github/v/release/dnguyenminh/SystemMonitor)](https://github.com/dnguyenminh/SystemMonitor/releases)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Windows](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows)
[![TLS](https://img.shields.io/badge/Email-TLS%20Encrypted-green.svg)](https://en.wikipedia.org/wiki/Transport_Layer_Security)

A professional C++ Windows system monitoring application with real-time process tracking, intelligent email alerting, and multiple display modes. Features secure TLS email notifications, comprehensive logging, and enterprise-grade configuration management.

...

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
