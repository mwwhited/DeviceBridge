#pragma once

#include "IFileSystem.h"
#include "IFileSystemPlugin.h"
#include "../Common/Types.h"
#include <Arduino.h>

namespace DeviceBridge::Storage {

/**
 * @brief Registry for filesystem plugins with automatic discovery
 * 
 * Provides centralized management of all filesystem implementations
 * with plugin-style architecture for easy extensibility.
 */
class FileSystemRegistry {
private:
    static constexpr uint8_t MAX_PLUGINS = 8;
    
    IFileSystemPlugin* _plugins[MAX_PLUGINS];
    uint8_t _pluginCount;
    
    // Singleton instance
    static FileSystemRegistry* _instance;
    
    FileSystemRegistry();
    
public:
    ~FileSystemRegistry();
    
    // Singleton access
    static FileSystemRegistry& getInstance();
    static void cleanup();
    
    // Plugin management
    bool registerPlugin(IFileSystemPlugin* plugin);
    bool unregisterPlugin(Common::StorageType storageType);
    IFileSystemPlugin* getPlugin(Common::StorageType storageType);
    
    // Plugin discovery
    uint8_t getAvailablePluginCount() const;
    IFileSystemPlugin* getPluginByIndex(uint8_t index);
    bool hasPlugin(Common::StorageType storageType) const;
    
    // Factory methods
    IFileSystem* createFileSystem(Common::StorageType storageType);
    bool canCreateFileSystem(Common::StorageType storageType) const;
    
    // System operations
    bool initializeAllPlugins();
    void shutdownAllPlugins();
    bool detectAvailableStorageTypes();
    
    // Information queries
    void listSupportedStorageTypes(char* buffer, uint16_t bufferSize);
    void getPluginCapabilities(Common::StorageType storageType, char* buffer, uint16_t bufferSize);
    
    // Hardware detection
    bool detectHardwareForStorageType(Common::StorageType storageType);
    void getHardwareInfo(Common::StorageType storageType, char* buffer, uint16_t bufferSize);
};

} // namespace DeviceBridge::Storage