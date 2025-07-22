#pragma once

#include "IFileSystemPlugin.h"
#include "SerialTransferFileSystem.h"
#include "../Common/Config.h"

namespace DeviceBridge::Storage {

/**
 * @brief Serial Transfer filesystem plugin implementation
 * 
 * Provides factory and management capabilities for Serial Transfer storage
 * with real-time data streaming and hex format support.
 */
class SerialTransferFileSystemPlugin : public IFileSystemPlugin {
private:
    bool _initialized;
    SerialTransferFileSystem* _activeFileSystem;
    
public:
    SerialTransferFileSystemPlugin();
    virtual ~SerialTransferFileSystemPlugin();
    
    // Plugin identification
    Common::StorageType getSupportedStorageType() const override;
    const char* getPluginName() const override;
    const char* getPluginVersion() const override;
    const char* getPluginDescription() const override;
    
    // Plugin capabilities
    bool supportsHotSwap() const override;
    bool supportsFormatting() const override;
    bool supportsWriteProtection() const override;
    bool supportsDirectoryOperations() const override;
    bool supportsFileReading() const override;
    bool requiresHardware() const override;
    
    // Factory methods
    IFileSystem* createFileSystem() override;
    bool canCreateFileSystem() const override;
    
    // Plugin lifecycle
    bool initialize() override;
    bool isAvailable() const override;
    void shutdown() override;
    
    // Hardware detection (Serial is always available)
    bool detectHardware() const override;
    const char* getHardwareInfo() const override;
};

} // namespace DeviceBridge::Storage