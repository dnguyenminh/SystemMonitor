#include "../include/Logger.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <windows.h>
#include <cerrno>
#include <filesystem>
#include <sstream>

// LoggerManager singleton implementation
LoggerManager* LoggerManager::instance = nullptr;

LoggerManager& LoggerManager::getInstance() {
    if (!instance) {
        instance = new LoggerManager();
    }
    return *instance;
}

void LoggerManager::setLogger(std::unique_ptr<ILogger> newLogger) {
    logger = std::move(newLogger);
}

void LoggerManager::debug(const std::string& message) {
    if (logger) {
        logger->debug(message);
    }
}

void LoggerManager::logProcesses(const std::vector<ProcessInfo>& processes, const SystemUsage& systemUsage) {
    if (logger) {
        logger->logProcesses(processes, systemUsage);
    }
}

bool LoggerManager::rotateIfNeeded() {
    if (logger) {
        return logger->rotateIfNeeded();
    }
    return true;
}

void LoggerManager::shutdown() {
    if (logger) {
        logger->shutdown();
    }
}

size_t LoggerManager::getQueueSize() const {
    if (logger) {
        return logger->getQueueSize();
    }
    return 0;
}

// AsyncFileLogger implementation
AsyncFileLogger::AsyncFileLogger(const LogConfig& logConfig) : config(logConfig) {}

AsyncFileLogger::~AsyncFileLogger() {
    shutdown();
}

bool AsyncFileLogger::initialize() {
    try {
        std::cout << "Initializing async file logger..." << std::endl;
        std::cout << "Log file: " << config.getLogPath() << std::endl;
        std::cout << "Max file size: " << config.getMaxFileSizeMB() << "MB" << std::endl;
        std::cout << "Max backup files: " << config.getMaxBackupFiles() << std::endl;
        std::cout << "Rotation enabled: " << (config.isRotationEnabled() ? "Yes" : "No") << std::endl;
        std::cout << "Queue max size: " << config.getQueueMaxSize() << " messages" << std::endl;
        
        // Create directory if it doesn't exist
        std::filesystem::path logPath(config.getLogPath());
        if (logPath.has_parent_path()) {
            std::filesystem::create_directories(logPath.parent_path());
        }
        
        // Create an empty log file if it doesn't exist
        std::ofstream createLog(config.getLogPath(), std::ios::app);
        if (!createLog.is_open()) {
            std::cerr << "Error: Could not initialize log file." << std::endl;
            return false;
        }
        createLog.close();
        
        // Start worker thread
        running = true;
        workerThread = std::thread(&AsyncFileLogger::workerThreadFunction, this);
        
        std::cout << "Async file logger initialized successfully with worker thread." << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during async logger initialization: " << e.what() << std::endl;
        return false;
    }
}

void AsyncFileLogger::shutdown() {
    if (running) {
        std::cout << "Shutting down async logger..." << std::endl;
        
        // Send shutdown message
        messageQueue.push(LogMessage());
        
        // Wait for worker thread to finish
        if (workerThread.joinable()) {
            workerThread.join();
        }
        
        running = false;
        
        if (debugLogStream.is_open()) {
            debugLogStream.close();
        }
        
        std::cout << "Async logger shutdown completed." << std::endl;
    }
}

void AsyncFileLogger::debug(const std::string& message) {
    if (running && messageQueue.size() < config.getQueueMaxSize()) {
        messageQueue.push(LogMessage(LogMessageType::DEBUG, message));
    } else if (running) {
        // Queue is full, log to console as fallback
        std::cout << "[DEBUG] (Queue full) " << message << std::endl;
    }
}

void AsyncFileLogger::logProcesses(const std::vector<ProcessInfo>& processes, const SystemUsage& systemUsage) {
    if (running && messageQueue.size() < config.getQueueMaxSize()) {
        messageQueue.push(LogMessage(processes, systemUsage));
    } else if (running) {
        // Queue is full, log to console as fallback
        std::cout << "[LOG] (Queue full) Process logging skipped. Queue size: " << messageQueue.size() << std::endl;
    }
}

bool AsyncFileLogger::rotateIfNeeded() {
    // For async logger, rotation is handled by worker thread
    return true;
}

void AsyncFileLogger::workerThreadFunction() {
    std::cout << "Logger worker thread started." << std::endl;
    
    LogMessage message;
    while (running) {
        if (messageQueue.pop(message)) {
            if (message.type == LogMessageType::SHUTDOWN) {
                std::cout << "Logger worker thread received shutdown signal." << std::endl;
                break;
            }
            
            try {
                processLogMessage(message);
            } catch (const std::exception& e) {
                std::cerr << "Exception in logger worker thread: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in logger worker thread." << std::endl;
            }
        }
    }
    
    // Process any remaining messages
    while (messageQueue.pop(message)) {
        if (message.type == LogMessageType::SHUTDOWN) break;
        try {
            processLogMessage(message);
        } catch (...) {
            // Ignore exceptions during shutdown
        }
    }
    
    std::cout << "Logger worker thread finished." << std::endl;
}

void AsyncFileLogger::processLogMessage(const LogMessage& message) {
    switch (message.type) {
        case LogMessageType::DEBUG:
            writeDebugMessage(message.content);
            break;
            
        case LogMessageType::PROCESS_INFO:
            // Check if rotation is needed before writing
            if (checkRotationNeeded()) {
                if (!performRotation()) {
                    std::cerr << "Warning: Log rotation failed, continuing with current log file." << std::endl;
                }
            }
            writeProcessMessage(message.processes, message.systemUsage);
            break;
            
        default:
            break;
    }
}

void AsyncFileLogger::writeDebugMessage(const std::string& content) {
    if (!debugLogStream.is_open()) {
        debugLogStream.open("SystemMonitor_debug.log", std::ios::app);
    }
    
    std::string timeStr = getCurrentTimeString();
    
    if (debugLogStream.is_open()) {
        debugLogStream << timeStr << " - " << content << std::endl;
        debugLogStream.flush();
    }
    
    std::cout << "[DEBUG] " << content << std::endl;
}

void AsyncFileLogger::writeProcessMessage(const std::vector<ProcessInfo>& processes, const SystemUsage& systemUsage) {
    std::ofstream log(config.getLogPath(), std::ios::app);
    if (!log.is_open()) {
        std::cerr << "Error: Could not open log file for writing: " << config.getLogPath() << std::endl;
        return;
    }
    
    std::string formattedTime = getCurrentTimeString();
    
    // Write start banner
    log << "===Start " << formattedTime 
        << " [System CPU " << std::fixed << std::setprecision(2) << systemUsage.getCpuPercent()
        << "%] [System RAM " << std::fixed << std::setprecision(2) << systemUsage.getRamPercent()
        << "%] [System Disk " << std::fixed << std::setprecision(2) << systemUsage.getDiskPercent() 
        << "%]===\n";
    
    int processCount = 0;
    for (const auto& process : processes) {
        if (process.hasSignificantUsage()) {
            log << formattedTime << ", " 
                << process.getName() << ", " 
                << process.getPid() 
                << ", [CPU " << std::fixed << std::setprecision(2) << process.getCpuPercent()
                << "%] [RAM " << std::fixed << std::setprecision(2) << process.getRamPercent()
                << "%] [Disk " << std::fixed << std::setprecision(2) << process.getDiskPercent() 
                << "%]\n";
            processCount++;
        }
    }
    
    // Write end banner
    log << "===End  " << formattedTime 
        << " [System CPU " << std::fixed << std::setprecision(2) << systemUsage.getCpuPercent()
        << "%] [System RAM " << std::fixed << std::setprecision(2) << systemUsage.getRamPercent()
        << "%] [System Disk " << std::fixed << std::setprecision(2) << systemUsage.getDiskPercent() 
        << "%]===\n\n";
    
    log.flush();
    
    if (processCount > 0) {
        std::cout << "Logged " << processCount << " processes to " << config.getLogPath() 
                  << " (Queue size: " << messageQueue.size() << ")" << std::endl;
    }
}

std::string AsyncFileLogger::getCurrentTimeString() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &now_c);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", &tm);
    return buf;
}

std::string AsyncFileLogger::getCurrentDateString(const std::string& format) const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &now_c);
    char buf[64];
    std::strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}

std::string AsyncFileLogger::getCurrentHourString() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &now_c);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y%m%d%H", &tm);
    return buf;
}

std::string AsyncFileLogger::generateRotatedFilename(const std::string& basePath, const std::string& dateSuffix, int index) const {
    std::filesystem::path path(basePath);
    std::string stem = path.stem().string();
    std::string extension = path.extension().string();
    
    if (config.shouldKeepDateInFilename()) {
        if (index == 0) {
            return stem + "_" + dateSuffix + extension;
        } else {
            return stem + "_" + dateSuffix + "." + std::to_string(index) + extension;
        }
    } else {
        // Traditional numeric suffix
        if (index == 0) {
            return basePath;
        } else {
            return basePath + "." + std::to_string(index);
        }
    }
}

bool AsyncFileLogger::checkRotationNeeded() {
    if (!config.isRotationEnabled()) {
        return false;
    }
    
    bool sizeRotationNeeded = false;
    bool dateRotationNeeded = false;
    
    // Check size-based rotation
    if (config.isSizeBasedRotation()) {
        sizeRotationNeeded = checkSizeRotationNeeded();
    }
    
    // Check date-based rotation
    if (config.isDateBasedRotation()) {
        dateRotationNeeded = checkDateRotationNeeded();
    }
    
    return sizeRotationNeeded || dateRotationNeeded;
}

bool AsyncFileLogger::checkSizeRotationNeeded() {
    try {
        if (!std::filesystem::exists(config.getLogPath())) {
            return false;
        }
        
        auto fileSize = std::filesystem::file_size(config.getLogPath());
        size_t maxSizeBytes = config.getMaxFileSizeMB() * 1024 * 1024;
        
        return fileSize >= maxSizeBytes;
        
    } catch (const std::exception& e) {
        std::cerr << "Error during size rotation check: " << e.what() << std::endl;
        return false;
    }
}

bool AsyncFileLogger::checkDateRotationNeeded() {
    try {
        std::string currentDate;
        std::string currentHour;
        
        switch (config.getDateFrequency()) {
            case DateRotationFrequency::DAILY:
                currentDate = getCurrentDateString("%Y%m%d");
                if (lastRotationDate.empty()) {
                    lastRotationDate = currentDate;
                    return false;
                }
                if (currentDate != lastRotationDate) {
                    lastRotationDate = currentDate;
                    return true;
                }
                break;
                
            case DateRotationFrequency::HOURLY:
                currentHour = getCurrentHourString();
                if (lastRotationHour.empty()) {
                    lastRotationHour = currentHour;
                    return false;
                }
                if (currentHour != lastRotationHour) {
                    lastRotationHour = currentHour;
                    return true;
                }
                break;
                
            case DateRotationFrequency::WEEKLY:
                // Check if it's Sunday (0) and we haven't rotated this week
                auto now = std::chrono::system_clock::now();
                std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                std::tm tm;
                localtime_s(&tm, &now_c);
                
                if (tm.tm_wday == 0) { // Sunday
                    std::string weekDate = getCurrentDateString("%Y%U"); // Year + Week number
                    if (lastRotationDate.empty()) {
                        lastRotationDate = weekDate;
                        return false;
                    }
                    if (weekDate != lastRotationDate) {
                        lastRotationDate = weekDate;
                        return true;
                    }
                }
                break;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "Error during date rotation check: " << e.what() << std::endl;
        return false;
    }
}

bool AsyncFileLogger::performRotation() {
    std::cout << "Performing log rotation..." << std::endl;
    
    switch (config.getRotationStrategy()) {
        case LogRotationStrategy::SIZE_BASED:
            return performSizeBasedRotation();
            
        case LogRotationStrategy::DATE_BASED:
            return performDateBasedRotation();
            
        case LogRotationStrategy::COMBINED:
            // For combined strategy, use date-based naming but also check size
            return performDateBasedRotation();
            
        default:
            return performSizeBasedRotation();
    }
}

bool AsyncFileLogger::performSizeBasedRotation() {
    try {
        // Remove the oldest backup
        std::string oldestBackup = config.getLogPath() + "." + std::to_string(config.getMaxBackupFiles());
        if (std::filesystem::exists(oldestBackup)) {
            std::filesystem::remove(oldestBackup);
        }
        
        // Shift existing backup files
        for (int i = config.getMaxBackupFiles() - 1; i >= 1; i--) {
            std::string currentBackup = config.getLogPath() + "." + std::to_string(i);
            std::string nextBackup = config.getLogPath() + "." + std::to_string(i + 1);
            
            if (std::filesystem::exists(currentBackup)) {
                std::filesystem::rename(currentBackup, nextBackup);
            }
        }
        
        // Move current log to .1
        std::string firstBackup = config.getLogPath() + ".1";
        std::filesystem::rename(config.getLogPath(), firstBackup);
        
        // Create new empty log file
        std::ofstream newLog(config.getLogPath());
        if (newLog.is_open()) {
            newLog.close();
            std::cout << "Size-based log rotation completed successfully." << std::endl;
            return true;
        } else {
            std::cerr << "Error: Could not create new log file after rotation." << std::endl;
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error during size-based log rotation: " << e.what() << std::endl;
        return false;
    }
}

bool AsyncFileLogger::performDateBasedRotation() {
    try {
        std::string dateSuffix;
        
        switch (config.getDateFrequency()) {
            case DateRotationFrequency::DAILY:
                dateSuffix = getCurrentDateString(config.getDateFormat());
                break;
            case DateRotationFrequency::HOURLY:
                dateSuffix = getCurrentDateString("%Y%m%d_%H");
                break;
            case DateRotationFrequency::WEEKLY:
                dateSuffix = getCurrentDateString("%Y_W%U");
                break;
        }
        
        // Generate rotated filename with date
        std::string rotatedFilename = generateRotatedFilename(config.getLogPath(), dateSuffix, 0);
        
        // If the target filename already exists, add a numeric suffix
        int counter = 1;
        std::string finalFilename = rotatedFilename;
        while (std::filesystem::exists(finalFilename)) {
            finalFilename = generateRotatedFilename(config.getLogPath(), dateSuffix, counter);
            counter++;
        }
        
        // Move current log to dated filename
        if (std::filesystem::exists(config.getLogPath())) {
            std::filesystem::rename(config.getLogPath(), finalFilename);
        }
        
        // Create new empty log file
        std::ofstream newLog(config.getLogPath());
        if (newLog.is_open()) {
            newLog.close();
            std::cout << "Date-based log rotation completed successfully. Rotated to: " 
                      << finalFilename << std::endl;
            
            // Clean up old log files
            cleanupOldLogFiles();
            return true;
        } else {
            std::cerr << "Error: Could not create new log file after date rotation." << std::endl;
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error during date-based log rotation: " << e.what() << std::endl;
        return false;
    }
}

void AsyncFileLogger::cleanupOldLogFiles() const {
    try {
        std::filesystem::path logDir = std::filesystem::path(config.getLogPath()).parent_path();
        if (logDir.empty()) logDir = ".";
        
        std::filesystem::path logFile = std::filesystem::path(config.getLogPath()).filename();
        std::string baseName = logFile.stem().string();
        std::string extension = logFile.extension().string();
        
        std::vector<std::filesystem::path> logFiles;
        
        // Find all log files with the same base name
        for (const auto& entry : std::filesystem::directory_iterator(logDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find(baseName) == 0 && filename != logFile) {
                    logFiles.push_back(entry.path());
                }
            }
        }
        
        // Sort by modification time (newest first)
        std::sort(logFiles.begin(), logFiles.end(), 
                  [](const std::filesystem::path& a, const std::filesystem::path& b) {
                      return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                  });
        
        // Remove excess files
        if (logFiles.size() > static_cast<size_t>(config.getMaxBackupFiles())) {
            for (size_t i = config.getMaxBackupFiles(); i < logFiles.size(); i++) {
                std::filesystem::remove(logFiles[i]);
                std::cout << "Removed old log file: " << logFiles[i].filename() << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error during log file cleanup: " << e.what() << std::endl;
    }
}

// LoggerFactory implementation
std::unique_ptr<ILogger> LoggerFactory::createAsyncFileLogger(const LogConfig& config) {
    return std::make_unique<AsyncFileLogger>(config);
}

std::unique_ptr<ILogger> LoggerFactory::createFileLogger(const LogConfig& config) {
    // Default to async implementation now
    return std::make_unique<AsyncFileLogger>(config);
}

std::unique_ptr<ILogger> LoggerFactory::createConsoleLogger() {
    // TODO: Implement console logger
    return nullptr;
}

std::unique_ptr<ILogger> LoggerFactory::createCompositeLogger(const std::vector<std::unique_ptr<ILogger>>& loggers) {
    // TODO: Implement composite logger
    return nullptr;
}
