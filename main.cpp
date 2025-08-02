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
#include "include/SystemMetrics.h"
#include "include/Configuration.h"
#include "include/Logger.h"
#include "include/ProcessManager.h"
#include "include/SystemMonitor.h"

    // Global flag to control console output during top-style display
bool g_suppressConsoleOutput = false;

// Application class for better organization
class SystemMonitorApplication {
private:
    std::unique_ptr<ConfigurationManager> configManager;
    std::shared_ptr<ISystemMonitor> systemMonitor;
    std::unique_ptr<IProcessManager> processManager;
    std::unique_ptr<ILogger> logger;
    bool isRunning = false;
    
    // Simple display variables
    HANDLE hConsole;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastDisplayUpdate;
    int displayMode = 0; // 0 = line-by-line, 1 = top-style
    bool firstDisplay = true;
    std::vector<ProcessInfo> lastProcesses;

    bool checkAdministratorPrivileges() const;
    void printStartupInfo() const;
    void initializeDisplay();
    void showTopStyleDisplay(const std::vector<ProcessInfo>& processes, const SystemUsage& systemUsage);
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
    
    // Initialize simple display
    initializeDisplay();
    
    // Check administrator privileges
    if (!checkAdministratorPrivileges()) {
        std::cout << "Warning: Running without administrator privileges. Some processes may not be accessible." << std::endl;
    }
    
    // Save current configuration
    if (configManager->saveToFile("config\\SystemMonitor.cfg")) {
        std::cout << "Saved configuration to config\\SystemMonitor.cfg" << std::endl;
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
    }
    std::cout << "Press 'q' to quit, 't' to toggle display mode." << std::endl;
    Sleep(2000); // Give user time to read the message
    
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
            if (displayMode == 1) {
                g_suppressConsoleOutput = true; // Suppress console output during top-style display
                if (shouldUpdateDisplay() || firstDisplay) {
                    showTopStyleDisplay(aggregatedProcesses, correctedSystemUsage);
                }
            } else {
                g_suppressConsoleOutput = false; // Allow console output in line-by-line mode
                // Line-by-line display
                std::cout << "[" << monitorCount << "] CPU: " << correctedSystemUsage.getCpuPercent() 
                          << "% RAM: " << correctedSystemUsage.getRamPercent() 
                          << "% Processes: " << aggregatedProcesses.size() << std::endl;
            }
            
            // Log processes when system resources exceed thresholds
            bool systemExceedsThresholds = 
                systemUsage.getCpuPercent() > config.getCpuThreshold() ||
                systemUsage.getRamPercent() > config.getRamThreshold() ||
                systemUsage.getDiskPercent() > config.getDiskThreshold();
            
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
    }
    std::cout << std::endl;
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
            displayMode = 2; // Future implementation
            break;
    }
    
    if (displayMode == 1) {
        hideCursor(); // Hide cursor for cleaner display in top-style mode
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
            displayMode = (displayMode == 0) ? 1 : 0;
            g_suppressConsoleOutput = (displayMode == 1); // Set based on new mode
            firstDisplay = true; // Force screen clear on mode change
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
