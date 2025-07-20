#pragma once

#include <Arduino.h>
#include "Types.h"

namespace DeviceBridge {

// Forward declarations
namespace Components {
    class ParallelPortManager;
    class FileSystemManager;
    class DisplayManager;
    class TimeManager;
    class SystemManager;
    class ConfigurationManager;
    class HeartbeatLEDManager;
}

namespace Common {
    class ConfigurationService;
}

namespace User {
    class Display;
}

/**
 * Central service registry for all system components
 * Eliminates null pointer issues and provides dependency validation
 */
class ServiceLocator {
private:
    // Component instances
    Components::ParallelPortManager* _parallelPortManager;
    Components::FileSystemManager* _fileSystemManager;
    Components::DisplayManager* _displayManager;
    Components::TimeManager* _timeManager;
    Components::SystemManager* _systemManager;
    Components::ConfigurationManager* _configurationManager;
    Components::HeartbeatLEDManager* _heartbeatLEDManager;
    Common::ConfigurationService* _configurationService;
    User::Display* _display;
    
    // Singleton instance
    static ServiceLocator* _instance;
    
    ServiceLocator();
    
public:
    // Singleton access
    static ServiceLocator& getInstance();
    static void initialize();
    static void destroy();
    
    // Component registration
    void registerParallelPortManager(Components::ParallelPortManager* manager);
    void registerFileSystemManager(Components::FileSystemManager* manager);
    void registerDisplayManager(Components::DisplayManager* manager);
    void registerTimeManager(Components::TimeManager* manager);
    void registerSystemManager(Components::SystemManager* manager);
    void registerConfigurationManager(Components::ConfigurationManager* manager);
    void registerHeartbeatLEDManager(Components::HeartbeatLEDManager* manager);
    void registerConfigurationService(Common::ConfigurationService* service);
    void registerDisplay(User::Display* display);
    
    // Component access
    Components::ParallelPortManager* getParallelPortManager() const;
    Components::FileSystemManager* getFileSystemManager() const;
    Components::DisplayManager* getDisplayManager() const;
    Components::TimeManager* getTimeManager() const;
    Components::SystemManager* getSystemManager() const;
    Components::ConfigurationManager* getConfigurationManager() const;
    Components::HeartbeatLEDManager* getHeartbeatLEDManager() const;
    Common::ConfigurationService* getConfigurationService() const;
    User::Display* getDisplay() const;
    
    // System validation
    bool validateAllDependencies() const;
    void printDependencyStatus() const;
    bool runSystemSelfTest() const;
    
    // Dependency checking
    bool isComponentRegistered(const char* componentName) const;
    uint8_t getRegisteredComponentCount() const;
    
private:
    void printComponentStatus(const char* name, void* ptr) const;
    void printComponentStatus(const __FlashStringHelper* name, void* ptr) const;
};

/**
 * Base interface for all system components
 * Provides common lifecycle and validation methods with encapsulated timing
 */
class IComponent {
public:
    virtual ~IComponent() = default;
    
    // Lifecycle management
    virtual bool initialize() = 0;
    virtual void update(unsigned long currentTime) = 0;
    virtual void stop() = 0;
    
    // Self-testing capabilities
    virtual bool selfTest() = 0;
    virtual const char* getComponentName() const = 0;
    
    // Dependency validation
    virtual bool validateDependencies() const = 0;
    virtual void printDependencyStatus() const = 0;
    
    // Time management
    virtual unsigned long getUpdateInterval() const = 0;
    virtual bool shouldUpdate(unsigned long currentTime) const {
        return (currentTime - _lastUpdateTime) >= getUpdateInterval();
    }
    virtual void markUpdated(unsigned long currentTime) {
        _lastUpdateTime = currentTime;
    }
    
protected:
    // Helper for getting services
    ServiceLocator& getServices() const { return ServiceLocator::getInstance(); }
    
    // Time tracking
    unsigned long _lastUpdateTime = 0;
};

} // namespace DeviceBridge