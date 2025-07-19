#pragma once

#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <SD.h>
#include "W25Q128Manager.h"
#include "../Common/Types.h"
#include "../Common/Config.h"

namespace DeviceBridge::Components {

class FileSystemManager {
private:
    QueueHandle_t _dataQueue;
    QueueHandle_t _displayQueue;
    SemaphoreHandle_t _spiMutex;
    TaskHandle_t _taskHandle;
    
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
    Common::FileType _fileType;
    
    // Task function (static for FreeRTOS)
    static void taskFunction(void* pvParameters);
    
    // Instance handling for task function
    void runTask();
    
    // Storage operations
    bool initializeSD();
    bool initializeEEPROM();
    bool createNewFile();
    bool writeDataChunk(const Common::DataChunk& chunk);
    bool closeCurrentFile();
    
    // Storage management
    bool switchStorage(Common::StorageType newType);
    void sendDisplayMessage(Common::DisplayMessage::Type type, const char* message);
    
    // File naming
    void generateFilename(char* buffer, size_t bufferSize);
    const char* getFileExtension() const;
    
public:
    FileSystemManager(QueueHandle_t dataQueue, QueueHandle_t displayQueue, SemaphoreHandle_t spiMutex);
    ~FileSystemManager();
    
    // Lifecycle management
    bool initialize();
    bool start();
    void stop();
    
    // Configuration
    void setPreferredStorage(Common::StorageType storage) { _preferredStorage = storage; }
    void setFileType(Common::FileType type) { _fileType = type; }
    
    // Status inquiry
    Common::StorageType getActiveStorage() const { return _activeStorage; }
    bool isSDAvailable() const { return _sdAvailable; }
    bool isEEPROMAvailable() const { return _eepromAvailable; }
    TaskHandle_t getTaskHandle() const { return _taskHandle; }
    
    // Statistics
    uint32_t getFilesStored() const { return _fileCounter; }
    const char* getCurrentFilename() const { return _currentFilename; }
    
private:
    // Statistics
    uint32_t _totalBytesWritten;
    uint32_t _writeErrors;
};

} // namespace DeviceBridge::Components