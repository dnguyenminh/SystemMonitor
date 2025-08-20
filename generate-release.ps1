param (
    [string]$version,
    [string]$releaseDir
)

$currentDate = Get-Date -Format 'yyyy-MM-dd HH:mm:ss UTC'

$deploymentContent = @'
# SystemMonitor $version - Deployment Package

## 🚀 Quick Start
# 1. Extract all files to your preferred directory
# 2. Copy config\SystemMonitor.cfg.template to config\SystemMonitor.cfg
# 3. Edit config\SystemMonitor.cfg with your email settings
# 4. Run: SystemMonitor.exe --display silence

## ✨ Features
# Real-time Monitoring: CPU, RAM, and Disk usage tracking
# TLS Email Alerts: Secure Gmail SMTP notifications
# Multiple Display Modes: Line, top, compact, silence
# Static Linking: No external dependencies required
# Enterprise Ready: Professional monitoring solution

## 📋 Requirements
# Windows 10/11 or Windows Server 2016+
# Visual C++ Redistributable 2022 (typically pre-installed)

## ⚙️ Configuration
Edit `config\SystemMonitor.cfg` to:
- Set monitoring thresholds (CPU/RAM/Disk %)
- Configure Gmail SMTP with App Password
- Customize alert timing and cooldown periods

## 🔗 Support
- **GitHub**: https://github.com/dnguyenminh/SystemMonitor
- **Documentation**: See `docs/` folder
- **Issues**: Report via GitHub Issues

## 🏷️ Version Information
- **Version**: $version
- **Build Date**: $currentDate
- **Compiler**: MSVC with static libcurl
- **Dependencies**: Statically linked (self-contained)
'@

$deploymentContent = $deploymentContent.Replace('$version', $version).Replace('$currentDate', $currentDate)
$deploymentContent | Out-File -FilePath "$releaseDir\DEPLOYMENT_README.md" -Encoding UTF8

Compress-Archive -Path "bin", "docs", "README.md" -DestinationPath "SystemMonitor-$version.zip" -Force

echo "RELEASE_FILE=SystemMonitor-$version.zip" >> $env:GITHUB_ENV
echo "RELEASE_DIR=$releaseDir" >> $env:GITHUB_ENV
echo "VERSION=$version" >> $env:GITHUB_ENV

$releaseNotes = @'
## SystemMonitor $version

### 🎯 What's New
- Enterprise-grade system monitoring with TLS email alerts
- Static linking for easy deployment (no dependencies)
- Multiple display modes for different use cases
- Configurable thresholds and smart alerting

### 📦 Deployment
- Self-contained: No vcpkg or external libraries needed
- Production ready: Suitable for server monitoring
- Easy setup: Extract, configure, and run

### 🔧 Features
- Real-time monitoring: CPU, RAM, Disk usage
- Email notifications: TLS-encrypted Gmail SMTP
- Silent operation: Perfect for background monitoring
- Professional logging: Rotation and debug support

### 📥 Installation
1. Download SystemMonitor-$version.zip
2. Extract to your target directory
3. Configure config/SystemMonitor.cfg
4. Run: SystemMonitor.exe --display silence

### 🏗️ Build Information
- Compiled: $currentDate
- Compiler: Visual Studio 2022 (MSVC)
- Dependencies: Statically linked (self-contained)
- Platform: Windows x64

Built automatically by GitHub Actions from commit $env:GITHUB_SHA
'@

$releaseNotes = $releaseNotes.Replace('$version', $version).Replace('$currentDate', $currentDate)
$releaseNotes | Out-File -FilePath "RELEASE_NOTES.md" -Encoding UTF8

echo "RELEASE_NOTES<<EOF" >> $env:GITHUB_ENV
echo "$releaseNotes" >> $env:GITHUB_ENV
echo "EOF" >> $env:GITHUB_ENV
