#pragma once

#include <windows.h>
#include <string>
#include <memory>

// Base class for all metrics
class SystemMetrics {
protected:
    double cpuPercent = 0.0;
    double ramPercent = 0.0;
    double diskPercent = 0.0;
    ULONGLONG diskIOTotal = 0;
    ULONGLONG totalSystemTime = 0;
    int numberOfProcessors = 1;
    DWORDLONG totalPhysicalMemory = 0;

public:
    SystemMetrics() = default;
    virtual ~SystemMetrics() = default;

    // Getters
    double getCpuPercent() const { return cpuPercent; }
    double getRamPercent() const { return ramPercent; }
    double getDiskPercent() const { return diskPercent; }
    ULONGLONG getDiskIOTotal() const { return diskIOTotal; }
    ULONGLONG getTotalSystemTime() const { return totalSystemTime; }
    int getNumberOfProcessors() const { return numberOfProcessors; }
    DWORDLONG getTotalPhysicalMemory() const { return totalPhysicalMemory; }

    // Setters
    void setCpuPercent(double value) { cpuPercent = value; }
    void setRamPercent(double value) { ramPercent = value; }
    void setDiskPercent(double value) { diskPercent = value; }
    void setDiskIOTotal(ULONGLONG value) { diskIOTotal = value; }
    void setTotalSystemTime(ULONGLONG value) { totalSystemTime = value; }
    void setNumberOfProcessors(int value) { numberOfProcessors = value; }
    void setTotalPhysicalMemory(DWORDLONG value) { totalPhysicalMemory = value; }
};

// CPU timing information
class CpuTimes {
private:
    ULONGLONG idleTime = 0;
    ULONGLONG kernelTime = 0;
    ULONGLONG userTime = 0;

public:
    CpuTimes() = default;
    CpuTimes(ULONGLONG idle, ULONGLONG kernel, ULONGLONG user)
        : idleTime(idle), kernelTime(kernel), userTime(user) {}

    ULONGLONG getIdleTime() const { return idleTime; }
    ULONGLONG getKernelTime() const { return kernelTime; }
    ULONGLONG getUserTime() const { return userTime; }

    void setIdleTime(ULONGLONG value) { idleTime = value; }
    void setKernelTime(ULONGLONG value) { kernelTime = value; }
    void setUserTime(ULONGLONG value) { userTime = value; }
};

// System usage snapshot
class SystemUsage {
private:
    double cpuPercent = 0.0;
    double ramPercent = 0.0;
    double diskPercent = 0.0;

public:
    SystemUsage() = default;
    SystemUsage(double cpu, double ram, double disk)
        : cpuPercent(cpu), ramPercent(ram), diskPercent(disk) {}

    double getCpuPercent() const { return cpuPercent; }
    double getRamPercent() const { return ramPercent; }
    double getDiskPercent() const { return diskPercent; }

    void setCpuPercent(double value) { cpuPercent = value; }
    void setRamPercent(double value) { ramPercent = value; }
    void setDiskPercent(double value) { diskPercent = value; }
};

// Process information class
class ProcessInfo {
private:
    DWORD pid = 0;
    DWORD ppid = 0;
    std::string name;
    double cpuPercent = 0.0;
    double ramPercent = 0.0;
    double diskPercent = 0.0;
    ULONGLONG diskIoBytes = 0;

public:
    ProcessInfo() = default;
    ProcessInfo(DWORD processId, DWORD parentId, const std::string& processName)
        : pid(processId), ppid(parentId), name(processName) {}

    // Getters
    DWORD getPid() const { return pid; }
    DWORD getPpid() const { return ppid; }
    const std::string& getName() const { return name; }
    double getCpuPercent() const { return cpuPercent; }
    double getRamPercent() const { return ramPercent; }
    double getDiskPercent() const { return diskPercent; }
    ULONGLONG getDiskIoBytes() const { return diskIoBytes; }

    // Setters
    void setPid(DWORD value) { pid = value; }
    void setPpid(DWORD value) { ppid = value; }
    void setName(const std::string& value) { name = value; }
    void setCpuPercent(double value) { cpuPercent = value; }
    void setRamPercent(double value) { ramPercent = value; }
    void setDiskPercent(double value) { diskPercent = value; }
    void setDiskIoBytes(ULONGLONG value) { diskIoBytes = value; }

    // Utility methods
    bool hasSignificantUsage() const {
        return cpuPercent > 0.1 || ramPercent > 0.1 || diskPercent > 0.1;
    }

    void addResourceUsage(const ProcessInfo& other) {
        cpuPercent += other.cpuPercent;
        ramPercent += other.ramPercent;
        diskIoBytes += other.diskIoBytes;
    }
};
