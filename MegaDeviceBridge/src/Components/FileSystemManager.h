#pragma once

#include <Arduino.h>
#include <SD.h>
#include "W25Q128Manager.h"
#include "../Common/Types.h"
#include "../Common/Config.h"

namespace DeviceBridge::Components {

// Forward declaration for callback
class DisplayManager;

class FileSystemManager {
private:
    DisplayManager* _displayManager;
    
    // Storage instances  
    File _currentFile;
    W25Q128Manager _eeprom;
    
    // Storage status
    bool _sdAvailable;
    bool _eepromAvailable;
    
    // EEPROM file management
    uint32_t _eepromCurrentAddress;
    uint32_t _eepromWriteBuffer[64]; // 256-byte buffer (64 * 4 bytes)
    uint16_t _eepromBufferIndex;
    Common::StorageType _activeStorage;
    Common::StorageType _preferredStorage;
    
    // File management
    uint32_t _fileCounter;
    char _currentFilename[Common::Limits::MAX_FILENAME_LENGTH];
    Common::FileType _fileType;          // Requested/configured file type
    Common::FileType _detectedFileType;  // Auto-detected file type (if auto-detection enabled)
    bool _isFileOpen;
    
    // Storage operations
    bool initializeSD();
    bool initializeEEPROM();
    bool createNewFile();
    bool writeDataChunk(const Common::DataChunk& chunk);
    bool closeCurrentFile();
    
    // Storage management
    bool switchStorage(Common::StorageType newType);
    void sendDisplayMessage(Common::DisplayMessage::Type type, const char* message);
    void sendDisplayMessage(Common::DisplayMessage::Type type, const __FlashStringHelper* message);
    
    // File naming
    void generateFilename(char* buffer, size_t bufferSize);
    const char* getFileExtension() const;
    
    // File type detection
    Common::FileType detectFileType(const uint8_t* data, uint16_t length);
    
public:
    FileSystemManager();
    ~FileSystemManager();
    
    // Set callback for display messages
    void setDisplayManager(DisplayManager* manager) { _displayManager = manager; }
    
    // Lifecycle management
    bool initialize();
    void update();  // Called from main loop
    void stop();
    
    // Data processing (called by ParallelPortManager)
    void processDataChunk(const Common::DataChunk& chunk);
    
    // Configuration
    void setPreferredStorage(Common::StorageType storage) { _preferredStorage.value = storage.value; }
    void setStorageType(Common::StorageType type);
    void setFileType(Common::FileType type) { _fileType.value = type.value; }
    
    // Status inquiry
    Common::StorageType getActiveStorage() const { return _activeStorage; }
    Common::StorageType getCurrentStorageType() const { return _activeStorage; }  // Alias for serial interface
    Common::FileType getFileType() const { return _fileType; }
    Common::FileType getDetectedFileType() const { return _detectedFileType; }
    bool isSDAvailable() const { return _sdAvailable; }
    bool isEEPROMAvailable() const { return _eepromAvailable; }
    
    // Statistics
    uint32_t getFilesStored() const { return _fileCounter; }
    const char* getCurrentFilename() const { return _currentFilename; }
    uint32_t getTotalBytesWritten() const { return _totalBytesWritten; }
    uint16_t getWriteErrors() const { return _writeErrors; }
    
private:
    // Statistics
    uint32_t _totalBytesWritten;
    uint16_t _writeErrors;
};

} // namespace DeviceBridge::Components