#include "FileSystemManager.h"
#include "DisplayManager.h"
#include <string.h>
#include <stdio.h>
#include <Arduino.h>

namespace DeviceBridge::Components {

FileSystemManager::FileSystemManager()
    : _displayManager(nullptr)
    , _eeprom(Common::Pins::EEPROM_CS)
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
    , _isFileOpen(false)
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

void FileSystemManager::update() {
    // Periodic storage health check (called from main loop)
    // Can be used for background maintenance tasks if needed
}

void FileSystemManager::stop() {
    closeCurrentFile();
}

void FileSystemManager::processDataChunk(const Common::DataChunk& chunk) {
    // Handle new file
    if (chunk.isNewFile) {
        closeCurrentFile();
        if (!createNewFile()) {
            sendDisplayMessage(Common::DisplayMessage::ERROR, "File Create Failed");
            return;
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

bool FileSystemManager::initializeSD() {
    return SD.begin(Common::Pins::SD_CS);
}

bool FileSystemManager::initializeEEPROM() {
    return _eeprom.initialize();
}

bool FileSystemManager::createNewFile() {
    generateFilename(_currentFilename, sizeof(_currentFilename));
    
    switch (_activeStorage) {
        case Common::StorageType::SD_CARD:
            if (_sdAvailable) {
                _currentFile = SD.open(_currentFilename, FILE_WRITE);
                _isFileOpen = (_currentFile != 0);
                return _isFileOpen;
            }
            break;
            
        case Common::StorageType::EEPROM:
            // TODO: Implement EEPROM file creation
            _isFileOpen = false;
            return false;
            
        case Common::StorageType::SERIAL_TRANSFER:
            // For serial transfer, we'll send data directly
            _isFileOpen = true;
            return true;
            
        default:
            _isFileOpen = false;
            return false;
    }
    
    _isFileOpen = false;
    return false;
}

bool FileSystemManager::writeDataChunk(const Common::DataChunk& chunk) {
    if (!_isFileOpen) {
        return false;
    }
    
    switch (_activeStorage) {
        case Common::StorageType::SD_CARD:
            if (_currentFile) {
                size_t written = _currentFile.write(chunk.data, chunk.length);
                _currentFile.flush(); // Ensure data is written
                
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
    if (!_isFileOpen) {
        return true; // Already closed
    }
    
    bool result = true;
    
    switch (_activeStorage) {
        case Common::StorageType::SD_CARD:
            if (_currentFile) {
                _currentFile.close();
            }
            break;
            
        case Common::StorageType::EEPROM:
            // TODO: Implement EEPROM file close
            break;
            
        case Common::StorageType::SERIAL_TRANSFER:
            // Nothing to close for serial transfer
            break;
    }
    
    _isFileOpen = false;
    return result;
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
    if (_displayManager) {
        _displayManager->displayMessage(type, message);
    }
}

void FileSystemManager::setStorageType(Common::StorageType type) {
    if (_activeStorage != type) {
        closeCurrentFile(); // Close any open file before switching
        _activeStorage = type;
        
        // Verify the new storage type is available
        switch (type) {
            case Common::StorageType::SD_CARD:
                if (!_sdAvailable) {
                    sendDisplayMessage(Common::DisplayMessage::ERROR, "SD Not Available");
                    _activeStorage = Common::StorageType::SERIAL_TRANSFER;
                }
                break;
                
            case Common::StorageType::EEPROM:
                if (!_eepromAvailable) {
                    sendDisplayMessage(Common::DisplayMessage::ERROR, "EEPROM Not Available");
                    _activeStorage = Common::StorageType::SERIAL_TRANSFER;
                }
                break;
                
            case Common::StorageType::SERIAL_TRANSFER:
                // Always available
                break;
                
            case Common::StorageType::AUTO_SELECT:
                if (_sdAvailable) {
                    _activeStorage = Common::StorageType::SD_CARD;
                } else if (_eepromAvailable) {
                    _activeStorage = Common::StorageType::EEPROM;
                } else {
                    _activeStorage = Common::StorageType::SERIAL_TRANSFER;
                }
                break;
        }
    }
}

} // namespace DeviceBridge::Components