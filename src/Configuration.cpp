#include "../include/Configuration.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <algorithm>
#include <vector>
#include <algorithm> // For std::find

// BaseConfig implementation
bool BaseConfig::validate() const {
    return cpuThreshold >= 0 && cpuThreshold <= 100 &&
           ramThreshold >= 0 && ramThreshold <= 100 &&
           diskThreshold >= 0 && diskThreshold <= 100 &&
           monitorInterval >= 1000;
}

void BaseConfig::setDefaults() {
    cpuThreshold = 80.0;
    ramThreshold = 80.0;
    diskThreshold = 80.0;
    monitorInterval = 5000;
    debugMode = false;
    displayMode = DisplayModeConfig::TOP_STYLE;
}

// MonitorConfig implementation
MonitorConfig::MonitorConfig() {
    setDefaults();
}

MonitorConfig::MonitorConfig(const LogConfig& logCfg) : logConfig(logCfg) {
    setDefaults();
    logFilePath = logConfig.getLogPath();
}

MonitorConfig::MonitorConfig(const LogConfig& logCfg, const EmailConfig& emailCfg) 
    : logConfig(logCfg), emailConfig(emailCfg) {
    setDefaults();
    logFilePath = logConfig.getLogPath();
}

bool MonitorConfig::validate() const {
    return BaseConfig::validate() && !logFilePath.empty();
}

void MonitorConfig::setDefaults() {
    BaseConfig::setDefaults();
    logFilePath = "SystemMonitor.log";
    logConfig = LogConfig();
}

// ConfigurationManager implementation
ConfigurationManager::ConfigurationManager() {
    config.setDefaults();
}

ConfigurationManager::ConfigurationManager(const MonitorConfig& initialConfig) : config(initialConfig) {}

double ConfigurationManager::validateThreshold(const std::string& value, const std::string& paramName) const {
    try {
        double val = std::stod(value);
        if (val < 0.0 || val > 100.0) {
            std::cerr << "Error: " << paramName << " threshold must be between 0 and 100. Using default value." << std::endl;
            return 80.0;
        }
        return val;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing " << paramName << " value '" << value << "': " << e.what() << std::endl;
        return 80.0;
    }
}

bool ConfigurationManager::isValidParameter(const std::string& param) const {
    const std::vector<std::string> validParams = {
        "--cpu", "--ram", "--disk", "-disk",
        "--help", "-h", "--interval", "--debug",
        "--log-size", "--log-backups", "--log-rotation",
        "--log-strategy", "--log-frequency", "--log-date-format",
        "--display", "--mode"
    };
    
    return std::find(validParams.begin(), validParams.end(), param) != validParams.end();
}

bool ConfigurationManager::loadFromFile(const std::string& filename) {
    std::ifstream configFile(filename);
    if (!configFile.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        size_t pos = line.find("=");
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Parse configuration values
        if (key == "CPU_THRESHOLD") {
            config.setCpuThreshold(validateThreshold(value, "CPU"));
        } else if (key == "RAM_THRESHOLD") {
            config.setRamThreshold(validateThreshold(value, "RAM"));
        } else if (key == "DISK_THRESHOLD") {
            config.setDiskThreshold(validateThreshold(value, "Disk"));
        } else if (key == "MONITOR_INTERVAL") {
            try {
                int interval = std::stoi(value);
                if (interval >= 1000) {
                    config.setMonitorInterval(interval);
                }
            } catch (...) {
                // Ignore parsing errors
            }
        } else if (key == "LOG_PATH") {
            config.setLogFilePath(value);
        } else if (key == "DEBUG_MODE") {
            config.setDebugMode(value == "true" || value == "1");
        } else if (key == "LOG_MAX_SIZE_MB") {
            try {
                int size = std::stoi(value);
                if (size > 0) {
                    config.getLogConfig().setMaxFileSizeMB(size);
                }
            } catch (...) {
                // Ignore parsing errors
            }
        } else if (key == "LOG_MAX_BACKUPS") {
            try {
                int backups = std::stoi(value);
                if (backups >= 0) {
                    config.getLogConfig().setMaxBackupFiles(backups);
                }
            } catch (...) {
                // Ignore parsing errors
            }
        } else if (key == "LOG_ROTATION_ENABLED") {
            config.getLogConfig().setRotationEnabled(value == "true" || value == "1");
        } else if (key == "LOG_ROTATION_STRATEGY") {
            if (value == "SIZE_BASED") {
                config.getLogConfig().setRotationStrategy(LogRotationStrategy::SIZE_BASED);
            } else if (value == "DATE_BASED") {
                config.getLogConfig().setRotationStrategy(LogRotationStrategy::DATE_BASED);
            } else if (value == "COMBINED") {
                config.getLogConfig().setRotationStrategy(LogRotationStrategy::COMBINED);
            }
        } else if (key == "LOG_DATE_FREQUENCY") {
            if (value == "DAILY") {
                config.getLogConfig().setDateFrequency(DateRotationFrequency::DAILY);
            } else if (value == "HOURLY") {
                config.getLogConfig().setDateFrequency(DateRotationFrequency::HOURLY);
            } else if (value == "WEEKLY") {
                config.getLogConfig().setDateFrequency(DateRotationFrequency::WEEKLY);
            }
        } else if (key == "LOG_DATE_FORMAT") {
            config.getLogConfig().setDateFormat(value);
        } else if (key == "LOG_KEEP_DATE_IN_FILENAME") {
            config.getLogConfig().setKeepDateInFilename(value == "true" || value == "1");
        } else if (key == "DISPLAY_MODE") {
            if (value == "LINE_BY_LINE" || value == "0") {
                config.setDisplayMode(DisplayModeConfig::LINE_BY_LINE);
            } else if (value == "TOP_STYLE" || value == "1") {
                config.setDisplayMode(DisplayModeConfig::TOP_STYLE);
            } else if (value == "COMPACT" || value == "2") {
                config.setDisplayMode(DisplayModeConfig::COMPACT);
            } else if (value == "SILENCE" || value == "3") {
                config.setDisplayMode(DisplayModeConfig::SILENCE);
            }
        }
        // Email configuration
        else if (key == "EMAIL_ENABLED") {
            config.getEmailConfig().enableEmailAlerts = (value == "true" || value == "1");
        } else if (key == "EMAIL_SMTP_SERVER") {
            config.getEmailConfig().smtpServer = value;
        } else if (key == "EMAIL_SMTP_PORT") {
            try {
                int port = std::stoi(value);
                if (port > 0 && port <= 65535) {
                    config.getEmailConfig().smtpPort = port;
                }
            } catch (...) {
                // Ignore parsing errors
            }
        } else if (key == "EMAIL_SENDER") {
            config.getEmailConfig().senderEmail = value;
        } else if (key == "EMAIL_PASSWORD") {
            config.getEmailConfig().senderPassword = value;
        } else if (key == "EMAIL_SENDER_NAME") {
            config.getEmailConfig().senderName = value;
        } else if (key == "EMAIL_RECIPIENTS") {
            // Parse comma-separated email list
            config.getEmailConfig().recipients.clear();
            std::string::size_type start = 0;
            std::string::size_type end = value.find(',');
            while (end != std::string::npos) {
                std::string email = value.substr(start, end - start);
                // Trim whitespace
                email.erase(0, email.find_first_not_of(" \t"));
                email.erase(email.find_last_not_of(" \t") + 1);
                if (!email.empty()) {
                    config.getEmailConfig().recipients.push_back(email);
                }
                start = end + 1;
                end = value.find(',', start);
            }
            // Add last email
            std::string email = value.substr(start);
            email.erase(0, email.find_first_not_of(" \t"));
            email.erase(email.find_last_not_of(" \t") + 1);
            if (!email.empty()) {
                config.getEmailConfig().recipients.push_back(email);
            }
        } else if (key == "EMAIL_USE_TLS") {
            config.getEmailConfig().useTLS = (value == "true" || value == "1");
        } else if (key == "EMAIL_USE_SSL") {
            config.getEmailConfig().useSSL = (value == "true" || value == "1");
        } else if (key == "EMAIL_TIMEOUT_SECONDS") {
            try {
                int timeout = std::stoi(value);
                if (timeout > 0) {
                    config.getEmailConfig().timeoutSeconds = timeout;
                }
            } catch (...) {
                // Ignore parsing errors
            }
        } else if (key == "EMAIL_ALERT_DURATION_SECONDS") {
            try {
                int duration = std::stoi(value);
                if (duration > 0) {
                    config.getEmailConfig().alertDurationSeconds = duration;
                }
            } catch (...) {
                // Ignore parsing errors
            }
        } else if (key == "EMAIL_COOLDOWN_MINUTES") {
            try {
                int cooldown = std::stoi(value);
                if (cooldown > 0) {
                    config.getEmailConfig().cooldownMinutes = cooldown;
                }
            } catch (...) {
                // Ignore parsing errors
            }
        } else if (key == "EMAIL_SEND_RECOVERY_ALERTS") {
            config.getEmailConfig().sendRecoveryAlerts = (value == "true" || value == "1");
        } else if (key == "EMAIL_RECOVERY_DURATION_SECONDS") {
            try {
                int duration = std::stoi(value);
                if (duration > 0) {
                    config.getEmailConfig().recoveryDurationSeconds = duration;
                }
            } catch (...) {
                // Ignore parsing errors
            }
        }
    }

    return true;
}

bool ConfigurationManager::saveToFile(const std::string& filename) const {
    std::ofstream configFile(filename);
    if (!configFile.is_open()) {
        return false;
    }

    configFile << "CPU_THRESHOLD=" << config.getCpuThreshold() << std::endl;
    configFile << "RAM_THRESHOLD=" << config.getRamThreshold() << std::endl;
    configFile << "DISK_THRESHOLD=" << config.getDiskThreshold() << std::endl;
    configFile << "MONITOR_INTERVAL=" << config.getMonitorInterval() << std::endl;
    configFile << "LOG_PATH=" << config.getLogFilePath() << std::endl;
    configFile << "DEBUG_MODE=" << (config.isDebugMode() ? "true" : "false") << std::endl;
    configFile << "LOG_MAX_SIZE_MB=" << config.getLogConfig().getMaxFileSizeMB() << std::endl;
    configFile << "LOG_MAX_BACKUPS=" << config.getLogConfig().getMaxBackupFiles() << std::endl;
    configFile << "LOG_ROTATION_ENABLED=" << (config.getLogConfig().isRotationEnabled() ? "true" : "false") << std::endl;
    
    // Date-based rotation settings
    configFile << "LOG_ROTATION_STRATEGY=";
    switch (config.getLogConfig().getRotationStrategy()) {
        case LogRotationStrategy::SIZE_BASED:
            configFile << "SIZE_BASED";
            break;
        case LogRotationStrategy::DATE_BASED:
            configFile << "DATE_BASED";
            break;
        case LogRotationStrategy::COMBINED:
            configFile << "COMBINED";
            break;
    }
    configFile << std::endl;
    
    configFile << "LOG_DATE_FREQUENCY=";
    switch (config.getLogConfig().getDateFrequency()) {
        case DateRotationFrequency::DAILY:
            configFile << "DAILY";
            break;
        case DateRotationFrequency::HOURLY:
            configFile << "HOURLY";
            break;
        case DateRotationFrequency::WEEKLY:
            configFile << "WEEKLY";
            break;
    }
    configFile << std::endl;
    
    configFile << "LOG_DATE_FORMAT=" << config.getLogConfig().getDateFormat() << std::endl;
    configFile << "LOG_KEEP_DATE_IN_FILENAME=" << (config.getLogConfig().shouldKeepDateInFilename() ? "true" : "false") << std::endl;
    
    // Display mode setting
    configFile << "DISPLAY_MODE=";
    switch (config.getDisplayMode()) {
        case DisplayModeConfig::LINE_BY_LINE:
            configFile << "LINE_BY_LINE";
            break;
        case DisplayModeConfig::TOP_STYLE:
            configFile << "TOP_STYLE";
            break;
        case DisplayModeConfig::COMPACT:
            configFile << "COMPACT";
            break;
        case DisplayModeConfig::SILENCE:
            configFile << "SILENCE";
            break;
    }
    configFile << std::endl;

    // Email configuration
    configFile << "EMAIL_ENABLED=" << (config.getEmailConfig().enableEmailAlerts ? "true" : "false") << std::endl;
    configFile << "EMAIL_SMTP_SERVER=" << config.getEmailConfig().smtpServer << std::endl;
    configFile << "EMAIL_SMTP_PORT=" << config.getEmailConfig().smtpPort << std::endl;
    configFile << "EMAIL_SENDER=" << config.getEmailConfig().senderEmail << std::endl;
    configFile << "EMAIL_PASSWORD=" << config.getEmailConfig().senderPassword << std::endl;
    configFile << "EMAIL_SENDER_NAME=" << config.getEmailConfig().senderName << std::endl;
    
    // Recipients as comma-separated list
    configFile << "EMAIL_RECIPIENTS=";
    for (size_t i = 0; i < config.getEmailConfig().recipients.size(); ++i) {
        if (i > 0) configFile << ",";
        configFile << config.getEmailConfig().recipients[i];
    }
    configFile << std::endl;
    
    configFile << "EMAIL_USE_TLS=" << (config.getEmailConfig().useTLS ? "true" : "false") << std::endl;
    configFile << "EMAIL_USE_SSL=" << (config.getEmailConfig().useSSL ? "true" : "false") << std::endl;
    configFile << "EMAIL_TIMEOUT_SECONDS=" << config.getEmailConfig().timeoutSeconds << std::endl;
    configFile << "EMAIL_ALERT_DURATION_SECONDS=" << config.getEmailConfig().alertDurationSeconds << std::endl;
    configFile << "EMAIL_COOLDOWN_MINUTES=" << config.getEmailConfig().cooldownMinutes << std::endl;
    configFile << "EMAIL_SEND_RECOVERY_ALERTS=" << (config.getEmailConfig().sendRecoveryAlerts ? "true" : "false") << std::endl;
    configFile << "EMAIL_RECOVERY_DURATION_SECONDS=" << config.getEmailConfig().recoveryDurationSeconds << std::endl;

    return configFile.good();
}

bool ConfigurationManager::parseCommandLine(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg[0] == '-') {
            if (!isValidParameter(arg)) {
                std::cerr << "Warning: Unknown parameter '" << arg << "'" << std::endl;
                continue;
            }
        }
        
        if (i + 1 < argc) {
            std::string value = argv[i + 1];
            
            if (arg == "--cpu") {
                config.setCpuThreshold(validateThreshold(value, "CPU"));
                i++;
            } else if (arg == "--ram") {
                config.setRamThreshold(validateThreshold(value, "RAM"));
                i++;
            } else if (arg == "--disk" || arg == "-disk") {
                config.setDiskThreshold(validateThreshold(value, "Disk"));
                i++;
            } else if (arg == "--interval") {
                try {
                    int interval = std::stoi(value);
                    if (interval >= 1000) {
                        config.setMonitorInterval(interval);
                    }
                } catch (...) {
                    std::cerr << "Invalid interval value: " << value << std::endl;
                }
                i++;
            } else if (arg == "--log-size") {
                try {
                    int size = std::stoi(value);
                    if (size > 0) {
                        config.getLogConfig().setMaxFileSizeMB(size);
                    }
                } catch (...) {
                    std::cerr << "Invalid log size value: " << value << std::endl;
                }
                i++;
            } else if (arg == "--log-backups") {
                try {
                    int backups = std::stoi(value);
                    if (backups >= 0) {
                        config.getLogConfig().setMaxBackupFiles(backups);
                    }
                } catch (...) {
                    std::cerr << "Invalid log backups value: " << value << std::endl;
                }
                i++;
            } else if (arg == "--log-strategy") {
                if (value == "SIZE_BASED") {
                    config.getLogConfig().setRotationStrategy(LogRotationStrategy::SIZE_BASED);
                } else if (value == "DATE_BASED") {
                    config.getLogConfig().setRotationStrategy(LogRotationStrategy::DATE_BASED);
                } else if (value == "COMBINED") {
                    config.getLogConfig().setRotationStrategy(LogRotationStrategy::COMBINED);
                } else {
                    std::cerr << "Invalid rotation strategy: " << value << std::endl;
                }
                i++;
            } else if (arg == "--log-frequency") {
                if (value == "DAILY") {
                    config.getLogConfig().setDateFrequency(DateRotationFrequency::DAILY);
                } else if (value == "HOURLY") {
                    config.getLogConfig().setDateFrequency(DateRotationFrequency::HOURLY);
                } else if (value == "WEEKLY") {
                    config.getLogConfig().setDateFrequency(DateRotationFrequency::WEEKLY);
                } else {
                    std::cerr << "Invalid date frequency: " << value << std::endl;
                }
                i++;
            } else if (arg == "--log-date-format") {
                config.getLogConfig().setDateFormat(value);
                i++;
            } else if (arg == "--display" || arg == "--mode") {
                if (value == "line" || value == "LINE_BY_LINE" || value == "0") {
                    config.setDisplayMode(DisplayModeConfig::LINE_BY_LINE);
                } else if (value == "top" || value == "TOP_STYLE" || value == "1") {
                    config.setDisplayMode(DisplayModeConfig::TOP_STYLE);
                } else if (value == "compact" || value == "COMPACT" || value == "2") {
                    config.setDisplayMode(DisplayModeConfig::COMPACT);
                } else if (value == "silence" || value == "SILENCE" || value == "3") {
                    config.setDisplayMode(DisplayModeConfig::SILENCE);
                } else {
                    std::cerr << "Invalid display mode: " << value << ". Use: line, top, compact, or silence" << std::endl;
                }
                i++;
            }
        } else if (arg == "--debug") {
            config.setDebugMode(true);
        } else if (arg == "--log-rotation") {
            config.getLogConfig().setRotationEnabled(true);
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return false; // Indicate that program should exit
        }
    }
    
    return true;
}

void ConfigurationManager::printUsage() const {
    std::cout << "SystemMonitor - A utility to monitor system resource usage\n\n"
              << "Usage: SystemMonitor [options]\n\n"
              << "Options:\n"
              << "  --cpu PERCENT        CPU threshold percentage (default: 80.0)\n"
              << "  --ram PERCENT        RAM threshold percentage (default: 80.0)\n"
              << "  --disk PERCENT       Disk threshold percentage (default: 80.0)\n"
              << "  --interval MS        Monitoring interval in milliseconds (default: 5000)\n"
              << "  --display MODE       Display mode: line, top, compact, silence (default: top)\n"
              << "  --mode MODE          Alias for --display\n"
              << "  --debug              Enable debug logging\n"
              << "  --log-size MB        Maximum log file size in MB (default: 10)\n"
              << "  --log-backups COUNT  Number of backup files to keep (default: 5)\n"
              << "  --log-rotation       Enable log rotation (default: enabled)\n"
              << "\n"
              << "Advanced Log Rotation Options:\n"
              << "  --log-strategy TYPE  Rotation strategy: SIZE_BASED, DATE_BASED, COMBINED (default: SIZE_BASED)\n"
              << "  --log-frequency FREQ Date rotation frequency: DAILY, HOURLY, WEEKLY (default: DAILY)\n"
              << "  --log-date-format FMT Date format for filenames (default: %Y%m%d)\n"
              << "  --help, -h           Display this help message\n"
              << "\n"
              << "Display Modes:\n"
              << "  line                 Traditional line-by-line output\n"
              << "  top                  Interactive table display like Linux top (default)\n"
              << "  compact              Compact table view\n"
              << "  silence              Silent mode - only shows output when thresholds are exceeded\n"
              << "\n"
              << "Examples:\n"
              << "  SystemMonitor --display top\n"
              << "  SystemMonitor --mode line --debug\n"
              << "  SystemMonitor --log-strategy DATE_BASED --log-frequency DAILY\n"
              << "  SystemMonitor --log-strategy COMBINED --log-frequency HOURLY\n";
}

void ConfigurationManager::resetToDefaults() {
    config.setDefaults();
}

bool ConfigurationManager::validateConfiguration() const {
    return config.validate();
}
