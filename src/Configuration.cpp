#include "../include/Configuration.h"
#include <iostream>
#include <fstream>
#include <string.h> // For strcmp

// Validate threshold is within range 0-100
double ValidateThreshold(const char* value, const char* paramName) {
    try {
        double val = std::stod(value);
        if (val < 0.0 || val > 100.0) {
            std::cerr << "Error: " << paramName << " threshold must be between 0 and 100. Using default value." << std::endl;
            return 80.0; // Default value
        }
        return val;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing " << paramName << " value '" << value << "': " << e.what() << std::endl;
        return 80.0; // Default value
    }
}

// Check if a string is a valid parameter name
bool IsValidParameter(const char* param) {
    const char* validParams[] = {
        "--cpu", "--ram", "--disk", "-disk", // Support both --disk and -disk
        "--help", "-h", 
        "--interval", "--debug"
    };
    for (const char* valid : validParams) {
        if (strcmp(param, valid) == 0) {
            return true;
        }
    }
    return false;
}

// Print help information
void PrintUsage() {
    std::cout << "SystemMonitor - A utility to monitor system resource usage\n\n"
              << "Usage: SystemMonitor [options]\n\n"
              << "Options:\n"
              << "  --cpu PERCENT     CPU threshold percentage (default: 80.0)\n"
              << "  --ram PERCENT     RAM threshold percentage (default: 80.0)\n"
              << "  --disk PERCENT    Disk threshold percentage (default: 80.0)\n"
              << "  --interval MS     Monitoring interval in milliseconds (default: 5000)\n"
              << "  --debug           Enable debug logging to SystemMonitor_debug.log\n"
              << "  --help, -h        Display this help message\n";
}

// Load configuration from a file
bool LoadConfigFile(MonitorConfig& config) {
    std::ifstream configFile("SystemMonitor.cfg");
    if (!configFile.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        size_t pos = line.find("=");
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Remove any whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (key == "CPU_THRESHOLD") {
            config.cpuThreshold = ValidateThreshold(value.c_str(), "CPU");
        } else if (key == "RAM_THRESHOLD") {
            config.ramThreshold = ValidateThreshold(value.c_str(), "RAM");
        } else if (key == "DISK_THRESHOLD") {
            config.diskThreshold = ValidateThreshold(value.c_str(), "Disk");
        } else if (key == "MONITOR_INTERVAL") {
            try {
                int interval = std::stoi(value);
                if (interval >= 1000) {
                    config.monitorInterval = interval;
                }
            } catch (...) {
                // Ignore parsing errors
            }
        } else if (key == "LOG_PATH") {
            config.logFilePath = value;
        } else if (key == "DEBUG_MODE") {
            config.debugMode = (value == "true" || value == "1" || value == "yes");
        }
    }

    return true;
}

// Save configuration to a file
bool SaveConfigFile(const MonitorConfig& config) {
    std::ofstream configFile("SystemMonitor.cfg");
    if (!configFile.is_open()) {
        return false;
    }

    configFile << "CPU_THRESHOLD=" << config.cpuThreshold << std::endl;
    configFile << "RAM_THRESHOLD=" << config.ramThreshold << std::endl;
    configFile << "DISK_THRESHOLD=" << config.diskThreshold << std::endl;
    configFile << "MONITOR_INTERVAL=" << config.monitorInterval << std::endl;
    configFile << "LOG_PATH=" << config.logFilePath << std::endl;
    configFile << "DEBUG_MODE=" << (config.debugMode ? "true" : "false") << std::endl;

    return configFile.good();
}

// Parse command line parameters for thresholds
void ParseCommandLineArgs(int argc, char* argv[], MonitorConfig& config) {
    for (int i = 1; i < argc; i++) {
        // Check if this is a valid parameter
        if (argv[i][0] == '-') {
            if (!IsValidParameter(argv[i])) {
                std::cerr << "Warning: Unknown parameter '" << argv[i] << "'" << std::endl;
                continue;
            }
        }
        
        // Parse known parameters with their values
        if (i + 1 < argc) {
            if (strcmp(argv[i], "--cpu") == 0) {
                config.cpuThreshold = ValidateThreshold(argv[i + 1], "CPU");
                i++;  // Skip the next argument
            } else if (strcmp(argv[i], "--ram") == 0) {
                config.ramThreshold = ValidateThreshold(argv[i + 1], "RAM");
                i++;
            } else if (strcmp(argv[i], "--disk") == 0 || strcmp(argv[i], "-disk") == 0) {
                config.diskThreshold = ValidateThreshold(argv[i + 1], "Disk");
                i++;
            } else if (strcmp(argv[i], "--interval") == 0) {
                try {
                    int interval = std::stoi(argv[i + 1]);
                    if (interval < 1000) {
                        std::cerr << "Warning: Interval too small, setting to minimum 1000ms" << std::endl;
                        config.monitorInterval = 1000;
                    } else {
                        config.monitorInterval = interval;
                    }
                } catch (...) {
                    std::cerr << "Error parsing interval value, using default" << std::endl;
                }
                i++;
            }
        } else if (strcmp(argv[i], "--debug") == 0) {
            config.debugMode = true;
        }
    }
}
