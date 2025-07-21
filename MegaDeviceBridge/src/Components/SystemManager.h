#pragma once

#include <Arduino.h>
#include "../Common/Types.h"
#include "../Common/Config.h"
#include "../Common/ServiceLocator.h"

namespace DeviceBridge::Components {

// Forward declarations
class ParallelPortManager;
class FileSystemManager;
class DisplayManager;
class TimeManager;

class SystemManager : public DeviceBridge::IComponent {
private:
    // Note: No longer storing direct references - using ServiceLocator
    
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
   
    // Lifecycle management (IComponent interface)
    bool initialize() override;
    void update(unsigned long currentTime) override;  // Called from main loop
    void stop() override;
    
    // IComponent interface implementation
    bool selfTest() override;
    const char* getComponentName() const override;
    bool validateDependencies() const override;
    void printDependencyStatus() const override;
    unsigned long getUpdateInterval() const override;

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
    uint16_t getFreeMemory() const;
    
    // Statistics and monitoring
    void printSystemInfo();
    void printMemoryInfo();
    uint16_t freeRam();
    void validateHardware();  // Hardware validation test
    
    // Serial heartbeat control
    void setSerialHeartbeatEnabled(bool enabled) { _debugFlags.serialHeartbeatEnabled = enabled ? 1 : 0; }
    bool isSerialHeartbeatEnabled() const { return _debugFlags.serialHeartbeatEnabled; }
    
    // Debug mode control
    void setLCDDebugEnabled(bool enabled) { _debugFlags.lcdDebugEnabled = enabled ? 1 : 0; }
    bool isLCDDebugEnabled() const { return _debugFlags.lcdDebugEnabled; }
    void setParallelDebugEnabled(bool enabled) { _debugFlags.parallelDebugEnabled = enabled ? 1 : 0; }
    bool isParallelDebugEnabled() const { return _debugFlags.parallelDebugEnabled; }
        
private:
    // Statistics tracking
    uint32_t _uptimeSeconds;
    uint32_t _errorCount;
    uint32_t _commandsProcessed;
    
    // Debug flags (bit field optimization)
    struct {
        uint8_t serialHeartbeatEnabled : 1;
        uint8_t lcdDebugEnabled : 1;
        uint8_t parallelDebugEnabled : 1;
        uint8_t reserved : 5;  // For future flags
    } _debugFlags;
};

} // namespace DeviceBridge::Components