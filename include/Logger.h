#pragma once

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>
#include "SystemMetrics.h"

// Log message types for the queue
enum class LogMessageType {
    DEBUG,
    PROCESS_INFO,
    SHUTDOWN
};

// Log message structure for the queue
struct LogMessage {
    LogMessageType type;
    std::string content;
    std::vector<ProcessInfo> processes;  // For process logging
    SystemUsage systemUsage;             // For process logging
    
    // Constructor for debug messages
    LogMessage(LogMessageType t, const std::string& msg) 
        : type(t), content(msg) {}
    
    // Constructor for process messages
    LogMessage(const std::vector<ProcessInfo>& procs, const SystemUsage& usage)
        : type(LogMessageType::PROCESS_INFO), processes(procs), systemUsage(usage) {}
    
    // Constructor for shutdown
    LogMessage() : type(LogMessageType::SHUTDOWN) {}
};

// Thread-safe blocking queue implementation
template<typename T>
class BlockingQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> shutdown_{false};

public:
    void push(T&& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!shutdown_) {
            queue_.push(std::move(item));
            cv_.notify_one();
        }
    }
    
    void push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!shutdown_) {
            queue_.push(item);
            cv_.notify_one();
        }
    }
    
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || shutdown_; });
        
        if (shutdown_ && queue_.empty()) {
            return false;
        }
        
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
        cv_.notify_all();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
};

// Log rotation strategy enumeration
enum class LogRotationStrategy {
    SIZE_BASED,     // Rotate when file size exceeds limit
    DATE_BASED,     // Rotate daily/hourly/etc.
    COMBINED        // Rotate on both size and date
};

// Date rotation frequency
enum class DateRotationFrequency {
    DAILY,    // Rotate every day at midnight
    HOURLY,   // Rotate every hour
    WEEKLY    // Rotate every week (Sunday at midnight)
};

// Log configuration class
class LogConfig {
private:
    std::string logPath = "SystemMonitor.log";
    size_t maxFileSizeMB = 10;
    int maxBackupFiles = 5;
    bool enableRotation = true;
    size_t queueMaxSize = 1000;  // Maximum messages in queue
    
    // Date-based rotation settings
    LogRotationStrategy rotationStrategy = LogRotationStrategy::SIZE_BASED;
    DateRotationFrequency dateFrequency = DateRotationFrequency::DAILY;
    std::string dateFormat = "%Y%m%d";  // Format for date suffix (YYYYMMDD)
    bool keepDateInFilename = true;     // Keep date in rotated filenames

public:
    LogConfig() = default;
    LogConfig(const std::string& path, size_t maxSize, int maxBackups, bool rotation, size_t queueSize = 1000)
        : logPath(path), maxFileSizeMB(maxSize), maxBackupFiles(maxBackups), 
          enableRotation(rotation), queueMaxSize(queueSize) {}

    // Traditional getters
    const std::string& getLogPath() const { return logPath; }
    size_t getMaxFileSizeMB() const { return maxFileSizeMB; }
    int getMaxBackupFiles() const { return maxBackupFiles; }
    bool isRotationEnabled() const { return enableRotation; }
    size_t getQueueMaxSize() const { return queueMaxSize; }

    // Date rotation getters
    LogRotationStrategy getRotationStrategy() const { return rotationStrategy; }
    DateRotationFrequency getDateFrequency() const { return dateFrequency; }
    const std::string& getDateFormat() const { return dateFormat; }
    bool shouldKeepDateInFilename() const { return keepDateInFilename; }

    // Traditional setters
    void setLogPath(const std::string& path) { logPath = path; }
    void setMaxFileSizeMB(size_t size) { maxFileSizeMB = size; }
    void setMaxBackupFiles(int count) { maxBackupFiles = count; }
    void setRotationEnabled(bool enabled) { enableRotation = enabled; }
    void setQueueMaxSize(size_t size) { queueMaxSize = size; }

    // Date rotation setters
    void setRotationStrategy(LogRotationStrategy strategy) { rotationStrategy = strategy; }
    void setDateFrequency(DateRotationFrequency frequency) { dateFrequency = frequency; }
    void setDateFormat(const std::string& format) { dateFormat = format; }
    void setKeepDateInFilename(bool keep) { keepDateInFilename = keep; }

    // Convenience methods
    bool isSizeBasedRotation() const { 
        return rotationStrategy == LogRotationStrategy::SIZE_BASED || 
               rotationStrategy == LogRotationStrategy::COMBINED; 
    }
    bool isDateBasedRotation() const { 
        return rotationStrategy == LogRotationStrategy::DATE_BASED || 
               rotationStrategy == LogRotationStrategy::COMBINED; 
    }
};

// Abstract base class for loggers
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual bool initialize() = 0;
    virtual void debug(const std::string& message) = 0;
    virtual void logProcesses(const std::vector<ProcessInfo>& processes, 
                             const SystemUsage& systemUsage) = 0;
    virtual bool rotateIfNeeded() = 0;
    virtual void shutdown() = 0;
    virtual size_t getQueueSize() const = 0;
};

// Asynchronous file logger with blocking queue
class AsyncFileLogger : public ILogger {
private:
    LogConfig config;
    BlockingQueue<LogMessage> messageQueue;
    std::thread workerThread;
    std::atomic<bool> running{false};
    mutable std::ofstream debugLogStream;
    
    // Date tracking for rotation
    mutable std::string lastRotationDate;
    mutable std::string lastRotationHour;
    
    // Worker thread methods
    void workerThreadFunction();
    void processLogMessage(const LogMessage& message);
    void writeDebugMessage(const std::string& content);
    void writeProcessMessage(const std::vector<ProcessInfo>& processes, 
                           const SystemUsage& systemUsage);
    
    // File operations (synchronous, called from worker thread)
    std::string getCurrentTimeString() const;
    std::string getCurrentDateString(const std::string& format) const;
    std::string getCurrentHourString() const;
    
    // Rotation methods
    bool performRotation();
    bool performSizeBasedRotation();
    bool performDateBasedRotation();
    bool checkRotationNeeded();
    bool checkSizeRotationNeeded();
    bool checkDateRotationNeeded();
    
    // Helper methods
    std::string generateRotatedFilename(const std::string& basePath, const std::string& dateSuffix, int index = 0) const;
    void cleanupOldLogFiles() const;

public:
    explicit AsyncFileLogger(const LogConfig& logConfig);
    ~AsyncFileLogger() override;

    // Disable copy constructor and assignment operator
    AsyncFileLogger(const AsyncFileLogger&) = delete;
    AsyncFileLogger& operator=(const AsyncFileLogger&) = delete;

    // ILogger interface implementation
    bool initialize() override;
    void debug(const std::string& message) override;
    void logProcesses(const std::vector<ProcessInfo>& processes, 
                     const SystemUsage& systemUsage) override;
    bool rotateIfNeeded() override;
    void shutdown() override;
    size_t getQueueSize() const override { return messageQueue.size(); }

    // Configuration management
    const LogConfig& getConfig() const { return config; }
    void updateConfig(const LogConfig& newConfig) { config = newConfig; }
    
    // Status methods
    bool isRunning() const { return running; }
};

// Logger factory for creating different types of loggers
class LoggerFactory {
public:
    static std::unique_ptr<ILogger> createAsyncFileLogger(const LogConfig& config);
    static std::unique_ptr<ILogger> createFileLogger(const LogConfig& config); // Legacy synchronous version
    static std::unique_ptr<ILogger> createConsoleLogger();
    static std::unique_ptr<ILogger> createCompositeLogger(const std::vector<std::unique_ptr<ILogger>>& loggers);
};

// Singleton logger manager
class LoggerManager {
private:
    static LoggerManager* instance;
    std::unique_ptr<ILogger> logger;

    LoggerManager() = default;

public:
    static LoggerManager& getInstance();
    void setLogger(std::unique_ptr<ILogger> newLogger);
    ILogger* getLogger() const { return logger.get(); }

    // Convenience methods
    void debug(const std::string& message);
    void logProcesses(const std::vector<ProcessInfo>& processes, const SystemUsage& systemUsage);
    bool rotateIfNeeded();
    void shutdown();
    size_t getQueueSize() const;
};
