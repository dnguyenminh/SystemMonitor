# SystemMonitor - Object-Oriented Architecture

## Overview

SystemMonitor is a high-performance Windows C++ console application that monitors CPU, RAM, and disk usage for all system processes using a modern Object-Oriented Programming (OOP) design. The application features process tree aggregation, configurable thresholds, and Log4J-style log rotation.

## Architecture

### Object-Oriented Design

The application has been completely refactored from a functional programming approach to a robust OOP architecture with the following key components:

#### Core Classes

1. **SystemMonitorApplication** (`main.cpp`)
   - Main application orchestrator
   - Manages lifecycle of all components
   - Handles initialization, execution, and cleanup

2. **SystemMetrics** (`include/SystemMetrics.h`, `src/SystemMetrics.cpp`)
   - `SystemMetrics`: Base class for all metric types
   - `CpuTimes`: CPU timing information management
   - `SystemUsage`: System-wide resource usage snapshot
   - `ProcessInfo`: Individual process information and resource usage

3. **Configuration Management** (`include/Configuration.h`, `src/Configuration.cpp`)
   - `BaseConfig`: Base configuration interface
   - `MonitorConfig`: Monitoring-specific configuration
   - `IConfigurationManager`: Configuration management interface
   - `ConfigurationManager`: Concrete configuration implementation

4. **Logging System** (`include/Logger.h`, `src/Logger.cpp`)
   - `LogConfig`: Log configuration settings
   - `ILogger`: Logger interface
   - `AsyncFileLogger`: **Asynchronous file-based logging with blocking queue**
   - `BlockingQueue<T>`: Thread-safe message queue template
   - `LogMessage`: Message structure for async logging
   - `LoggerFactory`: Factory for creating loggers
   - `LoggerManager`: Singleton logger management

5. **System Monitoring** (`include/SystemMonitor.h`, `src/SystemMonitor.cpp`)
   - `ISystemMonitor`: System monitoring interface
   - `WindowsSystemMonitor`: Windows-specific system monitoring
   - `SystemMonitorFactory`: Factory for creating system monitors

6. **Process Management** (`include/ProcessManager.h`, `src/ProcessManager.cpp`)
   - `IProcessManager`: Process management interface
   - `WindowsProcessManager`: Windows-specific process management
   - `ProcessTreeAggregator`: Process hierarchy aggregation utility
   - `ProcessFilter`: Process filtering utilities
   - `ProcessManagerFactory`: Factory for creating process managers

### Design Patterns Used

- **Factory Pattern**: Used for creating platform-specific implementations
- **Singleton Pattern**: Used for logger management
- **Interface Segregation**: Clean interfaces for each major component
- **Dependency Injection**: Components receive dependencies through constructors
- **RAII**: Proper resource management with smart pointers

## Features

### Core Monitoring Features
- **Real-time Process Monitoring**: Tracks CPU, RAM, and disk usage for all system processes
- **Process Tree Aggregation**: Aggregates resource usage for parent processes and their children
- **Threshold-based Alerting**: Configurable thresholds for CPU, RAM, and disk usage
- **Windows API Integration**: Uses native Windows APIs for accurate system information

### Advanced Features
- **Asynchronous Logging**: Non-blocking log system with worker thread and blocking queue
- **Date-Based Log Rotation**: Time-based rotation (daily, hourly, weekly) with intelligent file naming
- **Combined Rotation Strategy**: Both size and date-based rotation triggers
- **Log4J-style Log Rotation**: Automatic log file rotation with configurable size limits and backup counts
- **Configurable Monitoring**: Command-line arguments and configuration file support
- **Debug Mode**: Enhanced logging for troubleshooting
- **Administrator Privilege Detection**: Warns when running without admin privileges
- **Performance Monitoring**: Real-time queue status and system metrics

### OOP Benefits
- **Modularity**: Each component is self-contained and can be modified independently
- **Extensibility**: Easy to add new monitoring platforms (Linux, macOS) through interfaces
- **Maintainability**: Clear separation of concerns makes the code easier to understand and modify
- **Testability**: Interface-based design allows for easy unit testing
- **Reusability**: Components can be reused in other applications
- **Performance**: Asynchronous logging prevents I/O blocking of main monitoring thread
- **Thread Safety**: Robust multi-threaded design with proper synchronization

## Building

Use the provided build script:

```batch
build.bat
```

Or compile manually:

```batch
cl /EHsc /std:c++17 /I. main.cpp src\*.cpp /Fe:SystemMonitor.exe advapi32.lib kernel32.lib psapi.lib
```

## Usage

### Command Line Options

```
SystemMonitor [options]

Options:
  --cpu PERCENT        CPU threshold percentage (default: 80.0)
  --ram PERCENT        RAM threshold percentage (default: 80.0)
  --disk PERCENT       Disk threshold percentage (default: 80.0)
  --interval MS        Monitoring interval in milliseconds (default: 5000)
  --debug              Enable debug logging
  --log-size MB        Maximum log file size in MB (default: 10)
  --log-backups COUNT  Number of backup files to keep (default: 5)
  --log-rotation       Enable log rotation (default: enabled)

Advanced Log Rotation Options:
  --log-strategy TYPE  Rotation strategy: SIZE_BASED, DATE_BASED, COMBINED
  --log-frequency FREQ Date rotation frequency: DAILY, HOURLY, WEEKLY
  --log-date-format FMT Date format for filenames (default: %Y%m%d)
  --help, -h           Display this help message
```

### Configuration File

The application uses `SystemMonitor.cfg` for persistent configuration:

```
CPU_THRESHOLD=80.0
RAM_THRESHOLD=80.0
DISK_THRESHOLD=80.0
MONITOR_INTERVAL=5000
DEBUG_MODE=false
LOG_FILE_PATH=SystemMonitor.log
LOG_MAX_SIZE_MB=10
LOG_MAX_BACKUPS=5
LOG_ROTATION_ENABLED=true
LOG_ROTATION_STRATEGY=SIZE_BASED
LOG_DATE_FREQUENCY=DAILY
LOG_DATE_FORMAT=%Y%m%d
LOG_KEEP_DATE_IN_FILENAME=true
```

### Example Usage

```batch
# Basic monitoring with default settings
SystemMonitor.exe

# Custom thresholds and interval
SystemMonitor.exe --cpu 90 --ram 85 --disk 75 --interval 3000

# Debug mode with log rotation
SystemMonitor.exe --debug --log-size 20 --log-backups 10

# Date-based daily rotation
SystemMonitor.exe --log-strategy DATE_BASED --log-frequency DAILY

# Combined rotation (size AND date triggers)
SystemMonitor.exe --log-strategy COMBINED --log-frequency HOURLY --log-size 50
```

## Output

### Log Files

- **SystemMonitor.log**: Main log file with process information
- **SystemMonitor_debug.log**: Debug information (when debug mode is enabled)
- **SystemMonitor.log.1, .2, etc.**: Rotated backup log files

### Log Format

```
===Start 01-08-2025 20:53:05 [System CPU 8.93%] [System RAM 42.35%] [System Disk 32.05%]===
01-08-2025 20:53:05, Code.exe, 15344, [CPU 6.38%] [RAM 15.35%] [Disk 0.00%]
===End  01-08-2025 20:53:05 [System CPU 8.93%] [System RAM 42.35%] [System Disk 32.05%]===
```

### Console Output (with Async Queue Status)
```
[1] System monitoring cycle completed (Log queue: 2 messages)
[2] System monitoring cycle completed (Log queue: 0 messages)
[3] System monitoring cycle completed (Log queue: 1 messages)
```

## Technical Details

### Dependencies
- Windows API (advapi32.lib, kernel32.lib, psapi.lib)
- C++17 Standard Library
- Visual Studio 2022 or compatible C++ compiler

### Platform Requirements
- Windows 10/11 or Windows Server 2016+
- x86 or x64 architecture
- Administrator privileges recommended for full process access

### Performance Considerations
- Efficient process enumeration using Windows toolhelp APIs
- Smart caching of process CPU times for accurate percentage calculations
- Configurable monitoring intervals to balance accuracy and performance
- Memory-efficient process tree aggregation

## Development

### Extending the System

The OOP architecture makes it easy to extend the system:

#### Adding New Platforms
1. Implement the `ISystemMonitor` interface for your platform
2. Implement the `IProcessManager` interface for your platform
3. Update the respective factories to create platform-specific instances

#### Adding New Metrics
1. Extend the `SystemMetrics` or `ProcessInfo` classes
2. Update the monitoring interfaces to collect new metrics
3. Modify the logging system to output new metrics

#### Adding New Log Outputs
1. Implement the `ILogger` interface for your output type
2. Update the `LoggerFactory` to create your logger type
3. Configure the application to use your new logger

### Code Organization

```
SystemMonitor/
├── include/           # Header files (.h)
│   ├── Configuration.h
│   ├── Logger.h
│   ├── ProcessManager.h
│   ├── SystemMetrics.h
│   └── SystemMonitor.h
├── src/              # Implementation files (.cpp)
│   ├── Configuration.cpp
│   ├── Logger.cpp
│   ├── ProcessManager.cpp
│   ├── SystemMetrics.cpp
│   └── SystemMonitor.cpp
├── main.cpp          # Application entry point
├── build.bat         # Build script
└── SystemMonitor.cfg # Configuration file
```

## License

This project is a demonstration of modern C++ OOP practices for system monitoring applications.

## Additional Documentation

- **[Asynchronous Logging System](ASYNC_LOGGING_README.md)**: Detailed documentation of the blocking queue and worker thread architecture
- **[Date-Based Log Rotation](DATE_ROTATION_README.md)**: Comprehensive guide to time-based log rotation features
- **[Log Rotation System](LOG_ROTATION_README.md)**: Log4J-style rotation implementation details
