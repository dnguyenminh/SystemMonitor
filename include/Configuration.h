#pragma once

#include <string>

// Configuration settings
struct MonitorConfig {
    double cpuThreshold;
    double ramThreshold;
    double diskThreshold;
    int monitorInterval;  // in milliseconds
    std::string logFilePath;
    bool debugMode;
};

// Validate threshold is within range 0-100
double ValidateThreshold(const char* value, const char* paramName);

// Check if a string is a valid parameter name
bool IsValidParameter(const char* param);

// Parse command line parameters for thresholds
void ParseCommandLineArgs(int argc, char* argv[], MonitorConfig& config);

// Load configuration from a file
bool LoadConfigFile(MonitorConfig& config);

// Save configuration to a file
bool SaveConfigFile(const MonitorConfig& config);

// Print help information
void PrintUsage();
