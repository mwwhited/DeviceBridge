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
    
    // Component access - inlined for maximum performance
    inline Components::ParallelPortManager* getParallelPortManager() const { return _parallelPortManager; }
    inline Components::FileSystemManager* getFileSystemManager() const { return _fileSystemManager; }
    inline Components::DisplayManager* getDisplayManager() const { return _displayManager; }
    inline Components::TimeManager* getTimeManager() const { return _timeManager; }
    inline Components::SystemManager* getSystemManager() const { return _systemManager; }
    inline Components::ConfigurationManager* getConfigurationManager() const { return _configurationManager; }
    inline Components::HeartbeatLEDManager* getHeartbeatLEDManager() const { return _heartbeatLEDManager; }
    inline Common::ConfigurationService* getConfigurationService() const { return _configurationService; }
    inline User::Display* getDisplay() const { return _display; }
    
    // System validation
    bool validateAllDependencies() const;
    void printDependencyStatus() const;
    bool runSystemSelfTest() const;
    
    // Dependency checking - inlined for performance
    bool isComponentRegistered(const char* componentName) const;
    inline uint8_t getRegisteredComponentCount() const {
        uint8_t count = 0;
        if (_display) count++;
        if (_parallelPortManager) count++;
        if (_fileSystemManager) count++;
        if (_displayManager) count++;
        if (_timeManager) count++;
        if (_systemManager) count++;
        if (_configurationManager) count++;
        return count;
    }
    
private:
    void printComponentStatus(const char* name, void* ptr) const;
    void printComponentStatus(const __FlashStringHelper* name, void* ptr) const;
    void triggerSOSError(const __FlashStringHelper* errorCode) const;
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
    // Helper for getting services (legacy - prefer cached pointers)
    ServiceLocator& getServices() const { return ServiceLocator::getInstance(); }
    
    // Cached service pointers for performance (initialized once in initialize())
    Components::ParallelPortManager* _cachedParallelPortManager = nullptr;
    Components::FileSystemManager* _cachedFileSystemManager = nullptr;
    Components::DisplayManager* _cachedDisplayManager = nullptr;
    Components::TimeManager* _cachedTimeManager = nullptr;
    Components::SystemManager* _cachedSystemManager = nullptr;
    Components::ConfigurationManager* _cachedConfigurationManager = nullptr;
    Components::HeartbeatLEDManager* _cachedHeartbeatLEDManager = nullptr;
    Common::ConfigurationService* _cachedConfigurationService = nullptr;
    User::Display* _cachedDisplay = nullptr;
    
    // Cache initialization method - call this FIRST in each component's initialize()
    void cacheServiceDependencies() {
        auto& services = ServiceLocator::getInstance();
        _cachedParallelPortManager = services.getParallelPortManager();
        _cachedFileSystemManager = services.getFileSystemManager();
        _cachedDisplayManager = services.getDisplayManager();
        _cachedTimeManager = services.getTimeManager();
        _cachedSystemManager = services.getSystemManager();
        _cachedConfigurationManager = services.getConfigurationManager();
        _cachedHeartbeatLEDManager = services.getHeartbeatLEDManager();
        _cachedConfigurationService = services.getConfigurationService();
        _cachedDisplay = services.getDisplay();
    }
    
    // Time tracking
    unsigned long _lastUpdateTime = 0;
};

} // namespace DeviceBridge