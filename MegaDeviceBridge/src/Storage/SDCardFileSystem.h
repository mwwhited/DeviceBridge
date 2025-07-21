#pragma once

#include "IFileSystem.h"
#include "../Common/Config.h"
#include <SD.h>
#include <Arduino.h>

namespace DeviceBridge::Storage {

/**
 * @brief SD Card file system implementation
 * 
 * Provides file operations for SD card storage with hardware detection,
 * write protection monitoring, and automatic error recovery.
 */
class SDCardFileSystem : public IFileSystem {
private:
    File _currentFile;
    bool _initialized;
    bool _writeProtected;
    uint32_t _totalSpace;
    uint32_t _freeSpace;
    
    // Hardware monitoring
    bool checkCardPresence() const;
    bool checkWriteProtection() const;
    void updateSpaceInfo();
    
public:
    SDCardFileSystem();
    virtual ~SDCardFileSystem();
    
    // Lifecycle management
    bool initialize() override;
    bool isAvailable() const override;
    void shutdown() override;
    
    // File operations
    bool createFile(const char* filename) override;
    bool openFile(const char* filename, bool append = false) override;
    bool writeData(const uint8_t* data, uint16_t length) override;
    bool closeFile() override;
    bool deleteFile(const char* filename) override;
    bool fileExists(const char* filename) override;
    
    // Directory operations
    bool listFiles(char* buffer, uint16_t bufferSize) override;
    uint32_t getFileCount() override;
    uint32_t getTotalSpace() override;
    uint32_t getFreeSpace() override;
    
    // Status inquiry
    Common::StorageType getStorageType() const override { 
        return Common::StorageType(Common::StorageType::SD_CARD); 
    }
    const char* getStorageName() const override { return "SD Card"; }
    bool isWriteProtected() const override { return _writeProtected; }
    bool hasActiveFile() const override { return (_currentFile ? true : false) && _hasActiveFile; }
    
    // Statistics
    uint32_t getBytesWritten() const override { return _bytesWritten; }
    uint32_t getFilesCreated() const override { return _filesCreated; }
    uint16_t getLastError() const override { return _lastError; }
    const char* getLastErrorMessage() const override { return _lastErrorMessage; }
    
    // SD Card specific features
    bool format() override;
    bool flush() override;
    bool sync() override;
    
    // Hot-swap support
    bool reinitialize();
    bool isCardInserted() const { return checkCardPresence(); }
};

} // namespace DeviceBridge::Storage