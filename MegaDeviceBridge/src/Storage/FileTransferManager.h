#pragma once

#include "IFileSystem.h"
#include "FileSystemRegistry.h"
#include "../Common/Types.h"
#include <Arduino.h>

namespace DeviceBridge::Storage {

/**
 * @brief Manager for transferring files between different storage types
 * 
 * Provides copyTo functionality to transfer files from one storage type to another,
 * with automatic format handling (hex for serial, binary for others).
 */
class FileTransferManager {
private:
    static constexpr uint16_t TRANSFER_BUFFER_SIZE = 128;  // Memory optimized (128 bytes saved)
    
    FileSystemRegistry& _registry;
    char _transferBuffer[TRANSFER_BUFFER_SIZE];
    
    // Progress callback
    void (*_progressCallback)(uint32_t percent) = nullptr;
    
    // Private transfer methods
    bool copyFileData(IFileSystem* source, IFileSystem* destination, const char* filename);
    void reportProgress(uint32_t current, uint32_t total);
    
public:
    FileTransferManager();
    ~FileTransferManager() = default;
    
    // File transfer operations
    bool copyTo(const char* filename, Common::StorageType sourceType, Common::StorageType destinationType);
    bool copyToWithNewName(const char* sourceFilename, Common::StorageType sourceType, 
                          const char* destinationFilename, Common::StorageType destinationType);
    
    // Batch operations
    bool copyAllFiles(Common::StorageType sourceType, Common::StorageType destinationType);
    
    // Transfer management
    void setProgressCallback(void (*callback)(uint32_t percent));
    bool isTransferSupported(Common::StorageType sourceType, Common::StorageType destinationType);
    
    // Information
    void getSupportedTransfers(char* buffer, uint16_t bufferSize);
    const char* getLastError() const { return _lastError; }
    
private:
    char _lastError[64] = {0};
    void setError(const char* error);
};

} // namespace DeviceBridge::Storage