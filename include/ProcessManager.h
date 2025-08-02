#pragma once

#include <windows.h>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include "SystemMetrics.h"
#include "SystemMonitor.h"

// Abstract base class for process management
class IProcessManager {
public:
    virtual ~IProcessManager() = default;
    virtual std::vector<ProcessInfo> getAllProcesses() = 0;
    virtual std::vector<ProcessInfo> getAggregatedProcessTree(const std::vector<ProcessInfo>& processes) = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
};

// Concrete Windows process manager
class WindowsProcessManager : public IProcessManager {
private:
    std::shared_ptr<ISystemMonitor> systemMonitor;
    std::map<DWORD, ULONGLONG> lastIOBytes;
    std::map<DWORD, FILETIME> lastProcessTimes;
    bool initialized = false;
    
    // System timing for accurate CPU calculation
    FILETIME lastSystemIdleTime;
    FILETIME lastSystemKernelTime;
    FILETIME lastSystemUserTime;
    bool systemTimesInitialized = false;

    // Helper methods
    std::string convertProcessNameToString(const TCHAR* name) const;
    std::map<DWORD, FILETIME> captureProcessCpuTimes() const;
    bool calculateProcessMetrics(ProcessInfo& processInfo, HANDLE hProcess, 
                                const std::map<DWORD, FILETIME>& lastTimes,
                                const std::map<DWORD, FILETIME>& currentTimes,
                                DWORDLONG totalPhysicalMemory);

public:
    explicit WindowsProcessManager(std::shared_ptr<ISystemMonitor> monitor);
    ~WindowsProcessManager() override;

    // Delete copy constructor and assignment operator
    WindowsProcessManager(const WindowsProcessManager&) = delete;
    WindowsProcessManager& operator=(const WindowsProcessManager&) = delete;

    // IProcessManager interface implementation
    std::vector<ProcessInfo> getAllProcesses() override;
    std::vector<ProcessInfo> getAggregatedProcessTree(const std::vector<ProcessInfo>& processes) override;
    bool initialize() override;
    void shutdown() override;

    // Windows-specific methods
    bool isInitialized() const { return initialized; }
    void clearCache();
};

// Process aggregator utility class
class ProcessTreeAggregator {
private:
    std::map<DWORD, std::vector<ProcessInfo>> processTree;
    std::set<DWORD> allPids;

    void buildProcessTree(const std::vector<ProcessInfo>& processes);
    void aggregateChildren(DWORD parentId, ProcessInfo& parent, int depth = 0);

public:
    std::vector<ProcessInfo> aggregate(const std::vector<ProcessInfo>& processes);
    void reset();
};

// Process filter utility class
class ProcessFilter {
public:
    // Filter predicates
    static bool hasSignificantUsage(const ProcessInfo& process);
    static bool exceedsThreshold(const ProcessInfo& process, double cpuThreshold, 
                                double ramThreshold, double diskThreshold);
    static bool isSystemProcess(const ProcessInfo& process);

    // Filter methods
    static std::vector<ProcessInfo> filterByUsage(const std::vector<ProcessInfo>& processes);
    static std::vector<ProcessInfo> filterByThresholds(const std::vector<ProcessInfo>& processes,
                                                      double cpuThreshold, double ramThreshold, 
                                                      double diskThreshold);
};

// Process manager factory
class ProcessManagerFactory {
public:
    static std::unique_ptr<IProcessManager> createWindowsManager(std::shared_ptr<ISystemMonitor> monitor);
    static std::unique_ptr<IProcessManager> createLinuxManager(std::shared_ptr<ISystemMonitor> monitor);
    static std::unique_ptr<IProcessManager> createCrossPlatformManager(std::shared_ptr<ISystemMonitor> monitor);
};
