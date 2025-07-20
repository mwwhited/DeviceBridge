#include "ServiceLocator.h"
#include "../Components/ParallelPortManager.h"
#include "../Components/FileSystemManager.h"
#include "../Components/DisplayManager.h"
#include "../Components/TimeManager.h"
#include "../Components/SystemManager.h"
#include "../Components/ConfigurationManager.h"
#include "../Components/HeartbeatLEDManager.h"
#include "ConfigurationService.h"
#include "../User/Display.h"
#include <Arduino.h>

namespace DeviceBridge {

// Static instance
ServiceLocator* ServiceLocator::_instance = nullptr;

ServiceLocator::ServiceLocator() 
    : _parallelPortManager(nullptr)
    , _fileSystemManager(nullptr)
    , _displayManager(nullptr)
    , _timeManager(nullptr)
    , _systemManager(nullptr)
    , _configurationManager(nullptr)
    , _heartbeatLEDManager(nullptr)
    , _configurationService(nullptr)
    , _display(nullptr)
{
}

ServiceLocator& ServiceLocator::getInstance() {
    if (_instance == nullptr) {
        _instance = new ServiceLocator();
    }
    return *_instance;
}

void ServiceLocator::initialize() {
    getInstance(); // Ensure instance exists
}

void ServiceLocator::destroy() {
    delete _instance;
    _instance = nullptr;
}

// Component registration methods
void ServiceLocator::registerParallelPortManager(Components::ParallelPortManager* manager) {
    _parallelPortManager = manager;
}

void ServiceLocator::registerFileSystemManager(Components::FileSystemManager* manager) {
    _fileSystemManager = manager;
}

void ServiceLocator::registerDisplayManager(Components::DisplayManager* manager) {
    _displayManager = manager;
}

void ServiceLocator::registerTimeManager(Components::TimeManager* manager) {
    _timeManager = manager;
}

void ServiceLocator::registerSystemManager(Components::SystemManager* manager) {
    _systemManager = manager;
}

void ServiceLocator::registerConfigurationManager(Components::ConfigurationManager* manager) {
    _configurationManager = manager;
}

void ServiceLocator::registerHeartbeatLEDManager(Components::HeartbeatLEDManager* manager) {
    _heartbeatLEDManager = manager;
}

void ServiceLocator::registerConfigurationService(Common::ConfigurationService* service) {
    _configurationService = service;
}

void ServiceLocator::registerDisplay(User::Display* display) {
    _display = display;
}

// Component access methods
Components::ParallelPortManager* ServiceLocator::getParallelPortManager() const {
    return _parallelPortManager;
}

Components::FileSystemManager* ServiceLocator::getFileSystemManager() const {
    return _fileSystemManager;
}

Components::DisplayManager* ServiceLocator::getDisplayManager() const {
    return _displayManager;
}

Components::TimeManager* ServiceLocator::getTimeManager() const {
    return _timeManager;
}

Components::SystemManager* ServiceLocator::getSystemManager() const {
    return _systemManager;
}

Components::ConfigurationManager* ServiceLocator::getConfigurationManager() const {
    return _configurationManager;
}

Components::HeartbeatLEDManager* ServiceLocator::getHeartbeatLEDManager() const {
    return _heartbeatLEDManager;
}

Common::ConfigurationService* ServiceLocator::getConfigurationService() const {
    return _configurationService;
}

User::Display* ServiceLocator::getDisplay() const {
    return _display;
}

bool ServiceLocator::validateAllDependencies() const {
    Serial.print(F("\r\n=== Service Locator Dependency Validation ===\r\n"));
    
    bool allValid = true;
    
    // Check core services
    printComponentStatus(F("Display"), _display);
    if (!_display) allValid = false;
    
    printComponentStatus(F("ParallelPortManager"), _parallelPortManager);
    if (!_parallelPortManager) allValid = false;
    
    printComponentStatus(F("FileSystemManager"), _fileSystemManager);
    if (!_fileSystemManager) allValid = false;
    
    printComponentStatus(F("DisplayManager"), _displayManager);
    if (!_displayManager) allValid = false;
    
    printComponentStatus(F("TimeManager"), _timeManager);
    if (!_timeManager) allValid = false;
    
    printComponentStatus(F("SystemManager"), _systemManager);
    if (!_systemManager) allValid = false;
    
    printComponentStatus(F("ConfigurationManager"), _configurationManager);
    if (!_configurationManager) allValid = false;
    
    printComponentStatus(F("HeartbeatLEDManager"), _heartbeatLEDManager);
    if (!_heartbeatLEDManager) allValid = false;
    
    printComponentStatus(F("ConfigurationService"), _configurationService);
    if (!_configurationService) allValid = false;
    
    Serial.print(F("\r\nDependency Validation: "));
    if (allValid) {
        Serial.print(F("✅ ALL DEPENDENCIES SATISFIED\r\n"));
    } else {
        Serial.print(F("❌ MISSING DEPENDENCIES DETECTED\r\n"));
    }
    Serial.print(F("===============================================\r\n"));
    
    return allValid;
}

void ServiceLocator::printDependencyStatus() const {
    validateAllDependencies();
}

bool ServiceLocator::runSystemSelfTest() const {
    Serial.print(F("\r\n=== System Self-Test via Service Locator ===\r\n"));
    
    if (!validateAllDependencies()) {
        Serial.print(F("❌ SELF-TEST FAILED: Missing dependencies\r\n"));
        Serial.print(F("===========================================\r\n"));
        return false;
    }
    
    bool allPassed = true;
    
    // Run individual component self-tests (if they implement IComponent)
    Serial.print(F("Running component self-tests...\r\n"));
    
    // For now, just validate hardware availability through managers
    if (_timeManager && !_timeManager->isRTCAvailable()) {
        Serial.print(F("⚠️  TimeManager: RTC not available\r\n"));
    }
    
    if (_fileSystemManager) {
        if (!_fileSystemManager->isSDAvailable()) {
            Serial.print(F("⚠️  FileSystemManager: SD card not available\r\n"));
        }
        if (!_fileSystemManager->isEEPROMAvailable()) {
            Serial.print(F("⚠️  FileSystemManager: EEPROM not available\r\n"));
        }
    }
    
    Serial.print(F("\r\nSystem Self-Test: "));
    if (allPassed) {
        Serial.print(F("✅ PASSED\r\n"));
    } else {
        Serial.print(F("⚠️  PASSED WITH WARNINGS\r\n"));
    }
    Serial.print(F("===========================================\r\n"));
    
    return allPassed;
}

bool ServiceLocator::isComponentRegistered(const char* componentName) const {
    // Simple string-based component checking
    if (strcmp(componentName, "Display") == 0) return _display != nullptr;
    if (strcmp(componentName, "ParallelPortManager") == 0) return _parallelPortManager != nullptr;
    if (strcmp(componentName, "FileSystemManager") == 0) return _fileSystemManager != nullptr;
    if (strcmp(componentName, "DisplayManager") == 0) return _displayManager != nullptr;
    if (strcmp(componentName, "TimeManager") == 0) return _timeManager != nullptr;
    if (strcmp(componentName, "SystemManager") == 0) return _systemManager != nullptr;
    if (strcmp(componentName, "ConfigurationManager") == 0) return _configurationManager != nullptr;
    return false;
}

uint8_t ServiceLocator::getRegisteredComponentCount() const {
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

void ServiceLocator::printComponentStatus(const char* name, void* ptr) const {
    Serial.print(F("  "));
    Serial.print(name);
    Serial.print(F(": "));
    if (ptr) {
        Serial.print(F("✅ Registered\r\n"));
    } else {
        Serial.print(F("❌ Missing\r\n"));
    }
}
void ServiceLocator::printComponentStatus(const __FlashStringHelper* name, void* ptr) const {
    Serial.print(F("  "));
    Serial.print(name);
    Serial.print(F(": "));
    if (ptr) {
        Serial.print(F("✅ Registered\r\n"));
    } else {
        Serial.print(F("❌ Missing\r\n"));
    }
}

} // namespace DeviceBridge