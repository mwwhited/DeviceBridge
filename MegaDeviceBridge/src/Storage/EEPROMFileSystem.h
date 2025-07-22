#pragma once

#include <Arduino.h>
#include "IFileSystem.h"
#include "../Components/W25Q128Manager.h"

namespace DeviceBridge::Storage {

/**
 * @brief Ultra-minimal EEPROM file system with no FAT caching
 * 
 * Features:
 * - Zero RAM directory caching (on-demand EEPROM scanning)
 * - Single directory with filename format: "00001122\334455.EXT"
 * - Basic operations: list, write, read segments, delete
 * - Optimized for Arduino Mega memory constraints
 */
class EEPROMFileSystem : public IFileSystem {
public:
    // Filesystem constants (public for older C++ standard compatibility)
    static constexpr uint32_t FLASH_SIZE = 16UL * 1024UL * 1024UL;  // 16MB W25Q128
    static constexpr uint32_t SECTOR_SIZE = 4096UL;             // 4KB sectors
    static constexpr uint8_t FILENAME_LENGTH = 32;            // "20250722/161810.bin" + margin
    static constexpr uint32_t DIRECTORY_ENTRIES_PER_SECTOR = SECTOR_SIZE / 48UL;  // 85 entries per sector
    static constexpr uint32_t MAX_FILES = 256UL;                // Total file limit
    static constexpr uint32_t FILE_DATA_START = 8192UL;        // Start after 2 directory sectors

private:
    
    DeviceBridge::Components::W25Q128Manager _eeprom;
    bool _initialized;
    bool _mounted;
    uint32_t _currentFileAddress;
    uint32_t _currentFileSize;
    char _currentFilename[FILENAME_LENGTH]; // Full path support "20250722/161810.bin"
    
    // Compact directory entry (48 bytes)
    struct DirectoryEntry {
        char filename[FILENAME_LENGTH]; // 32 bytes - "20250722/161810.bin" + margin
        uint32_t address;              // 4 bytes - file start address
        uint32_t size;                 // 4 bytes - file size
        uint32_t crc32;                // 4 bytes - filename CRC for quick lookup
        uint32_t reserved;             // 4 bytes - reserved/flags
    } __attribute__((packed));
    
    static_assert(sizeof(DirectoryEntry) == 48, "DirectoryEntry must be 48 bytes");
    
    // File flags in reserved field
    static constexpr uint32_t FLAG_UNUSED = 0x00000000;
    static constexpr uint32_t FLAG_USED = 0x55aa55aa;
    static constexpr uint32_t FLAG_DELETED = 0xffffffff;
    
    // Private methods - no FAT caching
    int scanForFile(const char* filename);
    int findFreeDirectorySlot();
    bool readDirectoryEntry(int index, DirectoryEntry& entry);
    bool writeDirectoryEntry(int index, const DirectoryEntry& entry, bool allowUpdate = false);
    bool isValidFilename(const char* filename);
    uint32_t calculateCRC32(const char* filename);
    uint32_t findNextFreeFileAddress();
    
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
    
    // Custom methods for minimal filesystem
    uint32_t getFileSize(const char* filename);
    bool readFileSegment(const char* filename, uint32_t offset, uint8_t* buffer, uint16_t length);
    
    // Enhanced file operations for feature requests
    bool readFile(const char* filename, char* buffer, uint16_t bufferSize) override;
    
    // IFileSystem interface implementation
    Common::StorageType getStorageType() const override { return Common::StorageType::EEPROM; }
    const char* getStorageName() const override { return "EEPROM Minimal"; }
    bool isWriteProtected() const override { return false; }
    bool hasActiveFile() const override { return _hasActiveFile; }
    uint32_t getBytesWritten() const override { return _bytesWritten; }
    uint32_t getFilesCreated() const override { return _filesCreated; }
    uint16_t getLastError() const override { return _lastError; }
    const char* getLastErrorMessage() const override { return _lastErrorMessage; }
};

} // namespace DeviceBridge::Storage