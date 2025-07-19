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
class SystemManager;

class ConfigurationManager {
private:
    // Component references
    ParallelPortManager* _parallelPortManager;
    FileSystemManager* _fileSystemManager;
    DisplayManager* _displayManager;
    TimeManager* _timeManager;
    SystemManager* _systemManager;
    
    // Serial command processing
    void processCommand(const String& command);
    void handleTimeSetCommand(const String& command);
    void handleStorageCommand(const String& command);
    void handleHeartbeatCommand(const String& command);
    
    // Command output methods
    void printHelpMenu();
    void printDetailedStatus();
    void printCurrentTime();
    void printButtonStatus();
    
    // Timing for updates
    unsigned long _lastCommandCheck;
    
public:
    ConfigurationManager();
    ~ConfigurationManager();
    
    // Component registration
    void setParallelPortManager(ParallelPortManager* manager) { _parallelPortManager = manager; }
    void setFileSystemManager(FileSystemManager* manager) { _fileSystemManager = manager; }
    void setDisplayManager(DisplayManager* manager) { _displayManager = manager; }
    void setTimeManager(TimeManager* manager) { _timeManager = manager; }
    void setSystemManager(SystemManager* manager) { _systemManager = manager; }
    
    // Lifecycle management
    bool initialize();
    void update();  // Called from main loop
    void stop();
    
    // Component management
    void setComponentManagers(ParallelPortManager* ppm, FileSystemManager* fsm, 
                             DisplayManager* dm, TimeManager* tm, SystemManager* sm);
    
    // Configuration interface
    void checkSerialCommands();
};

} // namespace DeviceBridge::Components