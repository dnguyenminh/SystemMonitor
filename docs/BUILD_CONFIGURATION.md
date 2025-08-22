# Build Configuration Guide

This guide explains how to configure the SystemMonitor build environment for different development setups.

## Environment Variables

The `build.bat` script uses environment variables to support flexible configuration across different development environments. If these variables are not set, the script uses default values.

### Core Build Variables

| Variable | Description | Default Value | Example Alternative |
|----------|-------------|---------------|-------------------|
| `VS_BUILD_TOOLS_PATH` | Visual Studio installation directory | `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` | `C:\Program Files\Microsoft Visual Studio\2022\Community` |
| `VCPKG_ROOT` | vcpkg package manager root directory | `c:\vcpkg` | `D:\tools\vcpkg` |
| `VCPKG_TARGET` | Target platform for vcpkg packages (e.g., `x64-windows` for dynamic, `x64-windows-static` for static linking) | `x64-windows-static` | `x86-windows` |
| `OUTPUT_DIR` | Directory for compiled executable | `bin` | `build` or `release` |
| `EXE_NAME` | Name of the output executable. For releases, this is `SystemMonitor.exe.bin` to avoid antivirus issues, and is run via `SystemMonitor.bat`. | `SystemMonitor.exe.bin` | `SystemMonitor.exe` |

## Linking Strategy (Static vs. Dynamic)

SystemMonitor is configured to use **static linking** for its core dependencies like `libcurl` and `zlib` when building with the `x64-windows-static` triplet. This results in a larger, self-contained executable (`SystemMonitor.exe.bin`) that does not require external DLLs for these libraries.

**Why static linking for core dependencies?**
*   **Portability:** The executable can be run on systems without needing to install specific DLLs.
*   **Reduced Deployment Complexity:** All necessary library code is bundled into the main executable.
*   **Antivirus Workaround:** In combination with the `.exe.bin` renaming, this helps mitigate false positives from antivirus software that might flag dynamically linked DLLs.

**Note on C Runtime (CRT) Linking:**
To ensure compatibility with statically linked libraries from vcpkg (e.g., `x64-windows-static` triplet), the application's C/C++ Runtime (CRT) is also linked statically (`/MT`). This is a requirement for proper functionality and does not mean all Windows system libraries are bundled.

## Configuration Methods

### Method 1: Environment Variables (Recommended)

Set environment variables in your development environment:

```powershell
# PowerShell
$env:VCPKG_ROOT = "D:\tools\vcpkg"
$env:VS_BUILD_TOOLS_PATH = "C:\Program Files\Microsoft Visual Studio\2022\Community"
$env:OUTPUT_DIR = "release"

# Command Prompt
set VCPKG_ROOT=D:\tools\vcpkg
set VS_BUILD_TOOLS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community
set OUTPUT_DIR=release

# Then run the build
.\build.bat
```

### Method 2: Batch File Wrapper

Create a custom build script that sets your environment:

```bat
@echo off
REM my_build.bat - Custom build configuration

echo Setting up custom build environment...
set VCPKG_ROOT=D:\development\vcpkg
set VS_BUILD_TOOLS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional
set OUTPUT_DIR=build
set EXE_NAME=SystemMonitor.exe

echo Calling main build script...
call build.bat
```

### Method 3: Direct Script Modification

Edit the default values directly in `build.bat`:

```bat
REM Visual Studio BuildTools installation path
if not defined VS_BUILD_TOOLS_PATH (
    set "VS_BUILD_TOOLS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
)

REM vcpkg installation path
if not defined VCPKG_ROOT (
    set "VCPKG_ROOT=D:\tools\vcpkg"
)
```

## Common Development Environments

### Visual Studio Community

```bat
set VS_BUILD_TOOLS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community
```

### Visual Studio Professional

```bat
set VS_BUILD_TOOLS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional
```

### Custom vcpkg Installation

```bat
set VCPKG_ROOT=D:\development\vcpkg
```

### Different Drive Installation

```bat
set VCPKG_ROOT=E:\vcpkg
set VS_BUILD_TOOLS_PATH=E:\Microsoft Visual Studio\2022\BuildTools
```

### 32-bit Target

```bat
set VCPKG_TARGET=x86-windows
```

## Verification

The build script displays the current configuration at startup:

```
Configuration:
  Visual Studio: C:\Program Files\Microsoft Visual Studio\2022\Community
  vcpkg Root:    D:\tools\vcpkg
  Target:        x64-windows
  Output Dir:    build
```

## Troubleshooting

### Path Issues

If you see errors like "Visual Studio BuildTools not found" or "libcurl not found":

1. **Check your paths:**
   ```bat
   echo %VS_BUILD_TOOLS_PATH%
   echo %VCPKG_ROOT%
   ```

2. **Verify directories exist:**
   ```bat
   dir "%VS_BUILD_TOOLS_PATH%\VC\Auxiliary\Build"
   dir "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\include\curl"
   ```

3. **Reset to defaults:**
   ```bat
   set VS_BUILD_TOOLS_PATH=
   set VCPKG_ROOT=
   set VCPKG_TARGET=
   set OUTPUT_DIR=
   set EXE_NAME=
   ```

### vcpkg Integration

Ensure vcpkg is properly integrated:

```bat
cd %VCPKG_ROOT%
.\vcpkg integrate install
.\vcpkg list curl
```

## CI/CD Integration

For automated builds, set environment variables in your CI/CD pipeline. The `build-release.yml` workflow is configured to use the `x64-windows-static` triplet for static linking and includes the `SystemMonitor.exe.bin` and `SystemMonitor.bat` in the release artifact.

**GitHub Actions:**
```yaml
env:
  BUILD_TYPE: Release
  VCPKG_ROOT: ${{ github.workspace }}\vcpkg
```

**Jenkins:**
```groovy
environment {
    VCPKG_ROOT = 'D:\\tools\\vcpkg'
    VS_BUILD_TOOLS_PATH = 'C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools'
    OUTPUT_DIR = 'artifacts'
}
```


## Team Development

For team development, consider:

1. **Document your team's standard paths** in a `BUILD_SETUP.md` file
2. **Use relative paths** where possible
3. **Provide team-specific build scripts** with pre-configured environments
4. **Use environment variable files** that team members can source

Example team setup script:
```bat
@echo off
REM team_setup.bat - Team standard configuration

echo Setting up team development environment...
set VCPKG_ROOT=C:\dev\vcpkg
set VS_BUILD_TOOLS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional
set OUTPUT_DIR=build
set EXE_NAME=SystemMonitor.exe

echo Environment configured for team development
echo Run: .\build.bat
```
