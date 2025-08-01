// main.cpp
// Entry point for SystemMonitor
#include <iostream>
#include <windows.h>
#include <memory>
#include <string.h>
#include "include/SystemMetrics.h"
#include "include/Configuration.h"
#include "include/Logger.h"
#include "include/ProcessManager.h"
#include "include/SystemMonitor.h"

// Application class for better organization
class SystemMonitorApplication {
private:
    std::unique_ptr<ConfigurationManager> configManager;
    std::shared_ptr<ISystemMonitor> systemMonitor;
    std::unique_ptr<IProcessManager> processManager;
    std::unique_ptr<ILogger> logger;
    bool isRunning = false;

    bool checkAdministratorPrivileges() const;
    void printStartupInfo() const;

public:
    SystemMonitorApplication();
    ~SystemMonitorApplication();

    bool initialize(int argc, char* argv[]);
    void run();
    void shutdown();

    // Getters
    const MonitorConfig& getConfig() const { return configManager->getConfig(); }
    bool getIsRunning() const { return isRunning; }
};

SystemMonitorApplication::SystemMonitorApplication() {
    configManager = std::make_unique<ConfigurationManager>();
}

SystemMonitorApplication::~SystemMonitorApplication() {
    shutdown();
}

bool SystemMonitorApplication::initialize(int argc, char* argv[]) {
    std::cout << "SystemMonitor initializing..." << std::endl;
    
    // Setup termination handler
    std::set_terminate([]() {
        std::cerr << "\nApplication terminated unexpectedly!" << std::endl;
        std::abort();
    });
    
    // Check for help flag first
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            configManager->printUsage();
            return false;
        }
    }
    
    // Load configuration from file
    if (configManager->loadFromFile("SystemMonitor.cfg")) {
        std::cout << "Loaded configuration from SystemMonitor.cfg" << std::endl;
    }
    
    // Parse command line arguments (overrides config file)
    if (!configManager->parseCommandLine(argc, argv)) {
        return false; // Help was shown or error occurred
    }
    
    // Validate configuration
    if (!configManager->validateConfiguration()) {
        std::cerr << "Invalid configuration. Please check your settings." << std::endl;
        return false;
    }
    
    printStartupInfo();
    
    // Initialize system monitor
    systemMonitor = SystemMonitorFactory::createWindowsMonitor();
    if (!systemMonitor || !systemMonitor->initialize()) {
        std::cerr << "Failed to initialize system monitor." << std::endl;
        return false;
    }
    
    // Initialize process manager
    processManager = ProcessManagerFactory::createWindowsManager(systemMonitor);
    if (!processManager || !processManager->initialize()) {
        std::cerr << "Failed to initialize process manager." << std::endl;
        return false;
    }
    
    // Initialize logger
    logger = LoggerFactory::createAsyncFileLogger(configManager->getConfig().getLogConfig());
    if (!logger || !logger->initialize()) {
        std::cerr << "Failed to initialize async logger." << std::endl;
        return false;
    }
    
    // Set up singleton logger manager
    LoggerManager::getInstance().setLogger(std::move(logger));
    
    // Check administrator privileges
    if (!checkAdministratorPrivileges()) {
        std::cout << "Warning: Running without administrator privileges. Some processes may not be accessible." << std::endl;
    }
    
    // Save current configuration
    if (configManager->saveToFile("SystemMonitor.cfg")) {
        std::cout << "Saved configuration to SystemMonitor.cfg" << std::endl;
    }
    
    isRunning = true;
    return true;
}

void SystemMonitorApplication::run() {
    if (!isRunning) {
        std::cerr << "Application not properly initialized." << std::endl;
        return;
    }
    
    std::cout << "Press Ctrl+C to exit.\n" << std::endl;
    
    unsigned int monitorCount = 0;
    const auto& config = configManager->getConfig();
    
    while (isRunning) {
        try {
            // Get system usage
            SystemUsage systemUsage = systemMonitor->getSystemUsage();
            
            // Get all processes
            std::vector<ProcessInfo> processes = processManager->getAllProcesses();
            
            // Aggregate process tree
            std::vector<ProcessInfo> aggregatedProcesses = processManager->getAggregatedProcessTree(processes);
            
            // Log processes (async - non-blocking)
            LoggerManager::getInstance().logProcesses(aggregatedProcesses, systemUsage);
            
            monitorCount++;
            size_t queueSize = LoggerManager::getInstance().getQueueSize();
            std::cout << "[" << monitorCount << "] System monitoring cycle completed (Log queue: " 
                      << queueSize << " messages)" << std::endl;
            
            // Sleep for the configured interval
            Sleep(config.getMonitorInterval());
            
        } catch (const std::exception& e) {
            LoggerManager::getInstance().debug("Exception in main loop: " + std::string(e.what()));
            Sleep(1000); // Brief pause on error
        } catch (...) {
            LoggerManager::getInstance().debug("Unknown exception in main loop");
            Sleep(1000); // Brief pause on error
        }
    }
}

void SystemMonitorApplication::shutdown() {
    isRunning = false;
    
    if (processManager) {
        processManager->shutdown();
    }
    
    if (systemMonitor) {
        systemMonitor->shutdown();
    }
    
    // Shutdown the async logger
    if (logger) {
        logger->shutdown();
    }
    
    std::cout << "SystemMonitor shutdown completed." << std::endl;
}

bool SystemMonitorApplication::checkAdministratorPrivileges() const {
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    
    return isAdmin != FALSE;
}

void SystemMonitorApplication::printStartupInfo() const {
    const auto& config = configManager->getConfig();
    
    std::cout << "SystemMonitor started with OOP architecture." << std::endl;
    std::cout << "Thresholds - CPU: " << config.getCpuThreshold() << "%, RAM: " 
              << config.getRamThreshold() << "%, Disk: " << config.getDiskThreshold() << "%\n";
    std::cout << "Monitoring interval: " << config.getMonitorInterval() << "ms\n";
    std::cout << "Log file: " << config.getLogFilePath() << std::endl;
    
    if (config.isDebugMode()) {
        std::cout << "Debug mode enabled" << std::endl;
    }
    
    const auto& logConfig = config.getLogConfig();
    std::cout << "Log rotation: " << (logConfig.isRotationEnabled() ? "Enabled" : "Disabled");
    if (logConfig.isRotationEnabled()) {
        std::cout << " (Max size: " << logConfig.getMaxFileSizeMB() << "MB, Backups: " 
                  << logConfig.getMaxBackupFiles() << ")";
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        SystemMonitorApplication app;
        
        if (!app.initialize(argc, argv)) {
            return 0; // Normal exit (help shown) or initialization failed
        }
        
        app.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred." << std::endl;
        return 1;
    }
    
    return 0;
}
