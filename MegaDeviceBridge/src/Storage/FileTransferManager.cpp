#include "FileTransferManager.h"
#include <string.h>
#include "../Common/ServiceLocator.h"
#include "EEPROMFileSystem.h"
#include "SerialTransferFileSystem.h"

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
    
    // Get FileSystemManager instance to avoid plugin registry
    auto& services = DeviceBridge::ServiceLocator::getInstance();
    auto* fileSystemManager = services.getFileSystemManager();
    if (!fileSystemManager) {
        setError("FileSystemManager not available");
        return false;
    }
    
    // For now, use a simplified approach that works directly with the existing storage
    // This avoids the plugin registry memory overhead
    if (sourceType.value != Common::StorageType::EEPROM || destinationType.value != Common::StorageType::SERIAL_TRANSFER) {
        setError("Only EEPROM to Serial transfer currently supported");
        return false;
    }
    
    // Get the active EEPROM filesystem and create a Serial transfer filesystem
    IFileSystem* sourceFS = nullptr;
    IFileSystem* destFS = nullptr;
    
    // Create temporary filesystem instances for transfer
    static DeviceBridge::Storage::EEPROMFileSystem eepromFS;
    static DeviceBridge::Storage::SerialTransferFileSystem serialFS;
    
    sourceFS = &eepromFS;
    destFS = &serialFS;
    
    // Initialize them if needed
    if (!sourceFS->isAvailable()) {
        if (!sourceFS->initialize()) {
            setError("Cannot initialize source filesystem");
            return false;
        }
    }
    
    if (!destFS->isAvailable()) {
        if (!destFS->initialize()) {
            setError("Cannot initialize destination filesystem");
            return false;
        }
    }
    
    // Check if source file exists
    if (!sourceFS->fileExists(sourceFilename)) {
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
            
            // Handle different source data formats
            uint16_t dataLength = 0;
            uint8_t binaryBuffer[128]; // Buffer for converted binary data
            
            if (sourceType.value == Common::StorageType::EEPROM) {
                // EEPROM returns formatted text with hex data
                // Format: "File: filename (size bytes)\r\n[hex data]"
                const char* dataStart = strstr(_transferBuffer, "\r\n");
                if (dataStart) {
                    dataStart += 2; // Skip the \r\n
                    // Skip optional second header line
                    const char* secondLine = strstr(dataStart, "\r\n");
                    if (secondLine) {
                        dataStart = secondLine + 2;
                    }
                    
                    // Convert hex string to binary data
                    dataLength = 0;
                    const char* hexPtr = dataStart;
                    while (*hexPtr && dataLength < sizeof(binaryBuffer)) {
                        // Skip whitespace and line breaks
                        if (*hexPtr == ' ' || *hexPtr == '\t' || *hexPtr == '\r' || *hexPtr == '\n') {
                            hexPtr++;
                            continue;
                        }
                        
                        // Parse hex pair
                        if (hexPtr[0] && hexPtr[1]) {
                            uint8_t byte = 0;
                            // Parse first hex digit
                            if (hexPtr[0] >= '0' && hexPtr[0] <= '9') {
                                byte = (hexPtr[0] - '0') << 4;
                            } else if (hexPtr[0] >= 'A' && hexPtr[0] <= 'F') {
                                byte = (hexPtr[0] - 'A' + 10) << 4;
                            } else if (hexPtr[0] >= 'a' && hexPtr[0] <= 'f') {
                                byte = (hexPtr[0] - 'a' + 10) << 4;
                            } else {
                                break; // Invalid hex character
                            }
                            
                            // Parse second hex digit
                            if (hexPtr[1] >= '0' && hexPtr[1] <= '9') {
                                byte |= (hexPtr[1] - '0');
                            } else if (hexPtr[1] >= 'A' && hexPtr[1] <= 'F') {
                                byte |= (hexPtr[1] - 'A' + 10);
                            } else if (hexPtr[1] >= 'a' && hexPtr[1] <= 'f') {
                                byte |= (hexPtr[1] - 'a' + 10);
                            } else {
                                break; // Invalid hex character
                            }
                            
                            binaryBuffer[dataLength++] = byte;
                            hexPtr += 2;
                        } else {
                            break; // Incomplete hex pair
                        }
                    }
                    
                    if (dataLength > 0) {
                        success = destFS->writeData(binaryBuffer, dataLength);
                    } else {
                        setError("No data to transfer");
                    }
                } else {
                    setError("Could not parse EEPROM file format");
                }
            } else {
                // For other storage types, use raw data
                dataLength = strlen(_transferBuffer);
                if (dataLength > 0) {
                    success = destFS->writeData((const uint8_t*)_transferBuffer, dataLength);
                }
            }
            
            // Close destination file
            destFS->closeFile();
        } else {
            setError("Failed to create destination file");
        }
    } else {
        setError("Failed to read source file");
    }
    
    // No cleanup needed for static instances
    
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