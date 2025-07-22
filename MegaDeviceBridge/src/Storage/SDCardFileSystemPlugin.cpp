#include "SDCardFileSystemPlugin.h"
#include "../Common/Config.h"
#include <Arduino.h>

namespace DeviceBridge::Storage {

SDCardFileSystemPlugin::SDCardFileSystemPlugin() 
    : _initialized(false), _hardwareDetected(false), _activeFileSystem(nullptr) {
}

SDCardFileSystemPlugin::~SDCardFileSystemPlugin() {
    shutdown();
}

Common::StorageType SDCardFileSystemPlugin::getSupportedStorageType() const {
    return Common::StorageType::SD_CARD;
}

const char* SDCardFileSystemPlugin::getPluginName() const {
    return "SD Card Plugin";
}

const char* SDCardFileSystemPlugin::getPluginVersion() const {
    return "1.0.0";
}

const char* SDCardFileSystemPlugin::getPluginDescription() const {
    return "SD Card storage with hot-swap support";
}

bool SDCardFileSystemPlugin::supportsHotSwap() const {
    return true;
}

bool SDCardFileSystemPlugin::supportsFormatting() const {
    return true;
}

bool SDCardFileSystemPlugin::supportsWriteProtection() const {
    return true;
}

bool SDCardFileSystemPlugin::supportsDirectoryOperations() const {
    return true;
}

bool SDCardFileSystemPlugin::supportsFileReading() const {
    return true;
}

bool SDCardFileSystemPlugin::requiresHardware() const {
    return true;
}

IFileSystem* SDCardFileSystemPlugin::createFileSystem() {
    if (!canCreateFileSystem()) {
        return nullptr;
    }
    
    SDCardFileSystem* fileSystem = new SDCardFileSystem();
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

bool SDCardFileSystemPlugin::canCreateFileSystem() const {
    return _initialized && _hardwareDetected;
}

bool SDCardFileSystemPlugin::initialize() {
    if (_initialized) {
        return true;
    }
    
    // Perform hardware detection
    _hardwareDetected = performHardwareDetection();
    _initialized = true;
    
    return _hardwareDetected;
}

bool SDCardFileSystemPlugin::isAvailable() const {
    return _initialized && _hardwareDetected;
}

void SDCardFileSystemPlugin::shutdown() {
    if (_activeFileSystem) {
        _activeFileSystem->shutdown();
        delete _activeFileSystem;
        _activeFileSystem = nullptr;
    }
    
    _initialized = false;
    _hardwareDetected = false;
}

bool SDCardFileSystemPlugin::detectHardware() const {
    return performHardwareDetection();
}

const char* SDCardFileSystemPlugin::getHardwareInfo() const {
    if (!_hardwareDetected) {
        return "SD Card not detected";
    }
    
    static char info[64];
    bool cardPresent = (digitalRead(Common::Pins::SD_CD) == LOW);
    bool writeProtected = (digitalRead(Common::Pins::SD_WP) == HIGH);
    
    snprintf(info, sizeof(info), "SD Card: %s, %s", 
             cardPresent ? "Present" : "Missing",
             writeProtected ? "Protected" : "Writable");
    
    return info;
}

bool SDCardFileSystemPlugin::performHardwareDetection() {
    // Check if SD card is physically present
    pinMode(Common::Pins::SD_CD, INPUT_PULLUP);
    pinMode(Common::Pins::SD_WP, INPUT_PULLUP);
    
    // SD_CD is active LOW (card present when LOW)
    bool cardPresent = (digitalRead(Common::Pins::SD_CD) == LOW);
    
    return cardPresent;
}

} // namespace DeviceBridge::Storage