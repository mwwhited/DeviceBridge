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
    // Component references
    ParallelPortManager* _parallelPortManager;
    FileSystemManager* _fileSystemManager;
    DisplayManager* _displayManager;
    TimeManager* _timeManager;
    
    // System state
    Common::SystemStatus _systemStatus;
    Common::ErrorCode _lastError;
    
    // Timing for updates
    unsigned long _lastSystemCheck;
    
    // Command processing
    void processCommand(const Common::SystemCommand& cmd);
    void processStorageSelectCommand(uint8_t value);
    void processFileTypeCommand(uint8_t value);
    void processTimeSetCommand(const char* data);
    void processConfigSaveCommand();
    
    // System monitoring
    void monitorSystemHealth();
    void logSystemStatus();
    
    // Error handling
    void handleError(Common::ErrorCode error);
    void sendDisplayMessage(Common::DisplayMessage::Type type, const char* message);
    void sendDisplayMessage(Common::DisplayMessage::Type type, const __FlashStringHelper* message);
    
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
    uint32_t getUptimeSeconds() const { return _uptimeSeconds; }
    uint16_t getErrorCount() const { return _errorCount; }
    uint32_t getCommandsProcessed() const { return _commandsProcessed; }
    
    // Statistics and monitoring
    void printSystemInfo();
    void printMemoryInfo();
    uint16_t freeRam();
    
    // Component management
    void setComponentManagers(ParallelPortManager* ppm, FileSystemManager* fsm, 
                             DisplayManager* dm, TimeManager* tm);
    
private:
    // Statistics tracking
    uint32_t _uptimeSeconds;
    uint32_t _errorCount;
    uint32_t _commandsProcessed;
};

} // namespace DeviceBridge::Components