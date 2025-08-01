# SystemMonitor

A native C++ Windows console application to monitor CPU, RAM, and Disk usage for all system processes. Aggregates usage for parent processes and logs details when thresholds are exceeded.

## Features
- Monitors system-wide CPU, RAM, and Disk usage
- Tracks per-process resource utilization
- Aggregates resource usage for parent processes (sums up child process usage)
- Logs process details when any resource exceeds user-defined thresholds
- Configurable thresholds via command line parameters or config file
- Detailed logging with timestamps and resource percentages
- Debug logging for troubleshooting

## Build Instructions

### Using Visual Studio Code
1. Make sure you have Visual Studio C++ build tools installed
2. Open the project folder in VS Code
3. Press Ctrl+Shift+B to build the project using the defined build task

### Using Command Line with MSVC
```
cl.exe /EHsc main.cpp ProcessMonitor.cpp SystemUtils.cpp Logger.cpp Config.cpp advapi32.lib
```

## Usage

### Command Line Options
```
SystemMonitor [options]

Options:
  --cpu PERCENT     CPU threshold percentage (default: 80.0)
  --ram PERCENT     RAM threshold percentage (default: 80.0)
  --disk PERCENT    Disk threshold percentage (default: 80.0)
  --interval MS     Monitoring interval in milliseconds (default: 5000)
  --config PATH     Path to config file (overrides command line options)
  --debug           Enable debug logging to SystemMonitor_debug.log
  --help, -h        Display this help message
```

### Examples
```
# Run with default thresholds
SystemMonitor.exe

# Custom thresholds
SystemMonitor.exe --cpu 90 --ram 75 --disk 85

# Faster monitoring interval (2 seconds) with debug logging
SystemMonitor.exe --interval 2000 --debug
```

### Configuration File
The application can also be configured using a configuration file (`SystemMonitor.cfg`). 
When run for the first time, the application will create this file with the current settings.

## Log Format

The application logs resource usage in the following format:

```
===Start dd-MM-yyyy HH:mm:ss [System CPU xx.xx%] [System RAM xx.xx%] [System Disk xx.xx%]===
dd-MM-yyyy HH:mm:ss, process name, process id, [CPU xx.xx%] [RAM xx.xx%] [Disk xx.xx%]
dd-MM-yyyy HH:mm:ss, process name, process id, [CPU xx.xx%] [RAM xx.xx%] [Disk xx.xx%]
...
===End dd-MM-yyyy HH:mm:ss [System CPU xx.xx%] [System RAM xx.xx%] [System Disk xx.xx%]===
```

## Note on Permissions

For complete monitoring capability, it's recommended to run the application with administrator privileges.
Without admin rights, some system processes may not be accessible for monitoring.
