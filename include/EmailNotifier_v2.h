#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>

// Email configuration structure
struct EmailConfig {
    bool enabled = false;
    std::string smtpServer;
    int smtpPort = 587;
    std::string senderEmail;
    std::string password;
    std::string senderName;
    std::string recipients;
    bool useTLS = true;
    bool useSSL = false;
    int timeoutSeconds = 30;
    int alertDurationSeconds = 10;
    int cooldownMinutes = 2;
    bool sendRecoveryAlerts = true;
    int recoveryDurationSeconds = 15;
};

// Alert history tracking
struct AlertHistory {
    std::chrono::steady_clock::time_point lastAlertTime;
    bool alertActive = false;
    std::chrono::steady_clock::time_point alertStartTime;
    std::string alertType;
};

// Enhanced EmailNotifier with libcurl TLS support
class EmailNotifier {
private:
    EmailConfig config;
    std::map<std::string, AlertHistory> alertHistory;
    bool curlAvailable = false;
    
    // Check if libcurl is available
    bool initializeCurl();
    
    // Send email using libcurl with TLS
    bool sendEmailCurl(const std::string& subject, const std::string& body);
    
    // Fallback to PowerShell for email sending
    bool sendEmailPowerShell(const std::string& subject, const std::string& body);
    
    // Log email to simulation file if other methods fail
    void logEmailSimulation(const std::string& subject, const std::string& body);
    
    // Generate professional email body
    std::string generateAlertBody(const std::string& alertType, const std::vector<std::string>& logs);
    std::string generateRecoveryBody(const std::string& alertType);
    
    // Check cooldown period
    bool isInCooldown(const std::string& alertType);
    
public:
    EmailNotifier() = default;
    ~EmailNotifier();
    
    // Initialize with configuration
    bool initialize(const EmailConfig& emailConfig);
    
    // Send alert email
    bool sendAlert(const std::string& alertType, const std::vector<std::string>& logs);
    
    // Send recovery email
    bool sendRecovery(const std::string& alertType);
    
    // Test email functionality
    bool sendTestEmail();
    
    // Get current configuration
    const EmailConfig& getConfig() const { return config; }
    
    // Check if email notifications are enabled and working
    bool isOperational() const;
};
