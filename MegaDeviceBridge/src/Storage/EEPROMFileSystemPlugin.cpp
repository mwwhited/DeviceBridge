#include "EEPROMFileSystemPlugin.h"
#include "../Components/W25Q128Manager.h"
#include <Arduino.h>

namespace DeviceBridge::Storage {

EEPROMFileSystemPlugin::EEPROMFileSystemPlugin() 
    : _initialized(false), _hardwareDetected(false), _activeFileSystem(nullptr) {
}

EEPROMFileSystemPlugin::~EEPROMFileSystemPlugin() {
    shutdown();
}

Common::StorageType EEPROMFileSystemPlugin::getSupportedStorageType() const {
    return Common::StorageType::EEPROM;
}

const char* EEPROMFileSystemPlugin::getPluginName() const {
    return "EEPROM Plugin";
}

const char* EEPROMFileSystemPlugin::getPluginVersion() const {
    return "2.0.0";
}

const char* EEPROMFileSystemPlugin::getPluginDescription() const {
    return "W25Q128 16MB EEPROM with minimal filesystem";
}

bool EEPROMFileSystemPlugin::supportsHotSwap() const {
    return false; // EEPROM is permanently mounted
}

bool EEPROMFileSystemPlugin::supportsFormatting() const {
    return true;
}

bool EEPROMFileSystemPlugin::supportsWriteProtection() const {
    return false; // W25Q128 doesn't have hardware write protection
}

bool EEPROMFileSystemPlugin::supportsDirectoryOperations() const {
    return true;
}

bool EEPROMFileSystemPlugin::supportsFileReading() const {
    return true;
}

bool EEPROMFileSystemPlugin::requiresHardware() const {
    return true;
}

IFileSystem* EEPROMFileSystemPlugin::createFileSystem() {
    if (!canCreateFileSystem()) {
        return nullptr;
    }
    
    EEPROMFileSystem* fileSystem = new EEPROMFileSystem();
    if (!fileSystem) {
        return nullptr;
    }
    
    // Initialize the filesystem
    if (!fileSystem->initialize()) {
        delete fileSystem;
        return nullptr;
    }
    
    return fileSystem;
}

bool EEPROMFileSystemPlugin::canCreateFileSystem() const {
    return _initialized && _hardwareDetected;
}

bool EEPROMFileSystemPlugin::initialize() {
    if (_initialized) {
        return true;
    }
    
    // Perform hardware detection
    _hardwareDetected = performHardwareDetection();
    _initialized = true;
    
    return _hardwareDetected;
}

bool EEPROMFileSystemPlugin::isAvailable() const {
    return _initialized && _hardwareDetected;
}

void EEPROMFileSystemPlugin::shutdown() {
    if (_activeFileSystem) {
        _activeFileSystem->shutdown();
        delete _activeFileSystem;
        _activeFileSystem = nullptr;
    }
    
    _initialized = false;
    _hardwareDetected = false;
}

bool EEPROMFileSystemPlugin::detectHardware() const {
    return performHardwareDetection();
}

const char* EEPROMFileSystemPlugin::getHardwareInfo() const {
    if (!_hardwareDetected) {
        return "W25Q128 EEPROM not detected";
    }
    
    static char info[64];
    snprintf(info, sizeof(info), "W25Q128: 16MB EEPROM detected on CS pin %d", 
             Common::Pins::EEPROM_CS);
    
    return info;
}

bool EEPROMFileSystemPlugin::performHardwareDetection() {
    // Test W25Q128 detection using temporary manager
    DeviceBridge::Components::W25Q128Manager testManager(Common::Pins::EEPROM_CS);
    
    bool detected = testManager.initialize();
    
    // Clean up test manager
    // No explicit cleanup needed for W25Q128Manager
    
    return detected;
}

} // namespace DeviceBridge::Storage