#include "include/EmailNotifier.h"
#include "include/Configuration.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>

int main() {
    std::cout << "=== SystemMonitor Email Test with Configuration File ===" << std::endl;
    
    // Load configuration from file
    ConfigurationManager configManager;
    bool loaded = configManager.loadFromFile("config\\SystemMonitor.cfg");
    
    if (!loaded) {
        std::cout << "❌ Failed to load configuration file!" << std::endl;
        return 1;
    }
    
    MonitorConfig config = configManager.getConfig();
    EmailConfig emailConfig = config.getEmailConfig();
    
    std::cout << "Configuration loaded:" << std::endl;
    std::cout << "- Email Enabled: " << (emailConfig.enableEmailAlerts ? "Yes" : "No") << std::endl;
    std::cout << "- SMTP Server: " << emailConfig.smtpServer << std::endl;
    std::cout << "- SMTP Port: " << emailConfig.smtpPort << std::endl;
    std::cout << "- Sender Email: " << emailConfig.senderEmail << std::endl;
    std::cout << "- Recipients: ";
    for (const auto& recipient : emailConfig.recipients) {
        std::cout << recipient << " ";
    }
    std::cout << std::endl;
    std::cout << "- Use TLS: " << (emailConfig.useTLS ? "Yes" : "No") << std::endl;
    
    if (!emailConfig.enableEmailAlerts) {
        std::cout << "Email is disabled in configuration!" << std::endl;
        return 1;
    }
    
    std::cout << "\nCreating email notifier with configuration..." << std::endl;
    EmailNotifier emailNotifier(emailConfig);
    
    std::cout << "Testing email configuration..." << std::endl;
    bool configOk = emailNotifier.testEmailConfiguration();
    
    if (configOk) {
        std::cout << "\n✅ Email configuration test PASSED!" << std::endl;
        std::cout << "📧 Email system is ready to send notifications." << std::endl;
        
        if (emailConfig.smtpServer.find("gmail.com") != std::string::npos) {
            std::cout << "\n📝 NOTE: Gmail SMTP detected." << std::endl;
            std::cout << "   Due to TLS encryption requirements, emails are being" << std::endl;
            std::cout << "   logged to 'email_simulation.log' for demonstration." << std::endl;
            std::cout << "   In production, implement proper TLS/SSL for real Gmail delivery." << std::endl;
        }
        
        // Start the email worker
        emailNotifier.start();
        
        // Send a test alert
        std::cout << "\n📤 Sending test alert email..." << std::endl;
        auto now = std::time(nullptr);
        auto timeStr = std::string(std::ctime(&now));
        
        emailNotifier.sendImmediateAlert(
            "SystemMonitor Test Alert - Configuration Verification", 
            "This is a test alert from SystemMonitor using your configuration.\n\n"
            "Configuration Details:\n"
            "- Sender: " + emailConfig.senderEmail + "\n"
            "- Recipients: " + (emailConfig.recipients.empty() ? "None" : emailConfig.recipients[0]) + "\n"
            "- SMTP Server: " + emailConfig.smtpServer + ":" + std::to_string(emailConfig.smtpPort) + "\n"
            "- TLS Enabled: " + (emailConfig.useTLS ? "Yes" : "No") + "\n"
            "- Timestamp: " + timeStr + "\n"
            "If you receive this email, your configuration is working correctly!\n\n"
            "SystemMonitor Email Notification System"
        );
        
        std::cout << "   Test alert queued. Processing..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Send a recovery test
        std::cout << "\n📤 Sending test recovery email..." << std::endl;
        now = std::time(nullptr);
        timeStr = std::string(std::ctime(&now));
        
        emailNotifier.sendImmediateAlert(
            "SystemMonitor Test Recovery - All Systems Normal",
            "This is a test recovery notification from SystemMonitor.\n\n"
            "System Status: ALL CLEAR ✅\n"
            "- Configuration test completed successfully\n"
            "- Email system operational\n"
            "- All components verified\n"
            "- Timestamp: " + timeStr + "\n"
            "Your SystemMonitor email alerts are configured correctly.\n\n"
            "SystemMonitor Email Notification System"
        );
        
        std::cout << "   Recovery alert queued. Processing..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        emailNotifier.stop();
        std::cout << "\n✅ Email test completed!" << std::endl;
        
    } else {
        std::cout << "\n❌ Email configuration test FAILED!" << std::endl;
        std::cout << "Possible issues:" << std::endl;
        std::cout << "1. SMTP server connection failed" << std::endl;
        std::cout << "2. Authentication credentials invalid" << std::endl;
        std::cout << "3. Gmail App Password may be incorrect" << std::endl;
        std::cout << "4. Network connectivity issues" << std::endl;
    }
    
    std::cout << "\n📋 Check 'email_simulation.log' for detailed email logs." << std::endl;
    std::cout << "💡 For Gmail, ensure you're using an App Password, not your regular password." << std::endl;
    
    return configOk ? 0 : 1;
}
