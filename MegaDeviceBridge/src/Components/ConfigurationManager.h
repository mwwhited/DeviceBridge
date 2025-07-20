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
    void processCommand(const String& command);
    void handleTimeSetCommand(const String& command);
    void handleStorageCommand(const String& command);
    void handleHeartbeatCommand(const String& command);
    void handleLEDCommand(const String& command);
    void handleListCommand(const String& command);
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
    bool initialize() override;
    void update() override;  // Called from main loop
    void stop() override;
    
    // IComponent interface implementation
    bool selfTest() override;
    const char* getComponentName() const override;
    bool validateDependencies() const override;
    void printDependencyStatus() const override;
    
    // Configuration interface
    void checkSerialCommands();
};

} // namespace DeviceBridge::Components