# Date-Based Log Rotation System

## Overview

The SystemMonitor logging system now supports **advanced date-based log rotation** in addition to size-based rotation. This enterprise-grade feature provides automatic log file management based on time intervals, making it easier to organize and manage log files for long-running applications.

## Features

### 🗓️ **Date-Based Rotation Strategies**

1. **SIZE_BASED**: Traditional rotation when file size exceeds limit
2. **DATE_BASED**: Rotation based on time intervals (daily, hourly, weekly)
3. **COMBINED**: Both size and date-based rotation (rotate on either condition)

### ⏰ **Time-Based Rotation Frequencies**

- **DAILY**: Rotate at midnight every day
- **HOURLY**: Rotate at the top of every hour
- **WEEKLY**: Rotate every Sunday at midnight

### 📁 **Filename Patterns**

Date-based rotation creates descriptive filenames with timestamps:

#### Daily Rotation Examples
- `SystemMonitor_20250801.log` (Current day)
- `SystemMonitor_20250731.log` (Previous day)
- `SystemMonitor_20250730.log` (Two days ago)

#### Hourly Rotation Examples
- `SystemMonitor_20250801_14.log` (2 PM today)
- `SystemMonitor_20250801_13.log` (1 PM today)
- `SystemMonitor_20250801_12.log` (12 PM today)

#### Weekly Rotation Examples
- `SystemMonitor_2025_W31.log` (Week 31 of 2025)
- `SystemMonitor_2025_W30.log` (Week 30 of 2025)

## Configuration

### Configuration File Settings

Add these settings to `SystemMonitor.cfg`:

```properties
# Basic rotation settings
LOG_ROTATION_ENABLED=true
LOG_MAX_BACKUPS=10

# Date-based rotation settings
LOG_ROTATION_STRATEGY=DATE_BASED
LOG_DATE_FREQUENCY=DAILY
LOG_DATE_FORMAT=%Y%m%d
LOG_KEEP_DATE_IN_FILENAME=true
```

### Configuration Options

| Setting | Values | Description |
|---------|--------|-------------|
| `LOG_ROTATION_STRATEGY` | `SIZE_BASED`, `DATE_BASED`, `COMBINED` | Rotation trigger strategy |
| `LOG_DATE_FREQUENCY` | `DAILY`, `HOURLY`, `WEEKLY` | How often to rotate based on time |
| `LOG_DATE_FORMAT` | strftime format | Custom date format for filenames |
| `LOG_KEEP_DATE_IN_FILENAME` | `true`, `false` | Include date in rotated filenames |

### Command Line Options

```bash
# Enable date-based rotation with daily frequency
SystemMonitor --log-strategy DATE_BASED --log-frequency DAILY

# Combined rotation (size + date)
SystemMonitor --log-strategy COMBINED --log-frequency HOURLY

# Custom date format
SystemMonitor --log-strategy DATE_BASED --log-date-format "%Y-%m-%d"
```

## Advanced Configuration Examples

### Example 1: Daily Rotation with Size Limit
```properties
LOG_ROTATION_STRATEGY=COMBINED
LOG_DATE_FREQUENCY=DAILY
LOG_MAX_SIZE_MB=50
LOG_MAX_BACKUPS=30
LOG_DATE_FORMAT=%Y%m%d
```

This configuration:
- Rotates daily at midnight
- Also rotates if file exceeds 50MB
- Keeps 30 backup files
- Uses YYYYMMDD format

### Example 2: Hourly Rotation for High-Frequency Monitoring
```properties
LOG_ROTATION_STRATEGY=DATE_BASED
LOG_DATE_FREQUENCY=HOURLY
LOG_MAX_BACKUPS=72
LOG_DATE_FORMAT=%Y%m%d_%H
```

This configuration:
- Rotates every hour
- Keeps 72 hours (3 days) of logs
- Uses YYYYMMDD_HH format

### Example 3: Weekly Rotation for Long-Term Storage
```properties
LOG_ROTATION_STRATEGY=DATE_BASED
LOG_DATE_FREQUENCY=WEEKLY
LOG_MAX_BACKUPS=52
LOG_DATE_FORMAT=%Y_W%U
```

This configuration:
- Rotates weekly on Sundays
- Keeps 52 weeks (1 year) of logs
- Uses YYYY_WWW format

## Technical Implementation

### Date Tracking
The system tracks rotation state using internal variables:
- `lastRotationDate`: Last date when rotation occurred
- `lastRotationHour`: Last hour when rotation occurred (for hourly rotation)

### Rotation Logic Flow
```cpp
bool checkDateRotationNeeded() {
    switch (config.getDateFrequency()) {
        case DAILY:
            currentDate = getCurrentDateString("%Y%m%d");
            return currentDate != lastRotationDate;
            
        case HOURLY:
            currentHour = getCurrentHourString();
            return currentHour != lastRotationHour;
            
        case WEEKLY:
            if (today is Sunday) {
                weekDate = getCurrentDateString("%Y%U");
                return weekDate != lastRotationDate;
            }
    }
}
```

### File Management
The system automatically:
1. **Rotates files** when date/time conditions are met
2. **Manages file counts** based on `LOG_MAX_BACKUPS` setting
3. **Cleans up old files** by modification time
4. **Handles naming conflicts** with numeric suffixes

## Benefits

### 📈 **Operational Benefits**
- **Predictable file sizes**: Daily/hourly logs are more manageable than large files
- **Easy log analysis**: Time-based filenames make it easy to find specific periods
- **Better archiving**: Automated cleanup based on age and count
- **Compliance ready**: Structured log retention for audit requirements

### 🔧 **Development Benefits**
- **Debugging**: Easier to isolate issues to specific time periods
- **Performance analysis**: Compare system behavior across different days/hours
- **Trend analysis**: Historical data organized by time periods
- **Log shipping**: Smaller, time-based files are easier to transfer and process

## Usage Examples

### Basic Daily Rotation
```bash
# Start with daily rotation
SystemMonitor --log-strategy DATE_BASED --log-frequency DAILY

# Monitor for a few days to see rotation in action
# Files created: SystemMonitor_20250801.log, SystemMonitor_20250802.log, etc.
```

### High-Frequency Monitoring
```bash
# Hourly rotation for intensive monitoring
SystemMonitor --log-strategy DATE_BASED --log-frequency HOURLY --interval 500

# Files created: SystemMonitor_20250801_09.log, SystemMonitor_20250801_10.log, etc.
```

### Production Environment
```bash
# Combined rotation with larger size limits
SystemMonitor --log-strategy COMBINED --log-frequency DAILY --log-size 100 --log-backups 90

# Rotates daily OR when file exceeds 100MB, keeps 90 days of logs
```

## File Cleanup Algorithm

The system uses an intelligent cleanup algorithm:

1. **Scan directory** for log files with matching base name
2. **Sort by modification time** (newest first)
3. **Keep configured number** of backup files
4. **Remove excess files** automatically
5. **Log cleanup actions** for audit trail

```cpp
void cleanupOldLogFiles() {
    // Find all log files
    std::vector<std::filesystem::path> logFiles;
    for (auto& entry : std::filesystem::directory_iterator(logDir)) {
        if (entry.path().filename().string().find(baseName) == 0) {
            logFiles.push_back(entry.path());
        }
    }
    
    // Sort by modification time
    std::sort(logFiles.begin(), logFiles.end(), 
              [](const auto& a, const auto& b) {
                  return std::filesystem::last_write_time(a) > 
                         std::filesystem::last_write_time(b);
              });
    
    // Remove excess files
    if (logFiles.size() > maxBackupFiles) {
        for (size_t i = maxBackupFiles; i < logFiles.size(); i++) {
            std::filesystem::remove(logFiles[i]);
        }
    }
}
```

## Integration with Async Logging

Date-based rotation works seamlessly with the asynchronous logging system:

- **Non-blocking**: Date checks happen in the worker thread
- **Efficient**: Minimal overhead for date string generation
- **Thread-safe**: Date tracking variables are accessed only from worker thread
- **Performance**: No impact on main monitoring thread performance

## Troubleshooting

### Common Issues

#### Issue: Rotation not happening
**Solution**: Check that `LOG_ROTATION_ENABLED=true` and verify date frequency settings

#### Issue: Too many log files
**Solution**: Adjust `LOG_MAX_BACKUPS` to control retention period

#### Issue: Date format errors
**Solution**: Use valid strftime format strings (e.g., `%Y%m%d`, `%Y-%m-%d_%H`)

### Debug Information
Enable debug mode to see rotation activity:
```bash
SystemMonitor --debug --log-strategy DATE_BASED
```

Debug output includes:
- Date rotation checks
- File creation/rotation events
- Cleanup operations
- Error conditions

## Conclusion

The date-based log rotation system provides enterprise-grade log management capabilities that complement the existing asynchronous logging architecture. It offers flexible configuration options, intelligent file management, and seamless integration with the OOP design principles of the SystemMonitor application.

### Key Advantages:
- ✅ **Time-based organization** for better log management
- ✅ **Flexible rotation strategies** (size, date, or combined)
- ✅ **Automatic cleanup** to prevent disk space issues
- ✅ **Enterprise-ready** with audit-friendly file naming
- ✅ **Performance optimized** with async worker thread integration
- ✅ **Configurable retention** policies for compliance requirements
