#include "../include/Logger.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <windows.h>
#include <cerrno>

// Utility to get current time as string
std::string GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &now_c);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", &tm);
    return buf;
}

// Debug log function
void DebugLog(const std::string& message) {
    static std::ofstream debugLog("SystemMonitor_debug.log", std::ios::app);
    std::string timeStr = GetCurrentTimeString();
    
    if (debugLog.is_open()) {
        debugLog << timeStr << " - " << message << std::endl;
        debugLog.flush();
    }
    
    std::cout << "[DEBUG] " << message << std::endl;
}

// Initialize log file
bool InitializeLogFile(const std::string& logPath) {
    // Delete any existing log file to start fresh
    std::cout << "Log file location: " << logPath << std::endl;
    if (std::remove(logPath.c_str()) == 0) {
        std::cout << "Removed old log file." << std::endl;
    } else if (errno != ENOENT) { // Only show error if it's not "file not found"
        char errBuff[256];
        strerror_s(errBuff, sizeof(errBuff), errno);
        std::cerr << "Warning: Could not remove old log file. Error: " << errBuff << std::endl;
    }
    
    // Create an empty log file to ensure we can write to it
    std::ofstream createLog(logPath);
    if (createLog.is_open()) {
        createLog.close();
        std::cout << "Created empty log file." << std::endl;
        return true;
    } else {
        std::cerr << "Warning: Could not create empty log file. Logging may not work." << std::endl;
        return false;
    }
}

// Log processes exceeding thresholds
void LogProcesses(const std::vector<ProcessInfo>& processes, double systemCpu, double systemRam, double systemDisk) {
    // Create a simplified log file name - only use current directory
    std::string logPath = "SystemMonitor.log";
    
    // Print current directory for diagnostic purposes
    char currDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currDir);
    std::cout << "Current directory: " << currDir << std::endl;
    
    std::cout << "Writing to log file at: " << logPath << std::endl;
    
    // Use a simpler approach - just open and write
    std::ofstream log(logPath, std::ios::app);
    
    // If we can't open the file, try the temp directory
    if (!log.is_open()) {
        std::cerr << "Failed to open log in current directory" << std::endl;
        
        // Get temp path as fallback
        char tempDir[MAX_PATH];
        DWORD tempLen = GetTempPathA(MAX_PATH, tempDir);
        
        if (tempLen > 0) {
            std::string tempPath = std::string(tempDir) + "SystemMonitor.log";
            std::cout << "Trying temp location: " << tempPath << std::endl;
            log.open(tempPath, std::ios::app);
            
            if (log.is_open()) {
                logPath = tempPath;
            } else {
                std::cerr << "Could not open log file in temp directory either. Giving up." << std::endl;
                return;
            }
        } else {
            std::cerr << "Could not get temp directory path. Giving up." << std::endl;
            return;
        }
    }
    
    // Get current time in the required format (dd-MM-yyyy HH:mm:ss)
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &now_c);
    char timeBuffer[32];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%d-%m-%Y %H:%M:%S", &tm);
    std::string formattedTime(timeBuffer);
    
    // Write start banner with system usage information
    log << "===Start " << formattedTime << " [System CPU " << std::fixed << std::setprecision(2) << systemCpu 
        << "%] [System RAM " << std::fixed << std::setprecision(2) << systemRam 
        << "%] [System Disk " << std::fixed << std::setprecision(2) << systemDisk << "%]===\n";
    
    bool processesLogged = false;
    int processCount = 0;
    
    try {
        std::cout << "Found " << processes.size() << " processes to evaluate for logging" << std::endl;
        
        // Verify sum of process resource usage doesn't exceed system totals
        double totalCpu = 0.0, totalRam = 0.0, totalDisk = 0.0;
        for (const auto& p : processes) {
            // Calculate sum of all process resources (ignore near-zero values)
            if (p.cpuPercent > 0.01) totalCpu += p.cpuPercent;
            if (p.ramPercent > 0.01) totalRam += p.ramPercent;
            if (p.diskPercent > 0.01) totalDisk += p.diskPercent;
        }
        
        // Log total process resource percentages vs system totals for debugging
        DebugLog("Total process CPU: " + std::to_string(totalCpu) + "% vs System: " + std::to_string(systemCpu) + "%");
        DebugLog("Total process RAM: " + std::to_string(totalRam) + "% vs System: " + std::to_string(systemRam) + "%");
        DebugLog("Total process Disk: " + std::to_string(totalDisk) + "% vs System: " + std::to_string(systemDisk) + "%");
        
        for (const auto& p : processes) {
            // Only log processes using significant resources
            if (p.cpuPercent > 0.1 || p.ramPercent > 0.1 || p.diskPercent > 0.1) {
                log << formattedTime << ", " 
                    << p.name << ", " 
                    << p.pid << ", [CPU " << std::fixed << std::setprecision(2) << p.cpuPercent 
                    << "%] [RAM " << std::fixed << std::setprecision(2) << p.ramPercent 
                    << "%] [Disk " << std::fixed << std::setprecision(2) << p.diskPercent << "%]\n";
                processesLogged = true;
                processCount++;
            }
        }
        
        // Write end banner
        log << "===End  " << formattedTime << " [System CPU " << std::fixed << std::setprecision(2) << systemCpu 
            << "%] [System RAM " << std::fixed << std::setprecision(2) << systemRam 
            << "%] [System Disk " << std::fixed << std::setprecision(2) << systemDisk << "%]===\n\n";
        
        // Flush to ensure data is written even if the program crashes
        log.flush();
        
        if (processesLogged) {
            std::cout << "Data successfully written to log file at: " << logPath 
                      << " (" << processCount << " processes logged)" << std::endl;
        } else {
            std::cout << "No processes exceeded the resource usage thresholds for logging." << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception while writing to log file: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception while writing to log file" << std::endl;
    }
}
