# SystemMonitor - Log Rotation Feature

## Overview
SystemMonitor now includes Log4J-style log rotation functionality that automatically manages log file sizes and creates rolling backup files.

## Configuration Options

### Log Rotation Settings
Add these settings to your `SystemMonitor.cfg` file:

```
LOG_MAX_SIZE_MB=10           # Maximum log file size in MB (default: 10)
LOG_MAX_BACKUPS=5            # Number of backup files to keep (default: 5)
LOG_ROTATION_ENABLED=true    # Enable/disable log rotation (default: true)
LOG_ROTATION_STRATEGY=SIZE_BASED  # SIZE_BASED, DATE_BASED, or COMBINED
# For DATE_BASED or COMBINED only:
# LOG_DATE_FREQUENCY=DAILY
# LOG_DATE_FORMAT=%Y%m%d
```

## How It Works

### Automatic Rotation
- When the main log file exceeds `LOG_MAX_SIZE_MB`, rotation is triggered
- The current log file is renamed with a `.1` extension
- A new empty log file is created
- Existing backup files are shifted (`.1` becomes `.2`, `.2` becomes `.3`, etc.)
- The oldest backup file is deleted if it exceeds `LOG_MAX_BACKUPS`

### File Naming Convention
```
SystemMonitor.log      # Current active log file
SystemMonitor.log.1    # Most recent backup
SystemMonitor.log.2    # Second most recent backup
SystemMonitor.log.3    # Third most recent backup
...
SystemMonitor.log.N    # Oldest backup (where N = LOG_MAX_BACKUPS)
```

## Benefits

1. **Automatic Management**: No manual intervention required
2. **Space Control**: Prevents log files from consuming unlimited disk space
3. **Historical Data**: Keeps recent historical data in backup files
4. **Performance**: Smaller active log files improve performance
5. **Compatibility**: Similar to popular logging frameworks like Log4J

## Example Configuration

For a production environment with frequent monitoring:
```
LOG_MAX_SIZE_MB=5        # Smaller files for faster processing
LOG_MAX_BACKUPS=10       # More backups for longer history
LOG_ROTATION_ENABLED=true
LOG_ROTATION_STRATEGY=SIZE_BASED
# For DATE_BASED or COMBINED only:
# LOG_DATE_FREQUENCY=DAILY
# LOG_DATE_FORMAT=%Y%m%d
```

For development or testing:
```
LOG_MAX_SIZE_MB=50       # Larger files for debugging
LOG_MAX_BACKUPS=2        # Minimal backups
LOG_ROTATION_ENABLED=false  # Disable rotation for continuous logging
```

## Log File Structure

Each log entry maintains the same format:
```
===Start DD-MM-YYYY HH:MM:SS [System CPU X.X%] [System RAM X.X%] [System Disk X.X%]===
DD-MM-YYYY HH:MM:SS, ProcessName.exe, PID, [CPU X.X%] [RAM X.X%] [Disk X.X%]
...
===End DD-MM-YYYY HH:MM:SS [System CPU X.X%] [System RAM X.X%] [System Disk X.X%]===
```

This ensures that log analysis tools can process both current and archived log files consistently.
