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
class SystemManager;

class ConfigurationManager : public DeviceBridge::IComponent {
private:
    // Note: No longer storing direct references - using ServiceLocator
    
    // Serial command processing
    void processCommand(const char* command, size_t commandLen);
    void handleTimeSetCommand(const char* command, size_t commandLen);
    void handleStorageCommand(const char* command, size_t commandLen);
    void handleHeartbeatCommand(const String& command);
    void handleLEDCommand(const char* command, size_t commandLen);
    void handleListCommand(const char* command, size_t commandLen);  // Already converted
    void handleFormatCommand(const String& command);
    void handleCopyToCommand(const String& command);
    void handleDebugCommand(const String& command);
    
    // Command output methods
    void printHelpMenu();
    void printDetailedStatus();
    void printCurrentTime();
    void printButtonStatus();
    void printParallelPortStatus();
    void testInterruptPin();
    void printLastFileInfo();
    void printStorageStatus();
    void handleTestWriteCommand(const String& command);
    void handleTestWriteLongCommand(const String& command);
    void testPrinterProtocol();
    void clearLPTBuffer();
    void resetCriticalState();
    void handleLCDThrottleCommand(const String& command);
    
    // Timing for updates
    unsigned long _lastCommandCheck;
    
public:
    ConfigurationManager();
    ~ConfigurationManager();
    
    // Lifecycle management (IComponent interface)
    bool initialize() override final;
    void update(unsigned long currentTime) override final;  // Called from main loop
    void stop() override final;
    
    // IComponent interface implementation
    bool selfTest() override final;
    const char* getComponentName() const override final;
    bool validateDependencies() const override final;
    void printDependencyStatus() const override final;
    unsigned long getUpdateInterval() const override final;
    
    // Configuration interface
    void checkSerialCommands();
    
private:
    // Hardware flow control commands
    void handleFlowControlCommand(const String& command);
    void printFlowControlStatistics();
};

} // namespace DeviceBridge::Components