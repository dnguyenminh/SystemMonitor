# SystemMonitor Technical Documentation

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Class Diagrams](#class-diagrams)
4. [Activity Diagrams](#activity-diagrams)
5. [Sequence Diagrams](#sequence-diagrams)
6. [State Diagrams](#state-diagrams)
7. [Component Interactions](#component-interactions)
7. [Data Flow](#data-flow)
8. [Configuration Management](#configuration-management)
9. [Threading Model](#threading-model)
10. [Error Handling](#error-handling)

## Overview

SystemMonitor is a professional C++ Windows system monitoring application designed with object-oriented principles and enterprise-grade architecture. The application provides real-time system monitoring, email alerting, and multiple display modes for different operational scenarios.

### Key Features
- Real-time CPU, RAM, and Disk monitoring
- Process-level resource tracking with aggregation
- TLS-encrypted email notifications via libcurl
- Multiple display modes (line, top, compact, silence)
- Asynchronous logging with rotation
- Comprehensive configuration management
- Professional email alerting with recovery notifications

## Architecture

The application follows a layered architecture pattern with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                    Presentation Layer                       │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │  Console UI     │  │  Display Modes  │  │  Key Handler │ │
│  │  (main.cpp)     │  │  (Line/Top/     │  │              │ │
│  │                 │  │  Compact/Silent)│  │              │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                     Business Layer                          │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │ SystemMonitor   │  │ ProcessManager  │  │ EmailNotifier│ │
│  │                 │  │                 │  │              │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                   Infrastructure Layer                      │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │  Configuration  │  │     Logger      │  │ SystemMetrics│ │
│  │   Management    │  │   (Async)       │  │  (Windows)   │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Class Diagrams

### Core Architecture Class Diagram

```mermaid
classDiagram
    class SystemMonitorApplication {
        -configManager: unique_ptr<ConfigurationManager>
        -systemMonitor: shared_ptr<ISystemMonitor>
        -processManager: unique_ptr<IProcessManager>
        -logger: unique_ptr<ILogger>
        -emailNotifier: unique_ptr<EmailNotifier>
        -isRunning: bool
        -displayMode: int
        +initialize(argc: int, argv: char*[]): bool
        +run(): void
        +shutdown(): void
        -checkAdministratorPrivileges(): bool
        -showTopStyleDisplay(): void
        -showCompactDisplay(): void
        -handleKeyPress(): void
    }

    class ISystemMonitor {
        <<interface>>
        +initialize(): bool
        +getSystemUsage(): SystemUsage
        +shutdown(): void
    }

    class SystemMonitor {
        -metrics: unique_ptr<ISystemMetrics>
        +initialize(): bool
        +getSystemUsage(): SystemUsage
        +shutdown(): void
    }

    class IProcessManager {
        <<interface>>
        +initialize(): bool
        +getAllProcesses(): vector<ProcessInfo>
        +getAggregatedProcessTree(): vector<ProcessInfo>
        +shutdown(): void
    }

    class ProcessManager {
        -systemMonitor: shared_ptr<ISystemMonitor>
        +initialize(): bool
        +getAllProcesses(): vector<ProcessInfo>
        +getAggregatedProcessTree(): vector<ProcessInfo>
        +shutdown(): void
    }

    class ConfigurationManager {
        -config: MonitorConfig
        +loadFromFile(filename: string): bool
        +saveToFile(filename: string): bool
        +parseCommandLine(argc: int, argv: char*[]): bool
        +getConfig(): MonitorConfig&
        +printUsage(): void
    }

    class EmailNotifier {
        -config: EmailConfig
        -alertState: AlertState
        -lastAlertTime: time_point
        +checkThresholds(exceeded: bool, logEntry: string): void
        +sendEmailWithLibcurl(): bool
        +testEmailConfiguration(): bool
        +start(): void
        +stop(): void
    }

    class ILogger {
        <<interface>>
        +initialize(): bool
        +log(level: LogLevel, message: string): void
        +logProcesses(processes: vector<ProcessInfo>, usage: SystemUsage): void
        +shutdown(): void
    }

    class AsyncFileLogger {
        -logConfig: LogConfig
        -messageQueue: queue<LogMessage>
        -workerThread: thread
        -isRunning: atomic<bool>
        +initialize(): bool
        +log(level: LogLevel, message: string): void
        +logProcesses(): void
        +shutdown(): void
        -workerLoop(): void
        -rotateLogFile(): void
    }

    SystemMonitorApplication --> ISystemMonitor
    SystemMonitorApplication --> IProcessManager
    SystemMonitorApplication --> ConfigurationManager
    SystemMonitorApplication --> EmailNotifier
    SystemMonitorApplication --> ILogger
    ISystemMonitor <|-- SystemMonitor
    IProcessManager <|-- ProcessManager
    ILogger <|-- AsyncFileLogger
    ProcessManager --> ISystemMonitor
```

### Configuration Management Class Diagram

```mermaid
classDiagram
    class BaseConfig {
        #cpuThreshold: double
        #ramThreshold: double
        #diskThreshold: double
        #monitorInterval: int
        #debugMode: bool
        #displayMode: DisplayModeConfig
        +getCpuThreshold(): double
        +setCpuThreshold(value: double): void
        +validate(): bool
        +setDefaults(): void
    }

    class MonitorConfig {
        -logFilePath: string
        -logConfig: LogConfig
        -emailConfig: EmailConfig
        +getLogConfig(): LogConfig&
        +getEmailConfig(): EmailConfig&
        +setLogFilePath(path: string): void
    }

    class LogConfig {
        -logPath: string
        -maxFileSizeMB: int
        -maxBackupFiles: int
        -rotationEnabled: bool
        -rotationStrategy: LogRotationStrategy
        -dateFrequency: DateRotationFrequency
        +setMaxFileSizeMB(size: int): void
        +setRotationEnabled(enabled: bool): void
    }

    class EmailConfig {
        +enableEmailAlerts: bool
        +smtpServer: string
        +smtpPort: int
        +senderEmail: string
        +senderPassword: string
        +recipients: vector<string>
        +useTLS: bool
        +useSSL: bool
        +alertDurationSeconds: int
        +cooldownMinutes: int
    }

    class DisplayModeConfig {
        <<enumeration>>
        LINE_BY_LINE
        TOP_STYLE
        COMPACT
        SILENCE
    }

    BaseConfig <|-- MonitorConfig
    MonitorConfig --> LogConfig
    MonitorConfig --> EmailConfig
    BaseConfig --> DisplayModeConfig
```

### Data Models Class Diagram

```mermaid
classDiagram
    class SystemUsage {
        -cpuPercent: double
        -ramPercent: double
        -diskPercent: double
        +SystemUsage(cpu: double, ram: double, disk: double)
        +getCpuPercent(): double
        +getRamPercent(): double
        +getDiskPercent(): double
    }

    class ProcessInfo {
        -name: string
        -pid: DWORD
        -cpuPercent: double
        -ramPercent: double
        -diskPercent: double
        -parentPid: DWORD
        +ProcessInfo(name: string, pid: DWORD)
        +getName(): string
        +getPid(): DWORD
        +getCpuPercent(): double
        +setCpuPercent(percent: double): void
        +addChildUsage(child: ProcessInfo): void
    }

    class LogMessage {
        -timestamp: time_point
        -level: LogLevel
        -message: string
        -threadId: string
        +LogMessage(level: LogLevel, msg: string)
        +getTimestamp(): time_point
        +getLevel(): LogLevel
        +getMessage(): string
    }

    class AlertState {
        <<enumeration>>
        NORMAL
        ALERTING
        COOLDOWN
    }

    ProcessInfo --> ProcessInfo : children
```

## Activity Diagrams

### Application Startup Activity Diagram

```mermaid
flowchart TD
    A[Start] --> B[Parse Command Line Arguments]
    B --> C{Help Flag?}
    C -->|Yes| D[Print Usage]
    C -->|No| E[Load Configuration File]
    D --> Z[Exit]
    E --> F[Validate Configuration]
    F --> G{Valid Config?}
    G -->|No| H[Print Error]
    G -->|Yes| I[Initialize System Monitor]
    H --> Z
    I --> J{Init Success?}
    J -->|No| K[Print Error]
    J -->|Yes| L[Initialize Process Manager]
    K --> Z
    L --> M{Init Success?}
    M -->|No| N[Print Error]
    M -->|Yes| O[Initialize Logger]
    N --> Z
    O --> P{Init Success?}
    P -->|No| Q[Print Error]
    P -->|Yes| R[Initialize Email Notifier]
    Q --> Z
    R --> S{Email Enabled?}
    S -->|Yes| T[Test Email Configuration]
    S -->|No| U[Initialize Display]
    T --> V{Email Test OK?}
    V -->|Yes| W[Start Email Service]
    V -->|No| X[Disable Email]
    W --> U
    X --> U
    U --> Y[Enter Main Loop]
    Y --> AA[Application Running]
```

### Main Monitoring Loop Activity Diagram

```mermaid
flowchart TD
    A[Main Loop Start] --> B[Check for Key Press]
    B --> C{Key Pressed?}
    C -->|Yes| D[Handle Key Input]
    C -->|No| E[Get System Usage]
    D --> F{Quit Key?}
    F -->|Yes| G[Shutdown Application]
    F -->|No| H{Toggle Key?}
    H -->|Yes| I[Change Display Mode]
    H -->|No| E
    I --> E
    E --> J[Get All Processes]
    J --> K[Calculate Disk I/O]
    K --> L[Aggregate Process Tree]
    L --> M[Check Display Mode]
    M --> N{Mode Type?}
    N -->|Top| O[Show Top Display]
    N -->|Compact| P[Show Compact Display]
    N -->|Line| Q[Show Line Output]
    N -->|Silence| R{Thresholds Exceeded?}
    O --> S[Check Thresholds]
    P --> S
    Q --> S
    R -->|Yes| T[Show Alert Output]
    R -->|No| S
    T --> S
    S --> U{Thresholds Exceeded?}
    U -->|Yes| V[Log Active Processes]
    U -->|No| W[Sleep for Interval]
    V --> X[Send Email Alert]
    X --> W
    W --> Y{Continue Running?}
    Y -->|Yes| A
    Y -->|No| G
```

### Email Alert Processing Activity Diagram

```mermaid
flowchart TD
    A[Threshold Exceeded] --> B[Check Alert State]
    B --> C{Current State?}
    C -->|NORMAL| D[Start Alert Timer]
    C -->|ALERTING| E[Check Alert Duration]
    C -->|COOLDOWN| F[Ignore Alert]
    D --> G[Set State to ALERTING]
    G --> H[Generate Detailed Log]
    H --> I[Send Alert Email]
    I --> J{Email Sent?}
    J -->|Yes| K[Log Success]
    J -->|No| L[Log Error]
    K --> M[Continue Monitoring]
    L --> M
    E --> N{Duration Exceeded?}
    N -->|Yes| O[Set State to COOLDOWN]
    N -->|No| P[Update Alert Log]
    O --> Q[Start Cooldown Timer]
    Q --> M
    P --> I
    F --> R[Check Cooldown Timer]
    R --> S{Cooldown Expired?}
    S -->|Yes| T[Reset to NORMAL]
    S -->|No| M
    T --> M
    M --> U[Threshold Resolved?]
    U -->|Yes| V{Recovery Alerts Enabled?}
    U -->|No| W[End]
    V -->|Yes| X[Send Recovery Email]
    V -->|No| Y[Reset State to NORMAL]
    X --> Y
    Y --> W
```

## Sequence Diagrams

### Application Initialization Sequence

```mermaid
sequenceDiagram
    participant Main as main()
    participant App as SystemMonitorApplication
    participant Config as ConfigurationManager
    participant SysMon as SystemMonitor
    participant ProcMgr as ProcessManager
    participant Logger as AsyncFileLogger
    participant Email as EmailNotifier

    Main->>App: create()
    Main->>App: initialize(argc, argv)
    
    App->>Config: parseCommandLine(argc, argv)
    Config-->>App: success/failure
    
    App->>Config: loadFromFile("config/SystemMonitor.cfg")
    Config-->>App: configuration loaded
    
    App->>Config: validateConfiguration()
    Config-->>App: validation result
    
    App->>SysMon: create()
    App->>SysMon: initialize()
    SysMon-->>App: initialization result
    
    App->>ProcMgr: create(systemMonitor)
    App->>ProcMgr: initialize()
    ProcMgr-->>App: initialization result
    
    App->>Logger: create(logConfig)
    App->>Logger: initialize()
    Logger->>Logger: startWorkerThread()
    Logger-->>App: initialization result
    
    App->>Email: create(emailConfig)
    App->>Email: testEmailConfiguration()
    Email-->>App: test result
    
    alt Email test successful
        App->>Email: start()
    else Email test failed
        App->>App: disable email notifications
    end
    
    App->>App: initializeDisplay()
    App-->>Main: initialization complete
```

### Monitoring Cycle Sequence

```mermaid
sequenceDiagram
    participant App as SystemMonitorApplication
    participant SysMon as SystemMonitor
    participant ProcMgr as ProcessManager
    participant Logger as AsyncFileLogger
    participant Email as EmailNotifier
    participant Display as DisplaySystem

    loop Main Monitoring Loop
        App->>App: checkForKeyPress()
        
        App->>SysMon: getSystemUsage()
        SysMon-->>App: SystemUsage
        
        App->>ProcMgr: getAllProcesses()
        ProcMgr-->>App: vector<ProcessInfo>
        
        App->>ProcMgr: getAggregatedProcessTree(processes)
        ProcMgr-->>App: aggregated processes
        
        App->>Display: showDisplay(mode, processes, usage)
        Display-->>App: display updated
        
        alt Thresholds Exceeded
            App->>Logger: logProcesses(processes, usage)
            Logger->>Logger: queueLogMessage()
            
            App->>Email: checkThresholds(true, detailedLog)
            Email->>Email: processAlert()
            
            alt Alert Conditions Met
                Email->>Email: sendEmailWithLibcurl()
                Email-->>App: email sent
            end
        else Normal Operation
            App->>Email: checkThresholds(false, "")
            Email->>Email: processNormalState()
        end
        
        App->>App: Sleep(monitorInterval)
    end
```

### Email Alert Sequence

```mermaid
sequenceDiagram
    participant App as SystemMonitorApplication
    participant Email as EmailNotifier
    participant CURL as libcurl
    participant SMTP as Gmail SMTP Server

    App->>Email: checkThresholds(true, logEntry)
    Email->>Email: checkAlertState()
    
    alt State is NORMAL
        Email->>Email: setState(ALERTING)
        Email->>Email: generateEmailContent(logEntry)
        Email->>CURL: curl_easy_init()
        CURL-->>Email: curl handle
        
        Email->>CURL: curl_easy_setopt(CURLOPT_URL, "smtps://smtp.gmail.com:465")
        Email->>CURL: curl_easy_setopt(CURLOPT_USE_SSL, CURLUSESSL_ALL)
        Email->>CURL: curl_easy_setopt(CURLOPT_USERNAME, sender)
        Email->>CURL: curl_easy_setopt(CURLOPT_PASSWORD, appPassword)
        Email->>CURL: curl_easy_setopt(CURLOPT_MAIL_FROM, sender)
        Email->>CURL: curl_easy_setopt(CURLOPT_MAIL_RCPT, recipients)
        Email->>CURL: curl_easy_setopt(CURLOPT_READDATA, emailPayload)
        
        Email->>CURL: curl_easy_perform()
        CURL->>SMTP: SMTP connection with TLS
        SMTP-->>CURL: connection established
        CURL->>SMTP: send email data
        SMTP-->>CURL: email accepted
        CURL-->>Email: CURLE_OK
        
        Email->>CURL: curl_easy_cleanup()
        Email->>Email: logEmailSuccess()
        
    else State is ALERTING
        Email->>Email: checkAlertDuration()
        alt Duration not exceeded
            Email->>Email: updateAlertLog()
        else Duration exceeded
            Email->>Email: setState(COOLDOWN)
        end
        
    else State is COOLDOWN
        Email->>Email: checkCooldownTimer()
        alt Cooldown expired
            Email->>Email: setState(NORMAL)
        end
    end
    
    Email-->>App: alert processing complete
```

### Asynchronous Logging Sequence

```mermaid
sequenceDiagram
    participant App as Application Thread
    participant Logger as AsyncFileLogger
    participant Queue as Message Queue
    participant Worker as Worker Thread
    participant File as Log File

    App->>Logger: logProcesses(processes, usage)
    Logger->>Logger: formatLogMessage()
    Logger->>Queue: push(logMessage)
    Logger-->>App: message queued
    
    par Async Processing
        Worker->>Queue: pop() [blocking]
        Queue-->>Worker: logMessage
        Worker->>Worker: formatWithTimestamp()
        Worker->>File: write(formattedMessage)
        Worker->>Worker: checkRotationNeeded()
        
        alt Rotation needed
            Worker->>File: close()
            Worker->>Worker: renameToBackup()
            Worker->>File: open(newFile)
        end
    end
    
    Note over Worker: Continuous loop processing queue
```

## State Diagrams

### Application State Machine

```mermaid
stateDiagram-v2
    [*] --> Initializing
    Initializing --> ConfigLoading : Start
    ConfigLoading --> Validating : Config Loaded
    ConfigLoading --> Error : Load Failed
    Validating --> ComponentInit : Valid Config
    Validating --> Error : Invalid Config
    ComponentInit --> Ready : All Components OK
    ComponentInit --> Error : Component Failed
    Ready --> Running : Enter Main Loop
    Running --> Processing : Monitor Cycle
    Processing --> Displaying : Data Collected
    Displaying --> Checking : Display Updated
    Checking --> Alerting : Thresholds Exceeded
    Checking --> Processing : Normal Operation
    Alerting --> Emailing : Alert Triggered
    Alerting --> Processing : Alert Handled
    Emailing --> Processing : Email Sent
    Processing --> KeyCheck : Cycle Complete
    KeyCheck --> ModeChange : Toggle Key
    KeyCheck --> Shutdown : Quit Key
    KeyCheck --> Processing : No Input
    ModeChange --> Processing : Mode Changed
    Shutdown --> Cleanup : Shutting Down
    Cleanup --> [*] : Exit
    Error --> [*] : Fatal Error
    
    state Initializing {
        [*] --> ParseArgs
        ParseArgs --> CheckHelp
        CheckHelp --> [*]
    }
    
    state Running {
        [*] --> MonitorLoop
        MonitorLoop --> Sleep
        Sleep --> MonitorLoop
    }
```

### Email Alert State Machine

```mermaid
stateDiagram-v2
    [*] --> NORMAL
    NORMAL --> ALERTING : Threshold Exceeded
    ALERTING --> ALERTING : Duration Not Exceeded
    ALERTING --> COOLDOWN : Alert Duration Reached
    COOLDOWN --> NORMAL : Cooldown Expired
    COOLDOWN --> COOLDOWN : Still in Cooldown
    NORMAL --> RECOVERY : Thresholds Normalized
    RECOVERY --> NORMAL : Recovery Email Sent
    
    state NORMAL {
        [*] --> Monitoring
        Monitoring --> Ready : No Alerts
    }
    
    state ALERTING {
        [*] --> SendingAlert
        SendingAlert --> WaitingDuration : Email Sent
        WaitingDuration --> CheckingDuration : Time Check
        CheckingDuration --> WaitingDuration : Continue Alert
        CheckingDuration --> [*] : Duration Exceeded
    }
    
    state COOLDOWN {
        [*] --> WaitingCooldown
        WaitingCooldown --> CheckingCooldown : Time Check
        CheckingCooldown --> WaitingCooldown : Continue Cooldown
        CheckingCooldown --> [*] : Cooldown Complete
    }
    
    state RECOVERY {
        [*] --> SendingRecovery
        SendingRecovery --> [*] : Recovery Email Sent
    }
```

### Display Mode State Machine

```mermaid
stateDiagram-v2
    [*] --> LINE_BY_LINE
    LINE_BY_LINE --> TOP_STYLE : Toggle Key ('t')
    TOP_STYLE --> COMPACT : Toggle Key ('t')
    COMPACT --> SILENCE : Toggle Key ('t')
    SILENCE --> LINE_BY_LINE : Toggle Key ('t')
    
    state LINE_BY_LINE {
        [*] --> ScrollingOutput
        ScrollingOutput --> PrintingStatus : Monitor Cycle
        PrintingStatus --> ScrollingOutput : Status Printed
        ScrollingOutput --> [*] : Mode Change
    }
    
    state TOP_STYLE {
        [*] --> FullScreen
        FullScreen --> ClearingScreen : Update Display
        ClearingScreen --> DrawingHeader : Screen Cleared
        DrawingHeader --> DrawingTable : Header Complete
        DrawingTable --> DrawingControls : Table Complete
        DrawingControls --> FullScreen : Controls Drawn
        FullScreen --> [*] : Mode Change
    }
    
    state COMPACT {
        [*] --> CompactView
        CompactView --> ShowingSummary : Update Display
        ShowingSummary --> ShowingTopProcesses : Summary Complete
        ShowingTopProcesses --> ShowingResourceSplit : Processes Shown
        ShowingResourceSplit --> CompactView : Split Shown
        CompactView --> [*] : Mode Change
    }
    
    state SILENCE {
        [*] --> BackgroundMode
        BackgroundMode --> CheckingThresholds : Monitor Cycle
        CheckingThresholds --> SilentOperation : Normal
        CheckingThresholds --> AlertOutput : Thresholds Exceeded
        SilentOperation --> BackgroundMode : Continue Silent
        AlertOutput --> BackgroundMode : Alert Shown
        BackgroundMode --> [*] : Mode Change
    }
```

### Logger State Machine

```mermaid
stateDiagram-v2
    [*] --> Initializing
    Initializing --> Ready : Worker Thread Started
    Initializing --> Failed : Initialization Error
    Ready --> Processing : Message Received
    Processing --> Writing : Message Formatted
    Writing --> CheckRotation : Data Written
    CheckRotation --> Rotating : Rotation Needed
    CheckRotation --> Ready : No Rotation
    Rotating --> Creating : Old File Renamed
    Creating --> Ready : New File Created
    Ready --> Shutting : Shutdown Signal
    Shutting --> Flushing : Processing Queue
    Flushing --> Stopped : Queue Empty
    Failed --> [*] : Error State
    Stopped --> [*] : Clean Shutdown
    
    state Processing {
        [*] --> QueueCheck
        QueueCheck --> MessagePop : Message Available
        QueueCheck --> Waiting : Queue Empty
        MessagePop --> Formatting : Message Retrieved
        Formatting --> [*] : Message Ready
        Waiting --> QueueCheck : Timeout/Signal
    }
    
    state Rotating {
        [*] --> ClosingCurrent
        ClosingCurrent --> RenamingFiles : File Closed
        RenamingFiles --> CreatingNew : Backup Created
        CreatingNew --> [*] : New File Ready
    }
```

### System Monitor Component States

```mermaid
stateDiagram-v2
    [*] --> Uninitialized
    Uninitialized --> Initializing : initialize() called
    Initializing --> Ready : Success
    Initializing --> Error : Failed
    Ready --> Collecting : getSystemUsage() called
    Collecting --> Processing : Data Retrieved
    Processing --> Ready : Usage Calculated
    Ready --> Shutting : shutdown() called
    Shutting --> [*] : Cleanup Complete
    Error --> [*] : Error State
    
    state Collecting {
        [*] --> CPUQuery
        CPUQuery --> RAMQuery : CPU Data Retrieved
        RAMQuery --> DiskQuery : RAM Data Retrieved
        DiskQuery --> [*] : All Data Collected
    }
    
    state Processing {
        [*] --> Calculating
        Calculating --> Validating : Calculations Done
        Validating --> [*] : Data Valid
    }
```

### Process Manager State Machine

```mermaid
stateDiagram-v2
    [*] --> Uninitialized
    Uninitialized --> Initializing : initialize() called
    Initializing --> Ready : Success
    Initializing --> Error : Failed
    Ready --> Enumerating : getAllProcesses() called
    Enumerating --> Collecting : Process List Retrieved
    Collecting --> Aggregating : Resource Data Collected
    Aggregating --> Ready : Tree Structure Built
    Ready --> Shutting : shutdown() called
    Shutting --> [*] : Cleanup Complete
    Error --> [*] : Error State
    
    state Enumerating {
        [*] --> ProcessSnapshot
        ProcessSnapshot --> ProcessIteration : Snapshot Created
        ProcessIteration --> ProcessIteration : More Processes
        ProcessIteration --> [*] : All Processes Found
    }
    
    state Collecting {
        [*] --> CPUUsage
        CPUUsage --> MemoryUsage : CPU Calculated
        MemoryUsage --> DiskUsage : Memory Calculated
        DiskUsage --> [*] : All Usage Calculated
    }
    
    state Aggregating {
        [*] --> ParentChild
        ParentChild --> ResourceSum : Relationships Built
        ResourceSum --> [*] : Aggregation Complete
    }
```

### Configuration Loading State Machine

```mermaid
stateDiagram-v2
    [*] --> Starting
    Starting --> DefaultsSet : Setting Defaults
    DefaultsSet --> FileLoading : Defaults Applied
    FileLoading --> FileLoaded : File Found & Parsed
    FileLoading --> CommandLineParsing : File Not Found/Error
    FileLoaded --> CommandLineParsing : File Processing Complete
    CommandLineParsing --> Validating : Arguments Processed
    Validating --> Ready : Configuration Valid
    Validating --> Error : Validation Failed
    Ready --> [*] : Configuration Complete
    Error --> [*] : Fatal Configuration Error
    
    state FileLoading {
        [*] --> CheckingFile
        CheckingFile --> ReadingFile : File Exists
        CheckingFile --> [*] : File Missing
        ReadingFile --> ParsingConfig : File Read
        ParsingConfig --> [*] : Config Parsed
    }
    
    state CommandLineParsing {
        [*] --> ArgumentLoop
        ArgumentLoop --> ProcessingArg : Next Argument
        ProcessingArg --> ValidatingArg : Argument Parsed
        ValidatingArg --> ArgumentLoop : Valid Argument
        ValidatingArg --> ArgumentLoop : Invalid Argument
        ArgumentLoop --> [*] : All Arguments Processed
    }
    
    state Validating {
        [*] --> ThresholdCheck
        ThresholdCheck --> IntervalCheck : Thresholds Valid
        IntervalCheck --> PathCheck : Interval Valid
        PathCheck --> [*] : All Validation Complete
    }
```

## Component Interactions

### Display Mode Architecture

The application supports four display modes, each optimized for different use cases:

1. **Line Mode**: Traditional scrolling output
2. **Top Mode**: Interactive full-screen display (like Linux htop)
3. **Compact Mode**: Space-efficient table view
4. **Silence Mode**: Background monitoring with alerts only

```mermaid
stateDiagram-v2
    [*] --> LineMode
    LineMode --> TopMode : 't' key
    TopMode --> CompactMode : 't' key
    CompactMode --> SilenceMode : 't' key
    SilenceMode --> LineMode : 't' key
    
    LineMode : Continuous scrolling output
    LineMode : Shows every monitoring cycle
    LineMode : Minimal resource usage
    
    TopMode : Full-screen interface
    TopMode : Real-time updates every 2s
    TopMode : Process table with sorting
    
    CompactMode : Space-efficient view
    CompactMode : Resource consumption summary
    CompactMode : Top consumers only
    
    SilenceMode : Background operation
    SilenceMode : Output only on threshold breach
    SilenceMode : Minimal resource usage
```

### Threading Model

```mermaid
graph TD
    A[Main Thread] --> B[UI/Display Management]
    A --> C[System Monitoring]
    A --> D[Process Enumeration]
    A --> E[Email Alert Management]
    
    F[Worker Thread] --> G[Async Logging]
    F --> H[Log Rotation]
    F --> I[File I/O Operations]
    
    J[Email Thread Pool] --> K[SMTP Connections]
    J --> L[TLS Handshakes]
    J --> M[Email Transmission]
    
    A -.->|Queue Messages| F
    A -.->|Email Requests| J
    
    style A fill:#e1f5fe
    style F fill:#f3e5f5
    style J fill:#e8f5e8
```

## Data Flow

### System Metrics Collection Flow

```mermaid
flowchart LR
    A[Windows APIs] --> B[SystemMetrics]
    B --> C[SystemMonitor]
    C --> D[SystemUsage Object]
    
    E[Process Enumeration] --> F[ProcessManager]
    F --> G[Process Tree Building]
    G --> H[Resource Aggregation]
    H --> I[ProcessInfo Collection]
    
    D --> J[Main Application]
    I --> J
    J --> K[Display Formatting]
    J --> L[Threshold Checking]
    J --> M[Logging]
    
    L --> N{Thresholds Exceeded?}
    N -->|Yes| O[Email Alert]
    N -->|No| P[Continue Monitoring]
    
    M --> Q[Async Log Queue]
    Q --> R[Worker Thread]
    R --> S[Log File]
```

### Configuration Loading Flow

```mermaid
flowchart TD
    A[Application Start] --> B[Command Line Parsing]
    B --> C[Default Values]
    C --> D[Config File Loading]
    D --> E[Command Line Override]
    E --> F[Validation]
    F --> G{Valid?}
    G -->|Yes| H[Configuration Ready]
    G -->|No| I[Error & Exit]
    
    H --> J[System Initialization]
    J --> K[Runtime Configuration]
    K --> L[Save Updated Config]
```

## Configuration Management

### Configuration Hierarchy

1. **Default Values** (lowest priority)
2. **Configuration File** (`config/SystemMonitor.cfg`)
3. **Command Line Arguments** (highest priority)

### Configuration Loading Process

```cpp
// Pseudo-code for configuration loading
class ConfigurationManager {
    bool initialize(int argc, char* argv[]) {
        // Step 1: Set defaults
        config.setDefaults();
        
        // Step 2: Load from file (if exists)
        if (fileExists("config/SystemMonitor.cfg")) {
            loadFromFile("config/SystemMonitor.cfg");
        }
        
        // Step 3: Override with command line
        parseCommandLine(argc, argv);
        
        // Step 4: Validate final configuration
        return config.validate();
    }
};
```

## Threading Model

### Main Thread Responsibilities
- User interface management
- Keyboard input handling
- System metrics collection
- Process enumeration
- Email alert coordination
- Display mode management

### Worker Thread (AsyncFileLogger)
- Message queue processing
- File I/O operations
- Log rotation management
- Thread-safe logging operations

### Thread Safety Mechanisms
- Atomic variables for thread coordination
- Message queues for async communication
- RAII for resource management
- Proper shutdown synchronization

## Error Handling

### Error Handling Strategy

1. **Graceful Degradation**: Non-critical failures don't stop the application
2. **Comprehensive Logging**: All errors are logged with context
3. **User Notification**: Critical errors are displayed to the user
4. **Recovery Mechanisms**: Automatic retry for transient failures

### Error Categories

#### System-Level Errors
- Process enumeration failures
- Performance counter access issues
- Memory allocation failures

#### Configuration Errors
- Invalid configuration values
- Missing configuration files
- Permission issues

#### Network Errors
- SMTP connection failures
- TLS handshake errors
- DNS resolution issues

#### File System Errors
- Log file write failures
- Disk space issues
- Permission denied

### Error Handling Flow

```mermaid
flowchart TD
    A[Error Detected] --> B{Error Type?}
    B -->|Critical| C[Log Error]
    B -->|Warning| D[Log Warning]
    B -->|Info| E[Log Info]
    
    C --> F[Notify User]
    D --> G[Continue Operation]
    E --> G
    
    F --> H{Recoverable?}
    H -->|Yes| I[Attempt Recovery]
    H -->|No| J[Graceful Shutdown]
    
    I --> K{Recovery Success?}
    K -->|Yes| G
    K -->|No| J
    
    G --> L[Continue Monitoring]
    J --> M[Exit Application]
```

---

## Conclusion

The SystemMonitor application demonstrates professional software architecture with clear separation of concerns, robust error handling, and extensible design patterns. The modular design allows for easy maintenance and future enhancements while providing reliable system monitoring capabilities.

### Key Architectural Benefits

1. **Modularity**: Each component has well-defined responsibilities
2. **Extensibility**: Interface-based design allows for easy component replacement
3. **Scalability**: Asynchronous processing prevents blocking operations
4. **Maintainability**: Clear class hierarchies and documented interfaces
5. **Reliability**: Comprehensive error handling and graceful degradation
6. **Performance**: Efficient resource usage and optimized display modes

The technical implementation showcases modern C++ practices, Windows API integration, and professional software development patterns suitable for enterprise environments.
