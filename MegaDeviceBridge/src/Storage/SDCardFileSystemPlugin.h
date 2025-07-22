#pragma once

#include "IFileSystemPlugin.h"
#include "SDCardFileSystem.h"
#include "../Common/Config.h"

namespace DeviceBridge::Storage {

/**
 * @brief SD Card filesystem plugin implementation
 * 
 * Provides factory and management capabilities for SD Card storage
 * with hot-swap detection and hardware monitoring.
 */
class SDCardFileSystemPlugin : public IFileSystemPlugin {
private:
    bool _initialized;
    bool _hardwareDetected;
    SDCardFileSystem* _activeFileSystem;
    
    // Hardware detection
    bool performHardwareDetection();
    
public:
    SDCardFileSystemPlugin();
    virtual ~SDCardFileSystemPlugin();
    
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
    
    // Hardware detection
    bool detectHardware() const override;
    const char* getHardwareInfo() const override;
};

} // namespace DeviceBridge::Storage