#pragma once

#include <stdint.h>
#include <string.h>
#include <Arduino.h>
#include "../Common/Types.h"

namespace DeviceBridge::Storage {

/**
 * @brief Abstract interface for all file system implementations
 * 
 * Provides a unified interface for SD Card, EEPROM, and Serial Transfer storage types.
 * Each storage implementation must provide these core operations while handling
 * their specific hardware requirements internally.
 */
class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    
    // Lifecycle management
    virtual bool initialize() = 0;
    virtual bool isAvailable() const = 0;
    virtual void shutdown() = 0;
    
    // File operations
    virtual bool createFile(const char* filename) = 0;
    virtual bool openFile(const char* filename, bool append = false) = 0;
    virtual bool writeData(const uint8_t* data, uint16_t length) = 0;
    virtual bool closeFile() = 0;
    virtual bool deleteFile(const char* filename) = 0;
    virtual bool fileExists(const char* filename) = 0;
    
    // Directory operations
    virtual bool listFiles(char* buffer, uint16_t bufferSize) = 0;
    virtual uint32_t getFileCount() = 0;
    virtual uint32_t getTotalSpace() = 0;
    virtual uint32_t getFreeSpace() = 0;
    
    // Status inquiry
    virtual Common::StorageType getStorageType() const = 0;
    virtual const char* getStorageName() const = 0;
    virtual bool isWriteProtected() const = 0;
    virtual bool hasActiveFile() const = 0;
    
    // Statistics
    virtual uint32_t getBytesWritten() const = 0;
    virtual uint32_t getFilesCreated() const = 0;
    virtual uint16_t getLastError() const = 0;
    virtual const char* getLastErrorMessage() const = 0;
    
    // Hardware-specific features (optional overrides)
    virtual bool format() { return false; } // Not supported by all storage types
    virtual bool flush() { return true; }   // Default: no-op for simple storage
    virtual bool sync() { return true; }    // Default: no-op for simple storage
    
protected:
    // Common error tracking
    uint16_t _lastError = 0;
    char _lastErrorMessage[64] = {0};
    uint32_t _bytesWritten = 0;
    uint32_t _filesCreated = 0;
    bool _hasActiveFile = false;
    
    void setError(uint16_t errorCode, const char* message) {
        _lastError = errorCode;
        strncpy(_lastErrorMessage, message, sizeof(_lastErrorMessage) - 1);
        _lastErrorMessage[sizeof(_lastErrorMessage) - 1] = '\0';
    }
    
    void clearError() {
        _lastError = 0;
        _lastErrorMessage[0] = '\0';
    }
};

// Error codes for IFileSystem implementations
namespace FileSystemErrors {
    static constexpr uint16_t NONE = 0;
    static constexpr uint16_t INIT_FAILED = 1;
    static constexpr uint16_t NOT_AVAILABLE = 2;
    static constexpr uint16_t FILE_CREATE_FAILED = 3;
    static constexpr uint16_t FILE_OPEN_FAILED = 4;
    static constexpr uint16_t FILE_WRITE_FAILED = 5;
    static constexpr uint16_t FILE_CLOSE_FAILED = 6;
    static constexpr uint16_t FILE_DELETE_FAILED = 7;
    static constexpr uint16_t DIRECTORY_READ_FAILED = 8;
    static constexpr uint16_t INSUFFICIENT_SPACE = 9;
    static constexpr uint16_t WRITE_PROTECTED = 10;
    static constexpr uint16_t HARDWARE_ERROR = 11;
    static constexpr uint16_t INVALID_FILENAME = 12;
    static constexpr uint16_t FILE_NOT_FOUND = 13;
    // Additional error codes for LittleFS support
    static constexpr uint16_t INVALID_PARAMETER = 14;
    static constexpr uint16_t CORRUPTION_DETECTED = 15;
    static constexpr uint16_t FILE_EXISTS = 16;
    static constexpr uint16_t INVALID_PATH = 17;
    static constexpr uint16_t DIRECTORY_NOT_EMPTY = 18;
    static constexpr uint16_t FILE_TOO_LARGE = 19;
    static constexpr uint16_t OUT_OF_MEMORY = 20;
    static constexpr uint16_t UNKNOWN_ERROR = 21;
}

} // namespace DeviceBridge::Storage