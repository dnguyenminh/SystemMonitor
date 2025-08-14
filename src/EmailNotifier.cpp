#include <condition_variable>
#include "../include/EmailNotifier.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <condition_variable>
#include <fstream>
#include <ctime>
#include <winsock2.h>
#include <ws2tcpip.h>

// Include libcurl for TLS email support
#ifdef _WIN32
#include <curl/curl.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "normaliz.lib")
#endif

// Base64 encoding table
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

// Email data structure for libcurl
struct EmailPayload {
    std::string content;
    size_t pos = 0;
};

// Callback function for libcurl to read email data
static size_t payload_source(void* ptr, size_t size, size_t nmemb, void* userp) {
    EmailPayload* data = static_cast<EmailPayload*>(userp);
    size_t room = size * nmemb;
    
    if (room < 1 || data->pos >= data->content.length()) {
        return 0;
    }
    
    size_t len = (room < (data->content.length() - data->pos)) ? room : (data->content.length() - data->pos);
    memcpy(ptr, data->content.c_str() + data->pos, len);
    data->pos += len;
    
    return len;
}

// Send email using libcurl with TLS support
bool sendEmailWithLibcurl(const EmailMessage& message, const EmailConfig& config) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "❌ Failed to initialize libcurl" << std::endl;
        return false;
    }
    
    // Create email content
    EmailPayload emailData;
    std::ostringstream emailContent;
    
    emailContent << "To: ";
    for (size_t i = 0; i < message.recipients.size(); ++i) {
        if (i > 0) emailContent << ", ";
        emailContent << message.recipients[i];
    }
    emailContent << "\r\n";
    emailContent << "From: " << config.senderName << " <" << config.senderEmail << ">\r\n";
    emailContent << "Subject: " << message.subject << "\r\n";
    emailContent << "Content-Type: text/plain; charset=UTF-8\r\n";
    emailContent << "\r\n";
    emailContent << message.body << "\r\n";
    
    emailData.content = emailContent.str();
    
    // Gmail SMTP URL with SSL (port 465 for SSL, 587 for STARTTLS)
    std::string smtpUrl = "smtps://" + config.smtpServer + ":465";
    
    // Set CURL options for Gmail with TLS
    curl_easy_setopt(curl, CURLOPT_URL, smtpUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_USERNAME, config.senderEmail.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, config.senderPassword.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)config.timeoutSeconds);
    
    // Recipients list
    struct curl_slist* recipients = nullptr;
    for (const auto& recipient : message.recipients) {
        recipients = curl_slist_append(recipients, recipient.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    
    // Sender
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, config.senderEmail.c_str());
    
    // Email content
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &emailData);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    
    // Enable verbose output for debugging (remove in production)
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    
    // Send the email
    CURLcode res = curl_easy_perform(curl);
    
    // Cleanup
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        std::cerr << "❌ libcurl email sending failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }
    
    std::cout << "✅ Email sent successfully via libcurl TLS!" << std::endl;
    return true;
}

// WindowsEmailSender Implementation
WindowsEmailSender::WindowsEmailSender() {
    initializeWinsock();
    // Initialize libcurl globally
    curl_global_init(CURL_GLOBAL_DEFAULT);
    std::cout << "✅ libcurl TLS support initialized" << std::endl;
}

WindowsEmailSender::~WindowsEmailSender() {
    cleanupWinsock();
    // Cleanup libcurl
    curl_global_cleanup();
}

bool WindowsEmailSender::initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    return result == 0;
}

void WindowsEmailSender::cleanupWinsock() {
    WSACleanup();
}

std::string WindowsEmailSender::base64Encode(const std::string& input) {
    std::string encoded;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (encoded.size() % 4) {
        encoded.push_back('=');
    }
    return encoded;
}

bool WindowsEmailSender::connectToSMTP(const EmailConfig& config, int& clientSocket) {
    struct addrinfo hints = {0}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::string portStr = std::to_string(config.smtpPort);
    if (getaddrinfo(config.smtpServer.c_str(), portStr.c_str(), &hints, &result) != 0) {
        return false;
    }

    clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (clientSocket == INVALID_SOCKET) {
        freeaddrinfo(result);
        return false;
    }

    // Set timeout
    DWORD timeout = config.timeoutSeconds * 1000;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    bool connected = (connect(clientSocket, result->ai_addr, (int)result->ai_addrlen) != SOCKET_ERROR);
    freeaddrinfo(result);

    if (!connected) {
        closesocket(clientSocket);
        return false;
    }

    // Read initial response
    std::string response = receiveResponse(clientSocket);
    return response.find("220") == 0;
}

std::string WindowsEmailSender::receiveResponse(int socket) {
    char buffer[1024] = {0};
    int bytesReceived = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        return std::string(buffer, bytesReceived);
    }
    return "";
}

bool WindowsEmailSender::sendSMTPCommand(int socket, const std::string& command, const std::string& expectedResponse) {
    std::string fullCommand = command + "\r\n";
    if (send(socket, fullCommand.c_str(), fullCommand.length(), 0) == SOCKET_ERROR) {
        return false;
    }

    if (!expectedResponse.empty()) {
        std::string response = receiveResponse(socket);
        return response.find(expectedResponse) == 0;
    }
    return true;
}

bool WindowsEmailSender::authenticateSMTP(int socket, const EmailConfig& config) {
    if (!sendSMTPCommand(socket, "EHLO localhost", "250")) {
        return false;
    }

    if (config.useTLS) {
        if (!sendSMTPCommand(socket, "STARTTLS", "220")) {
            return false;
        }
        // Note: In a production environment, you would implement TLS/SSL here
        // For this example, we'll continue without TLS encryption
        std::cout << "Warning: TLS requested but not implemented in this demo\n";
    }

    if (!sendSMTPCommand(socket, "AUTH LOGIN", "334")) {
        return false;
    }

    std::string encodedUser = base64Encode(config.senderEmail);
    if (!sendSMTPCommand(socket, encodedUser, "334")) {
        return false;
    }

    std::string encodedPass = base64Encode(config.senderPassword);
    if (!sendSMTPCommand(socket, encodedPass, "235")) {
        return false;
    }

    return true;
}

bool WindowsEmailSender::sendEmail(const EmailMessage& message, const EmailConfig& config) {
    // Use libcurl TLS for Gmail and other SMTP servers
    if (config.smtpServer.find("gmail.com") != std::string::npos) {
        std::cout << "Sending email via Gmail with libcurl TLS..." << std::endl;
        return sendEmailWithLibcurl(message, config);
    }
    
    // For other SMTP servers, try libcurl first, then fallback to basic SMTP
    if (sendEmailWithLibcurl(message, config)) {
        return true;
    }
    
    std::cout << "libcurl failed, trying fallback SMTP..." << std::endl;
    
    // Fallback to basic SMTP for non-Gmail servers
    int clientSocket;
    if (!connectToSMTP(config, clientSocket)) {
        std::cerr << "Failed to connect to SMTP server\n";
        return false;
    }

    if (!authenticateSMTP(clientSocket, config)) {
        std::cerr << "SMTP authentication failed\n";
        closesocket(clientSocket);
        return false;
    }

    // Send MAIL FROM
    std::string mailFrom = "MAIL FROM:<" + config.senderEmail + ">";
    if (!sendSMTPCommand(clientSocket, mailFrom, "250")) {
        closesocket(clientSocket);
        return false;
    }

    // Send RCPT TO for each recipient
    for (const auto& recipient : message.recipients) {
        std::string rcptTo = "RCPT TO:<" + recipient + ">";
        if (!sendSMTPCommand(clientSocket, rcptTo, "250")) {
            closesocket(clientSocket);
            return false;
        }
    }

    // Send DATA command
    if (!sendSMTPCommand(clientSocket, "DATA", "354")) {
        closesocket(clientSocket);
        return false;
    }

    // Prepare email content
    std::ostringstream emailContent;
    emailContent << "From: " << config.senderName << " <" << config.senderEmail << ">\r\n";
    emailContent << "To: ";
    for (size_t i = 0; i < message.recipients.size(); ++i) {
        if (i > 0) emailContent << ", ";
        emailContent << message.recipients[i];
    }
    emailContent << "\r\n";
    emailContent << "Subject: " << message.subject << "\r\n";
    emailContent << "Content-Type: text/plain; charset=UTF-8\r\n";
    emailContent << "\r\n";
    emailContent << message.body << "\r\n";
    emailContent << ".\r\n";

    if (!sendSMTPCommand(clientSocket, emailContent.str(), "250")) {
        closesocket(clientSocket);
        return false;
    }

    sendSMTPCommand(clientSocket, "QUIT");
    closesocket(clientSocket);
    return true;
}

bool WindowsEmailSender::testConnection(const EmailConfig& config) {
    // For Gmail, use libcurl to test TLS connection
    if (config.smtpServer.find("gmail.com") != std::string::npos) {
        std::cout << "Testing Gmail connection with libcurl TLS..." << std::endl;
        
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "Failed to initialize libcurl" << std::endl;
            return false;
        }

        // Set up SMTP URL
        std::string smtpUrl = "smtps://" + config.smtpServer + ":465";
        curl_easy_setopt(curl, CURLOPT_URL, smtpUrl.c_str());
        
        // Set credentials
        curl_easy_setopt(curl, CURLOPT_USERNAME, config.senderEmail.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, config.senderPassword.c_str());
        
        // Enable TLS/SSL
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        
        // Set timeout
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)config.timeoutSeconds);
        
        // Just test connection (no actual email)
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
        
        CURLcode res = curl_easy_perform(curl);
        bool success = (res == CURLE_OK);
        
        if (success) {
            std::cout << "✅ Gmail TLS connection test successful!" << std::endl;
        } else {
            std::cerr << "❌ Gmail TLS connection test failed: " << curl_easy_strerror(res) << std::endl;
        }
        
        curl_easy_cleanup(curl);
        return success;
    }
    
    // For other SMTP servers, use basic socket connection test
    int clientSocket;
    if (!connectToSMTP(config, clientSocket)) {
        return false;
    }

    bool authSuccess = authenticateSMTP(clientSocket, config);
    sendSMTPCommand(clientSocket, "QUIT");
    closesocket(clientSocket);
    return authSuccess;
}

// EmailNotifier Implementation
EmailNotifier::EmailNotifier() : running(false) {
    emailSender = std::make_unique<WindowsEmailSender>();
}

EmailNotifier::EmailNotifier(const EmailConfig& emailConfig) 
    : config(emailConfig), running(false) {
    emailSender = std::make_unique<WindowsEmailSender>();
}

EmailNotifier::~EmailNotifier() {
    stop();
}

void EmailNotifier::setConfig(const EmailConfig& emailConfig) {
    std::lock_guard<std::mutex> lock(alertMutex);
    config = emailConfig;
}

void EmailNotifier::emailWorkerLoop() {
    while (running.load()) {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        queueCondition.wait(lock, [this] { 
            return !emailQueue.empty() || !running.load(); 
        });

        if (!running.load()) break;

        if (!emailQueue.empty()) {
            EmailMessage message = emailQueue.front();
            emailQueue.pop();
            lock.unlock();

            try {
                if (config.isValid()) {
                    bool success = emailSender->sendEmail(message, config);
                    if (!success) {
                        std::cerr << "Failed to send email: " << message.subject << std::endl;
                    } else {
                        std::cout << "Email sent successfully: " << message.subject << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception while sending email: " << e.what() << std::endl;
            }
        }
    }
}

bool EmailNotifier::shouldSendAlert() const {
    if (!isEnabled()) return false;
    
    auto now = std::chrono::system_clock::now();
    
    // Check cooldown period
    if (alertHistory.alertSent) {
        auto timeSinceLastAlert = std::chrono::duration_cast<std::chrono::minutes>(
            now - alertHistory.lastAlertSent).count();
        if (timeSinceLastAlert < config.cooldownMinutes) {
            return false;
        }
    }
    
    // Check if thresholds have been exceeded for the required duration
    if (alertHistory.isCurrentlyExceeded && !alertHistory.alertSent) {
        auto alertDuration = std::chrono::duration_cast<std::chrono::seconds>(
            now - alertHistory.thresholdExceededStart).count();
        return alertDuration >= config.alertDurationSeconds;
    }
    
    return false;
}

bool EmailNotifier::shouldSendRecoveryAlert() const {
    if (!isEnabled() || !config.sendRecoveryAlerts) return false;
    
    // Only send recovery if we previously sent an alert and are now waiting for recovery
    if (!alertHistory.waitingForRecovery) return false;
    
    auto now = std::chrono::system_clock::now();
    
    // Check if thresholds have been normal for the required duration
    if (!alertHistory.isCurrentlyExceeded) {
        auto recoveryDuration = std::chrono::duration_cast<std::chrono::seconds>(
            now - alertHistory.thresholdNormalStart).count();
        return recoveryDuration >= config.recoveryDurationSeconds;
    }
    
    return false;
}

std::string EmailNotifier::generateAlertEmail(const std::vector<std::string>& logs) const {
    std::ostringstream body;
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);   
	
    body << "SYSTEM MONITOR ALERT\n";
    body << "====================\n\n";
    body << "Alert Generated: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n";
    body << "Alert Duration: " << config.alertDurationSeconds << " seconds\n";
    body << "Threshold Monitoring Period: " << logs.size() << " log entries\n\n";
    
    body << "SYSTEM RESOURCE THRESHOLDS EXCEEDED:\n";
    body << "- CPU Threshold: " << config.alertDurationSeconds << " seconds of monitoring\n";
    body << "- RAM/Disk activity detected above normal levels\n\n";
    
    body << "DETAILED LOG ANALYSIS\n";
    body << "=======================\n";
    
    if (logs.empty()) {
        body << "No detailed logs available for this alert period.\n\n";
    } else {
        body << "Complete system monitoring logs during alert period:\n\n";
        
        // Include all logs exactly as they appear in the console
        for (const auto& logEntry : logs) {
            body << logEntry << "\n";
			// Add an extra newline if line contains "===" or "+"
			if (logEntry.find("===") != std::string::npos || 
				logEntry.find("+") != std::string::npos) 
			{
				body << "\n";
			}
        }
        body << "\n";
    }
    
    body << "RECOMMENDATIONS\n";
    body << "================\n";
    body << "1. Check for resource-intensive processes\n";
    body << "2. Monitor disk I/O activity\n";
    body << "3. Verify system memory usage patterns\n";
    body << "4. Consider scaling resources if this is a recurring issue\n\n";
    
    body << "This alert was automatically generated by SystemMonitor.\n";
    body << "Next alert will be suppressed for " << config.cooldownMinutes << " minutes.\n";   
    return body.str();
}

std::string EmailNotifier::generateRecoveryEmail(const std::vector<std::string>& alertLogs, 
                                                 const std::vector<std::string>& recoveryLogs) const {
    std::ostringstream body;
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);   
	
    body << "SYSTEM MONITOR RECOVERY ALERT\n";
    body << "==============================\n\n";
    body << "Recovery Detected: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n";
    body << "Recovery Duration: " << config.recoveryDurationSeconds << " seconds below thresholds\n";
    body << "Original Alert Period: " << alertLogs.size() << " log entries\n";
    body << "Recovery Period: " << recoveryLogs.size() << " log entries\n\n";
    
    body << "SYSTEM STATUS: ALL CLEAR\n";
    body << "=========================\n";
    body << "✅ All system resources have returned to normal levels\n";
    body << "✅ Thresholds no longer exceeded\n";
    body << "✅ System performance stabilized\n\n";
    
    body << "RECOVERY SYSTEM ANALYSIS\n";
    body << "========================\n";
    
    if (recoveryLogs.empty()) {
        body << "No detailed recovery logs available.\n\n";
    } else {
        body << "Recent system state showing normal operation:\n\n";
        // Show recent recovery logs to demonstrate normal state
        size_t logsToShow = (recoveryLogs.size() < 10) ? recoveryLogs.size() : 10;
        size_t startIndex = (recoveryLogs.size() >= logsToShow) ? recoveryLogs.size() - logsToShow : 0;
        
        for (size_t i = startIndex; i < recoveryLogs.size(); ++i) {
            body << recoveryLogs[i] << "\n";
			// Add an extra newline if line contains "===" or "+"
			if (recoveryLogs[i].find("===") != std::string::npos || 
				recoveryLogs[i].find("+") != std::string::npos) 
			{
				body << "\n";
			}
        }
        body << "\n";
    }
    
    if (!alertLogs.empty()) {
        body << "ORIGINAL ALERT SYSTEM ANALYSIS\n";
        body << "===============================\n";
        body << "For reference, the original alert was triggered by:\n\n";
        
        // Show first portion of alert logs for context
        size_t logsToShow = (alertLogs.size() < 15) ? alertLogs.size() : 15;
        for (size_t i = 0; i < logsToShow; ++i) {
            body << alertLogs[i] << "\n";
			// Add an extra newline if line contains "===" or "+"
			if (alertLogs[i].find("===") != std::string::npos || 
				alertLogs[i].find("+") != std::string::npos) 
			{
				body << "\n";
			}
        }
        
        if (alertLogs.size() > logsToShow) {
            body << "\n... (" << (alertLogs.size() - logsToShow) << " additional alert log entries) ...\n";
        }
        body << "\n";
    }
    
    body << "NEXT STEPS\n";
    body << "===========\n";
    body << "1. ✅ System monitoring continues normally\n";
    body << "2. ✅ Performance issue appears resolved\n";
    body << "3. 📊 Review logs to understand what caused the original issue\n";
    body << "4. 🔧 Consider preventive measures if this was a recurring problem\n\n";
    
    body << "This recovery alert was automatically generated by SystemMonitor.\n";
    body << "Normal monitoring and alerting will resume.\n";	
    
    return body.str();
}

std::string EmailNotifier::formatLogEntry(const std::string& logEntry) const {
    // Parse and format log entry for better readability
    if (logEntry.find("===Start") != std::string::npos) {
        return logEntry; // Keep system analysis headers as-is
    }
    if (logEntry.find("SYSTEM ANALYSIS:") != std::string::npos) {
        return logEntry; // Keep system analysis as-is
    }
    if (logEntry.find("TOTALS:") != std::string::npos) {
        return logEntry; // Keep totals as-is
    }
    if (logEntry.find("SYSTEM OVERHEAD:") != std::string::npos) {
        return logEntry; // Keep overhead analysis as-is
    }
    
    return logEntry;
}

void EmailNotifier::checkThresholds(bool thresholdsExceeded, const std::string& currentLogEntry) {
    std::lock_guard<std::mutex> lock(alertMutex);
    auto now = std::chrono::system_clock::now();
    
    if (thresholdsExceeded) {
        if (!alertHistory.isCurrentlyExceeded) {
            // Thresholds just started being exceeded
            alertHistory.isCurrentlyExceeded = true;
            alertHistory.thresholdExceededStart = now;
            alertHistory.logsDuringAlert.clear();
            
            // If we were waiting for recovery, cancel that state
            alertHistory.waitingForRecovery = false;
            alertHistory.logsDuringRecovery.clear();
        }
        
        // Add current log entry to alert logs
        if (!currentLogEntry.empty()) {
            alertHistory.logsDuringAlert.push_back(currentLogEntry);
        }
        
        // Check if we should send an alert
        if (shouldSendAlert()) {
            //std::string subject = "SystemMonitor Alert: Resource Thresholds Exceeded";			
			//std::string subject = config.get("EMAIL_SUBJECT_ALERT", "SystemMonitor Alert: Resource Thresholds Exceeded");
			std::string subject = config.subjectAlert;
            std::string body = generateAlertEmail(alertHistory.logsDuringAlert);
			EmailMessage alert(subject, body, config.recipients);
			queueEmail(alert);            
            alertHistory.alertSent = true;
            alertHistory.lastAlertSent = now;
            alertHistory.waitingForRecovery = true; // Start watching for recovery
        }
    } else {
        // Thresholds are no longer exceeded
        if (alertHistory.isCurrentlyExceeded) {
            // Just transitioned from exceeded to normal
            alertHistory.isCurrentlyExceeded = false;
            alertHistory.thresholdNormalStart = now;
            
            // If we sent an alert and recovery alerts are enabled, start tracking recovery
            if (alertHistory.alertSent && config.sendRecoveryAlerts) {
                alertHistory.waitingForRecovery = true;
                alertHistory.logsDuringRecovery.clear();
            }
        }
        
        // If we're tracking recovery, add current log entry
        if (alertHistory.waitingForRecovery && !currentLogEntry.empty()) {
            alertHistory.logsDuringRecovery.push_back(currentLogEntry);
        }
        
        // Check if we should send a recovery alert
        if (shouldSendRecoveryAlert()) {
            //std::string subject = "SystemMonitor Recovery: All Systems Normal";
			//std::string subject = config.get("EMAIL_SUBJECT_RECORVER", "SystemMonitor Recovery: All Systems Normal");
            std::string subject = config.subjectRecover;
			 // Wrap your alert body in HTML
			std::string htmlBody =
				"<html><body><pre>" +
				generateRecoveryEmail(alertHistory.logsDuringAlert,alertHistory.logsDuringRecovery) +
				"</pre></body></html>";

			// Add MIME headers so SMTP knows it's HTML
			std::string fullBody =
				"MIME-Version: 1.0\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"\r\n" +
				htmlBody;
			//std::string body = generateRecoveryEmail(alertHistory.logsDuringAlert,alertHistory.logsDuringRecovery);            
            EmailMessage recoveryAlert(subject, fullBody, config.recipients);
            queueEmail(recoveryAlert);
			
            
            // Reset the alert history after sending recovery email
            alertHistory.reset();
        }
    }
}

void EmailNotifier::sendImmediateAlert(const std::string& subject, const std::string& message) {
    if (!isEnabled()) return;
    std::string contentType = message.find("<html>") == 0 
    ? "text/html; charset=UTF-8" 
    : "text/plain; charset=UTF-8";
    EmailMessage alert(subject, message, config.recipients);
    queueEmail(alert);
}

bool EmailNotifier::testEmailConfiguration() {
    if (!config.isValid()) {
        std::cerr << "Email configuration is invalid\n";
        return false;
    }
    
    return emailSender->testConnection(config);
}

void EmailNotifier::queueEmail(const EmailMessage& message) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        emailQueue.push(message);
    }
    queueCondition.notify_one();
}

size_t EmailNotifier::getQueueSize() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queueMutex));
    return emailQueue.size();
}

bool EmailNotifier::start() {
    if (running.load()) return true;
    
    running.store(true);
    emailWorkerThread = std::thread(&EmailNotifier::emailWorkerLoop, this);
    return true;
}

void EmailNotifier::stop() {
    if (!running.load()) return;
    
    running.store(false);
    queueCondition.notify_all();
    
    if (emailWorkerThread.joinable()) {
        emailWorkerThread.join();
    }
}

std::chrono::system_clock::time_point EmailNotifier::getLastAlertTime() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(alertMutex));
    return alertHistory.lastAlertSent;
}

bool EmailNotifier::isInCooldownPeriod() const {
    if (!alertHistory.alertSent) return false;
    
    auto now = std::chrono::system_clock::now();
    auto timeSinceLastAlert = std::chrono::duration_cast<std::chrono::minutes>(
        now - alertHistory.lastAlertSent).count();
    return timeSinceLastAlert < config.cooldownMinutes;
}

// EmailNotifierFactory Implementation
std::unique_ptr<EmailNotifier> EmailNotifierFactory::createNotifier(const EmailConfig& config) {
    return std::make_unique<EmailNotifier>(config);
}

std::unique_ptr<IEmailSender> EmailNotifierFactory::createEmailSender() {
    return std::make_unique<WindowsEmailSender>();
}

EmailConfig EmailNotifierFactory::createDefaultConfig() {
    EmailConfig config;
    config.smtpServer = "smtp.gmail.com";
    config.smtpPort = 587;
    config.useTLS = true;
    config.alertDurationSeconds = 300; // 5 minutes
    config.cooldownMinutes = 60;       // 1 hour
    config.enableEmailAlerts = false;
    return config;
}
