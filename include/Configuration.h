#pragma once

#include <string>
#include <memory>
#include "Logger.h"

// Display mode enumeration
enum class DisplayModeConfig {
    LINE_BY_LINE = 0,    // Traditional line output
    TOP_STYLE = 1,       // Interactive table display like Linux top
    COMPACT = 2          // Compact table view
};

// Base configuration class
class BaseConfig {
protected:
    double cpuThreshold = 80.0;
    double ramThreshold = 80.0;
    double diskThreshold = 80.0;
    int monitorInterval = 5000;
    bool debugMode = false;
    DisplayModeConfig displayMode = DisplayModeConfig::TOP_STYLE; // Default to top-style

public:
    BaseConfig() = default;
    virtual ~BaseConfig() = default;

    // Getters
    double getCpuThreshold() const { return cpuThreshold; }
    double getRamThreshold() const { return ramThreshold; }
    double getDiskThreshold() const { return diskThreshold; }
    int getMonitorInterval() const { return monitorInterval; }
    bool isDebugMode() const { return debugMode; }
    DisplayModeConfig getDisplayMode() const { return displayMode; }

    // Setters
    void setCpuThreshold(double value) { cpuThreshold = value; }
    void setRamThreshold(double value) { ramThreshold = value; }
    void setDiskThreshold(double value) { diskThreshold = value; }
    void setMonitorInterval(int value) { monitorInterval = value; }
    void setDebugMode(bool value) { debugMode = value; }
    void setDisplayMode(DisplayModeConfig mode) { displayMode = mode; }

    // Virtual methods for extensibility
    virtual bool validate() const;
    virtual void setDefaults();
};

// Monitor configuration class
class MonitorConfig : public BaseConfig {
private:
    std::string logFilePath = "SystemMonitor.log";
    LogConfig logConfig;

public:
    MonitorConfig();
    explicit MonitorConfig(const LogConfig& logCfg);

    // Getters
    const std::string& getLogFilePath() const { return logFilePath; }
    const LogConfig& getLogConfig() const { return logConfig; }
    LogConfig& getLogConfig() { return logConfig; }

    // Setters
    void setLogFilePath(const std::string& path) { 
        logFilePath = path;
        logConfig.setLogPath(path);
    }
    void setLogConfig(const LogConfig& config) { logConfig = config; }

    // Override virtual methods
    bool validate() const override;
    void setDefaults() override;
};

// Abstract configuration manager interface
class IConfigurationManager {
public:
    virtual ~IConfigurationManager() = default;
    virtual bool loadFromFile(const std::string& filename) = 0;
    virtual bool saveToFile(const std::string& filename) const = 0;
    virtual bool parseCommandLine(int argc, char* argv[]) = 0;
    virtual MonitorConfig& getConfig() = 0;
    virtual const MonitorConfig& getConfig() const = 0;
    virtual void printUsage() const = 0;
};

// Concrete configuration manager
class ConfigurationManager : public IConfigurationManager {
private:
    MonitorConfig config;

    double validateThreshold(const std::string& value, const std::string& paramName) const;
    bool isValidParameter(const std::string& param) const;

public:
    ConfigurationManager();
    explicit ConfigurationManager(const MonitorConfig& initialConfig);

    // IConfigurationManager interface implementation
    bool loadFromFile(const std::string& filename) override;
    bool saveToFile(const std::string& filename) const override;
    bool parseCommandLine(int argc, char* argv[]) override;
    MonitorConfig& getConfig() override { return config; }
    const MonitorConfig& getConfig() const override { return config; }
    void printUsage() const override;

    // Additional utility methods
    void resetToDefaults();
    bool validateConfiguration() const;
};
