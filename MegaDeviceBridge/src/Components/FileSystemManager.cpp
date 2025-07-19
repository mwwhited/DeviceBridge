#include "FileSystemManager.h"
#include <string.h>
#include <stdio.h>

namespace DeviceBridge::Components {

FileSystemManager::FileSystemManager(QueueHandle_t dataQueue, QueueHandle_t displayQueue, SemaphoreHandle_t spiMutex)
    : _dataQueue(dataQueue)
    , _displayQueue(displayQueue)
    , _spiMutex(spiMutex)
    , _taskHandle(nullptr)
    , _eeprom(Common::Pins::EEPROM_CS, spiMutex)
    , _sdAvailable(false)
    , _eepromAvailable(false)
    , _eepromCurrentAddress(0)
    , _eepromBufferIndex(0)
    , _activeStorage(Common::StorageType::AUTO_SELECT)
    , _preferredStorage(Common::StorageType::SD_CARD)
    , _fileCounter(0)
    , _fileType(Common::FileType::AUTO_DETECT)
    , _totalBytesWritten(0)
    , _writeErrors(0)
{
    memset(_currentFilename, 0, sizeof(_currentFilename));
}

FileSystemManager::~FileSystemManager() {
    stop();
}

bool FileSystemManager::initialize() {
    _sdAvailable = initializeSD();
    _eepromAvailable = initializeEEPROM();
    
    // Select initial storage type
    if (_preferredStorage == Common::StorageType::SD_CARD && _sdAvailable) {
        _activeStorage = Common::StorageType::SD_CARD;
    } else if (_preferredStorage == Common::StorageType::EEPROM && _eepromAvailable) {
        _activeStorage = Common::StorageType::EEPROM;
    } else if (_sdAvailable) {
        _activeStorage = Common::StorageType::SD_CARD;
    } else if (_eepromAvailable) {
        _activeStorage = Common::StorageType::EEPROM;
    } else {
        _activeStorage = Common::StorageType::SERIAL_TRANSFER;
        sendDisplayMessage(Common::DisplayMessage::ERROR, "No Storage!");
    }
    
    return _sdAvailable || _eepromAvailable;
}

bool FileSystemManager::start() {
    if (_taskHandle != nullptr) {
        return false; // Already running
    }
    
    BaseType_t result = xTaskCreate(
        taskFunction,
        "FileManager",
        Common::RTOS::FILE_MANAGER_STACK,
        this,
        Common::RTOS::FILE_MANAGER_PRIORITY,
        &_taskHandle
    );
    
    return result == pdPASS;
}

void FileSystemManager::stop() {
    if (_taskHandle != nullptr) {
        // Close any open file first
        closeCurrentFile();
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
}

void FileSystemManager::taskFunction(void* pvParameters) {
    FileSystemManager* manager = static_cast<FileSystemManager*>(pvParameters);
    manager->runTask();
}

void FileSystemManager::runTask() {
    Common::DataChunk chunk;
    
    for (;;) {
        if (xQueueReceive(_dataQueue, &chunk, portMAX_DELAY) == pdTRUE) {
            
            // Handle new file
            if (chunk.isNewFile) {
                closeCurrentFile();
                if (!createNewFile()) {
                    sendDisplayMessage(Common::DisplayMessage::ERROR, "File Create Failed");
                    continue;
                }
                sendDisplayMessage(Common::DisplayMessage::STATUS, "Storing...");
            }
            
            // Write data
            if (chunk.length > 0) {
                if (!writeDataChunk(chunk)) {
                    _writeErrors++;
                    sendDisplayMessage(Common::DisplayMessage::ERROR, "Write Failed");
                }
            }
            
            // Handle end of file
            if (chunk.isEndOfFile) {
                if (closeCurrentFile()) {
                    char message[32];
                    snprintf(message, sizeof(message), "Saved: %s", _currentFilename);
                    sendDisplayMessage(Common::DisplayMessage::INFO, message);
                } else {
                    sendDisplayMessage(Common::DisplayMessage::ERROR, "Close Failed");
                }
            }
        }
    }
}

bool FileSystemManager::initializeSD() {
    if (xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(Common::FileSystem::SD_TIMEOUT_MS)) == pdTRUE) {
        bool result = _sd.begin(Common::Pins::SD_CS);
        xSemaphoreGive(_spiMutex);
        return result;
    }
    return false;
}

bool FileSystemManager::initializeEEPROM() {
    return _eeprom.initialize();
}

bool FileSystemManager::createNewFile() {
    generateFilename(_currentFilename, sizeof(_currentFilename));
    
    switch (_activeStorage) {
        case Common::StorageType::SD_CARD:
            if (_sdAvailable && xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(Common::FileSystem::SD_TIMEOUT_MS)) == pdTRUE) {
                bool result = _currentFile.open(_currentFilename, O_CREAT | O_WRITE | O_TRUNC);
                xSemaphoreGive(_spiMutex);
                return result;
            }
            break;
            
        case Common::StorageType::EEPROM:
            // TODO: Implement EEPROM file creation
            return false;
            
        case Common::StorageType::SERIAL_TRANSFER:
            // For serial transfer, we'll send data directly
            return true;
            
        default:
            return false;
    }
    
    return false;
}

bool FileSystemManager::writeDataChunk(const Common::DataChunk& chunk) {
    switch (_activeStorage) {
        case Common::StorageType::SD_CARD:
            if (_currentFile.isOpen() && xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(Common::FileSystem::SD_TIMEOUT_MS)) == pdTRUE) {
                size_t written = _currentFile.write(chunk.data, chunk.length);
                _currentFile.sync(); // Ensure data is written
                xSemaphoreGive(_spiMutex);
                
                if (written == chunk.length) {
                    _totalBytesWritten += chunk.length;
                    return true;
                }
            }
            break;
            
        case Common::StorageType::EEPROM:
            // TODO: Implement EEPROM write
            return false;
            
        case Common::StorageType::SERIAL_TRANSFER:
            // TODO: Implement serial transfer
            return true;
            
        default:
            return false;
    }
    
    return false;
}

bool FileSystemManager::closeCurrentFile() {
    switch (_activeStorage) {
        case Common::StorageType::SD_CARD:
            if (_currentFile.isOpen() && xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(Common::FileSystem::SD_TIMEOUT_MS)) == pdTRUE) {
                _currentFile.close();
                xSemaphoreGive(_spiMutex);
                return true;
            }
            break;
            
        case Common::StorageType::EEPROM:
            // TODO: Implement EEPROM file close
            return false;
            
        case Common::StorageType::SERIAL_TRANSFER:
            return true;
            
        default:
            return false;
    }
    
    return true; // Consider it closed if file wasn't open
}

void FileSystemManager::generateFilename(char* buffer, size_t bufferSize) {
    const char* extension = getFileExtension();
    snprintf(buffer, bufferSize, "%s%04lu%s", 
             Common::FileSystem::DEFAULT_FILE_PREFIX, 
             _fileCounter++, 
             extension);
}

const char* FileSystemManager::getFileExtension() const {
    switch (_fileType) {
        case Common::FileType::BITMAP:
            return ".bmp";
        case Common::FileType::PNG:
            return ".png";
        case Common::FileType::TEXT:
            return ".txt";
        case Common::FileType::BINARY:
        case Common::FileType::AUTO_DETECT:
        default:
            return Common::FileSystem::DEFAULT_FILE_EXTENSION;
    }
}

void FileSystemManager::sendDisplayMessage(Common::DisplayMessage::Type type, const char* message) {
    Common::DisplayMessage msg;
    msg.type = type;
    strncpy(msg.message, message, sizeof(msg.message) - 1);
    msg.message[sizeof(msg.message) - 1] = '\0';
    
    xQueueSend(_displayQueue, &msg, 0); // Non-blocking
}

} // namespace DeviceBridge::Components