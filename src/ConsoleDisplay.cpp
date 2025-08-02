#include "../include/ConsoleDisplay.h"
#include "../include/SystemMetrics.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <conio.h>
#include <cctype>

#ifndef NOMINMAX
#define NOMINMAX
#endif

// Global flag to control console output (defined in main.cpp)
extern bool g_suppressConsoleOutput;

ConsoleDisplay::ConsoleDisplay() 
    : hConsole(nullptr), consoleWidth(80), consoleHeight(25), 
      isInteractiveMode(false), currentMode(DisplayMode::LINE_BY_LINE),
      totalCycles(0), sortBy(SortColumn::CPU), sortDescending(true) {
    initializeConsole();
    startTime = std::chrono::steady_clock::now();
}

ConsoleDisplay::~ConsoleDisplay() {
    if (isInteractiveMode) {
        showCursor();
        setTextColor(COLOR_NORMAL);
    }
}

void ConsoleDisplay::initializeConsole() {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        getConsoleSize();
    }
}

void ConsoleDisplay::getConsoleSize() {
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        consoleHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
}

void ConsoleDisplay::clearScreen() {
    if (!g_suppressConsoleOutput) {
        system("cls");
    }
}

void ConsoleDisplay::setCursorPosition(int x, int y) {
    if (hConsole != INVALID_HANDLE_VALUE) {
        COORD coord = {static_cast<SHORT>(x), static_cast<SHORT>(y)};
        SetConsoleCursorPosition(hConsole, coord);
    }
}

void ConsoleDisplay::setTextColor(int color) {
    if (hConsole != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(hConsole, static_cast<WORD>(color));
    }
}

void ConsoleDisplay::hideCursor() {
    if (hConsole != INVALID_HANDLE_VALUE) {
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }
}

void ConsoleDisplay::showCursor() {
    if (hConsole != INVALID_HANDLE_VALUE) {
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = TRUE;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }
}

std::string ConsoleDisplay::formatBytes(ULONGLONG bytes) const {
    std::stringstream ss;
    if (bytes >= 1024ULL * 1024 * 1024) {
        ss << std::fixed << std::setprecision(1) << (double)bytes / (1024.0 * 1024 * 1024) << "GB";
    } else if (bytes >= 1024ULL * 1024) {
        ss << std::fixed << std::setprecision(1) << (double)bytes / (1024.0 * 1024) << "MB";
    } else if (bytes >= 1024ULL) {
        ss << std::fixed << std::setprecision(1) << (double)bytes / 1024.0 << "KB";
    } else {
        ss << bytes << "B";
    }
    return ss.str();
}

std::string ConsoleDisplay::formatPercentage(double percent, int width) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << std::setw(width) << percent << "%";
    return ss.str();
}

std::string ConsoleDisplay::truncateString(const std::string& str, size_t maxLength) const {
    if (str.length() <= maxLength) {
        return str;
    }
    return str.substr(0, maxLength - 3) + "...";
}

void ConsoleDisplay::sortProcesses(std::vector<ProcessInfo>& processes) const {
    std::sort(processes.begin(), processes.end(), [this](const ProcessInfo& a, const ProcessInfo& b) {
        bool result = false;
        switch (sortBy) {
            case SortColumn::PID:
                result = a.getPid() < b.getPid();
                break;
            case SortColumn::NAME:
                result = a.getName() < b.getName();
                break;
            case SortColumn::CPU:
                result = a.getCpuPercent() < b.getCpuPercent();
                break;
            case SortColumn::RAM:
                result = a.getRamPercent() < b.getRamPercent();
                break;
            case SortColumn::DISK:
                result = a.getDiskPercent() < b.getDiskPercent();
                break;
        }
        return sortDescending ? !result : result;
    });
}

void ConsoleDisplay::setDisplayMode(DisplayMode mode) {
    currentMode = mode;
    isInteractiveMode = (mode == DisplayMode::TOP_STYLE);
    
    if (isInteractiveMode) {
        hideCursor();
    } else {
        showCursor();
    }
}

void ConsoleDisplay::showTopStyleDisplay(const std::vector<ProcessInfo>& processes, 
                                       const SystemUsage& systemUsage) {
    if (g_suppressConsoleOutput) {
        return;
    }
    
    // Clear screen and move to top
    clearScreen();
    setCursorPosition(0, 0);
    
    // Make a copy for sorting
    std::vector<ProcessInfo> sortedProcesses = processes;
    sortProcesses(sortedProcesses);
    
    // Show header with system information
    showHeader(systemUsage, processes.size());
    
    // Show column headers
    showColumnHeaders();
    
    // Show process table (limited to screen height)
    int maxRows = consoleHeight - 8; // Reserve space for header and footer
    if (maxRows > 0) {
        size_t displayCount = (maxRows < static_cast<int>(sortedProcesses.size())) ? 
                               static_cast<size_t>(maxRows) : sortedProcesses.size();
        std::vector<ProcessInfo> displayProcesses(sortedProcesses.begin(), 
                                                sortedProcesses.begin() + displayCount);
        showProcessTable(displayProcesses);
    }
    
    // Show footer
    showFooter();
    
    // Update statistics
    updateStats();
}

void ConsoleDisplay::showLineByLineDisplay(const std::vector<ProcessInfo>& processes, 
                                         const SystemUsage& systemUsage) {
    if (g_suppressConsoleOutput) {
        return;
    }
    
    std::cout << "\n=== System Monitor ===" << std::endl;
    std::cout << "System Usage: CPU: " << std::fixed << std::setprecision(1) << systemUsage.getCpuPercent() 
              << "%, RAM: " << std::fixed << std::setprecision(1) << systemUsage.getRamPercent() 
              << "%, Disk I/O: " << std::fixed << std::setprecision(1) << systemUsage.getDiskPercent() << "%" << std::endl;
    std::cout << "\nTop Processes by CPU Usage:" << std::endl;
    
    // Sort by CPU usage
    std::vector<ProcessInfo> sortedProcesses = processes;
    std::sort(sortedProcesses.begin(), sortedProcesses.end(), 
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.getCpuPercent() > b.getCpuPercent();
              });
    
    // Display top 10 processes
    size_t displayCount = (sortedProcesses.size() < 10) ? sortedProcesses.size() : 10;
    for (size_t i = 0; i < displayCount; ++i) {
        const auto& proc = sortedProcesses[i];
        std::cout << std::left << std::setw(25) << truncateString(proc.getName(), 24)
                  << " PID: " << std::setw(8) << proc.getPid()
                  << " CPU: " << formatPercentage(proc.getCpuPercent(), 5)
                  << " RAM: " << formatPercentage(proc.getRamPercent(), 5)
                  << " Disk: " << formatPercentage(proc.getDiskPercent(), 5) << std::endl;
    }
}

void ConsoleDisplay::showCompactDisplay(const std::vector<ProcessInfo>& processes, 
                                      const SystemUsage& systemUsage) {
    if (g_suppressConsoleOutput) {
        return;
    }
    
    std::cout << "SYS: CPU:" << std::fixed << std::setprecision(1) << systemUsage.getCpuPercent() 
              << "% RAM:" << systemUsage.getRamPercent() 
              << "% DISK:" << systemUsage.getDiskPercent() << "% | ";
    
    // Show top 3 processes by CPU
    std::vector<ProcessInfo> sortedProcesses = processes;
    std::sort(sortedProcesses.begin(), sortedProcesses.end(), 
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.getCpuPercent() > b.getCpuPercent();
              });
    
    size_t displayCount = (sortedProcesses.size() < 3) ? sortedProcesses.size() : 3;
    for (size_t i = 0; i < displayCount; ++i) {
        if (i > 0) std::cout << ", ";
        const auto& proc = sortedProcesses[i];
        std::cout << truncateString(proc.getName(), 12) << "(" 
                  << std::fixed << std::setprecision(1) << proc.getCpuPercent() << "%)";
    }
    std::cout << std::endl;
}

void ConsoleDisplay::showHeader(const SystemUsage& systemUsage, size_t processCount) {
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    
    setTextColor(COLOR_HEADER);
    std::cout << "System Monitor - Uptime: " << uptime.count() << "s, Processes: " << processCount 
              << ", Cycles: " << totalCycles << std::endl;
    
    std::cout << "CPU: ";
    setTextColor(systemUsage.getCpuPercent() > 80 ? COLOR_HIGH_CPU : COLOR_GOOD);
    std::cout << formatPercentage(systemUsage.getCpuPercent(), 5);
    
    setTextColor(COLOR_HEADER);
    std::cout << "  RAM: ";
    setTextColor(systemUsage.getRamPercent() > 80 ? COLOR_HIGH_RAM : COLOR_GOOD);
    std::cout << formatPercentage(systemUsage.getRamPercent(), 5);
    
    setTextColor(COLOR_HEADER);
    std::cout << "  Disk I/O: ";
    setTextColor(systemUsage.getDiskPercent() > 50 ? COLOR_WARNING : COLOR_GOOD);
    std::cout << formatPercentage(systemUsage.getDiskPercent(), 5) << std::endl;
    
    setTextColor(COLOR_NORMAL);
    std::cout << std::endl;
}

void ConsoleDisplay::showColumnHeaders() {
    setTextColor(COLOR_HEADER);
    std::cout << std::left 
              << std::setw(8) << "PID"
              << std::setw(25) << "Process Name"
              << std::setw(8) << "CPU%"
              << std::setw(8) << "RAM%"
              << std::setw(8) << "Disk%" << std::endl;
    
    std::cout << std::string(consoleWidth - 1, '-') << std::endl;
    setTextColor(COLOR_NORMAL);
}

void ConsoleDisplay::showProcessTable(const std::vector<ProcessInfo>& processes) {
    for (const auto& proc : processes) {
        // Color coding based on usage
        if (proc.getCpuPercent() > 50) {
            setTextColor(COLOR_HIGH_CPU);
        } else if (proc.getRamPercent() > 30) {
            setTextColor(COLOR_HIGH_RAM);
        } else {
            setTextColor(COLOR_NORMAL);
        }
        
        std::cout << std::left 
                  << std::setw(8) << proc.getPid()
                  << std::setw(25) << truncateString(proc.getName(), 24)
                  << std::setw(8) << std::fixed << std::setprecision(1) << proc.getCpuPercent() << "%"
                  << std::setw(8) << std::fixed << std::setprecision(1) << proc.getRamPercent() << "%"
                  << std::setw(8) << std::fixed << std::setprecision(1) << proc.getDiskPercent() << "%" << std::endl;
    }
    setTextColor(COLOR_NORMAL);
}

void ConsoleDisplay::showFooter() {
    setTextColor(COLOR_HEADER);
    std::cout << std::string(consoleWidth - 1, '-') << std::endl;
    std::cout << "Press 'q' to quit, 'h' for help, 'c' for CPU sort, 'm' for RAM sort, 'd' for disk sort" << std::endl;
    setTextColor(COLOR_NORMAL);
}

void ConsoleDisplay::showHelp() {
    clearScreen();
    setTextColor(COLOR_HEADER);
    std::cout << "System Monitor Help" << std::endl;
    std::cout << "==================" << std::endl;
    setTextColor(COLOR_NORMAL);
    std::cout << "q, Q - Quit the application" << std::endl;
    std::cout << "c, C - Sort by CPU usage" << std::endl;
    std::cout << "m, M - Sort by RAM usage" << std::endl;
    std::cout << "d, D - Sort by Disk I/O" << std::endl;
    std::cout << "p, P - Sort by Process ID" << std::endl;
    std::cout << "n, N - Sort by Process Name" << std::endl;
    std::cout << "r, R - Reverse sort order" << std::endl;
    std::cout << "h, H - Show this help" << std::endl;
    std::cout << std::endl;
    std::cout << "Press any key to continue..." << std::endl;
    _getch();
}

void ConsoleDisplay::updateStats() {
    totalCycles++;
}

bool ConsoleDisplay::checkForKeyPress() {
    return _kbhit() != 0;
}

char ConsoleDisplay::getKeyPress() {
    return _getch();
}

void ConsoleDisplay::handleKeyPress(char key, std::vector<ProcessInfo>& processes) {
    switch (tolower(key)) {
        case 'q':
            exit(0);
            break;
        case 'c':
            sortBy = SortColumn::CPU;
            break;
        case 'm':
            sortBy = SortColumn::RAM;
            break;
        case 'd':
            sortBy = SortColumn::DISK;
            break;
        case 'p':
            sortBy = SortColumn::PID;
            break;
        case 'n':
            sortBy = SortColumn::NAME;
            break;
        case 'r':
            sortDescending = !sortDescending;
            break;
        case 'h':
            showHelp();
            break;
    }
}

// DisplayFactory implementation
std::unique_ptr<ConsoleDisplay> DisplayFactory::createTopStyleDisplay() {
    auto display = std::make_unique<ConsoleDisplay>();
    display->setDisplayMode(DisplayMode::TOP_STYLE);
    return display;
}

std::unique_ptr<ConsoleDisplay> DisplayFactory::createLineDisplay() {
    auto display = std::make_unique<ConsoleDisplay>();
    display->setDisplayMode(DisplayMode::LINE_BY_LINE);
    return display;
}

std::unique_ptr<ConsoleDisplay> DisplayFactory::createCompactDisplay() {
    auto display = std::make_unique<ConsoleDisplay>();
    display->setDisplayMode(DisplayMode::COMPACT);
    return display;
}
