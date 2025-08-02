# SystemMonitor Project Structure

This document describes the organized folder structure of the SystemMonitor project.

## Directory Structure

```
SystemMonitor/
├── doc/                    # Documentation files
│   ├── README.md          # Main project documentation
│   ├── OOP_README.md      # Object-oriented architecture documentation
│   ├── ASYNC_LOGGING_README.md
│   └── ...               # Other documentation files
├── src/                   # Source code files (.cpp)
│   ├── SystemMonitor.cpp
│   ├── ProcessManager.cpp
│   ├── Logger.cpp
│   └── ...
├── include/               # Header files (.h)
│   ├── SystemMonitor.h
│   ├── ProcessManager.h
│   ├── Logger.h
│   └── ...
├── config/                # Configuration files
│   └── SystemMonitor.cfg # Main configuration file
├── bin/                   # Compiled output (executables, object files)
│   ├── SystemMonitor.exe
│   ├── *.obj files
│   └── ...
├── main.cpp              # Main entry point
├── build.bat             # Unified build script with configurable paths
└── .gitignore           # Git ignore file
```

## Building the Project

Use the provided build script:
```bash
build.bat
```

This will:
- Check for required dependencies (Visual Studio, vcpkg, libcurl)
- Display configuration paths for transparency
- Create the `bin/` directory if it doesn't exist
- Compile all source files with libcurl TLS support
- Output the executable to the configured directory
- Clean up object files after successful compilation

The build script supports configurable environment variables:
- `VS_BUILD_TOOLS_PATH`: Visual Studio installation path
- `VCPKG_ROOT`: vcpkg installation directory
- `VCPKG_TARGET`: Target platform (x64-windows, x86-windows)
- `OUTPUT_DIR`: Output directory for executable
- `EXE_NAME`: Name of output executable

## Running the Application

After building, run the application from the project root:
```bash
bin\SystemMonitor_TLS.exe --display top
```

## Documentation

All project documentation is organized in the `doc/` folder:
- **README.md**: Main project documentation
- **PROJECT_STRUCTURE.md**: Project organization guide (this file)
- **OOP_README.md**: Object-oriented architecture details
- **ASYNC_LOGGING_README.md**: Asynchronous logging implementation
- **LOG_ROTATION_README.md**: Log rotation functionality
- **DATE_ROTATION_README.md**: Date-based rotation features
- **OOP_CONVERSION_SUMMARY.md**: Conversion to OOP summary
- **ASYNC_IMPLEMENTATION_SUMMARY.md**: Async implementation details

## Configuration

Configuration files are stored in the `config/` folder:
- **SystemMonitor.cfg**: Main configuration file (created automatically)
- **SystemMonitor.cfg.sample**: Sample configuration with all available options

The application automatically loads configuration from `config/SystemMonitor.cfg` on startup and saves any changes back to this location.

## Version Control

The `.gitignore` file is configured to exclude:
- Compiled output (`bin/` directory)
- Configuration files (`config/` directory and `*.cfg`)
- Log files (`*.log`)
- Temporary and build artifact files

This ensures that only source code and documentation are tracked in version control.
