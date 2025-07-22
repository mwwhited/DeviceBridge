#include "SerialTransferFileSystemPlugin.h"
#include <Arduino.h>

namespace DeviceBridge::Storage {

SerialTransferFileSystemPlugin::SerialTransferFileSystemPlugin() 
    : _initialized(false), _activeFileSystem(nullptr) {
}

SerialTransferFileSystemPlugin::~SerialTransferFileSystemPlugin() {
    shutdown();
}

Common::StorageType SerialTransferFileSystemPlugin::getSupportedStorageType() const {
    return Common::StorageType::SERIAL_TRANSFER;
}

const char* SerialTransferFileSystemPlugin::getPluginName() const {
    return "Serial Transfer Plugin";
}

const char* SerialTransferFileSystemPlugin::getPluginVersion() const {
    return "1.5.0";
}

const char* SerialTransferFileSystemPlugin::getPluginDescription() const {
    return "Real-time serial transfer with hex stream format";
}

bool SerialTransferFileSystemPlugin::supportsHotSwap() const {
    return false; // Serial connection doesn't hot-swap
}

bool SerialTransferFileSystemPlugin::supportsFormatting() const {
    return false; // Serial doesn't need formatting
}

bool SerialTransferFileSystemPlugin::supportsWriteProtection() const {
    return false; // Serial doesn't have write protection
}

bool SerialTransferFileSystemPlugin::supportsDirectoryOperations() const {
    return false; // Serial doesn't support file listing
}

bool SerialTransferFileSystemPlugin::supportsFileReading() const {
    return false; // Serial doesn't support reading files back
}

bool SerialTransferFileSystemPlugin::requiresHardware() const {
    return false; // Serial is built-in
}

IFileSystem* SerialTransferFileSystemPlugin::createFileSystem() {
    if (!canCreateFileSystem()) {
        return nullptr;
    }
    
    SerialTransferFileSystem* fileSystem = new SerialTransferFileSystem();
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

bool SerialTransferFileSystemPlugin::canCreateFileSystem() const {
    return _initialized && Serial;
}

bool SerialTransferFileSystemPlugin::initialize() {
    if (_initialized) {
        return true;
    }
    
    // Serial is always available on Arduino
    _initialized = true;
    return true;
}

bool SerialTransferFileSystemPlugin::isAvailable() const {
    return _initialized && Serial;
}

void SerialTransferFileSystemPlugin::shutdown() {
    if (_activeFileSystem) {
        _activeFileSystem->shutdown();
        delete _activeFileSystem;
        _activeFileSystem = nullptr;
    }
    
    _initialized = false;
}

bool SerialTransferFileSystemPlugin::detectHardware() const {
    // Serial hardware is always present on Arduino
    return true;
}

const char* SerialTransferFileSystemPlugin::getHardwareInfo() const {
    static char info[64];
    snprintf(info, sizeof(info), "Serial: USB connection at %lu baud", 115200UL);
    
    return info;
}

} // namespace DeviceBridge::Storage