#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "ParallelPortManager.h"
#include "SystemManager.h"
#include "TimeManager.h"
#include "../Common/ConfigurationService.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace DeviceBridge::Components {

FileSystemManager::FileSystemManager()
    : _eeprom(Common::Pins::EEPROM_CS), _sdAvailable(false), _eepromAvailable(false), _eepromCurrentAddress(0),
      _eepromBufferIndex(0), _activeStorage(Common::StorageType::AUTO_SELECT),
      _preferredStorage(Common::StorageType::SD_CARD), _fileCounter(0), _fileType(Common::FileType::AUTO_DETECT),
      _detectedFileType(Common::FileType::AUTO_DETECT), _totalBytesWritten(0), _currentFileBytesWritten(0), 
      _writeErrors(0), _isFileOpen(false) {
    memset(_currentFilename, 0, sizeof(_currentFilename));
}

FileSystemManager::~FileSystemManager() { stop(); }

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

void FileSystemManager::update(unsigned long currentTime) {
    // Periodic storage health check (called from main loop)
    // Can be used for background maintenance tasks if needed
}

void FileSystemManager::stop() { closeCurrentFile(); }

void FileSystemManager::processDataChunk(const Common::DataChunk &chunk) {
    // Debug logging for data chunk processing
    auto systemManager = getServices().getSystemManager();
    if (systemManager && systemManager->isParallelDebugEnabled()) {
        Serial.print(F("[DEBUG-FS] PROCESSING CHUNK - Length: "));
        Serial.print(chunk.length);
        Serial.print(F(", new file: "));
        Serial.print(chunk.isNewFile ? F("YES") : F("NO"));
        Serial.print(F(", end of file: "));
        Serial.print(chunk.isEndOfFile ? F("YES") : F("NO"));
        Serial.print(F("\r\n"));
    }

    // Handle new file
    if (chunk.isNewFile) {
        closeCurrentFile();
        
        if (systemManager && systemManager->isParallelDebugEnabled()) {
            Serial.print(F("[DEBUG-FS] CREATING NEW FILE...\r\n"));
        }
        
        if (!createNewFile()) {
            if (systemManager && systemManager->isParallelDebugEnabled()) {
                Serial.print(F("[DEBUG-FS] FILE CREATION FAILED! Signaling error to TDS2024\r\n"));
            }
            
            // Signal error to TDS2024 to stop sending data
            auto parallelPortManager = getServices().getParallelPortManager();
            if (parallelPortManager) {
                parallelPortManager->setPrinterError(true);    // Set ERROR signal active
                parallelPortManager->setPrinterPaperOut(true); // Set PAPER_OUT to indicate problem
                parallelPortManager->clearBuffer();           // Clear any buffered data
                
                if (systemManager && systemManager->isParallelDebugEnabled()) {
                    Serial.print(F("[DEBUG-FS] ERROR signals sent to TDS2024, buffer cleared\r\n"));
                }
            }
            
            sendDisplayMessage(Common::DisplayMessage::ERROR, F("File Create Failed"));
            return;
        }
        
        if (systemManager && systemManager->isParallelDebugEnabled()) {
            Serial.print(F("[DEBUG-FS] FILE CREATED SUCCESSFULLY: "));
            Serial.print(_currentFilename);
            Serial.print(F("\r\n"));
        }
        
        // Clear any error signals to TDS2024 on successful file creation
        auto parallelPortManager = getServices().getParallelPortManager();
        if (parallelPortManager) {
            parallelPortManager->setPrinterError(false);   // Clear ERROR signal
            parallelPortManager->setPrinterPaperOut(false); // Clear PAPER_OUT signal
        }
        
        sendDisplayMessage(Common::DisplayMessage::STATUS, F("Storing..."));

        // Detect file type from first chunk if auto-detection is enabled
        if (_fileType.value == Common::FileType::AUTO_DETECT && chunk.length > 0) {
            _detectedFileType = detectFileType(chunk.data, chunk.length);
        } else {
            _detectedFileType = _fileType; // Use configured type
        }
    } else {
        // Don't spam LCD with status messages for every data chunk
        // The continuous activity is already shown by L2 LED
    }

    // Write data - show activity even if file isn't open
    if (chunk.length > 0) {
        // Flash L2 LED to show write activity attempt
        digitalWrite(Common::Pins::DATA_WRITE_LED, HIGH);
        
        if (systemManager && systemManager->isParallelDebugEnabled()) {
            Serial.print(F("[DEBUG-FS] WRITING DATA - "));
            Serial.print(chunk.length);
            Serial.print(F(" bytes, file open: "));
            Serial.print(_isFileOpen ? F("YES") : F("NO"));
            Serial.print(F("\r\n"));
        }
        
        if (_isFileOpen) {
            if (!writeDataChunk(chunk)) {
                _writeErrors++;
                if (systemManager && systemManager->isParallelDebugEnabled()) {
                    Serial.print(F("[DEBUG-FS] WRITE FAILED - Error count now: "));
                    Serial.print(_writeErrors);
                    Serial.print(F("\r\n"));
                }
                sendDisplayMessage(Common::DisplayMessage::ERROR, F("Write Failed"));
            } else {
                if (systemManager && systemManager->isParallelDebugEnabled()) {
                    Serial.print(F("[DEBUG-FS] WRITE SUCCESS - "));
                    Serial.print(chunk.length);
                    Serial.print(F(" bytes written\r\n"));
                }
            }
        } else {
            // File not open - don't spam errors, just count them
            _writeErrors++;
            if (systemManager && systemManager->isParallelDebugEnabled()) {
                Serial.print(F("[DEBUG-FS] WRITE ERROR - No file open! Error count: "));
                Serial.print(_writeErrors);
                Serial.print(F("\r\n"));
            }
            
            // Signal error to TDS2024 after multiple consecutive write errors
            if (_writeErrors >= 5) {  // After 5 errors, signal TDS2024 to stop
                auto parallelPortManager = getServices().getParallelPortManager();
                if (parallelPortManager) {
                    parallelPortManager->setPrinterError(true);    // Set ERROR signal active
                    parallelPortManager->setPrinterPaperOut(true); // Set PAPER_OUT to indicate problem
                    
                    if (systemManager && systemManager->isParallelDebugEnabled()) {
                        Serial.print(F("[DEBUG-FS] Multiple write errors - signaling TDS2024 to stop\r\n"));
                    }
                }
            }
            
            // Only send error message once per file to avoid LCD spam
            static bool errorSent = false;
            if (chunk.isNewFile || !errorSent) {
                sendDisplayMessage(Common::DisplayMessage::ERROR, F("No File Open"));
                errorSent = true;
            }
            if (chunk.isEndOfFile) {
                errorSent = false; // Reset for next file
            }
        }
        
        // Brief delay to make LED flash visible, then turn off
        delay(2);
        digitalWrite(Common::Pins::DATA_WRITE_LED, LOW);
    }

    // Handle end of file
    if (chunk.isEndOfFile) {
        if (systemManager && systemManager->isParallelDebugEnabled()) {
            Serial.print(F("[DEBUG-FS] END OF FILE - Closing file: "));
            Serial.print(_currentFilename);
            Serial.print(F("\r\n"));
        }
        
        if (closeCurrentFile()) {
            char message[32];
            snprintf(message, sizeof(message), "Saved: %s", _currentFilename);
            sendDisplayMessage(Common::DisplayMessage::INFO, message);
            
            if (systemManager && systemManager->isParallelDebugEnabled()) {
                Serial.print(F("[DEBUG-FS] FILE CLOSED SUCCESSFULLY - "));
                Serial.print(_currentFilename);
                Serial.print(F("\r\n"));
            }
        } else {
            if (systemManager && systemManager->isParallelDebugEnabled()) {
                Serial.print(F("[DEBUG-FS] FILE CLOSE FAILED - "));
                Serial.print(_currentFilename);
                Serial.print(F("\r\n"));
            }
            sendDisplayMessage(Common::DisplayMessage::ERROR, F("Close Failed"));
        }
    }
}

bool FileSystemManager::initializeSD() {
    // Initialize LED pins
    pinMode(Common::Pins::DATA_WRITE_LED, OUTPUT);
    digitalWrite(Common::Pins::DATA_WRITE_LED, LOW);

    // Initialize SD hardware detect pins
    pinMode(Common::Pins::SD_CD, INPUT_PULLUP); // Card Detect (active LOW)
    pinMode(Common::Pins::SD_WP, INPUT_PULLUP); // Write Protect (active HIGH)

    return SD.begin(Common::Pins::SD_CS);
}

bool FileSystemManager::initializeEEPROM() { return _eeprom.initialize(); }

bool FileSystemManager::createNewFile() {
    // Notify display manager that storage operation is starting
    auto displayManager = getServices().getDisplayManager();
    displayManager->setStorageOperationActive(true);
    
    generateFilename(_currentFilename, sizeof(_currentFilename));

    switch (_activeStorage.value) {
    case Common::StorageType::SD_CARD:
        if (_sdAvailable) {
            sendDisplayMessage(Common::DisplayMessage::INFO, _currentFilename);

            String currentPath = "/" + String(_currentFilename);
            int lastSlash = currentPath.lastIndexOf('/');
            if (lastSlash >= 0) {
                String dirPath = currentPath.substring(0, lastSlash);
                if (!SD.exists(dirPath.c_str())) {
                    // Create directory without leading slash for Arduino SD library
                    String createPath = dirPath.substring(1); // Remove leading '/'
                    if (!SD.mkdir(createPath.c_str())) {
                        // Try without subdirectory - fallback to root
                        sendDisplayMessage(Common::DisplayMessage::ERROR, F("Dir Failed - Using Root"));
                        // Use just the filename without directory structure
                        auto fileName = currentPath.substring(lastSlash + 1);
                        currentPath = "/" + fileName;
                    } else {
                        sendDisplayMessage(Common::DisplayMessage::INFO, F("Dir Created"));
                    }
                } else {
                    sendDisplayMessage(Common::DisplayMessage::INFO, F("Dir Exists"));
                }
            }
            auto fileName = currentPath.substring(lastSlash + 1);
            sendDisplayMessage(Common::DisplayMessage::INFO, ("File: " + fileName).c_str());

            // Lock LPT port during SD operations to prevent interference
            getServices().getParallelPortManager()->lockPort();
            
            _currentFile = SD.open(currentPath, FILE_WRITE);
            
            // Unlock LPT port immediately after SD operation
            getServices().getParallelPortManager()->unlockPort();
            
            _isFileOpen = (_currentFile != 0);
            if (_isFileOpen) {
                _currentFileBytesWritten = 0; // Reset counter for new file
            }
            
            if (_isFileOpen) {
                sendDisplayMessage(Common::DisplayMessage::INFO, F("SD Opened"));
            } else {
                sendDisplayMessage(Common::DisplayMessage::ERROR, F("SD Open Failed"));
                // Try to understand why it failed
                if (!isSDCardPresent()) {
                    sendDisplayMessage(Common::DisplayMessage::ERROR, F("No SD Card"));
                } else if (isSDWriteProtected()) {
                    sendDisplayMessage(Common::DisplayMessage::ERROR, F("SD Protected"));
                } else {
                    sendDisplayMessage(Common::DisplayMessage::ERROR, F("SD Busy/Error"));
                }
            }
            
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
        _currentFileBytesWritten = 0; // Reset counter for new file
        _fileCounter++; // Increment counter for serial transfer files too
        return true;

    default:
        _isFileOpen = false;
        return false;
    }

    _isFileOpen = false;
    return false;
}

bool FileSystemManager::writeDataChunk(const Common::DataChunk &chunk) {
    if (!_isFileOpen) {
        return false;
    }

    // Ensure storage operation mode is active during writes
    auto displayManager = getServices().getDisplayManager();
    displayManager->setStorageOperationActive(true);

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
                _currentFileBytesWritten += chunk.length;
                success = true;
            }
        }
        break;

    case Common::StorageType::EEPROM:
        // TODO: Implement EEPROM write
        return false;

    case Common::StorageType::SERIAL_TRANSFER:
        // TODO: Implement serial transfer
        _totalBytesWritten += chunk.length;
        _currentFileBytesWritten += chunk.length;
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
    
    // Clear any error signals to TDS2024 on successful file closure
    auto parallelPortManager = getServices().getParallelPortManager();
    if (parallelPortManager) {
        parallelPortManager->setPrinterError(false);   // Clear ERROR signal
        parallelPortManager->setPrinterPaperOut(false); // Clear PAPER_OUT signal
    }
    
    // Notify display manager that storage operation is ending
    auto displayManager = getServices().getDisplayManager();
    displayManager->setStorageOperationActive(false);
    
    return result;
}

void FileSystemManager::generateFilename(char *buffer, size_t bufferSize) {
    // Use timestamp-based filename format: yyyyMMddHHmmss.ext
    generateTimestampFilename(buffer, bufferSize);
}

void FileSystemManager::generateTimestampFilename(char *buffer, size_t bufferSize) {
    const char *extension = getFileExtension();

    // Use ServiceLocator to get TimeManager safely
    TimeManager *timeManager = getServices().getTimeManager();

    if (timeManager && timeManager->isRTCAvailable()) {
        // Get formatted datetime and parse it for compact format
        auto rtc = timeManager->getRTC();
        auto now = rtc.now();
        snprintf(buffer, bufferSize, "%04d%02d%02d/%02d%02d%02d%s", now.year(), now.month(), now.day(), now.hour(),
                 now.minute(), now.second(), extension);
    } else {
        // Fallback to millis-based timestamp if no RTC
        snprintf(buffer, bufferSize, "DAT%lu%s", millis(), extension);
    }
}

const char *FileSystemManager::getFileExtension() const { return _fileType.getFileExtension(); }

void FileSystemManager::sendDisplayMessage(Common::DisplayMessage::Type type, const char *message) {
    auto displayManager = getServices().getDisplayManager();
    displayManager->displayMessage(type, message);
}

void FileSystemManager::sendDisplayMessage(Common::DisplayMessage::Type type, const __FlashStringHelper *message) {
    auto displayManager = getServices().getDisplayManager();
    displayManager->displayMessage(type, message);
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

    return countFilesRecursive("/");
}

uint32_t FileSystemManager::countFilesRecursive(const char* dirPath) const {
    uint32_t fileCount = 0;
    File dir = SD.open(dirPath);

    if (!dir) {
        return 0;
    }

    // Count all files and recurse into subdirectories
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) {
            break; // No more files
        }

        if (entry.isDirectory()) {
            // Recurse into subdirectory
            fileCount += countFilesRecursive(entry.name());
        } else {
            fileCount++;
        }
        entry.close();
    }

    dir.close();
    return fileCount;
}

bool FileSystemManager::isSDCardPresent() const {
    return digitalRead(Common::Pins::SD_CD) == LOW; // Active LOW
}

bool FileSystemManager::isSDWriteProtected() const {
    return digitalRead(Common::Pins::SD_WP) == HIGH; // Active HIGH
}

Common::FileType FileSystemManager::detectFileType(const uint8_t *data, uint16_t length) {
    // Need at least 4 bytes for detection
    if (length < 4) {
        return Common::FileType::BINARY;
    }

    // Check for common file format headers using ConfigurationService
    auto* config = getServices().getConfigurationService();
    
    // BMP files start with "BM"
    if (data[0] == config->getBmpSignature1() && data[1] == config->getBmpSignature2()) {
        return Common::FileType::BMP;
    }

    // PCX files start with 0x0A
    if (data[0] == config->getPcxSignature()) {
        return Common::FileType::PCX;
    }

    // TIFF files start with "II" (little-endian) or "MM" (big-endian)
    if (config->isTiffLittleEndian(data[0], data[1], data[2], data[3]) ||
        config->isTiffBigEndian(data[0], data[1], data[2], data[3])) {
        return Common::FileType::TIFF;
    }

    // PostScript/EPS files start with "%!"
    if (data[0] == config->getPsSignature1() && data[1] == config->getPsSignature2()) {
        return Common::FileType::EPSIMAGE;
    }

    // Check for printer command sequences (HP PCL commands often start with ESC)
    if (data[0] == config->getEscCharacter()) { // ESC character
        if (length >= 3) {
            // HP PCL commands: ESC E (reset), ESC & (parameterized command)
            if (data[1] == 0x45 || data[1] == 0x26) {
                return Common::FileType::LASERJET; // Default HP printer format
            }
        }
        return Common::FileType::LASERJET; // Generic escape sequence
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

const char *FileSystemManager::getComponentName() const { return "FileSystemManager"; }

bool FileSystemManager::validateDependencies() const {
    bool valid = true;

    auto displayManager = getServices().getDisplayManager();
    if (!displayManager) {
        Serial.print(F("  Missing DisplayManager dependency\r\n"));
        valid = false;
    }

    TimeManager *timeManager = getServices().getTimeManager();
    if (!timeManager) {
        Serial.print(F("  Missing TimeManager dependency\r\n"));
        valid = false;
    }

    return valid;
}

void FileSystemManager::printDependencyStatus() const {
    Serial.print(F("FileSystemManager Dependencies:\r\n"));

    auto displayManager = getServices().getDisplayManager();
    Serial.print(F("  DisplayManager: "));
    Serial.print(displayManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));

    TimeManager *timeManager = getServices().getTimeManager();
    Serial.print(F("  TimeManager: "));
    Serial.print(timeManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));
}

unsigned long FileSystemManager::getUpdateInterval() const {
    auto configService = getServices().getConfigurationService();
    return configService ? configService->getFileSystemInterval() : 10; // Default 10ms
}

} // namespace DeviceBridge::Components