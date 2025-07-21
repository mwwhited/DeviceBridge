#pragma once

#include "IFileSystem.h"
#include "../Common/Config.h"
#include "../Components/W25Q128Manager.h"
#include <Arduino.h>

namespace DeviceBridge::Storage {

/**
 * @brief EEPROM file system implementation
 * 
 * Provides file operations for EEPROM storage with wear leveling,
 * circular buffer management, and sequential file organization.
 */
class EEPROMFileSystem : public IFileSystem {
private:
    DeviceBridge::Components::W25Q128Manager _eeprom;
    bool _initialized;
    uint16_t _currentAddress;
    uint16_t _fileStartAddress;
    uint16_t _availableSpace;
    
    // EEPROM layout constants
    static constexpr uint16_t HEADER_SIZE = 8;           // File header size
    static constexpr uint16_t MAX_FILENAME_SIZE = 32;    // Maximum filename length
    static constexpr uint16_t FILE_ENTRY_OVERHEAD = 48;  // Header + filename + metadata
    
    // File management
    struct FileHeader {
        uint16_t magic;        // File validity marker
        uint16_t fileSize;     // Size of file data
        uint8_t filenameLen;   // Length of filename
        uint8_t fileType;      // File type identifier
        uint16_t checksum;     // Data integrity checksum
    };
    
    // Private methods
    uint16_t calculateAvailableSpace();
    bool writeFileHeader(const char* filename, uint16_t dataSize);
    bool findNextWriteAddress();
    uint16_t calculateChecksum(const uint8_t* data, uint16_t length);
    bool verifyIntegrity(uint16_t address, uint16_t expectedSize);
    
public:
    EEPROMFileSystem();
    virtual ~EEPROMFileSystem();
    
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
        return Common::StorageType(Common::StorageType::EEPROM); 
    }
    const char* getStorageName() const override { return "EEPROM"; }
    bool isWriteProtected() const override { return false; } // EEPROM not write-protectable
    bool hasActiveFile() const override { return _hasActiveFile; }
    
    // Statistics
    uint32_t getBytesWritten() const override { return _bytesWritten; }
    uint32_t getFilesCreated() const override { return _filesCreated; }
    uint16_t getLastError() const override { return _lastError; }
    const char* getLastErrorMessage() const override { return _lastErrorMessage; }
    
    // EEPROM specific features
    bool format() override;
    bool flush() override { return true; }  // EEPROM writes are immediate
    bool sync() override { return true; }   // EEPROM writes are immediate
    
    // EEPROM-specific methods
    bool defragment();                      // Compact storage space
    uint16_t getWearLevel();               // Get wear leveling statistics
    bool verifyAllFiles();                 // Verify integrity of all stored files
};

} // namespace DeviceBridge::Storage