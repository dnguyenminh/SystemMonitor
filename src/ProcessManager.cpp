#include "../include/ProcessManager.h"
#include "../include/Logger.h"
#include <psapi.h>
#include <tlhelp32.h>
#include <iostream>
#include <functional>
#include <set>
#include <algorithm>

// WindowsProcessManager implementation
WindowsProcessManager::WindowsProcessManager(std::shared_ptr<ISystemMonitor> monitor)
    : systemMonitor(monitor), initialized(false) {
}

WindowsProcessManager::~WindowsProcessManager() {
    shutdown();
}

bool WindowsProcessManager::initialize() {
    if (initialized) {
        return true;
    }

    try {
        // Initialize the process manager
        lastProcessTimes.clear();
        lastIOBytes.clear();
        
        // Capture initial CPU times for baseline
        lastProcessTimes = captureProcessCpuTimes();
        
        initialized = true;
        return true;
    }
    catch (const std::exception& e) {
        // Use logging if available
        initialized = false;
        return false;
    }
}

void WindowsProcessManager::shutdown() {
    if (initialized) {
        lastProcessTimes.clear();
        lastIOBytes.clear();
        initialized = false;
    }
}

void WindowsProcessManager::clearCache() {
    lastProcessTimes.clear();
    lastIOBytes.clear();
}

std::string WindowsProcessManager::convertProcessNameToString(const TCHAR* name) const {
    #ifdef UNICODE
        // Convert wide string to narrow string
        int len = WideCharToMultiByte(CP_UTF8, 0, name, -1, NULL, 0, NULL, NULL);
        if (len > 0) {
            std::vector<char> buf(len);
            WideCharToMultiByte(CP_UTF8, 0, name, -1, buf.data(), len, NULL, NULL);
            return std::string(buf.data());
        }
        return "";
    #else
        // Already a char array, just return it as a string
        return std::string(name);
    #endif
}

std::map<DWORD, FILETIME> WindowsProcessManager::captureProcessCpuTimes() const {
    std::map<DWORD, FILETIME> processTimes;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processTimes;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess != NULL) {
                FILETIME createTime, exitTime, kernelTime, userTime;
                if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                    FILETIME totalTime;
                    ULONGLONG kernelULL = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
                    ULONGLONG userULL = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;
                    ULONGLONG totalULL = kernelULL + userULL;
                    
                    totalTime.dwLowDateTime = (DWORD)(totalULL & 0xFFFFFFFF);
                    totalTime.dwHighDateTime = (DWORD)(totalULL >> 32);
                    processTimes[pe32.th32ProcessID] = totalTime;
                }
                CloseHandle(hProcess);
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processTimes;
}

bool WindowsProcessManager::calculateProcessMetrics(ProcessInfo& processInfo, HANDLE hProcess,
                                                   const std::map<DWORD, FILETIME>& lastTimes,
                                                   const std::map<DWORD, FILETIME>& currentTimes,
                                                   DWORDLONG totalPhysicalMemory) const {
    try {
        // Get memory info
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            processInfo.setRamPercent(100.0 * (double)pmc.PrivateUsage / (double)totalPhysicalMemory);
        }
        
        // Get CPU info
        DWORD pid = processInfo.getPid();
        auto lastIt = lastTimes.find(pid);
        auto currentIt = currentTimes.find(pid);
        
        if (lastIt != lastTimes.end() && currentIt != currentTimes.end()) {
            FILETIME lastTime = lastIt->second;
            FILETIME currentTime = currentIt->second;
            
            ULONGLONG lastULL = ((ULONGLONG)lastTime.dwHighDateTime << 32) | lastTime.dwLowDateTime;
            ULONGLONG currentULL = ((ULONGLONG)currentTime.dwHighDateTime << 32) | currentTime.dwLowDateTime;
            
            if (currentULL > lastULL) {
                ULONGLONG processDelta = currentULL - lastULL;
                double singleCorePercent = (double)processDelta / 100000.0; // Convert to percentage
                processInfo.setCpuPercent(singleCorePercent);
            }
        }
        
        // Get I/O info
        IO_COUNTERS ioCounters;
        if (GetProcessIoCounters(hProcess, &ioCounters)) {
            ULONGLONG totalIO = ioCounters.ReadTransferCount + ioCounters.WriteTransferCount;
            processInfo.setDiskIoBytes(totalIO);
        }
        
        return true;
    }
    catch (...) {
        return false;
    }
}

std::vector<ProcessInfo> WindowsProcessManager::getAllProcesses() {
    if (!initialized) {
        if (!initialize()) {
            return std::vector<ProcessInfo>();
        }
    }

    std::vector<ProcessInfo> processes;
    
    try {
        // Wait briefly and capture current CPU times for delta calculation
        Sleep(100); // 100 ms delay for CPU usage calculation
        std::map<DWORD, FILETIME> currentProcessTimes = captureProcessCpuTimes();
        
        // Get system memory info
        MEMORYSTATUSEX memInfo = { sizeof(MEMORYSTATUSEX) };
        GlobalMemoryStatusEx(&memInfo);
        DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
        
        // Take process snapshot
        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            return processes;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hProcessSnap, &pe32)) {
            do {
                ProcessInfo procInfo(pe32.th32ProcessID, pe32.th32ParentProcessID, 
                                   convertProcessNameToString(pe32.szExeFile));
                
                // Get process details
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    calculateProcessMetrics(procInfo, hProcess, lastProcessTimes, currentProcessTimes, totalPhysMem);
                    CloseHandle(hProcess);
                }
                
                processes.push_back(procInfo);
                
            } while (Process32Next(hProcessSnap, &pe32));
        }
        
        CloseHandle(hProcessSnap);
        
        // Update last times for next iteration
        lastProcessTimes = currentProcessTimes;
        
        return processes;
        
    } catch (const std::exception& e) {
        return processes;
    } catch (...) {
        return processes;
    }
}

std::vector<ProcessInfo> WindowsProcessManager::getAggregatedProcessTree(const std::vector<ProcessInfo>& processes) {
    ProcessTreeAggregator aggregator;
    return aggregator.aggregate(processes);
}

// ProcessTreeAggregator implementation
void ProcessTreeAggregator::buildProcessTree(const std::vector<ProcessInfo>& processes) {
    processTree.clear();
    allPids.clear();
    
    for (const auto& proc : processes) {
        allPids.insert(proc.getPid());
        processTree[proc.getPpid()].push_back(proc);
    }
}

void ProcessTreeAggregator::aggregateChildren(DWORD parentId, ProcessInfo& parent, int depth) {
    if (depth > 100) return; // Prevent infinite recursion
    
    auto it = processTree.find(parentId);
    if (it != processTree.end()) {
        for (const auto& child : it->second) {
            parent.addResourceUsage(child);
            aggregateChildren(child.getPid(), parent, depth + 1);
        }
    }
}

std::vector<ProcessInfo> ProcessTreeAggregator::aggregate(const std::vector<ProcessInfo>& processes) {
    std::vector<ProcessInfo> result;
    
    try {
        buildProcessTree(processes);
        
        // Process each potential parent
        for (const auto& proc : processes) {
            // Skip if this is a child process (has parent in our list)
            if (proc.getPpid() != 0 && allPids.find(proc.getPpid()) != allPids.end()) {
                continue;
            }
            
            ProcessInfo aggregated = proc;
            aggregateChildren(proc.getPid(), aggregated, 0);
            result.push_back(aggregated);
        }
        
        return result;
    }
    catch (...) {
        return std::vector<ProcessInfo>();
    }
}

void ProcessTreeAggregator::reset() {
    processTree.clear();
    allPids.clear();
}

// ProcessFilter implementation
bool ProcessFilter::hasSignificantUsage(const ProcessInfo& process) {
    return process.hasSignificantUsage();
}

bool ProcessFilter::exceedsThreshold(const ProcessInfo& process, double cpuThreshold, 
                                    double ramThreshold, double diskThreshold) {
    return process.getCpuPercent() > cpuThreshold ||
           process.getRamPercent() > ramThreshold ||
           process.getDiskPercent() > diskThreshold;
}

bool ProcessFilter::isSystemProcess(const ProcessInfo& process) {
    const std::string& name = process.getName();
    return (name == "System" || name == "Registry" || name == "smss.exe" ||
            name == "csrss.exe" || name == "wininit.exe" || name == "winlogon.exe");
}

std::vector<ProcessInfo> ProcessFilter::filterByUsage(const std::vector<ProcessInfo>& processes) {
    std::vector<ProcessInfo> filtered;
    std::copy_if(processes.begin(), processes.end(), std::back_inserter(filtered),
                 hasSignificantUsage);
    return filtered;
}

std::vector<ProcessInfo> ProcessFilter::filterByThresholds(const std::vector<ProcessInfo>& processes,
                                                          double cpuThreshold, double ramThreshold, 
                                                          double diskThreshold) {
    std::vector<ProcessInfo> filtered;
    std::copy_if(processes.begin(), processes.end(), std::back_inserter(filtered),
                 [=](const ProcessInfo& p) { 
                     return exceedsThreshold(p, cpuThreshold, ramThreshold, diskThreshold); 
                 });
    return filtered;
}

// ProcessManagerFactory implementation
std::unique_ptr<IProcessManager> ProcessManagerFactory::createWindowsManager(std::shared_ptr<ISystemMonitor> monitor) {
    return std::make_unique<WindowsProcessManager>(monitor);
}

std::unique_ptr<IProcessManager> ProcessManagerFactory::createLinuxManager(std::shared_ptr<ISystemMonitor> monitor) {
    // Stub for future Linux implementation
    return nullptr;
}

std::unique_ptr<IProcessManager> ProcessManagerFactory::createCrossPlatformManager(std::shared_ptr<ISystemMonitor> monitor) {
#ifdef _WIN32
    return createWindowsManager(monitor);
#elif __linux__
    return createLinuxManager(monitor);
#else
    return nullptr;
#endif
}
