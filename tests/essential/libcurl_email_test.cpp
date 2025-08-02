#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

// Include libcurl for TLS email support
#ifdef _WIN32
#include <curl/curl.h>
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "normaliz.lib")
#endif

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

bool sendEmailWithLibcurl(const std::string& to, const std::string& subject, const std::string& body) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "❌ Failed to initialize libcurl" << std::endl;
        return false;
    }
    
    // Gmail configuration
    std::string gmail_server = "smtp.gmail.com";
    std::string gmail_user = "dnguyenminh@gmail.com";
    std::string gmail_password = "hvdcnfzfkfowfkgo";
    
    // Create email content
    EmailPayload emailData;
    std::ostringstream emailContent;
    
    emailContent << "To: " << to << "\r\n";
    emailContent << "From: System Monitor <" << gmail_user << ">\r\n";
    emailContent << "Subject: " << subject << "\r\n";
    emailContent << "Content-Type: text/plain; charset=UTF-8\r\n";
    emailContent << "\r\n";
    emailContent << body << "\r\n";
    
    emailData.content = emailContent.str();
    
    std::cout << "📧 Email Content Preview:\n" << std::string(50, '=') << std::endl;
    std::cout << emailData.content << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    // Gmail SMTP URL with SSL (port 465 for SSL)
    std::string smtpUrl = "smtps://" + gmail_server + ":465";
    
    // Set CURL options for Gmail with TLS
    curl_easy_setopt(curl, CURLOPT_URL, smtpUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_USERNAME, gmail_user.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, gmail_password.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    // Recipients list
    struct curl_slist* recipients = nullptr;
    recipients = curl_slist_append(recipients, to.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    
    // Sender
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, gmail_user.c_str());
    
    // Email content
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &emailData);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    
    // Enable verbose output for debugging
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    
    std::cout << "🔐 Connecting to Gmail SMTP with TLS..." << std::endl;
    
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

bool testGmailConnection() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "❌ Failed to initialize libcurl" << std::endl;
        return false;
    }

    std::string gmail_server = "smtp.gmail.com";
    std::string gmail_user = "dnguyenminh@gmail.com";
    std::string gmail_password = "hvdcnfzfkfowfkgo";
    
    // Set up SMTP URL
    std::string smtpUrl = "smtps://" + gmail_server + ":465";
    curl_easy_setopt(curl, CURLOPT_URL, smtpUrl.c_str());
    
    // Set credentials
    curl_easy_setopt(curl, CURLOPT_USERNAME, gmail_user.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, gmail_password.c_str());
    
    // Enable TLS/SSL
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    // Set timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    // Just test connection (no actual email)
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
    
    std::cout << "🔐 Testing Gmail SMTP TLS connection..." << std::endl;
    
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

int main() {
    std::cout << "🔧 SystemMonitor libcurl TLS Email Integration Test" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Initialize libcurl globally
    curl_global_init(CURL_GLOBAL_DEFAULT);
    std::cout << "✅ libcurl TLS support initialized" << std::endl;
    
    // Test 1: Connection Test
    std::cout << "\n📡 Test 1: Gmail TLS Connection Test" << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    if (!testGmailConnection()) {
        std::cerr << "❌ Connection test failed - aborting email test" << std::endl;
        curl_global_cleanup();
        return 1;
    }
    
    // Test 2: Send Real Email
    std::cout << "\n📧 Test 2: Sending Real Email via libcurl TLS" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    std::string to = "layland.ernst@freedrops.org";
    std::string subject = "SystemMonitor libcurl TLS Integration Test";
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream body;
    body << "SYSTEMMONITOR LIBCURL TLS INTEGRATION TEST\n";
    body << "==========================================\n\n";
    body << "Test Timestamp: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n";
    body << "Integration: libcurl with TLS/SSL support\n";
    body << "SMTP Server: Gmail (smtp.gmail.com:465)\n";
    body << "Encryption: TLS/SSL (CURLUSESSL_ALL)\n\n";
    body << "✅ libcurl successfully integrated into SystemMonitor\n";
    body << "✅ Gmail simulation mode replaced with real TLS email\n";
    body << "✅ Email notifications now use secure SMTP with authentication\n\n";
    body << "TECHNICAL DETAILS:\n";
    body << "- Compiler: Visual Studio 2022 x64\n";
    body << "- libcurl: Installed via vcpkg\n";
    body << "- TLS Support: Full SSL/TLS encryption\n";
    body << "- Authentication: App Password\n\n";
    body << "SystemMonitor email alerts are now fully functional with TLS security!\n\n";
    body << "This message was sent automatically by the SystemMonitor libcurl integration test.\n";
    
    bool emailSent = sendEmailWithLibcurl(to, subject, body.str());
    
    if (emailSent) {
        std::cout << "\n🎉 SUCCESS: libcurl TLS email integration working!" << std::endl;
        std::cout << "📫 Email delivered to: " << to << std::endl;
    } else {
        std::cout << "\n❌ FAILED: libcurl TLS email integration failed!" << std::endl;
    }
    
    // Cleanup libcurl
    curl_global_cleanup();
    
    std::cout << "\n🔧 Test completed. SystemMonitor now uses libcurl TLS for real email delivery!" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    return emailSent ? 0 : 1;
}
