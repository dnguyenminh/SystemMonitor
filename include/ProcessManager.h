#pragma once

#include <windows.h>
#include <vector>
#include <map>
#include "SystemMetrics.h"
#include "SystemMonitor.h"

class ProcessManager {
public:
    ProcessManager(SystemMonitor& monitor);
    ~ProcessManager() = default;

    // Delete copy constructor and assignment operator
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;

    // Get process information
    std::vector<ProcessInfo> GetAllProcessesInfo();
    std::vector<ProcessInfo> AggregateProcessTree(const std::vector<ProcessInfo>& processes);

private:
    // Helper functions
    std::string GetProcessNameString(const TCHAR* name);
    std::map<DWORD, FILETIME> GetProcessCPUTimes();

    // Reference to system monitor for metrics
    SystemMonitor& systemMonitor;
    std::map<DWORD, ULONGLONG> lastIOBytes;
};
