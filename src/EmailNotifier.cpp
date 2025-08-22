#include <condition_variable>
#include "../include/EmailNotifier.h"
#include "../include/SystemInfo.h"
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
    if (message.isHtml) {
        emailContent << "Content-Type: text/html; charset=UTF-8\r\n";
    } else {
        emailContent << "Content-Type: text/plain; charset=UTF-8\r\n";
    }
    
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
     if (message.isHtml) {
        emailContent << "Content-Type: text/html; charset=UTF-8\r\n";
    } else {
        emailContent << "Content-Type: text/plain; charset=UTF-8\r\n";
    }
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

// std::string EmailNotifier::generateAlertEmail(const std::vector<std::string>& logs) const {
//     std::ostringstream body;
//     auto now = std::chrono::system_clock::now();
//     auto time_t = std::chrono::system_clock::to_time_t(now);   
	
// 	body << "<html><body>";
//     body << "<h2 style='color:red;'>⚠️ SystemMonitor Alert: Resource Thresholds Exceeded</h2>";
//     body << "<p>The following log entries triggered the alert:</p>";
//     body << "<ul>";
//     body << "SYSTEM MONITOR ALERT<br>";
//     body << "====================<br><br>";
//     body << "Alert Generated: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "<br>";
//     body << "Alert Duration: " << config.alertDurationSeconds << " seconds<br>";
//     body << "Threshold Monitoring Period: " << logs.size() << " log entries<br><br>";
    
//     body << "SYSTEM RESOURCE THRESHOLDS EXCEEDED:<br>";
//     body << "- CPU Threshold: " << config.alertDurationSeconds << " seconds of monitoring<br>";
//     body << "- RAM/Disk activity detected above normal levels<br><br>";
    
//     body << "DETAILED LOG ANALYSIS<br>";
//     body << "=======================<br>";
    
//     if (logs.empty()) {
//         body << "No detailed logs available for this alert period.<br><br>";
//     } else {
//         body << "Complete system monitoring logs during alert period:<br><br>";
        
//         // Include all logs exactly as they appear in the console
//         for (const auto& logEntry : logs) {
// 			//std::string modifiedLog = logEntry;
// 			 body << "<li><pre>" << logEntry << "</pre></li>";
//              if (logEntry.find("===End") != std::string::npos) 
// 			{
// 				body << "<br>"; // Ensure new section starts on a fresh line
// 			}
			
// 		}

//         body << "<br>";
//     }
    
//     body << "RECOMMENDATIONS<br>";
//     body << "================<br>";
//     body << "1. Check for resource-intensive processes<br>";
//     body << "2. Monitor disk I/O activity<br>";
//     body << "3. Verify system memory usage patterns<br>";
//     body << "4. Consider scaling resources if this is a recurring issue<br><br>";
    
//     body << "This alert was automatically generated by SystemMonitor.<br>";
//     body << "Next alert will be suppressed for " << config.cooldownMinutes << " minutes.<br>";
// 	body << "</ul>";
//     body << "<br><br>";
//     body << "Thanks,";
//     body << "<br>";
//     body << "IT of App Risk Team";
//     body << "</body></html>";
//     return body.str();
// }
std::string EmailNotifier::generateAlertEmail(const std::vector<std::string>& logs) const {
    std::ostringstream body;
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);   
     std::string hostname = getComputerName();
    body << "<html><body style='background-color:#ff9800; font-family:Arial, sans-serif; color:black;'>";

    body << "<h2 style='color:red;'>⚠️ SystemMonitor Alert: Resource Thresholds Exceeded</h2>";
    body << "<p><strong>SYSTEM MONITOR ALERT</strong><br>";
    body << "Alert Generated on: <b>" << hostname << "</b></p>";
    body << "Alert Generated: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "<br>";
    body << "<strong>Alert Duration:</strong> " << config.alertDurationSeconds << " seconds<br>";
    body << "<strong>Threshold Monitoring Period:</strong> " << logs.size() << " log entries</p>";

    body << "<hr style='border:1px solid #444;'>";

    body << "<h3>SYSTEM RESOURCE THRESHOLDS EXCEEDED</h3>";
    body << "<p>⚡ CPU Threshold: " << config.alertDurationSeconds << " seconds of monitoring<br>";
    body << "🗂 RAM/Disk activity detected above normal levels</p>"; 
    body << "<h3>DETAILED LOG ANALYSIS</h3>";
    if (logs.empty()) {
        body << "<p>No detailed logs available for this alert period.</p>";
    } else {
        body << "<p>Complete system monitoring logs during alert period:</p>";
        // Dark background section for logs
        body << "<div style='background-color:#222; color:white; padding:10px; border-radius:8px; margin-top:15px;'>";
        body << "<ul>";
        for (const auto& logEntry : logs) {
            body << "<li><pre style='white-space:pre-wrap; color:white;'>" 
                 << logEntry << "</pre></li>";
            if (logEntry.find("===End") != std::string::npos) {
                body << "<br>";
            }
        }
        body << "</ul>";
    }

    body << "</div>"; // end dark background section

    body << "<h3>RECOMMENDATIONS</h3>";
    body << "<p>1. Check for resource-intensive processes<br>";
    body << "2. Monitor disk I/O activity<br>";
    body << "3. Verify system memory usage patterns<br>";
    body << "4. Consider scaling resources if this is a recurring issue</p>";

    body << "<p>This alert was automatically generated by SystemMonitor.<br>";
    body << "Next alert will be suppressed for " << config.cooldownMinutes << " minutes.</p>";

    body << "<br>Thanks,<br>IT of App Risk Team";

    body << "</body></html>";
    return body.str();
}



std::string EmailNotifier::generateRecoveryEmail(const std::vector<std::string>& alertLogs, 
                                                 const std::vector<std::string>& recoveryLogs) const {
    std::ostringstream body;
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::string hostname = getComputerName();

    body << R"(<!DOCTYPE html>
    <html lang="en">
    <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>SystemMonitor Recovery</title>
    <style>
    body{margin:0;background:#0a5f0a;color:#1f2937;font-family:Segoe UI,Roboto,Arial,sans-serif;line-height:1.45}
    .wrapper{width:100%;table-layout:fixed;background:#0a5f0a;padding:24px 12px}
    .container{max-width:800px;margin:0 auto;background:#ffffff;border-radius:12px;box-shadow:0 2px 8px rgba(0,0,0,.06);overflow:hidden}
    .header{background:#0f7b0f;color:#fff;padding:18px 22px;font-weight:700;font-size:18px}
    .subhead{font-weight:600;color:#e8ffe8;opacity:.9;margin-top:6px;font-size:12px}
    .content{padding:22px}
    h2{margin:20px 0 10px;font-size:16px;color:#0f172a}
    hr{border:none;border-top:1px solid #e5e7eb;margin:16px 0}
    .badge{display:inline-block;padding:4px 8px;border-radius:999px;background:#ecfdf5;color:#065f46;font-weight:600;font-size:12px;border:1px solid #a7f3d0}
    .ok{color:#065f46}
    .muted{color:#6b7280}
    .list{margin:8px 0 0 20px}
    .kvd{display:grid;grid-template-columns:220px 1fr;gap:8px 12px;margin-top:8px}
    .kvd div{padding:6px 10px;border:1px solid #e5e7eb;border-radius:8px}
    .mono{font-family:Consolas,Monaco,Menlo,monospace;background:#0b1020;color:#e6edf3;border-radius:10px;padding:14px;overflow:auto;font-size:12px}
    .check{margin-right:8px}
    .section{background:#fafafa;border:1px solid #eee;border-radius:10px;padding:14px}
    .foot{font-size:12px;color:#6b7280;text-align:center;padding:16px}
    @media (prefers-color-scheme: dark){
        body{background:#0b0e14;color:#e5e7eb}
        .container{background:#0f1320;box-shadow:none;border:1px solid #1f2937}
        .header{background:#0a5f0a}
        h2{color:#e5e7eb}
        hr{border-top-color:#243041}
        .kvd div{border-color:#243041}
        .section{background:#0b1020;border-color:#1f2937}
        .mono{background:#060a16;color:#e6edf3}
        .foot{color:#9ca3af}
    }
    </style>
    </head>
    <body>
    <table role="presentation" class="wrapper" width="100%" cellspacing="0" cellpadding="0">
        <tr><td align="center">
        <table role="presentation" class="container" width="100%" cellspacing="0" cellpadding="0">
            <tr><td class="header">
            SystemMonitor Recovery: All Systems Normal -- Generated on )"
         << hostname <<
        R"(
            <div class="subhead">SYSTEM MONITOR RECOVERY ALERT</div>
            </td></tr>
            <tr><td class="content">

            <span class="badge">All Clear</span>

            <h2>Recovery Summary</h2>
            <div class="kvd">)";

    // Insert dynamic data here
    body << "<div><strong>Recovery Detected</strong><br><span class='muted'>" 
         << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "</span></div>";
    body << "<div><strong>Recovery Duration</strong><br><span class='muted'>" 
         << config.recoveryDurationSeconds << " seconds below thresholds</span></div>";
    body << "<div><strong>Original Alert Period</strong><br><span class='muted'>" 
         << alertLogs.size() << " log entries</span></div>";
    body << "<div><strong>Recovery Period</strong><br><span class='muted'>" 
         << recoveryLogs.size() << " log entries</span></div>";

    body << R"(</div>
          <hr>

          <h2>SYSTEM STATUS: ALL CLEAR</h2>
          <div class="section">
            <p><span class="check">✅</span>All system resources have returned to normal levels</p>
            <p><span class="check">✅</span>Thresholds no longer exceeded</p>
            <p><span class="check">✅</span>System performance stabilized</p>
          </div>

          <h2>RECOVERY SYSTEM ANALYSIS</h2>
          <div class="section">)";

    if (recoveryLogs.empty()) {
        body << "No detailed recovery logs available.";
    } else {
        body << "Recent system state showing normal operation:<br><br><ul>";
        size_t logsToShow = (recoveryLogs.size() < 10) ? recoveryLogs.size() : 10;
        size_t startIndex = recoveryLogs.size() - logsToShow;
        for (size_t i = startIndex; i < recoveryLogs.size(); ++i) {
            body << "<li><pre>" << recoveryLogs[i] << "</pre></li>";
            if (recoveryLogs[i].find("===End") != std::string::npos) {
                body << "<br>"; // Ensure new section starts on a fresh line
            }
        }
        body << "</ul>";
    }

    body << R"(</div>

          <h2>ORIGINAL ALERT SYSTEM ANALYSIS</h2>
          <p class="muted">For reference, the original alert was triggered by:</p>
          <pre class="mono">)";

    size_t logsToShow = (alertLogs.size() < 15) ? alertLogs.size() : 15;
    for (size_t i = 0; i < logsToShow; ++i) {
        body << "• " << alertLogs[i] << "\n";
        if (alertLogs[i].find("===End") != std::string::npos) {
            body << "<br>";
        }   
    }
    if (alertLogs.size() > logsToShow) {
        body << "\n... (" << (alertLogs.size() - logsToShow) << " additional alert log entries) ...\n";         
    }

    body << R"(</pre>

          <h2>NEXT STEPS</h2>
          <div class="section">
            1. ✅ System monitoring continues normally<br>
            2. ✅ Performance issue appears resolved<br>
            3. 📊 Review logs to understand what caused the original issue<br>
            4. 🔧 Consider preventive measures if this was a recurring problem<br>
          </div>
          <div class="foot">Generated automatically by SystemMonitor – IT of App Risk Team</div>
        </td></tr>
      </table>
    </td></tr>
  </table>
  <p style='color: #ffffff'><br>Thanks,<br>IT of App Risk Team</p>
</body>
</html>)";

    return body.str();
}


// std::string EmailNotifier::generateRecoveryEmail(const std::vector<std::string>& alertLogs, 
//                                                  const std::vector<std::string>& recoveryLogs) const {
//     std::ostringstream body;
//     auto now = std::chrono::system_clock::now();
//     auto time_t = std::chrono::system_clock::to_time_t(now);   
//     body << "<html><body>";
//     body << "<h2 style='color:blue;'>⚠️ SystemMonitor Recovery: All Systems Normal</h2>";
//     body << "<p>The following log entries triggered the alert:</p>";
//     body << "<ul>";
	
//     body << "SYSTEM MONITOR RECOVERY ALERT<br>";
//     body << "==============================<br><br>";
//     body << "Recovery Detected: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "<br>";
//     body << "Recovery Duration: " << config.recoveryDurationSeconds << " seconds below thresholds<br>";
//     body << "Original Alert Period: " << alertLogs.size() << " log entries<br>";
//     body << "Recovery Period: " << recoveryLogs.size() << " log entries<br><br>";
    
//     body << "SYSTEM STATUS: ALL CLEAR<br>";
//     body << "=========================<br>";
//     body << "✅ All system resources have returned to normal levels<br>";
//     body << "✅ Thresholds no longer exceeded<br>";
//     body << "✅ System performance stabilized<br><br>";
    
//     body << "RECOVERY SYSTEM ANALYSIS<br>";
//     body << "========================<br>";
    
//     if (recoveryLogs.empty()) {
//         body << "No detailed recovery logs available.<br><br>";
//     } else {
//         body << "Recent system state showing normal operation:<br><br>";
//         // Show recent recovery logs to demonstrate normal state
//         size_t logsToShow = (recoveryLogs.size() < 10) ? recoveryLogs.size() : 10;
//         size_t startIndex = (recoveryLogs.size() >= logsToShow) ? recoveryLogs.size() - logsToShow : 0;
        
//         for (size_t i = startIndex; i < recoveryLogs.size(); ++i) {            		
//              body << "<li><pre>" << recoveryLogs[i] << "</pre></li>";
//              if (recoveryLogs[i].find("===End") != std::string::npos) 
// 			{
// 				body << "<br>"; // Ensure new section starts on a fresh line
// 			}
//         }
//         body << "<br>";
//     }
    
//     if (!alertLogs.empty()) {
//         body << "ORIGINAL ALERT SYSTEM ANALYSIS<br>";
//         body << "===============================<br>";
//         body << "For reference, the original alert was triggered by:<br><br>";
        
//         // Show first portion of alert logs for context
//         size_t logsToShow = (alertLogs.size() < 15) ? alertLogs.size() : 15;
//         for (size_t i = 0; i < logsToShow; ++i) {
//             body << "<li><pre>" << alertLogs[i] << "</pre></li>";
//             if (alertLogs[i].find("===End") != std::string::npos) 
// 			{
// 				body << "<br>"; // Ensure new section starts on a fresh line
// 			}           
//         }
        
//         if (alertLogs.size() > logsToShow) {
//             body << "<br>... (" << (alertLogs.size() - logsToShow) << " additional alert log entries) ...<br>";
//         }
//         body << "<br>";
//     }
    
//     body << "NEXT STEPS<br>";
//     body << "===========<br>";
//     body << "1. ✅ System monitoring continues normally<br>";
//     body << "2. ✅ Performance issue appears resolved<br>";
//     body << "3. 📊 Review logs to understand what caused the original issue<br>";
//     body << "4. 🔧 Consider preventive measures if this was a recurring problem<br><br>";
    
//     body << "This recovery alert was automatically generated by SystemMonitor.<br>";
//     body << "Normal monitoring and alerting will resume.<br>";	
//     body << "</ul>";
//      body << "<br><br>";
//     body << "Thanks,";
//     body << "<br>";
//     body << "IT of App Risk Team";
//     body << "</body></html>";
    
//     return body.str();
// }

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
			EmailMessage alert(subject, body, config.recipients,true);
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
			std::string body = generateRecoveryEmail(alertHistory.logsDuringAlert,alertHistory.logsDuringRecovery);            
            EmailMessage recoveryAlert(subject, body, config.recipients, true);
            queueEmail(recoveryAlert);
			
            
            // Reset the alert history after sending recovery email
            alertHistory.reset();
        }
    }
}

void EmailNotifier::sendImmediateAlert(const std::string& subject, const std::string& message) {
    if (!isEnabled()) return;   
    EmailMessage alert(subject, message, config.recipients);
    queueEmail(alert);
}

bool EmailNotifier::testEmailConfiguration() {
    if (!config.isValid()) {
        std::cerr << "Email configuration is invalid<br>";
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
