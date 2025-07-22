#pragma once

#include "IFileSystem.h"
#include "W25Q128BlockDevice.h"
#include "../Common/Config.h"
#include "../Components/W25Q128Manager.h"
#include <Arduino.h>
#include "lfs.h"

namespace DeviceBridge::Storage {

/**
 * @brief EEPROM file system implementation using LittleFS
 * 
 * Provides a robust file system for W25Q128 EEPROM storage with:
 * - Wear leveling and power-loss resilience
 * - Directory structure support
 * - File operations (create, read, write, delete)
 * - Metadata and file listing capabilities
 */
class EEPROMFileSystem : public IFileSystem {
private:
    DeviceBridge::Components::W25Q128Manager _eeprom;
    W25Q128BlockDevice _blockDevice;
    lfs_t _lfs;
    lfs_config _lfsConfig;
    lfs_file_t _currentFile;
    bool _initialized;
    bool _mounted;
    char _currentFilename[LFS_NAME_MAX];
    
    // LittleFS configuration constants
    static constexpr uint32_t LFS_BLOCK_CYCLES = 500;    // Wear leveling cycles
    static constexpr uint32_t LFS_CACHE_SIZE = 256;      // Cache size
    static constexpr uint32_t LFS_LOOKAHEAD_SIZE = 128;  // Lookahead buffer
    
    // File system statistics
    uint32_t _totalFiles;
    uint32_t _totalSpace;
    uint32_t _usedSpace;
    
    // Private methods
    bool initializeLittleFS();
    bool mountFileSystem();
    void unmountFileSystem();
    bool formatFileSystem();
    void updateStatistics();
    int convertLfsError(int lfsError);
    const char* getLfsErrorMessage(int lfsError);
    
public:
    EEPROMFileSystem();
    virtual ~EEPROMFileSystem();
    
    // Lifecycle management
    bool initialize() override final;
    bool isAvailable() const override final;
    void shutdown() override final;
    
    // File operations
    bool createFile(const char* filename) override final;
    bool openFile(const char* filename, bool append = false) override final;
    bool writeData(const uint8_t* data, uint16_t length) override final;
    bool closeFile() override final;
    bool deleteFile(const char* filename) override final;
    bool fileExists(const char* filename) override final;
    
    // Directory operations
    bool listFiles(char* buffer, uint16_t bufferSize) override final;
    uint32_t getFileCount() override final;
    uint32_t getTotalSpace() override final;
    uint32_t getFreeSpace() override final;
    
    // Status inquiry
    Common::StorageType getStorageType() const override { 
        return Common::StorageType(Common::StorageType::EEPROM); 
    }
    const char* getStorageName() const override { return "LittleFS-EEPROM"; }
    bool isWriteProtected() const override { return false; } // EEPROM not write-protectable
    bool hasActiveFile() const override { return _hasActiveFile; }
    
    // Statistics
    uint32_t getBytesWritten() const override { return _bytesWritten; }
    uint32_t getFilesCreated() const override { return _filesCreated; }
    uint16_t getLastError() const override { return _lastError; }
    const char* getLastErrorMessage() const override { return _lastErrorMessage; }
    
    // LittleFS-enhanced features
    bool format() override final;
    bool flush() override final;             // Sync file system to storage
    bool sync() override final;              // Sync file system to storage
    
    // LittleFS-specific methods
    bool createDirectory(const char* path);  // Create directory
    bool removeDirectory(const char* path);  // Remove directory
    bool isDirectory(const char* path);      // Check if path is directory
    uint32_t getFileSize(const char* filename); // Get file size
    bool renameFile(const char* oldname, const char* newname); // Rename file
    bool copyFile(const char* src, const char* dst); // Copy file
    
    // Advanced file operations
    int readFile(const char* filename, uint8_t* buffer, uint32_t bufferSize); // Read entire file
    bool writeFile(const char* filename, const uint8_t* data, uint32_t length); // Write entire file
    bool appendToFile(const char* filename, const uint8_t* data, uint32_t length); // Append to file
    
    // File system integrity and maintenance
    bool checkFileSystem();                 // Check file system integrity
    uint32_t getBlockSize() const;          // Get block size
    uint32_t getTotalBlocks() const;        // Get total block count
    uint32_t getFreeBlocks() const;         // Get free block count
    uint32_t getBadBlocks() const;          // Get bad block count
    float getFragmentation() const;         // Get fragmentation percentage
};

} // namespace DeviceBridge::Storage