#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "TimeManager.h"
#include "ParallelPortManager.h"
#include <string.h>
#include <stdio.h>
#include <Arduino.h>

namespace DeviceBridge::Components {

FileSystemManager::FileSystemManager()
    : _eeprom(Common::Pins::EEPROM_CS)
    , _sdAvailable(false)
    , _eepromAvailable(false)
    , _eepromCurrentAddress(0)
    , _eepromBufferIndex(0)
    , _activeStorage(Common::StorageType::AUTO_SELECT)
    , _preferredStorage(Common::StorageType::SD_CARD)
    , _fileCounter(0)
    , _fileType(Common::FileType::AUTO_DETECT)
    , _detectedFileType(Common::FileType::AUTO_DETECT)
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
    if (_preferredStorage.value == Common::StorageType::SD_CARD && _sdAvailable) {
        _activeStorage = Common::StorageType::SD_CARD;
    } else if (_preferredStorage.value == Common::StorageType::EEPROM && _eepromAvailable) {
        _activeStorage = Common::StorageType::EEPROM;
    } else if (_sdAvailable) {
        _activeStorage = Common::StorageType::SD_CARD;
    } else if (_eepromAvailable) {
        _activeStorage = Common::StorageType::EEPROM;
    } else {
        _activeStorage = Common::StorageType::SERIAL_TRANSFER;
        sendDisplayMessage(Common::DisplayMessage::ERROR, F("No Storage!"));
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
            sendDisplayMessage(Common::DisplayMessage::ERROR, F("File Create Failed"));
            return;
        }
        sendDisplayMessage(Common::DisplayMessage::STATUS, F("Storing..."));
        
        // Detect file type from first chunk if auto-detection is enabled
        if (_fileType.value == Common::FileType::AUTO_DETECT && chunk.length > 0) {
            _detectedFileType = detectFileType(chunk.data, chunk.length);
        } else {
            _detectedFileType = _fileType;  // Use configured type
        }
    }
    
    // Write data
    if (chunk.length > 0) {
        if (!writeDataChunk(chunk)) {
            _writeErrors++;
            sendDisplayMessage(Common::DisplayMessage::ERROR, F("Write Failed"));
        }
    }
    
    // Handle end of file
    if (chunk.isEndOfFile) {
        if (closeCurrentFile()) {
            char message[32];
            snprintf(message, sizeof(message), "Saved: %s", _currentFilename);
            sendDisplayMessage(Common::DisplayMessage::INFO, message);
        } else {
            sendDisplayMessage(Common::DisplayMessage::ERROR, F("Close Failed"));
        }
    }
}

bool FileSystemManager::initializeSD() {
    // Initialize LED pins
    pinMode(Common::Pins::DATA_WRITE_LED, OUTPUT);
    digitalWrite(Common::Pins::DATA_WRITE_LED, LOW);
    
    // Initialize SD hardware detect pins
    pinMode(Common::Pins::SD_CD, INPUT_PULLUP);   // Card Detect (active LOW)
    pinMode(Common::Pins::SD_WP, INPUT_PULLUP);   // Write Protect (active HIGH)
    
    return SD.begin(Common::Pins::SD_CS);
}

bool FileSystemManager::initializeEEPROM() {
    return _eeprom.initialize();
}

bool FileSystemManager::createNewFile() {
    generateFilename(_currentFilename, sizeof(_currentFilename));
    
    switch (_activeStorage.value) {
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
            _fileCounter++;  // Increment counter for serial transfer files too
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
    
    // Turn on write activity LED
    digitalWrite(Common::Pins::DATA_WRITE_LED, HIGH);
    
    bool success = false;
    
    switch (_activeStorage.value) {
        case Common::StorageType::SD_CARD:
            if (_currentFile) {

                // Lock LPT port during SPI operations to prevent interference
                getServices().getParallelPortManager()->lockPort();
                
                size_t written = _currentFile.write(chunk.data, chunk.length);
                _currentFile.flush(); // Ensure data is written
                
                // Unlock LPT port
                getServices().getParallelPortManager()->unlockPort();
                
                if (written == chunk.length) {
                    _totalBytesWritten += chunk.length;
                    success = true;
                }
            }
            break;
            
        case Common::StorageType::EEPROM:
            // TODO: Implement EEPROM write
            return false;
            
        case Common::StorageType::SERIAL_TRANSFER:
            // TODO: Implement serial transfer
            success = true;
            break;
            
        default:
            success = false;
            break;
    }
    
    // Turn off write activity LED
    digitalWrite(Common::Pins::DATA_WRITE_LED, LOW);
    
    return success;
}

bool FileSystemManager::closeCurrentFile() {
    if (!_isFileOpen) {
        return true; // Already closed
    }
    
    bool result = true;
    
    switch (_activeStorage.value) {
        case Common::StorageType::SD_CARD:
            if (_currentFile) {
                _currentFile.close();
                _fileCounter++; // Increment counter for successful SD card file
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
    // Use timestamp-based filename format: yyyyMMddHHmmss.ext
    generateTimestampFilename(buffer, bufferSize);
}

void FileSystemManager::generateTimestampFilename(char* buffer, size_t bufferSize) {
    const char* extension = getFileExtension();
    
    // Use ServiceLocator to get TimeManager safely
    TimeManager* timeManager = getServices().getTimeManager();
    
    if (timeManager && timeManager->isRTCAvailable()) {
        // Get formatted datetime and parse it for compact format
        auto rtc = timeManager->getRTC();
        auto now = rtc.now();
        snprintf(buffer, bufferSize, "%04d%02d%02d%02d%02d%02d%s",
            now.year(),
            now.month(),
            now.day(),
            now.hour(),
            now.minute(),
            now.second(),
            extension
        );
    } else {
        // Fallback to millis-based timestamp if no RTC
        snprintf(buffer, bufferSize, "DAT%lu%s", millis(), extension);
    }
}

const char* FileSystemManager::getFileExtension() const {
    return _fileType.getFileExtension();
}

void FileSystemManager::sendDisplayMessage(Common::DisplayMessage::Type type, const char* message) {
    DisplayManager* displayManager = getServices().getDisplayManager();
    if (displayManager) {
        displayManager->displayMessage(type, message);
    }
}

void FileSystemManager::sendDisplayMessage(Common::DisplayMessage::Type type, const __FlashStringHelper* message) {
    DisplayManager* displayManager = getServices().getDisplayManager();
    if (displayManager) {
        displayManager->displayMessage(type, message);
    }
}

void FileSystemManager::setStorageType(Common::StorageType type) {
    if (_activeStorage.value != type.value) {
        closeCurrentFile(); // Close any open file before switching
        _activeStorage.value = type.value;
        
        // Verify the new storage type is available
        switch (type.value) {
            case Common::StorageType::SD_CARD:
                if (!_sdAvailable) {
                    sendDisplayMessage(Common::DisplayMessage::ERROR, F("SD Not Available"));
                    _activeStorage.value = Common::StorageType::SERIAL_TRANSFER;
                }
                break;
                
            case Common::StorageType::EEPROM:
                if (!_eepromAvailable) {
                    sendDisplayMessage(Common::DisplayMessage::ERROR, F("EEPROM Not Available"));
                    _activeStorage.value = Common::StorageType::SERIAL_TRANSFER;
                }
                break;
                
            case Common::StorageType::SERIAL_TRANSFER:
                // Always available
                break;
                
            case Common::StorageType::AUTO_SELECT:
                if (_sdAvailable) {
                    _activeStorage.value = Common::StorageType::SD_CARD;
                } else if (_eepromAvailable) {
                    _activeStorage.value = Common::StorageType::EEPROM;
                } else {
                    _activeStorage.value = Common::StorageType::SERIAL_TRANSFER;
                }
                break;
        }
    }
}

uint32_t FileSystemManager::getFilesStored() const {
    // Return count based on active storage type
    switch (_activeStorage.value) {
        case Common::StorageType::SD_CARD:
            return getSDCardFileCount();
        case Common::StorageType::EEPROM:
            // For EEPROM, fall back to file counter for now
            return _fileCounter;
        case Common::StorageType::SERIAL_TRANSFER:
            // For serial transfer, use file counter
            return _fileCounter;
        default:
            return 0;
    }
}

uint32_t FileSystemManager::getSDCardFileCount() const {
    if (!_sdAvailable) {
        return 0;
    }
    
    uint32_t fileCount = 0;
    File root = SD.open("/");
    
    if (!root) {
        return 0;
    }
    
    // Count all files in root directory
    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
            break;  // No more files
        }
        
        if (!entry.isDirectory()) {
            fileCount++;
        }
        entry.close();
    }
    
    root.close();
    return fileCount;
}

bool FileSystemManager::isSDCardPresent() const {
    return digitalRead(Common::Pins::SD_CD) == LOW;  // Active LOW
}

bool FileSystemManager::isSDWriteProtected() const {
    return digitalRead(Common::Pins::SD_WP) == HIGH; // Active HIGH
}

Common::FileType FileSystemManager::detectFileType(const uint8_t* data, uint16_t length) {
    // Need at least 4 bytes for detection
    if (length < 4) {
        return Common::FileType::BINARY;
    }
    
    // Check for common file format headers
    // BMP files start with "BM"
    if (data[0] == 0x42 && data[1] == 0x4D) {
        return Common::FileType::BMP;
    }
    
    // PCX files start with 0x0A
    if (data[0] == 0x0A) {
        return Common::FileType::PCX;
    }
    
    // TIFF files start with "II" (little-endian) or "MM" (big-endian) 
    if ((data[0] == 0x49 && data[1] == 0x49 && data[2] == 0x2A && data[3] == 0x00) ||
        (data[0] == 0x4D && data[1] == 0x4D && data[2] == 0x00 && data[3] == 0x2A)) {
        return Common::FileType::TIFF;
    }
    
    // PostScript/EPS files start with "%!"
    if (data[0] == 0x25 && data[1] == 0x21) {
        return Common::FileType::EPSIMAGE;
    }
    
    // Check for printer command sequences (HP PCL commands often start with ESC)
    if (data[0] == 0x1B) {  // ESC character
        if (length >= 3) {
            // HP PCL commands: ESC E (reset), ESC & (parameterized command)
            if (data[1] == 0x45 || data[1] == 0x26) {
                return Common::FileType::LASERJET;  // Default HP printer format
            }
        }
        return Common::FileType::LASERJET;  // Generic escape sequence
    }
    
    // If no specific format detected, assume binary
    return Common::FileType::BINARY;
}

// IComponent interface implementation
bool FileSystemManager::selfTest() {
    Serial.print(F("FileSystemManager Self-Test:\r\n"));
    
    bool result = true;
    
    // Test SD card
    if (_sdAvailable) {
        Serial.print(F("  SD Card: ✅ Available\r\n"));
    } else {
        Serial.print(F("  SD Card: ❌ Not Available\r\n"));
        result = false;
    }
    
    // Test EEPROM
    if (_eepromAvailable) {
        Serial.print(F("  EEPROM: ✅ Available\r\n"));
    } else {
        Serial.print(F("  EEPROM: ⚠️  Not Available\r\n"));
        // EEPROM not critical for basic operation
    }
    
    // Test dependencies
    if (!validateDependencies()) {
        result = false;
    }
    
    return result;
}

const char* FileSystemManager::getComponentName() const {
    return "FileSystemManager";
}

bool FileSystemManager::validateDependencies() const {
    bool valid = true;
    
    DisplayManager* displayManager = getServices().getDisplayManager();
    if (!displayManager) {
        Serial.print(F("  Missing DisplayManager dependency\r\n"));
        valid = false;
    }
    
    TimeManager* timeManager = getServices().getTimeManager();
    if (!timeManager) {
        Serial.print(F("  Missing TimeManager dependency\r\n"));
        valid = false;
    }
    
    return valid;
}

void FileSystemManager::printDependencyStatus() const {
    Serial.print(F("FileSystemManager Dependencies:\r\n"));
    
    DisplayManager* displayManager = getServices().getDisplayManager();
    Serial.print(F("  DisplayManager: "));
    Serial.print(displayManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));
    
    TimeManager* timeManager = getServices().getTimeManager();
    Serial.print(F("  TimeManager: "));
    Serial.print(timeManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));
}

} // namespace DeviceBridge::Components