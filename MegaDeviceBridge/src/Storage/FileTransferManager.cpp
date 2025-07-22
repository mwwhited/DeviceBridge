#include "FileTransferManager.h"
#include <string.h>

namespace DeviceBridge::Storage {

FileTransferManager::FileTransferManager() : _registry(FileSystemRegistry::getInstance()) {
    memset(_transferBuffer, 0, sizeof(_transferBuffer));
    memset(_lastError, 0, sizeof(_lastError));
}

bool FileTransferManager::copyTo(const char* filename, Common::StorageType sourceType, Common::StorageType destinationType) {
    return copyToWithNewName(filename, sourceType, filename, destinationType);
}

bool FileTransferManager::copyToWithNewName(const char* sourceFilename, Common::StorageType sourceType, 
                                          const char* destinationFilename, Common::StorageType destinationType) {
    if (!sourceFilename || !destinationFilename) {
        setError("Invalid filename parameters");
        return false;
    }
    
    if (sourceType.value == destinationType.value) {
        setError("Source and destination cannot be the same");
        return false;
    }
    
    // Get source filesystem
    IFileSystem* sourceFS = _registry.createFileSystem(sourceType);
    if (!sourceFS) {
        setError("Cannot create source filesystem");
        return false;
    }
    
    // Get destination filesystem  
    IFileSystem* destFS = _registry.createFileSystem(destinationType);
    if (!destFS) {
        delete sourceFS;
        setError("Cannot create destination filesystem");
        return false;
    }
    
    // Check if source file exists
    if (!sourceFS->fileExists(sourceFilename)) {
        delete sourceFS;
        delete destFS;
        setError("Source file not found");
        return false;
    }
    
    bool success = false;
    
    // Read from source
    if (sourceFS->readFile(sourceFilename, _transferBuffer, sizeof(_transferBuffer))) {
        // Create destination file
        if (destFS->createFile(destinationFilename)) {
            // Write to destination (each filesystem handles its own format)
            // For Serial: writeData will output hex stream with BEGIN/END delimiters
            // For SD/EEPROM: writeData will write binary data
            
            // Extract the actual data from the read buffer (skip header info)
            const char* dataStart = strstr(_transferBuffer, "\r\n");
            if (dataStart) {
                dataStart += 2; // Skip the \r\n
                // Look for second line (if present)
                const char* secondLine = strstr(dataStart, "\r\n");
                if (secondLine) {
                    dataStart = secondLine + 2; // Skip second header line
                }
            } else {
                dataStart = _transferBuffer; // No header found, use entire buffer
            }
            
            uint16_t dataLength = strlen(dataStart);
            if (dataLength > 0) {
                success = destFS->writeData((const uint8_t*)dataStart, dataLength);
            }
            
            // Close destination file
            destFS->closeFile();
        }
    }
    
    // Cleanup
    delete sourceFS;
    delete destFS;
    
    if (success) {
        snprintf(_lastError, sizeof(_lastError), "Copy successful: %s -> %s", 
                sourceFilename, destinationFilename);
    } else {
        setError("Copy operation failed");
    }
    
    return success;
}

bool FileTransferManager::copyAllFiles(Common::StorageType sourceType, Common::StorageType destinationType) {
    // Get source filesystem to list files
    IFileSystem* sourceFS = _registry.createFileSystem(sourceType);
    if (!sourceFS) {
        setError("Cannot create source filesystem");
        return false;
    }
    
    // Get file list
    char fileList[512];
    if (!sourceFS->listFiles(fileList, sizeof(fileList))) {
        delete sourceFS;
        setError("Cannot list source files");
        return false;
    }
    
    delete sourceFS;
    
    // Parse file list and copy each file
    // This is a simplified implementation - would need more robust parsing
    setError("Batch copy not yet implemented");
    return false;
}

void FileTransferManager::setProgressCallback(void (*callback)(uint32_t percent)) {
    _progressCallback = callback;
}

bool FileTransferManager::isTransferSupported(Common::StorageType sourceType, Common::StorageType destinationType) {
    // Check if both storage types are supported
    IFileSystemPlugin* sourcePlugin = _registry.getPlugin(sourceType);
    IFileSystemPlugin* destPlugin = _registry.getPlugin(destinationType);
    
    if (!sourcePlugin || !destPlugin) {
        return false;
    }
    
    // Source must support file reading
    if (!sourcePlugin->supportsFileReading()) {
        return false;
    }
    
    // Destination must be available for writing
    if (!destPlugin->canCreateFileSystem()) {
        return false;
    }
    
    return true;
}

void FileTransferManager::getSupportedTransfers(char* buffer, uint16_t bufferSize) {
    if (!buffer || bufferSize < 50) {
        return;
    }
    
    uint16_t offset = 0;
    offset += snprintf(buffer + offset, bufferSize - offset, "Supported transfers:\r\n");
    
    // List all possible transfers
    const char* storageNames[] = {"SD", "EEPROM", "Serial"};
    Common::StorageType storageTypes[] = {
        Common::StorageType::SD_CARD,
        Common::StorageType::EEPROM,
        Common::StorageType::SERIAL_TRANSFER
    };
    
    for (int src = 0; src < 3 && offset < bufferSize - 50; src++) {
        for (int dest = 0; dest < 3 && offset < bufferSize - 30; dest++) {
            if (src != dest) {
                bool supported = isTransferSupported(storageTypes[src], storageTypes[dest]);
                offset += snprintf(buffer + offset, bufferSize - offset,
                                 "  %s -> %s: %s\r\n",
                                 storageNames[src], storageNames[dest],
                                 supported ? "Yes" : "No");
            }
        }
    }
}

bool FileTransferManager::copyFileData(IFileSystem* source, IFileSystem* destination, const char* filename) {
    // This method would implement chunked copying for large files
    // Currently simplified to use the single buffer approach
    return false;
}

void FileTransferManager::reportProgress(uint32_t current, uint32_t total) {
    if (_progressCallback && total > 0) {
        uint32_t percent = (current * 100) / total;
        _progressCallback(percent);
    }
}

void FileTransferManager::setError(const char* error) {
    if (error) {
        strncpy(_lastError, error, sizeof(_lastError) - 1);
        _lastError[sizeof(_lastError) - 1] = '\0';
    }
}

} // namespace DeviceBridge::Storage