// main.cpp
// Entry point for SystemMonitor
#include <iostream>
#include <windows.h>
#include <memory>
#include <string.h>
#include <iomanip>
#include <conio.h>
#include <algorithm>
#include <chrono>
#include <sstream>
#include "include/SystemMetrics.h"
#include "include/Configuration.h"
#include "include/Logger.h"
#include "include/ProcessManager.h"
#include "include/SystemMonitor.h"
#include "include/EmailNotifier.h"
#include "include/SystemInfo.h"

    // Global flag to control console output during top-style display
bool g_suppressConsoleOutput = false;

// Application class for better organization
class SystemMonitorApplication {
private:
    std::unique_ptr<ConfigurationManager> configManager;
    std::shared_ptr<ISystemMonitor> systemMonitor;
    std::unique_ptr<IProcessManager> processManager;
    std::unique_ptr<ILogger> logger;
    std::unique_ptr<EmailNotifier> emailNotifier;
    bool isRunning = false;
    
    // Simple display variables
    HANDLE hConsole;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastDisplayUpdate;
    int displayMode = 0; // 0 = line-by-line, 1 = top-style, 2 = compact
    bool firstDisplay = true;
    std::vector<ProcessInfo> lastProcesses;

    bool checkAdministratorPrivileges() const;
    void printStartupInfo() const;
    void initializeDisplay();
    void showTopStyleDisplay(const std::vector<ProcessInfo>& processes, const SystemUsage& systemUsage);
    void showCompactDisplay(const std::vector<ProcessInfo>& processes, const SystemUsage& systemUsage);
    void clearScreen();
    void setCursorPosition(int row, int col);
    void hideCursor();
    void showCursor();
    bool shouldUpdateDisplay();
    bool checkForKeyPress();
    void handleKeyPress();

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
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    startTime = std::chrono::steady_clock::now();
    lastDisplayUpdate = std::chrono::steady_clock::now();
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
    if (configManager->loadFromFile("config\\SystemMonitor.cfg")) {
        std::cout << "Loaded configuration from config\\SystemMonitor.cfg" << std::endl;
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
    
    // Initialize email notifier
    emailNotifier = std::make_unique<EmailNotifier>(configManager->getConfig().getEmailConfig());
    if (emailNotifier->isEnabled()) {
        if (emailNotifier->testEmailConfiguration()) {
            std::cout << "Email notifications enabled and configured correctly." << std::endl;
            emailNotifier->start();
        } else {
            std::cout << "Warning: Email configuration test failed. Email alerts disabled." << std::endl;
            emailNotifier.reset(); // Disable email notifier
        }
    } else {
        std::cout << "Email notifications disabled." << std::endl;
    }
    
    // Initialize simple display
    initializeDisplay();
    
    // Check administrator privileges
    if (!checkAdministratorPrivileges()) {
        std::cout << "Warning: Running without administrator privileges. Some processes may not be accessible." << std::endl;
    }
    
    // Save current configuration only if config file does not exist
    DWORD attrib = GetFileAttributesA("config\\SystemMonitor.cfg");
    if (attrib == INVALID_FILE_ATTRIBUTES) {
        if (configManager->saveToFile("config\\SystemMonitor.cfg")) {
            std::cout << "Saved configuration to config\\SystemMonitor.cfg" << std::endl;
        }
    }
    
    isRunning = true;
    return true;
}

void SystemMonitorApplication::run() {
    if (!isRunning) {
        std::cerr << "Application not properly initialized." << std::endl;
        return;
    }
    
    std::cout << "SystemMonitor started in ";
    switch (displayMode) {
        case 0:
            std::cout << "line-by-line display mode." << std::endl;
            break;
        case 1:
            std::cout << "top-style display mode." << std::endl;
            break;
        case 2:
            std::cout << "compact display mode." << std::endl;
            break;
        case 3:
            std::cout << "silence display mode." << std::endl;
            break;
    }
    if (displayMode != 3) {
        std::cout << "Press 'q' to quit, 't' to toggle display mode." << std::endl;
        Sleep(2000); // Give user time to read the message
    } else {
        std::cout << "Silence mode: Output will be shown only when thresholds are exceeded." << std::endl;
        std::cout << "Press 'q' to quit, 't' to toggle display mode." << std::endl;
        Sleep(3000); // Give user more time to read silence mode message
    }
    
    unsigned int monitorCount = 0;
    const auto& config = configManager->getConfig();
    
    while (isRunning) {
        try {
            // Check for keyboard input
            if (checkForKeyPress()) {
                handleKeyPress();
            }
            
            // Get system usage
            SystemUsage systemUsage = systemMonitor->getSystemUsage();
            
            // Get all processes
            std::vector<ProcessInfo> processes = processManager->getAllProcesses();
            
            // Calculate system disk I/O by aggregating process values
            double totalDiskActivity = 0.0;
            for (const auto& process : processes) {
                totalDiskActivity += process.getDiskPercent();
            }
            
            // Create corrected system usage with aggregated disk I/O
            SystemUsage correctedSystemUsage(systemUsage.getCpuPercent(), 
                                           systemUsage.getRamPercent(), 
                                           totalDiskActivity);
            
            // Aggregate process tree
            std::vector<ProcessInfo> aggregatedProcesses = processManager->getAggregatedProcessTree(processes);
            
            // Display using simple system - only update every 2 seconds to reduce flashing
            bool systemExceedsThresholds = 
                systemUsage.getCpuPercent() > config.getCpuThreshold() ||
                systemUsage.getRamPercent() > config.getRamThreshold() ||
                systemUsage.getDiskPercent() > config.getDiskThreshold();
            
            if (displayMode == 1) {
                g_suppressConsoleOutput = true; // Suppress console output during top-style display
                if (shouldUpdateDisplay() || firstDisplay) {
                    showTopStyleDisplay(aggregatedProcesses, correctedSystemUsage);
                }
            } else if (displayMode == 2) {
                g_suppressConsoleOutput = true; // Suppress console output during compact display
                if (shouldUpdateDisplay() || firstDisplay) {
                    showCompactDisplay(aggregatedProcesses, correctedSystemUsage);
                }
            } else if (displayMode == 3) {
                g_suppressConsoleOutput = false; // Allow console output in silence mode when needed
                // Silence mode - only show output when thresholds are exceeded
                if (systemExceedsThresholds) {
                    // Get current time for the alert
                    auto now = std::chrono::system_clock::now();
                    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                    std::tm tm;
                    localtime_s(&tm, &now_c);
                    char timeStr[32];
                    std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm);
                    
                    std::cout << "[" << timeStr << "] THRESHOLD EXCEEDED - CPU: " 
                              << std::fixed << std::setprecision(1) << correctedSystemUsage.getCpuPercent() 
                              << "% (>" << config.getCpuThreshold() << "%) RAM: " 
                              << correctedSystemUsage.getRamPercent() << "% (>" << config.getRamThreshold() 
                              << "%) Disk: " << correctedSystemUsage.getDiskPercent() << "% (>" 
                              << config.getDiskThreshold() << "%)" << std::endl;
                    
                    // Show top 5 resource-consuming processes
                    std::vector<ProcessInfo> topProcesses = aggregatedProcesses;
                    std::sort(topProcesses.begin(), topProcesses.end(), 
                              [](const ProcessInfo& a, const ProcessInfo& b) {
                                  return (a.getCpuPercent() + a.getRamPercent() + a.getDiskPercent()) > 
                                         (b.getCpuPercent() + b.getRamPercent() + b.getDiskPercent());
                              });
                    
                    std::cout << "    Top processes: ";
                    for (size_t i = 0; i < std::min<size_t>(3, topProcesses.size()); ++i) {
                        const auto& proc = topProcesses[i];
                        if (i > 0) std::cout << ", ";
                        std::cout << proc.getName() << "[" << proc.getPid() << "] "
                                  << "(" << std::fixed << std::setprecision(1) 
                                  << proc.getCpuPercent() << "% CPU)";
                    }
                    std::cout << std::endl;
                }
            } else {
                g_suppressConsoleOutput = false; // Allow console output in line-by-line mode
                // Line-by-line display
                std::cout << "[" << monitorCount << "] CPU: " << correctedSystemUsage.getCpuPercent() 
                          << "% RAM: " << correctedSystemUsage.getRamPercent() 
                          << "% Processes: " << aggregatedProcesses.size() << std::endl;
            }
            
            // Log processes when system resources exceed thresholds
            if (systemExceedsThresholds || config.isDebugMode()) {
                // When system exceeds thresholds, log all processes consuming resources
                std::vector<ProcessInfo> processesToLog;
                
                for (const auto& process : aggregatedProcesses) {
                    // Log processes that are actively consuming resources (not idle)
                    if (process.getCpuPercent() > 0.1 || 
                        process.getRamPercent() > 0.1 || 
                        process.getDiskPercent() > 0.1) {
                        processesToLog.push_back(process);
                    }
                }
                
                // Log all active processes when system thresholds are exceeded
                LoggerManager::getInstance().logProcesses(processesToLog, correctedSystemUsage);
                
                // Email alerting for threshold violations
                if (emailNotifier && systemExceedsThresholds) {
                    // Generate detailed log entry for email alert (same format as logger)
                    std::ostringstream detailedLogEntry;
                    
                    // Calculate process usage totals
                    double totalProcessCpu = 0.0;
                    double totalProcessRam = 0.0;
                    double totalProcessDisk = 0.0;
                    
                    for (const auto& process : processesToLog) {
                        totalProcessCpu += process.getCpuPercent();
                        totalProcessRam += process.getRamPercent();
                        totalProcessDisk += process.getDiskPercent();
                    }
                    
                    // Calculate "unaccounted" usage (system overhead, kernel, cache, etc.)
                    double unaccountedCpu = correctedSystemUsage.getCpuPercent() - totalProcessCpu;
                    double unaccountedRam = correctedSystemUsage.getRamPercent() - totalProcessRam;
                    double unaccountedDisk = correctedSystemUsage.getDiskPercent() - totalProcessDisk;
                    if (unaccountedCpu < 0) unaccountedCpu = 0.0;
                    if (unaccountedRam < 0) unaccountedRam = 0.0;
                    if (unaccountedDisk < 0) unaccountedDisk = 0.0;
                    
                    // Get current time
                    auto now = std::chrono::system_clock::now();
                    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                    std::tm tm;
                    localtime_s(&tm, &now_c);
                    char timeStr[32];
                    std::strftime(timeStr, sizeof(timeStr), "%d-%m-%Y %H:%M:%S", &tm);
                    
                    // Create the detailed log entry in the same format as the logger
                    detailedLogEntry << "===Start " << timeStr 
                        << " [System CPU " << std::fixed << std::setprecision(2) << correctedSystemUsage.getCpuPercent()
                        << "%] [System RAM " << std::fixed << std::setprecision(2) << correctedSystemUsage.getRamPercent()
                        << "%] [System Disk " << std::fixed << std::setprecision(2) << correctedSystemUsage.getDiskPercent() 
                        << "%]===\n";
                    
                    // System analysis lines
                    detailedLogEntry << "SYSTEM ANALYSIS: CPU: Processes=" << std::fixed << std::setprecision(2) << totalProcessCpu 
                        << "% + System/Kernel=" << std::fixed << std::setprecision(2) << unaccountedCpu 
                        << "% = Total=" << std::fixed << std::setprecision(2) << correctedSystemUsage.getCpuPercent() << "%\n";
                    detailedLogEntry << "SYSTEM ANALYSIS: RAM: Processes=" << std::fixed << std::setprecision(2) << totalProcessRam 
                        << "% + System/Kernel=" << std::fixed << std::setprecision(2) << unaccountedRam 
                        << "% = Total=" << std::fixed << std::setprecision(2) << correctedSystemUsage.getRamPercent() << "%\n";
                    detailedLogEntry << "SYSTEM ANALYSIS: DISK: Processes=" << std::fixed << std::setprecision(2) << totalProcessDisk 
                        << "% + System/Kernel=" << std::fixed << std::setprecision(2) << unaccountedDisk 
                        << "% = Total=" << std::fixed << std::setprecision(2) << correctedSystemUsage.getDiskPercent() << "%\n";
                    
                    // Individual process entries
                    for (const auto& process : processesToLog) {
                        detailedLogEntry << timeStr << ", " 
                            << process.getName() << ", " 
                            << process.getPid() 
                            << ", [CPU " << std::fixed << std::setprecision(2) << process.getCpuPercent()
                            << "%] [RAM " << std::fixed << std::setprecision(2) << process.getRamPercent()
                            << "%] [Disk " << std::fixed << std::setprecision(2) << process.getDiskPercent() 
                            << "%]\n";
                    }
                    
                    // Resource totals
                    detailedLogEntry << "TOTALS: [Process CPU " << std::fixed << std::setprecision(2) << totalProcessCpu
                        << "%] [Process RAM " << std::fixed << std::setprecision(2) << totalProcessRam
                        << "%] [Process Disk " << std::fixed << std::setprecision(2) << totalProcessDisk << "%]\n";
                    
                    if (unaccountedRam > 5.0) { // Show significant system overhead
                        detailedLogEntry << "SYSTEM OVERHEAD: [CPU " << std::fixed << std::setprecision(2) << unaccountedCpu
                            << "%] [RAM " << std::fixed << std::setprecision(2) << unaccountedRam
                            << "%] [Disk " << std::fixed << std::setprecision(2) << unaccountedDisk << "%] (Kernel/Cache/Buffers)\n";
                    }
                    
                    // End banner
                    detailedLogEntry << "===End  " << timeStr 
                        << " [System CPU " << std::fixed << std::setprecision(2) << correctedSystemUsage.getCpuPercent()
                        << "%] [System RAM " << std::fixed << std::setprecision(2) << correctedSystemUsage.getRamPercent()
                        << "%] [System Disk " << std::fixed << std::setprecision(2) << correctedSystemUsage.getDiskPercent() 
                        << "%]===";
                    
                    emailNotifier->checkThresholds(true, detailedLogEntry.str());
                }
            } else if (emailNotifier) {
                // Reset email alert state when thresholds are no longer exceeded
                emailNotifier->checkThresholds(false, "");
            }
            
            monitorCount++;
            
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
    
    // Restore cursor visibility
    showCursor();
    
    // Shutdown email notifier
    if (emailNotifier) {
        emailNotifier->stop();
    }
    
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
    std::cout << "Display mode: ";
    switch (config.getDisplayMode()) {
        case DisplayModeConfig::LINE_BY_LINE:
            std::cout << "Line-by-line";
            break;
        case DisplayModeConfig::TOP_STYLE:
            std::cout << "Top-style table";
            break;
        case DisplayModeConfig::COMPACT:
            std::cout << "Compact table";
            break;
        case DisplayModeConfig::SILENCE:
            std::cout << "Silence mode";
            break;
    }
    std::cout << std::endl;
    std::cout << "Log file: " << config.getLogFilePath() << std::endl;
    
    if (config.isDebugMode()) {
        std::cout << "Debug mode enabled" << std::endl;
    }
    
    const auto& logConfig = config.getLogConfig();
    std::cout << "Log rotation: " << (logConfig.isRotationEnabled() ? "Enabled" : "Disabled");
    if (logConfig.isRotationEnabled()) {
        std::string strategyStr = "UNKNOWN";
        switch (logConfig.getRotationStrategy()) {
            case LogRotationStrategy::SIZE_BASED: strategyStr = "SIZE_BASED"; break;
            case LogRotationStrategy::DATE_BASED: strategyStr = "DATE_BASED"; break;
            case LogRotationStrategy::COMBINED: strategyStr = "COMBINED"; break;
        }
        std::cout << " (Strategy: " << strategyStr;
        if (logConfig.isSizeBasedRotation()) {
            std::cout << ", Max size: " << logConfig.getMaxFileSizeMB() << "MB";
        }
        std::cout << ", Backups: " << logConfig.getMaxBackupFiles() << ")";
    }
    std::cout << std::endl;
    // Only show log date frequency and format if not SIZE_BASED rotation
    if (logConfig.getRotationStrategy() != LogRotationStrategy::SIZE_BASED) {
        std::string dateFreqStr = "UNKNOWN";
        switch (logConfig.getDateFrequency()) {
            case DateRotationFrequency::DAILY: dateFreqStr = "DAILY"; break;
            // Add more cases here if more enum values are added in the future
        }
        std::cout << "Log date frequency: " << dateFreqStr << std::endl;
        std::cout << "Log date format: " << logConfig.getDateFormat() << std::endl;
    }
    
    // Email configuration info
    const auto& emailConfig = config.getEmailConfig();
    std::cout << "Email alerts: " << (emailConfig.enableEmailAlerts ? "Enabled" : "Disabled");
    if (emailConfig.enableEmailAlerts) {
        std::cout << " (Alert duration: " << emailConfig.alertDurationSeconds 
                  << "s, Cooldown: " << emailConfig.cooldownMinutes << "m)";
    }
    std::cout << std::endl;
}

void SystemMonitorApplication::initializeDisplay() {
    // Set initial display mode from configuration
    DisplayModeConfig configMode = configManager->getConfig().getDisplayMode();
    switch (configMode) {
        case DisplayModeConfig::LINE_BY_LINE:
            displayMode = 0;
            break;
        case DisplayModeConfig::TOP_STYLE:
            displayMode = 1;
            break;
        case DisplayModeConfig::COMPACT:
            displayMode = 2; // Now implemented
            break;
        case DisplayModeConfig::SILENCE:
            displayMode = 3; // Silence mode
            break;
    }
    
    if (displayMode == 1 || displayMode == 2) {
        hideCursor(); // Hide cursor for cleaner display in top-style and compact modes
    }
}

void SystemMonitorApplication::clearScreen() {
    COORD coordScreen = { 0, 0 };
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;
    
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        return;
    }
    
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
    
    FillConsoleOutputCharacter(hConsole, ' ', dwConSize, coordScreen, &cCharsWritten);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
    SetConsoleCursorPosition(hConsole, coordScreen);
}

void SystemMonitorApplication::setCursorPosition(int row, int col) {
    COORD coord;
    coord.X = col;
    coord.Y = row;
    SetConsoleCursorPosition(hConsole, coord);
}

void SystemMonitorApplication::hideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void SystemMonitorApplication::showCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

bool SystemMonitorApplication::shouldUpdateDisplay() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDisplayUpdate).count();
    
    // Update display every 2 seconds instead of every monitoring cycle
    if (elapsed >= 2000) {
        lastDisplayUpdate = now;
        return true;
    }
    return false;
}

void SystemMonitorApplication::showTopStyleDisplay(const std::vector<ProcessInfo>& processes, const SystemUsage& systemUsage) {
    // Only clear screen on first display or mode change
    if (firstDisplay) {
        clearScreen();
        firstDisplay = false;
    }
    
    // Always go to top of screen for updates
    setCursorPosition(0, 0);
    
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    
    // Header information (always update for real-time data)
    std::cout << "SystemMonitor - Uptime: " << std::setw(4) << uptime << "s | Processes: " << std::setw(3) << processes.size();
    // Clear rest of line to remove any leftover characters
    std::cout << std::string(20, ' ') << "\n";
    
    std::cout << "CPU: " << std::fixed << std::setprecision(1) << std::setw(5) << systemUsage.getCpuPercent() << "%";
    std::cout << " | RAM: " << std::fixed << std::setprecision(1) << std::setw(5) << systemUsage.getRamPercent() << "%";
    std::cout << " | Disk: " << std::fixed << std::setprecision(1) << std::setw(5) << systemUsage.getDiskPercent() << "%";
    std::cout << std::string(20, ' ') << "\n"; // Clear rest of line
    
    std::cout << std::string(80, '-') << "\n";
    std::cout << std::setw(8) << "PID" << std::setw(20) << "Process Name" 
              << std::setw(8) << "CPU%" << std::setw(8) << "RAM%" << std::setw(8) << "Disk%" << "\n";
    std::cout << std::string(80, '-') << "\n";
    
    // Sort processes by CPU usage
    std::vector<ProcessInfo> sortedProcesses = processes;
    std::sort(sortedProcesses.begin(), sortedProcesses.end(), 
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.getCpuPercent() > b.getCpuPercent();
              });
    
    // Show top processes (limit to 20 to fit on screen)
    size_t maxToShow = (sortedProcesses.size() < 20) ? sortedProcesses.size() : 20;
    for (size_t i = 0; i < maxToShow; ++i) {
        const auto& process = sortedProcesses[i];
        std::string name = process.getName();
        if (name.length() > 19) {
            name = name.substr(0, 16) + "...";
        }
        
        std::cout << std::setw(8) << process.getPid()
                  << std::setw(20) << name
                  << std::setw(7) << std::fixed << std::setprecision(1) << process.getCpuPercent() << "%"
                  << std::setw(7) << std::fixed << std::setprecision(1) << process.getRamPercent() << "%"
                  << std::setw(7) << std::fixed << std::setprecision(1) << process.getDiskPercent() << "%";
        std::cout << std::string(15, ' ') << "\n"; // Clear rest of line
    }
    
    // Fill remaining lines with spaces to clear old content
    for (size_t i = maxToShow; i < 20; ++i) {
        std::cout << std::string(80, ' ') << "\n";
    }
    
    std::cout << std::string(80, '-') << "\n";
    std::cout << "Controls: [q]uit [t]oggle display mode" << std::string(30, ' ') << "\n";
    
    // Flush output to ensure immediate display
    std::cout.flush();
}

void SystemMonitorApplication::showCompactDisplay(const std::vector<ProcessInfo>& processes, const SystemUsage& systemUsage) {
    // Only clear screen on first display or mode change
    if (firstDisplay) {
        clearScreen();
        firstDisplay = false;
    }
    
    // Always go to top of screen for updates
    setCursorPosition(0, 0);
    
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    
    // Compact header - single line with all key info
    std::cout << "SystemMonitor [" << std::setw(4) << uptime << "s] CPU:" 
              << std::fixed << std::setprecision(1) << std::setw(5) << systemUsage.getCpuPercent() << "% RAM:"
              << std::fixed << std::setprecision(1) << std::setw(5) << systemUsage.getRamPercent() << "% Disk:"
              << std::fixed << std::setprecision(1) << std::setw(5) << systemUsage.getDiskPercent() << "% Proc:"
              << std::setw(3) << processes.size();
    std::cout << std::string(15, ' ') << "\n"; // Clear rest of line
    
    // Sort processes by total resource usage (CPU + RAM + Disk)
    std::vector<ProcessInfo> sortedProcesses = processes;
    std::sort(sortedProcesses.begin(), sortedProcesses.end(), 
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  double totalA = a.getCpuPercent() + a.getRamPercent() + a.getDiskPercent();
                  double totalB = b.getCpuPercent() + b.getRamPercent() + b.getDiskPercent();
                  return totalA > totalB;
              });
    
    // Compact process list - only show processes using significant resources
    std::cout << "Top Resource Consumers:" << std::string(40, ' ') << "\n";
    int lineCount = 0;
    const int maxLines = 10; // Limit to 10 lines for compact view
    
    for (const auto& process : sortedProcesses) {
        // Only show processes with significant resource usage
        if ((process.getCpuPercent() > 0.5 || process.getRamPercent() > 1.0 || process.getDiskPercent() > 0.1) 
            && lineCount < maxLines) {
            
            std::string name = process.getName();
            if (name.length() > 12) {
                name = name.substr(0, 9) + "...";
            }
            
            // Compact format: name[pid] C:x.x% R:x.x% D:x.x%
            std::cout << std::setw(13) << name << "[" << std::setw(5) << process.getPid() << "] "
                      << "C:" << std::fixed << std::setprecision(1) << std::setw(4) << process.getCpuPercent() << "% "
                      << "R:" << std::fixed << std::setprecision(1) << std::setw(4) << process.getRamPercent() << "% "
                      << "D:" << std::fixed << std::setprecision(1) << std::setw(4) << process.getDiskPercent() << "%";
            std::cout << std::string(20, ' ') << "\n"; // Clear rest of line
            lineCount++;
        }
    }
    
    // Fill remaining lines with spaces if needed
    for (int i = lineCount; i < maxLines; ++i) {
        std::cout << std::string(70, ' ') << "\n";
    }
    
    // Calculate system resource distribution
    double totalProcessCpu = 0.0, totalProcessRam = 0.0, totalProcessDisk = 0.0;
    for (const auto& process : processes) {
        totalProcessCpu += process.getCpuPercent();
        totalProcessRam += process.getRamPercent();
        totalProcessDisk += process.getDiskPercent();
    }
    
    double systemCpu = systemUsage.getCpuPercent() - totalProcessCpu;
    double systemRam = systemUsage.getRamPercent() - totalProcessRam;
    double systemDisk = systemUsage.getDiskPercent() - totalProcessDisk;
    if (systemCpu < 0) systemCpu = 0.0;
    if (systemRam < 0) systemRam = 0.0;
    if (systemDisk < 0) systemDisk = 0.0;
    
    // Compact resource summary
    std::cout << "Resource Split: Processes[C:" << std::fixed << std::setprecision(1) << totalProcessCpu 
              << "% R:" << totalProcessRam << "% D:" << totalProcessDisk << "%] System[C:" 
              << systemCpu << "% R:" << systemRam << "% D:" << systemDisk << "%]";
    std::cout << std::string(5, ' ') << "\n"; // Clear rest of line
    
    // Compact status and controls
    std::cout << "Status: " << (systemUsage.getCpuPercent() > 80 || systemUsage.getRamPercent() > 80 ? "HIGH LOAD" : "Normal")
              << " | Controls: [q]uit [t]oggle mode" << std::string(20, ' ') << "\n";
    
    // Fill remaining screen with blank lines
    for (int i = 0; i < 5; ++i) {
        std::cout << std::string(70, ' ') << "\n";
    }
    
    // Flush output to ensure immediate display
    std::cout.flush();
}

bool SystemMonitorApplication::checkForKeyPress() {
    return _kbhit() != 0;
}

void SystemMonitorApplication::handleKeyPress() {
    char key = _getch();
    switch (tolower(key)) {
        case 'q':
            g_suppressConsoleOutput = false; // Restore console output before exiting
            showCursor(); // Show cursor before exiting
            isRunning = false;
            break;
        case 't':
            displayMode = (displayMode + 1) % 4; // Cycle through 0, 1, 2, 3 (line, top, compact, silence)
            g_suppressConsoleOutput = (displayMode == 1 || displayMode == 2); // Set based on new mode
            firstDisplay = true; // Force screen clear on mode change
            if (displayMode == 1 || displayMode == 2) {
                hideCursor();
            } else {
                showCursor();
            }
            break;
    }
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
