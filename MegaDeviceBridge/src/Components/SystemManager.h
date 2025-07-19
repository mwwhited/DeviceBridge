#pragma once

#include <Arduino.h>
#include "../Common/Types.h"
#include "../Common/Config.h"

namespace DeviceBridge::Components {

// Forward declarations
class ParallelPortManager;
class FileSystemManager;
class DisplayManager;
class TimeManager;

class SystemManager {
private:
    DisplayManager* _displayManager;
    
    // Component references
    ParallelPortManager* _parallelPortManager;
    FileSystemManager* _fileSystemManager;
    DisplayManager* _displayManager;
    TimeManager* _timeManager;
    
    // System state
    Common::SystemStatus _systemStatus;
    Common::ErrorCode _lastError;
    
    // System management
    
    // Command processing
    void processCommand(const Common::SystemCommand& cmd);
    void processStorageSelectCommand(uint8_t value);
    void processFileTypeCommand(uint8_t value);
    void processTimeSetCommand(const char* data);
    void processConfigSaveCommand();
    
    // System monitoring
    void monitorSystemHealth();
    void checkTaskStacks();
    void checkQueueLevels();
    void logSystemStatus();
    
    // Error handling
    void handleError(Common::ErrorCode error);
    void sendDisplayMessage(Common::DisplayMessage::Type type, const char* message);
    
public:
    SystemManager();
    ~SystemManager();
    
    // Component registration
    void setParallelPortManager(ParallelPortManager* manager) { _parallelPortManager = manager; }
    void setFileSystemManager(FileSystemManager* manager) { _fileSystemManager = manager; }
    void setDisplayManager(DisplayManager* manager) { _displayManager = manager; }
    void setTimeManager(TimeManager* manager) { _timeManager = manager; }
    
    // Lifecycle management
    bool initialize();
    void update();  // Called from main loop
    void stop();
    
    // Command processing (called by other components)
    void processSystemCommand(const Common::SystemCommand& cmd);
    
    // System control
    void setSystemStatus(Common::SystemStatus status);
    void reportError(Common::ErrorCode error);
    
    // Status inquiry
    Common::SystemStatus getSystemStatus() const { return _systemStatus; }
    Common::ErrorCode getLastError() const { return _lastError; }
    
    // Statistics and monitoring
    void printSystemInfo();
    void printTaskInfo();
    void printMemoryInfo();
    
private:
    // Statistics tracking
    uint32_t _uptimeSeconds;
    uint32_t _errorCount;
    uint32_t _commandsProcessed;
    
    // Health monitoring
    struct TaskHealth {
        TaskHandle_t handle;
        const char* name;
        UBaseType_t lastHighWaterMark;
        bool healthy;
    };
    
    static constexpr uint8_t MAX_MONITORED_TASKS = 8;
    TaskHealth _monitoredTasks[MAX_MONITORED_TASKS];
    uint8_t _monitoredTaskCount;
    
    void addTaskToMonitor(TaskHandle_t handle, const char* name);
    void checkTaskHealth(TaskHealth& task);
};

} // namespace DeviceBridge::Components