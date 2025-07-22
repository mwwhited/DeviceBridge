#pragma once

#include "IFileSystemPlugin.h"
#include "EEPROMFileSystem.h"
#include "../Common/Config.h"

namespace DeviceBridge::Storage {

/**
 * @brief EEPROM filesystem plugin implementation
 * 
 * Provides factory and management capabilities for EEPROM storage
 * with W25Q128 hardware detection and minimal filesystem support.
 */
class EEPROMFileSystemPlugin : public IFileSystemPlugin {
private:
    bool _initialized;
    bool _hardwareDetected;
    EEPROMFileSystem* _activeFileSystem;
    
    // Hardware detection
    bool performHardwareDetection();
    
public:
    EEPROMFileSystemPlugin();
    virtual ~EEPROMFileSystemPlugin();
    
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