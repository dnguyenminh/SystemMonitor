#pragma once

#include "SystemMetrics.h"
#include <mutex>
#include <atomic>
#include <memory>

class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor() = default;

    // Delete copy constructor and assignment operator
    SystemMonitor(const SystemMonitor&) = delete;
    SystemMonitor& operator=(const SystemMonitor&) = delete;

    // Get current system metrics
    SystemUsage GetSystemUsage();
    
    // Get the current system metrics snapshot
    SystemMetrics GetCurrentMetrics() const;

private:
    // Helper functions
    CpuTimes GetSystemCpuTimes();
    double GetDiskUsage();
    void UpdateSystemInfo();

    // Member variables
    mutable std::mutex metricsMutex;
    SystemMetrics currentMetrics;
    std::atomic<bool> isFirstMeasurement;
    CpuTimes lastCpuTimes;
};
