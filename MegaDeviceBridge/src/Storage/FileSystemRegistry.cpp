#include "FileSystemRegistry.h"
#include <string.h>

namespace DeviceBridge::Storage {

// Singleton instance
FileSystemRegistry* FileSystemRegistry::_instance = nullptr;

FileSystemRegistry::FileSystemRegistry() : _pluginCount(0) {
    memset(_plugins, 0, sizeof(_plugins));
}

FileSystemRegistry::~FileSystemRegistry() {
    shutdownAllPlugins();
}

FileSystemRegistry& FileSystemRegistry::getInstance() {
    if (!_instance) {
        _instance = new FileSystemRegistry();
    }
    return *_instance;
}

void FileSystemRegistry::cleanup() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

bool FileSystemRegistry::registerPlugin(IFileSystemPlugin* plugin) {
    if (!plugin || _pluginCount >= MAX_PLUGINS) {
        return false;
    }
    
    // Check if plugin for this storage type already exists
    if (hasPlugin(plugin->getSupportedStorageType())) {
        return false;
    }
    
    _plugins[_pluginCount] = plugin;
    _pluginCount++;
    
    return true;
}

bool FileSystemRegistry::unregisterPlugin(Common::StorageType storageType) {
    for (uint8_t i = 0; i < _pluginCount; i++) {
        if (_plugins[i] && _plugins[i]->getSupportedStorageType().value == storageType.value) {
            // Shift remaining plugins
            for (uint8_t j = i; j < _pluginCount - 1; j++) {
                _plugins[j] = _plugins[j + 1];
            }
            _plugins[_pluginCount - 1] = nullptr;
            _pluginCount--;
            return true;
        }
    }
    return false;
}

IFileSystemPlugin* FileSystemRegistry::getPlugin(Common::StorageType storageType) {
    for (uint8_t i = 0; i < _pluginCount; i++) {
        if (_plugins[i] && _plugins[i]->getSupportedStorageType().value == storageType.value) {
            return _plugins[i];
        }
    }
    return nullptr;
}

uint8_t FileSystemRegistry::getAvailablePluginCount() const {
    return _pluginCount;
}

IFileSystemPlugin* FileSystemRegistry::getPluginByIndex(uint8_t index) {
    if (index >= _pluginCount) {
        return nullptr;
    }
    return _plugins[index];
}

bool FileSystemRegistry::hasPlugin(Common::StorageType storageType) const {
    for (uint8_t i = 0; i < _pluginCount; i++) {
        if (_plugins[i] && _plugins[i]->getSupportedStorageType().value == storageType.value) {
            return true;
        }
    }
    return false;
}

IFileSystem* FileSystemRegistry::createFileSystem(Common::StorageType storageType) {
    IFileSystemPlugin* plugin = getPlugin(storageType);
    if (!plugin || !plugin->canCreateFileSystem()) {
        return nullptr;
    }
    
    return plugin->createFileSystem();
}

bool FileSystemRegistry::canCreateFileSystem(Common::StorageType storageType) const {
    for (uint8_t i = 0; i < _pluginCount; i++) {
        if (_plugins[i] && 
            _plugins[i]->getSupportedStorageType().value == storageType.value) {
            return _plugins[i]->canCreateFileSystem();
        }
    }
    return false;
}

bool FileSystemRegistry::initializeAllPlugins() {
    bool allSuccessful = true;
    
    for (uint8_t i = 0; i < _pluginCount; i++) {
        if (_plugins[i]) {
            if (!_plugins[i]->initialize()) {
                allSuccessful = false;
            }
        }
    }
    
    return allSuccessful;
}

void FileSystemRegistry::shutdownAllPlugins() {
    for (uint8_t i = 0; i < _pluginCount; i++) {
        if (_plugins[i]) {
            _plugins[i]->shutdown();
        }
    }
}

bool FileSystemRegistry::detectAvailableStorageTypes() {
    bool anyDetected = false;
    
    for (uint8_t i = 0; i < _pluginCount; i++) {
        if (_plugins[i]) {
            if (_plugins[i]->detectHardware()) {
                anyDetected = true;
            }
        }
    }
    
    return anyDetected;
}

void FileSystemRegistry::listSupportedStorageTypes(char* buffer, uint16_t bufferSize) {
    if (!buffer || bufferSize < 20) {
        return;
    }
    
    uint16_t offset = 0;
    offset += snprintf(buffer + offset, bufferSize - offset, "Storage Types:\r\n");
    
    for (uint8_t i = 0; i < _pluginCount && offset < bufferSize - 50; i++) {
        if (_plugins[i]) {
            offset += snprintf(buffer + offset, bufferSize - offset,
                             "  %s: %s (%s)\r\n",
                             _plugins[i]->getPluginName(),
                             _plugins[i]->getPluginDescription(),
                             _plugins[i]->isAvailable() ? "Available" : "Unavailable");
        }
    }
}

void FileSystemRegistry::getPluginCapabilities(Common::StorageType storageType, char* buffer, uint16_t bufferSize) {
    if (!buffer || bufferSize < 20) {
        return;
    }
    
    IFileSystemPlugin* plugin = getPlugin(storageType);
    if (!plugin) {
        snprintf(buffer, bufferSize, "Plugin not found");
        return;
    }
    
    uint16_t offset = 0;
    offset += snprintf(buffer + offset, bufferSize - offset,
                      "%s v%s:\r\n", plugin->getPluginName(), plugin->getPluginVersion());
    
    if (offset < bufferSize - 100) {
        offset += snprintf(buffer + offset, bufferSize - offset,
                          "  Hot-swap: %s\r\n", plugin->supportsHotSwap() ? "Yes" : "No");
        offset += snprintf(buffer + offset, bufferSize - offset,
                          "  Format: %s\r\n", plugin->supportsFormatting() ? "Yes" : "No");
        offset += snprintf(buffer + offset, bufferSize - offset,
                          "  Write Protection: %s\r\n", plugin->supportsWriteProtection() ? "Yes" : "No");
        offset += snprintf(buffer + offset, bufferSize - offset,
                          "  File Reading: %s\r\n", plugin->supportsFileReading() ? "Yes" : "No");
    }
}

bool FileSystemRegistry::detectHardwareForStorageType(Common::StorageType storageType) {
    IFileSystemPlugin* plugin = getPlugin(storageType);
    if (!plugin) {
        return false;
    }
    
    return plugin->detectHardware();
}

void FileSystemRegistry::getHardwareInfo(Common::StorageType storageType, char* buffer, uint16_t bufferSize) {
    if (!buffer || bufferSize < 10) {
        return;
    }
    
    IFileSystemPlugin* plugin = getPlugin(storageType);
    if (!plugin) {
        snprintf(buffer, bufferSize, "No plugin");
        return;
    }
    
    const char* info = plugin->getHardwareInfo();
    strncpy(buffer, info, bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
}

} // namespace DeviceBridge::Storage