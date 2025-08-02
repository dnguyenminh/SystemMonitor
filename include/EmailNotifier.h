#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>

// Email configuration structure
struct EmailConfig {
    std::string smtpServer = "smtp.gmail.com";
    int smtpPort = 587;
    std::string senderEmail = "";
    std::string senderPassword = "";
    std::string senderName = "SystemMonitor";
    std::vector<std::string> recipients;
    bool useTLS = true;
    bool useSSL = false;
    int timeoutSeconds = 30;
    
    // Alert configuration
    int alertDurationSeconds = 300;  // 5 minutes
    int cooldownMinutes = 60;        // 1 hour cooldown between alerts
    bool enableEmailAlerts = false;
    bool sendRecoveryAlerts = true;  // Send "all clear" emails when thresholds return to normal
    int recoveryDurationSeconds = 120; // 2 minutes below threshold before sending recovery email
    
    bool isValid() const {
        return !senderEmail.empty() && 
               !senderPassword.empty() && 
               !recipients.empty() &&
               !smtpServer.empty();
    }
};

// Email message structure
struct EmailMessage {
    std::string subject;
    std::string body;
    std::vector<std::string> recipients;
    std::chrono::system_clock::time_point timestamp;
    
    EmailMessage(const std::string& subj, const std::string& content, 
                 const std::vector<std::string>& recips)
        : subject(subj), body(content), recipients(recips), 
          timestamp(std::chrono::system_clock::now()) {}
};

// Abstract email sender interface
class IEmailSender {
public:
    virtual ~IEmailSender() = default;
    virtual bool sendEmail(const EmailMessage& message, const EmailConfig& config) = 0;
    virtual bool testConnection(const EmailConfig& config) = 0;
};

// Windows SMTP email sender implementation
class WindowsEmailSender : public IEmailSender {
private:
    bool initializeWinsock();
    void cleanupWinsock();
    bool connectToSMTP(const EmailConfig& config, int& socket);
    bool authenticateSMTP(int socket, const EmailConfig& config);
    bool sendSMTPCommand(int socket, const std::string& command, const std::string& expectedResponse = "");
    std::string receiveResponse(int socket);
    std::string base64Encode(const std::string& input);
    
public:
    WindowsEmailSender();
    ~WindowsEmailSender();
    
    bool sendEmail(const EmailMessage& message, const EmailConfig& config) override;
    bool testConnection(const EmailConfig& config) override;
};

// Alert tracking structure
struct AlertHistory {
    std::chrono::system_clock::time_point thresholdExceededStart;
    std::chrono::system_clock::time_point lastAlertSent;
    std::chrono::system_clock::time_point thresholdNormalStart;
    bool isCurrentlyExceeded = false;
    bool alertSent = false;
    bool waitingForRecovery = false;  // Tracking if we need to send recovery email
    std::vector<std::string> logsDuringAlert;
    std::vector<std::string> logsDuringRecovery;
    
    void reset() {
        isCurrentlyExceeded = false;
        alertSent = false;
        waitingForRecovery = false;
        logsDuringAlert.clear();
        logsDuringRecovery.clear();
    }
};

// Email notification manager
class EmailNotifier {
private:
    EmailConfig config;
    std::unique_ptr<IEmailSender> emailSender;
    AlertHistory alertHistory;
    
    // Thread-safe email queue
    std::queue<EmailMessage> emailQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::thread emailWorkerThread;
    std::atomic<bool> running;
    
    // Alert state tracking
    std::mutex alertMutex;
    
    void emailWorkerLoop();
    bool shouldSendAlert() const;
    bool shouldSendRecoveryAlert() const;
    std::string generateAlertEmail(const std::vector<std::string>& logs) const;
    std::string generateRecoveryEmail(const std::vector<std::string>& alertLogs, 
                                     const std::vector<std::string>& recoveryLogs) const;
    std::string formatLogEntry(const std::string& logEntry) const;
    
public:
    EmailNotifier();
    explicit EmailNotifier(const EmailConfig& emailConfig);
    ~EmailNotifier();
    
    // Delete copy constructor and assignment operator
    EmailNotifier(const EmailNotifier&) = delete;
    EmailNotifier& operator=(const EmailNotifier&) = delete;
    
    // Configuration
    void setConfig(const EmailConfig& emailConfig);
    const EmailConfig& getConfig() const { return config; }
    bool isEnabled() const { return config.enableEmailAlerts && config.isValid(); }
    
    // Alert management
    void checkThresholds(bool thresholdsExceeded, const std::string& currentLogEntry);
    void sendImmediateAlert(const std::string& subject, const std::string& message);
    bool testEmailConfiguration();
    
    // Queue management
    void queueEmail(const EmailMessage& message);
    size_t getQueueSize() const;
    
    // Lifecycle
    bool start();
    void stop();
    bool isRunning() const { return running.load(); }
    
    // Statistics
    std::chrono::system_clock::time_point getLastAlertTime() const;
    bool isInCooldownPeriod() const;
    int getAlertDurationSeconds() const { return config.alertDurationSeconds; }
    int getCooldownMinutes() const { return config.cooldownMinutes; }
};

// Email notification factory
class EmailNotifierFactory {
public:
    static std::unique_ptr<EmailNotifier> createNotifier(const EmailConfig& config);
    static std::unique_ptr<IEmailSender> createEmailSender();
    static EmailConfig createDefaultConfig();
};
