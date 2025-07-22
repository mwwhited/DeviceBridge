#pragma once

#include <Arduino.h>
#include "IFileSystem.h"
#include "../Components/W25Q128Manager.h"

namespace DeviceBridge::Storage {

/**
 * @brief Simple EEPROM file system implementation using direct W25Q128 access
 * 
 * Provides a lightweight file system for W25Q128 EEPROM storage with:
 * - Simple file operations (create, read, write, delete)
 * - Minimal memory footprint optimized for Arduino Mega (1.5KB directory)
 * - Direct flash memory access without complex filesystem overhead
 * - Sequential file allocation with simple directory structure
 */
class EEPROMFileSystem : public IFileSystem {
private:
    DeviceBridge::Components::W25Q128Manager _eeprom;
    bool _initialized;
    bool _mounted;
    uint32_t _currentFileAddress;
    uint32_t _currentFileSize;
    char _currentFilename[64];
    
    // Simple filesystem constants
    static constexpr uint32_t FLASH_SIZE = 16 * 1024 * 1024;  // 16MB W25Q128
    static constexpr uint32_t SECTOR_SIZE = 4096;             // 4KB sectors
    static constexpr uint32_t DIRECTORY_ADDRESS = 0x0000;     // Directory at start of flash
    static constexpr uint32_t DIRECTORY_SIZE = SECTOR_SIZE;   // One sector for directory
    static constexpr uint32_t FILE_DATA_START = SECTOR_SIZE;  // Files start after directory
    static constexpr uint16_t MAX_FILES = 32;                 // Maximum number of files (reduced for RAM constraints)
    static constexpr uint8_t FILENAME_LENGTH = 32;            // Maximum filename length
    
    // Directory entry structure (48 bytes per entry)
    struct DirectoryEntry {
        char filename[FILENAME_LENGTH];  // 32 bytes - filename
        uint32_t address;               // 4 bytes - file start address  
        uint32_t size;                  // 4 bytes - file size
        uint8_t flags;                  // 1 byte - file flags (used/deleted/etc)
        uint8_t reserved[7];            // 7 bytes - reserved for future use
    } __attribute__((packed));
    
    static_assert(sizeof(DirectoryEntry) == 48, "DirectoryEntry must be 48 bytes");
    
    // File flags
    static constexpr uint8_t FLAG_UNUSED = 0x00;
    static constexpr uint8_t FLAG_USED = 0x01;
    static constexpr uint8_t FLAG_DELETED = 0x02;
    
    // File system statistics
    uint32_t _totalFiles;
    uint32_t _totalSpace;
    uint32_t _usedSpace;
    uint32_t _nextFreeAddress;
    
    // Private methods
    bool initializeDirectory();
    bool loadDirectory();
    bool saveDirectory();
    int findFileEntry(const char* filename);
    int findFreeEntry();
    uint32_t calculateNextFreeAddress();
    bool eraseFile(uint32_t address, uint32_t size);
    
public:
    EEPROMFileSystem();
    ~EEPROMFileSystem();
    
    // IFileSystem interface implementation
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
    bool listFiles(char* buffer, uint16_t bufferSize) override;
    
    // File system information
    uint32_t getFileCount() override;
    uint32_t getTotalSpace() override;
    uint32_t getFreeSpace() override;
    bool format() override;
    bool flush() override;
    bool sync() override;
    
    // Simple filesystem methods
    uint32_t getFileSize(const char* filename);
    bool defragment();
    
    // IFileSystem interface - missing pure virtual methods
    Common::StorageType getStorageType() const override { return Common::StorageType::EEPROM; }
    const char* getStorageName() const override { return "EEPROM Simple"; }
    bool isWriteProtected() const override { return false; } // W25Q128 is not write-protected
    bool hasActiveFile() const override { return _hasActiveFile; }
    uint32_t getBytesWritten() const override { return _bytesWritten; }
    uint32_t getFilesCreated() const override { return _filesCreated; }
    uint16_t getLastError() const override { return _lastError; }
    const char* getLastErrorMessage() const override { return _lastErrorMessage; }
    
private:
    DirectoryEntry _directory[MAX_FILES];
    bool _directoryLoaded;
    bool _directoryModified;
};

} // namespace DeviceBridge::Storage