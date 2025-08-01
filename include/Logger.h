#pragma once

#include <string>
#include <vector>
#include "SystemMetrics.h"

// Get current time as formatted string
std::string GetCurrentTimeString();

// Debug log function
void DebugLog(const std::string& message);

// Initialize log file
bool InitializeLogFile(const std::string& logPath);

// Log processes exceeding thresholds
void LogProcesses(const std::vector<ProcessInfo>& processes, double systemCpu, double systemRam, double systemDisk);
