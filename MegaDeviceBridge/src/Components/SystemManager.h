#pragma once

#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
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
    QueueHandle_t _commandQueue;
    QueueHandle_t _displayQueue;
    SemaphoreHandle_t _serialMutex;
    TaskHandle_t _taskHandle;
    
    // Component references
    ParallelPortManager* _parallelPortManager;
    FileSystemManager* _fileSystemManager;
    DisplayManager* _displayManager;
    TimeManager* _timeManager;
    
    // System state
    Common::SystemStatus _systemStatus;
    Common::ErrorCode _lastError;
    
    // Task function (static for FreeRTOS)
    static void taskFunction(void* pvParameters);
    
    // Instance handling for task function
    void runTask();
    
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
    SystemManager(QueueHandle_t commandQueue, QueueHandle_t displayQueue, SemaphoreHandle_t serialMutex);
    ~SystemManager();
    
    // Component registration
    void setParallelPortManager(ParallelPortManager* manager) { _parallelPortManager = manager; }
    void setFileSystemManager(FileSystemManager* manager) { _fileSystemManager = manager; }
    void setDisplayManager(DisplayManager* manager) { _displayManager = manager; }
    void setTimeManager(TimeManager* manager) { _timeManager = manager; }
    
    // Lifecycle management
    bool initialize();
    bool start();
    void stop();
    
    // System control
    void setSystemStatus(Common::SystemStatus status);
    void reportError(Common::ErrorCode error);
    
    // Status inquiry
    Common::SystemStatus getSystemStatus() const { return _systemStatus; }
    Common::ErrorCode getLastError() const { return _lastError; }
    TaskHandle_t getTaskHandle() const { return _taskHandle; }
    
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