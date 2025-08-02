#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include "SystemMetrics.h"

// Console display modes
enum class DisplayMode {
    LINE_BY_LINE,    // Traditional line output
    TOP_STYLE,       // Interactive table display like Linux top
    COMPACT          // Compact table view
};

// Console control and formatting
class ConsoleDisplay {
private:
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int consoleWidth;
    int consoleHeight;
    bool isInteractiveMode;
    DisplayMode currentMode;
    
    // Display state
    std::chrono::steady_clock::time_point startTime;
    size_t totalCycles;
    
    // Sorting and filtering
    enum class SortColumn {
        PID, NAME, CPU, RAM, DISK
    };
    SortColumn sortBy;
    bool sortDescending;
    
    // Private methods
    void initializeConsole();
    void getConsoleSize();
    void clearScreen();
    void setCursorPosition(int x, int y);
    void setTextColor(int color);
    void hideCursor();
    void showCursor();
    std::string formatBytes(ULONGLONG bytes) const;
    std::string formatPercentage(double percent, int width = 6) const;
    std::string truncateString(const std::string& str, size_t maxLength) const;
    void sortProcesses(std::vector<ProcessInfo>& processes) const;

public:
    ConsoleDisplay();
    ~ConsoleDisplay();
    
    // Disable copy constructor and assignment operator
    ConsoleDisplay(const ConsoleDisplay&) = delete;
    ConsoleDisplay& operator=(const ConsoleDisplay&) = delete;
    
    // Display mode management
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const { return currentMode; }
    
    // Main display methods
    void showTopStyleDisplay(const std::vector<ProcessInfo>& processes, 
                           const SystemUsage& systemUsage);
    void showLineByLineDisplay(const std::vector<ProcessInfo>& processes, 
                             const SystemUsage& systemUsage);
    void showCompactDisplay(const std::vector<ProcessInfo>& processes, 
                          const SystemUsage& systemUsage);
    
    // Interactive controls
    void showHeader(const SystemUsage& systemUsage, size_t processCount);
    void showColumnHeaders();
    void showProcessTable(const std::vector<ProcessInfo>& processes);
    void showFooter();
    void showHelp();
    
    // Utility methods
    void updateStats();
    bool checkForKeyPress();
    char getKeyPress();
    void handleKeyPress(char key, std::vector<ProcessInfo>& processes);
    
    // Status methods
    bool isInteractive() const { return isInteractiveMode; }
    void setInteractive(bool interactive) { isInteractiveMode = interactive; }
    
    // Console colors
    static constexpr int COLOR_NORMAL = 7;
    static constexpr int COLOR_HEADER = 11;    // Bright cyan
    static constexpr int COLOR_HIGH_CPU = 12;  // Bright red
    static constexpr int COLOR_HIGH_RAM = 14;  // Bright yellow
    static constexpr int COLOR_GOOD = 10;      // Bright green
    static constexpr int COLOR_WARNING = 6;    // Dark yellow
};

// Display factory for creating different display types
class DisplayFactory {
public:
    static std::unique_ptr<ConsoleDisplay> createTopStyleDisplay();
    static std::unique_ptr<ConsoleDisplay> createLineDisplay();
    static std::unique_ptr<ConsoleDisplay> createCompactDisplay();
};
