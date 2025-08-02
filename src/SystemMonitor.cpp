#include "../include/SystemMonitor.h"
#include "../include/Logger.h"
#include <iostream>
#include <iomanip>
#include <psapi.h>
#include <winioctl.h>
#include <chrono>
#include <tlhelp32.h>
#include <cmath>
#include <cstdlib>

// External flag to control console output
extern bool g_suppressConsoleOutput;

// WindowsSystemMonitor implementation
WindowsSystemMonitor::WindowsSystemMonitor() 
    : isFirstMeasurement(true), initialized(false), diskMeasurementInitialized(false) {}

WindowsSystemMonitor::~WindowsSystemMonitor() {
    shutdown();
}

bool WindowsSystemMonitor::initialize() {
    try {
        lastCpuTimes = getSystemCpuTimes();
        updateSystemInfo();
        
        // Initialize disk I/O tracking
        lastDiskReadBytes = 0;
        lastDiskWriteBytes = 0;
        lastDiskMeasurement = std::chrono::steady_clock::now();
        diskMeasurementInitialized = false; // Will be initialized on first measurement
        
        initialized = true;
        
        LoggerManager::getInstance().debug("WindowsSystemMonitor initialized successfully");
        return true;
    } catch (const std::exception& e) {
        LoggerManager::getInstance().debug("Failed to initialize WindowsSystemMonitor: " + std::string(e.what()));
        return false;
    }
}

void WindowsSystemMonitor::shutdown() {
    initialized = false;
    diskMeasurementInitialized = false;
    LoggerManager::getInstance().debug("WindowsSystemMonitor shutdown completed");
}

CpuTimes WindowsSystemMonitor::getSystemCpuTimes() const {
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)) {
        LoggerManager::getInstance().debug("Failed to get system times. Error: " + std::to_string(GetLastError()));
        return CpuTimes();
    }

    ULONGLONG idleTime = ((ULONGLONG)idle.dwHighDateTime << 32) | idle.dwLowDateTime;
    ULONGLONG kernelTime = ((ULONGLONG)kernel.dwHighDateTime << 32) | kernel.dwLowDateTime;
    ULONGLONG userTime = ((ULONGLONG)user.dwHighDateTime << 32) | user.dwLowDateTime;
    
    return CpuTimes(idleTime, kernelTime, userTime);
}

double WindowsSystemMonitor::calculateDiskIOActivity() {
    // This method will be called after process manager calculates individual process I/O
    // We'll aggregate the process I/O values to get the system total
    // For now, return 0 and let the system aggregation happen in getSystemUsage()
    return 0.0;
}

void WindowsSystemMonitor::updateSystemInfo() {
    MEMORYSTATUSEX mem = { sizeof(mem) };
    if (!GlobalMemoryStatusEx(&mem)) {
        LoggerManager::getInstance().debug("Failed to get memory status. Error: " + std::to_string(GetLastError()));
        return;
    }

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    std::lock_guard<std::mutex> lock(metricsMutex);
    currentMetrics.setNumberOfProcessors(sysInfo.dwNumberOfProcessors);
    currentMetrics.setTotalPhysicalMemory(mem.ullTotalPhys);
}

SystemUsage WindowsSystemMonitor::getSystemUsage() {
    if (!initialized) {
        LoggerManager::getInstance().debug("SystemMonitor not initialized");
        return SystemUsage();
    }

    try {
        // Get CPU usage
        Sleep(100);  // Small delay for accurate measurement
        CpuTimes now = getSystemCpuTimes();
        
        ULONGLONG idle = now.getIdleTime() - lastCpuTimes.getIdleTime();
        ULONGLONG kernel = now.getKernelTime() - lastCpuTimes.getKernelTime();
        ULONGLONG user = now.getUserTime() - lastCpuTimes.getUserTime();
        ULONGLONG total = kernel + user;
        
        double cpuPercent = (total > 0) ? 100.0 * (total - idle) / total : 0.0;
        lastCpuTimes = now;

        // Get RAM usage
        MEMORYSTATUSEX mem = { sizeof(mem) };
        double ramPercent = 0.0;
        if (GlobalMemoryStatusEx(&mem)) {
            // Use actual memory usage calculation instead of dwMemoryLoad for consistency
            // Calculate: (Total - Available) / Total * 100
            DWORDLONG usedMemory = mem.ullTotalPhys - mem.ullAvailPhys;
            ramPercent = 100.0 * (double)usedMemory / (double)mem.ullTotalPhys;
        } else {
            LoggerManager::getInstance().debug("Failed to get memory status. Error: " + std::to_string(GetLastError()));
        }

        // Get disk I/O activity
        double diskPercent = calculateDiskIOActivity();
        
        // Update metrics
        {
            std::lock_guard<std::mutex> lock(metricsMutex);
            currentMetrics.setCpuPercent(cpuPercent);
            currentMetrics.setRamPercent(ramPercent);
            currentMetrics.setDiskPercent(diskPercent);
            currentMetrics.setTotalSystemTime(total);
        }
        
        // Handle first measurement outside of the lock to avoid deadlock
        if (isFirstMeasurement.exchange(false)) {
            updateSystemInfo();
        }
        
        // Note: Console output is handled by the display system in main.cpp
        // This keeps the SystemMonitor class focused on data collection only
                  
        return SystemUsage(cpuPercent, ramPercent, diskPercent);
        
    } catch (const std::exception& e) {
        LoggerManager::getInstance().debug("Exception in getSystemUsage: " + std::string(e.what()));
        return SystemUsage();
    }
}

SystemMetrics WindowsSystemMonitor::getCurrentMetrics() const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    return currentMetrics;
}

void WindowsSystemMonitor::reset() {
    std::lock_guard<std::mutex> lock(metricsMutex);
    currentMetrics = SystemMetrics();
    isFirstMeasurement.store(true);
    lastCpuTimes = getSystemCpuTimes();
}

// SystemMonitorFactory implementation
std::unique_ptr<ISystemMonitor> SystemMonitorFactory::createWindowsMonitor() {
    return std::make_unique<WindowsSystemMonitor>();
}

std::unique_ptr<ISystemMonitor> SystemMonitorFactory::createLinuxMonitor() {
    // TODO: Implement Linux monitor
    return nullptr;
}

std::unique_ptr<ISystemMonitor> SystemMonitorFactory::createCrossPlatformMonitor() {
    // TODO: Implement cross-platform monitor
#ifdef _WIN32
    return createWindowsMonitor();
#elif __linux__
    return createLinuxMonitor();
#else
    return nullptr;
#endif
}
