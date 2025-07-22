#pragma once

#include "IFileSystem.h"
#include "../Common/Types.h"

namespace DeviceBridge::Storage {

/**
 * @brief Plugin interface for filesystem implementations
 * 
 * Provides a factory pattern for creating filesystem instances and
 * managing their capabilities in a modular way.
 */
class IFileSystemPlugin {
public:
    virtual ~IFileSystemPlugin() = default;
    
    // Plugin identification
    virtual Common::StorageType getSupportedStorageType() const = 0;
    virtual const char* getPluginName() const = 0;
    virtual const char* getPluginVersion() const = 0;
    virtual const char* getPluginDescription() const = 0;
    
    // Plugin capabilities
    virtual bool supportsHotSwap() const = 0;
    virtual bool supportsFormatting() const = 0;
    virtual bool supportsWriteProtection() const = 0;
    virtual bool supportsDirectoryOperations() const = 0;
    virtual bool supportsFileReading() const = 0;
    virtual bool requiresHardware() const = 0;
    
    // Factory methods
    virtual IFileSystem* createFileSystem() = 0;
    virtual bool canCreateFileSystem() const = 0;
    
    // Plugin lifecycle
    virtual bool initialize() = 0;
    virtual bool isAvailable() const = 0;
    virtual void shutdown() = 0;
    
    // Hardware detection (for hardware-based plugins)
    virtual bool detectHardware() const { return true; }
    virtual const char* getHardwareInfo() const { return "No hardware info"; }
};

} // namespace DeviceBridge::Storage