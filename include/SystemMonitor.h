#pragma once

#include "SystemMetrics.h"
#include <mutex>
#include <atomic>
#include <memory>
#include <chrono>

// Abstract base class for system monitoring
class ISystemMonitor {
public:
    virtual ~ISystemMonitor() = default;
    virtual SystemUsage getSystemUsage() = 0;
    virtual SystemMetrics getCurrentMetrics() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
};

// Concrete system monitor implementation
class WindowsSystemMonitor : public ISystemMonitor {
private:
    mutable std::mutex metricsMutex;
    SystemMetrics currentMetrics;
    std::atomic<bool> isFirstMeasurement;
    CpuTimes lastCpuTimes;
    bool initialized;
    
    // Disk I/O tracking
    ULONGLONG lastDiskReadBytes;
    ULONGLONG lastDiskWriteBytes;
    std::chrono::steady_clock::time_point lastDiskMeasurement;
    bool diskMeasurementInitialized;

    // Helper methods
    CpuTimes getSystemCpuTimes() const;
    double calculateDiskIOActivity();
    void updateSystemInfo();

public:
    WindowsSystemMonitor();
    ~WindowsSystemMonitor() override;

    // Delete copy constructor and assignment operator
    WindowsSystemMonitor(const WindowsSystemMonitor&) = delete;
    WindowsSystemMonitor& operator=(const WindowsSystemMonitor&) = delete;

    // ISystemMonitor interface implementation
    SystemUsage getSystemUsage() override;
    SystemMetrics getCurrentMetrics() const override;
    bool initialize() override;
    void shutdown() override;

    // Windows-specific methods
    bool isInitialized() const { return initialized; }
    void reset();
};

// System monitor factory
class SystemMonitorFactory {
public:
    static std::unique_ptr<ISystemMonitor> createWindowsMonitor();
    static std::unique_ptr<ISystemMonitor> createLinuxMonitor();  // Future expansion
    static std::unique_ptr<ISystemMonitor> createCrossPlatformMonitor();
};
