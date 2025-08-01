#include "../include/ProcessManager.h"
#include "../include/Logger.h"
#include <psapi.h>
#include <tlhelp32.h>
#include <iostream>
#include <functional>
#include <set>

ProcessManager::ProcessManager(SystemMonitor& monitor) 
    : systemMonitor(monitor) {
}

std::map<DWORD, FILETIME> ProcessManager::GetProcessCPUTimes() {
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
                    totalTime.dwLowDateTime = kernelTime.dwLowDateTime + userTime.dwLowDateTime;
                    totalTime.dwHighDateTime = kernelTime.dwHighDateTime + userTime.dwHighDateTime;
                    processTimes[pe32.th32ProcessID] = totalTime;
                }
                CloseHandle(hProcess);
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processTimes;
}

// Helper to convert process name to string safely
std::string ProcessManager::GetProcessNameString(const TCHAR* name) {
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

// Enumerate all processes and gather resource usage info
std::vector<ProcessInfo> ProcessManager::GetAllProcessesInfo() {
    DebugLog("Entering GetAllProcessesInfo()");
    std::vector<ProcessInfo> processes;
    
    try {
        // Get initial CPU usage snapshot
        static std::map<DWORD, FILETIME> lastProcessTimes = GetProcessCPUTimes();
        Sleep(100); // 100 ms delay for CPU usage calculation
        std::map<DWORD, FILETIME> currentProcessTimes = GetProcessCPUTimes();
        
        // Get system metrics
        MEMORYSTATUSEX memInfo = { sizeof(MEMORYSTATUSEX) };
        GlobalMemoryStatusEx(&memInfo);
        DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
        
        // Take process snapshot
        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            DebugLog("Failed to create process snapshot");
            return processes;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hProcessSnap, &pe32)) {
            do {
                ProcessInfo procInfo;
                procInfo.pid = pe32.th32ProcessID;
                procInfo.ppid = pe32.th32ParentProcessID;
                
                try {
                    procInfo.name = GetProcessNameString(pe32.szExeFile);
                } catch (...) {
                    procInfo.name = "Unknown";
                }
                
                // Get process details
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    // Get memory info
                    PROCESS_MEMORY_COUNTERS_EX pmc;
                    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                        procInfo.ramPercent = 100.0 * (double)pmc.PrivateUsage / (double)totalPhysMem;
                    }
                    
                    // Get CPU info
                    if (lastProcessTimes.find(pe32.th32ProcessID) != lastProcessTimes.end() &&
                        currentProcessTimes.find(pe32.th32ProcessID) != currentProcessTimes.end()) {
                        
                        FILETIME lastTime = lastProcessTimes[pe32.th32ProcessID];
                        FILETIME currentTime = currentProcessTimes[pe32.th32ProcessID];
                        
                        ULONGLONG lastULL = ((ULONGLONG)lastTime.dwHighDateTime << 32) | lastTime.dwLowDateTime;
                        ULONGLONG currentULL = ((ULONGLONG)currentTime.dwHighDateTime << 32) | currentTime.dwLowDateTime;
                        
                        if (currentULL > lastULL) {
                            ULONGLONG processDelta = currentULL - lastULL;
                            double singleCorePercent = (double)processDelta / 10000.0;
                            procInfo.cpuPercent = singleCorePercent;
                        }
                    }
                    
                    // Get I/O info
                    IO_COUNTERS ioCounters;
                    if (GetProcessIoCounters(hProcess, &ioCounters)) {
                        procInfo.diskIoBytes = ioCounters.ReadTransferCount + ioCounters.WriteTransferCount;
                    }
                    
                    CloseHandle(hProcess);
                }
                
                processes.push_back(procInfo);
                
            } while (Process32Next(hProcessSnap, &pe32));
        }
        
        CloseHandle(hProcessSnap);
        lastProcessTimes = currentProcessTimes;
        
        return processes;
        
    } catch (const std::exception& e) {
        DebugLog("Exception in GetAllProcessesInfo: " + std::string(e.what()));
        throw;
    } catch (...) {
        DebugLog("Unknown exception in GetAllProcessesInfo");
        throw;
    }
}

// Aggregate resource usage for parent processes
std::vector<ProcessInfo> ProcessManager::AggregateProcessTree(const std::vector<ProcessInfo>& processes) {
    std::map<DWORD, std::vector<ProcessInfo>> processTree;
    std::set<DWORD> allPids;
    std::vector<ProcessInfo> result;
    
    try {
        // Build process tree
        for (const auto& proc : processes) {
            allPids.insert(proc.pid);
            processTree[proc.ppid].push_back(proc);
        }
        
        // Process each parent
        for (const auto& proc : processes) {
            // Skip if this is a child process
            if (proc.ppid != 0 && allPids.find(proc.ppid) != allPids.end()) {
                continue;
            }
            
            ProcessInfo aggregated = proc;
            
            // Recursive lambda to aggregate children
            std::function<void(DWORD, ProcessInfo&, int)> aggregateChildren = 
                [&](DWORD parentId, ProcessInfo& parent, int depth) {
                    if (depth > 100) return;
                    
                    if (processTree.find(parentId) != processTree.end()) {
                        for (const auto& child : processTree[parentId]) {
                            parent.cpuPercent += child.cpuPercent;
                            parent.ramPercent += child.ramPercent;
                            parent.diskIoBytes += child.diskIoBytes;
                            aggregateChildren(child.pid, parent, depth + 1);
                        }
                    }
                };
            
            // Aggregate children's resource usage
            aggregateChildren(proc.pid, aggregated, 0);
            result.push_back(aggregated);
        }
        
        return result;
    }
    catch (const std::exception& e) {
        DebugLog("Exception in AggregateProcessTree: " + std::string(e.what()));
        return std::vector<ProcessInfo>();
    }
    catch (...) {
        DebugLog("Unknown exception in AggregateProcessTree");
        return std::vector<ProcessInfo>();
    }
}
