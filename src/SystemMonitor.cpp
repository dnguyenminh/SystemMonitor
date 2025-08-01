#include "../include/SystemMonitor.h"
#include "../include/Logger.h"
#include <iostream>
#include <iomanip>
#include <psapi.h>

SystemMonitor::SystemMonitor() 
    : isFirstMeasurement(true)
{
    lastCpuTimes = GetSystemCpuTimes();
    currentMetrics = SystemMetrics();
}

CpuTimes SystemMonitor::GetSystemCpuTimes() {
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)) {
        DebugLog("Failed to get system times. Error: " + std::to_string(GetLastError()));
        return CpuTimes();
    }

    CpuTimes times;
    times.idleTime = ((ULONGLONG)idle.dwHighDateTime << 32) | idle.dwLowDateTime;
    times.kernelTime = ((ULONGLONG)kernel.dwHighDateTime << 32) | kernel.dwLowDateTime;
    times.userTime = ((ULONGLONG)user.dwHighDateTime << 32) | user.dwLowDateTime;
    return times;
}

double SystemMonitor::GetDiskUsage() {
    ULARGE_INTEGER available = {0};
    ULARGE_INTEGER total = {0};
    ULARGE_INTEGER totalFree = {0};

    if (!GetDiskFreeSpaceExW(L"C:\\", &available, &total, &totalFree)) {
        DebugLog("Failed to get disk space. Error: " + std::to_string(GetLastError()));
        return 0.0;
    }

    double diskUsagePercent = 100.0 * (total.QuadPart - available.QuadPart) / total.QuadPart;
    
    {
        std::lock_guard<std::mutex> lock(metricsMutex);
        currentMetrics.diskPercent = diskUsagePercent;
    }
    
    return diskUsagePercent;
}

void SystemMonitor::UpdateSystemInfo() {
    MEMORYSTATUSEX mem = { sizeof(mem) };
    if (!GlobalMemoryStatusEx(&mem)) {
        DebugLog("Failed to get memory status. Error: " + std::to_string(GetLastError()));
        return;
    }

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    std::lock_guard<std::mutex> lock(metricsMutex);
    currentMetrics.numberOfProcessors = sysInfo.dwNumberOfProcessors;
    currentMetrics.totalPhysicalMemory = mem.ullTotalPhys;
}

SystemUsage SystemMonitor::GetSystemUsage() {
    SystemUsage usage;
    
    // Get CPU usage
    Sleep(100);  // Small delay for accurate measurement
    CpuTimes now = GetSystemCpuTimes();
    
    ULONGLONG idle = now.idleTime - lastCpuTimes.idleTime;
    ULONGLONG kernel = now.kernelTime - lastCpuTimes.kernelTime;
    ULONGLONG user = now.userTime - lastCpuTimes.userTime;
    ULONGLONG total = kernel + user;
    
    usage.cpuPercent = (total > 0) ? 100.0 * (total - idle) / total : 0.0;
    lastCpuTimes = now;

    // Get RAM usage
    MEMORYSTATUSEX mem = { sizeof(mem) };
    if (GlobalMemoryStatusEx(&mem)) {
        usage.ramPercent = mem.dwMemoryLoad;
    } else {
        usage.ramPercent = 0.0;
        DebugLog("Failed to get memory status. Error: " + std::to_string(GetLastError()));
    }

    // Get disk usage
    usage.diskPercent = GetDiskUsage();
    
    // Update metrics
    {
        std::lock_guard<std::mutex> lock(metricsMutex);
        currentMetrics.cpuPercent = usage.cpuPercent;
        currentMetrics.ramPercent = usage.ramPercent;
        currentMetrics.diskPercent = usage.diskPercent;
        currentMetrics.totalSystemTime = total;
        
        if (isFirstMeasurement.exchange(false)) {
            UpdateSystemInfo();
        }
    }
    
    // Log current usage
    std::cout << "System Usage: CPU: " << std::fixed << std::setprecision(1) << usage.cpuPercent 
              << "%, RAM: " << std::fixed << std::setprecision(1) << usage.ramPercent 
              << "%, Disk: " << std::fixed << std::setprecision(1) << usage.diskPercent << "%" << std::endl;
              
    return usage;
}

SystemMetrics SystemMonitor::GetCurrentMetrics() const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    return currentMetrics;
}
