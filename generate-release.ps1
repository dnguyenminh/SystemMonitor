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

Compress-Archive -Path "$releaseDir\*" -DestinationPath "SystemMonitor-$version.zip" -Force

echo "RELEASE_FILE=SystemMonitor-$version.zip" >> $env:GITHUB_ENV
echo "RELEASE_DIR=$releaseDir" >> $env:GITHUB_ENV
echo "VERSION=$version" >> $env:GITHUB_ENV


