// main.cpp
// Entry point for SystemMonitor
#include <iostream>
#include <windows.h>
#include <set>
#include <string.h>
#include "include/SystemMetrics.h"
#include "include/Configuration.h"
#include "include/Logger.h"
#include "include/ProcessManager.h"
#include "include/SystemMonitor.h"

int main(int argc, char* argv[]) {
    std::cout << "SystemMonitor initializing..." << std::endl;
    
    // Default configuration
    MonitorConfig config;
    config.cpuThreshold = 80.0;
    config.ramThreshold = 80.0;
    config.diskThreshold = 80.0;
    config.monitorInterval = 5000;
    config.logFilePath = "SystemMonitor.log";
    config.debugMode = false;
    
    // Register a termination handler
    std::set_terminate([]() {
        std::cerr << "\nApplication terminated unexpectedly!" << std::endl;
        std::abort();
    });
    
    // Check for help flag first
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            PrintUsage();
            return 0;
        }
    }
    
    // Try to load config file first
    if (LoadConfigFile(config)) {
        std::cout << "Loaded configuration from SystemMonitor.cfg" << std::endl;
    }
    
    // Parse command line arguments (overrides config file)
    ParseCommandLineArgs(argc, argv, config);
    
    std::cout << "SystemMonitor started." << std::endl;
    std::cout << "Thresholds - CPU: " << config.cpuThreshold << "%, RAM: " 
              << config.ramThreshold << "%, Disk: " << config.diskThreshold << "%\n";
    std::cout << "Monitoring interval: " << config.monitorInterval << "ms\n";
    std::cout << "Log file: " << config.logFilePath << std::endl;
    
    if (config.debugMode) {
        std::cout << "Debug mode enabled (logging to SystemMonitor_debug.log)" << std::endl;
    }
    
    std::cout << "Press Ctrl+C to exit.\n";
    
    // Initialize log file
    if (!InitializeLogFile(config.logFilePath)) {
        std::cerr << "Failed to initialize log file. Exiting." << std::endl;
        return 1;
    }
    
    // Check for administrator privileges
    BOOL isAdmin = FALSE;
    {
        HANDLE hToken = NULL;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elevation;
            DWORD size = sizeof(TOKEN_ELEVATION);
            if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
                isAdmin = elevation.TokenIsElevated;
            }
            CloseHandle(hToken);
        }
    }
    
    if (!isAdmin) {
        std::cout << "Warning: Running without administrator privileges. Some processes may not be accessible." << std::endl;
    }
    
    // Save the current configuration for future runs
    if (SaveConfigFile(config)) {
        std::cout << "Saved configuration to SystemMonitor.cfg" << std::endl;
    } else {
        std::cerr << "Warning: Could not save configuration file." << std::endl;
    }
    
    // Create system monitor and process manager instances
    SystemMonitor sysMonitor;
    ProcessManager processManager(sysMonitor);
    unsigned int monitorCount = 0;
    
    while (true) {
        try {
            // Get system-wide resource usage
            SystemUsage sysUsage = sysMonitor.GetSystemUsage();
            
            // Get all process information
            std::vector<ProcessInfo> processes = processManager.GetAllProcessesInfo();
            
            // Aggregate process tree
            std::vector<ProcessInfo> aggregatedProcesses = processManager.AggregateProcessTree(processes);
            
            // Log processes exceeding thresholds
            LogProcesses(aggregatedProcesses, sysUsage.cpuPercent, sysUsage.ramPercent, sysUsage.diskPercent);
            
            monitorCount++;
            std::cout << "[" << monitorCount << "] System monitored successfully" << std::endl;
            
            // Sleep for the configured interval
            Sleep(config.monitorInterval);
        }
        catch (const std::exception& e) {
            DebugLog("Exception in main loop: " + std::string(e.what()));
            Sleep(1000); // Brief pause on error
        }
        catch (...) {
            DebugLog("Unknown exception in main loop");
            Sleep(1000); // Brief pause on error
        }
    }
    
    return 0;
}
