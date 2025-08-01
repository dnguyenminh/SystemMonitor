#pragma once

#include <windows.h>
#include <string>

// System-wide metrics
struct SystemMetrics {
    double cpuPercent = 0.0;     // Overall CPU usage percentage
    double ramPercent = 0.0;     // Overall RAM usage percentage
    double diskPercent = 0.0;    // Overall disk space usage percentage
    ULONGLONG diskIOTotal = 0;   // Total disk IO activity (bytes/sec)
    ULONGLONG totalSystemTime = 0; // Total system CPU time for normalization
    int numberOfProcessors = 1;  // Number of CPU cores/processors
    DWORDLONG totalPhysicalMemory = 0; // Total physical memory in bytes
};

// System resource usage structure
struct SystemUsage {
    double cpuPercent;
    double ramPercent;
    double diskPercent;
};

// CPU times structure for usage calculation
struct CpuTimes {
    ULONGLONG idleTime;
    ULONGLONG kernelTime;
    ULONGLONG userTime;
};

// Process information structure
struct ProcessInfo {
    DWORD pid;
    DWORD ppid;
    std::string name;
    double cpuPercent;
    double ramPercent;
    double diskPercent;
    ULONGLONG diskIoBytes;    // Raw IO bytes for disk activity calculation
    
    // Initialize values
    ProcessInfo() : pid(0), ppid(0), name(""), cpuPercent(0.0), ramPercent(0.0), 
                    diskPercent(0.0), diskIoBytes(0) {}
};
