# Email Alert Configuration Guide

The SystemMonitor now includes advanced email alerting capabilities that notify operators when system resource thresholds are exceeded for a specified duration.

## Features

### Email Alert System
- **Duration-based alerting**: Sends alerts only when thresholds are exceeded for a specified duration (default: 5 minutes)
- **Cooldown period**: Prevents spam by enforcing a cooldown period between alerts (default: 60 minutes)
- **Comprehensive log analysis**: Includes all log entries during the alert period for detailed analysis
- **SMTP support**: Works with standard SMTP servers (Gmail, Outlook, corporate mail servers)
- **Multiple recipients**: Supports sending alerts to multiple email addresses

### Alert Workflow
1. **Threshold monitoring**: Continuously monitors CPU, RAM, and Disk usage against configured thresholds
2. **Duration tracking**: Tracks how long thresholds have been exceeded
3. **Alert generation**: Sends email alert when thresholds are exceeded for the specified duration
4. **Log collection**: Includes all system logs from the alert period for analysis
5. **Cooldown enforcement**: Prevents additional alerts for the configured cooldown period

## Configuration

### Email Settings in SystemMonitor.cfg

```properties
# Email Alert Configuration
EMAIL_ENABLED=true
EMAIL_SMTP_SERVER=smtp.gmail.com
EMAIL_SMTP_PORT=587
EMAIL_SENDER=your-monitoring@company.com
EMAIL_PASSWORD=your-app-password
EMAIL_SENDER_NAME=SystemMonitor
EMAIL_RECIPIENTS=operator1@company.com,operator2@company.com,sysadmin@company.com
EMAIL_USE_TLS=true
EMAIL_USE_SSL=false
EMAIL_TIMEOUT_SECONDS=30
EMAIL_ALERT_DURATION_SECONDS=300
EMAIL_COOLDOWN_MINUTES=60
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `EMAIL_ENABLED` | false | Enable/disable email alerts |
| `EMAIL_SMTP_SERVER` | smtp.gmail.com | SMTP server hostname |
| `EMAIL_SMTP_PORT` | 587 | SMTP server port (587 for TLS, 465 for SSL) |
| `EMAIL_SENDER` | - | Sender email address |
| `EMAIL_PASSWORD` | - | Email password or app-specific password |
| `EMAIL_SENDER_NAME` | SystemMonitor | Display name for sender |
| `EMAIL_RECIPIENTS` | - | Comma-separated list of recipient emails |
| `EMAIL_USE_TLS` | true | Use TLS encryption (recommended) |
| `EMAIL_USE_SSL` | false | Use SSL encryption |
| `EMAIL_TIMEOUT_SECONDS` | 30 | Connection timeout |
| `EMAIL_ALERT_DURATION_SECONDS` | 300 | Duration (5 minutes) thresholds must be exceeded before alerting |
| `EMAIL_COOLDOWN_MINUTES` | 60 | Cooldown period (1 hour) between alerts |

## Email Providers Setup

### Gmail Configuration
1. Enable 2-factor authentication on your Gmail account
2. Generate an App Password:
   - Go to Google Account settings
   - Security → 2-Step Verification → App passwords
   - Generate password for "Mail"
3. Use the app password in `EMAIL_PASSWORD`

```properties
EMAIL_SMTP_SERVER=smtp.gmail.com
EMAIL_SMTP_PORT=587
EMAIL_SENDER=your-email@gmail.com
EMAIL_PASSWORD=your-16-char-app-password
EMAIL_USE_TLS=true
```

### Outlook/Hotmail Configuration
```properties
EMAIL_SMTP_SERVER=smtp-mail.outlook.com
EMAIL_SMTP_PORT=587
EMAIL_SENDER=your-email@outlook.com
EMAIL_PASSWORD=your-password
EMAIL_USE_TLS=true
```

### Corporate Exchange Server
```properties
EMAIL_SMTP_SERVER=mail.yourcompany.com
EMAIL_SMTP_PORT=587
EMAIL_SENDER=monitoring@yourcompany.com
EMAIL_PASSWORD=your-password
EMAIL_USE_TLS=true
```

## Alert Email Format

The email alert includes:

### Header Information
- Alert generation timestamp
- Alert duration threshold
- Number of log entries during alert period

### System Analysis
- Detailed resource usage breakdown
- Process vs system/kernel resource consumption
- CPU, RAM, and Disk I/O analysis

### Process Information
- All active processes during the alert period
- Resource usage for each process
- Process IDs and names

### Recommendations
- Suggested troubleshooting steps
- Performance optimization recommendations

## Usage Examples

### Basic Setup for Production Monitoring
```properties
EMAIL_ENABLED=true
EMAIL_SMTP_SERVER=smtp.company.com
EMAIL_SENDER=monitoring@company.com
EMAIL_RECIPIENTS=ops-team@company.com,sysadmin@company.com
EMAIL_ALERT_DURATION_SECONDS=300  # 5-minute threshold
EMAIL_COOLDOWN_MINUTES=60         # 1-hour cooldown
```

### High-Sensitivity Development Environment
```properties
EMAIL_ENABLED=true
EMAIL_ALERT_DURATION_SECONDS=120  # 2-minute threshold
EMAIL_COOLDOWN_MINUTES=30         # 30-minute cooldown
EMAIL_RECIPIENTS=dev-team@company.com
```

### Low-Frequency Long-Term Monitoring
```properties
EMAIL_ENABLED=true
EMAIL_ALERT_DURATION_SECONDS=600  # 10-minute threshold
EMAIL_COOLDOWN_MINUTES=240        # 4-hour cooldown
```

## Testing Email Configuration

The SystemMonitor automatically tests email configuration on startup:
- Tests SMTP connection
- Validates authentication
- Reports configuration status in console output

## Security Considerations

1. **App Passwords**: Use app-specific passwords instead of main account passwords
2. **TLS Encryption**: Always enable TLS for secure email transmission
3. **Restricted Access**: Limit email account permissions to sending only
4. **Password Security**: Store email passwords securely and rotate regularly

## Troubleshooting

### Common Issues

**Authentication Failed**
- Verify email and password are correct
- Check if 2FA is enabled and app password is required
- Ensure SMTP server and port are correct

**Connection Timeout**
- Check network connectivity
- Verify firewall allows SMTP traffic
- Try different SMTP ports (587, 465, 25)

**No Alerts Received**
- Check if thresholds are actually being exceeded for the required duration
- Verify cooldown period hasn't suppressed alerts
- Check spam/junk folders

**TLS Errors**
- Some environments may need TLS disabled for testing
- Try EMAIL_USE_TLS=false for troubleshooting

### Debug Mode
Enable debug mode to see detailed email processing information:
```properties
DEBUG_MODE=true
```

## Performance Impact

The email notification system is designed for minimal performance impact:
- **Asynchronous processing**: Email sending doesn't block monitoring
- **Background worker**: Dedicated thread handles email queue
- **Efficient alerting**: Only sends emails when necessary
- **Resource monitoring**: Email system itself is lightweight

## Integration with Existing Systems

The email alerts can be integrated with:
- **Incident management systems**: Parse emails for automatic ticket creation
- **Monitoring dashboards**: Use email notifications as triggers
- **Log aggregation**: Forward email alerts to centralized logging
- **Alerting platforms**: Chain with PagerDuty, Slack, or other notification systems
