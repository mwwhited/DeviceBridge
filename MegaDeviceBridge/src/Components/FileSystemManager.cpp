#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "ParallelPortManager.h"
#include "SystemManager.h"
#include "TimeManager.h"
#include "../Common/ConfigurationService.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

// PROGMEM component name for memory optimization
static const char component_name[] PROGMEM = "FileSystemManager";

namespace DeviceBridge::Components {

FileSystemManager::FileSystemManager()
    : _activeFileSystem(nullptr), _eeprom(Common::Pins::EEPROM_CS), _eepromCurrentAddress(0),
      _eepromBufferIndex(0), _activeStorage(Common::StorageType::AUTO_SELECT),
      _preferredStorage(Common::StorageType::SD_CARD), _fileCounter(0), _fileType(Common::FileType::AUTO_DETECT),
      _detectedFileType(Common::FileType::AUTO_DETECT), _totalBytesWritten(0), _currentFileBytesWritten(0), 
      _writeErrors(0), _lastSDCardCheckTime(0) {
    // Initialize bit field flags
    _flags.sdAvailable = 0;
    _flags.eepromAvailable = 0;
    _flags.lastSDCardDetectState = 0;
    _flags.isFileOpen = 0;
    _flags.reserved = 0;
    memset(_currentFilename, 0, sizeof(_currentFilename));
}

FileSystemManager::~FileSystemManager() { stop(); }

bool FileSystemManager::initialize() {
    // Cache service dependencies first (performance optimization)
    cacheServiceDependencies();
    
    // Initialize modular file system
    if (!initializeFileSystem()) {
        sendDisplayMessage(Common::DisplayMessage::ERROR, F("FileSystem Init Failed"));
        return false;
    }
    
    // Legacy initialization for compatibility
    _flags.sdAvailable = initializeSD() ? 1 : 0;
    _flags.eepromAvailable = initializeEEPROM() ? 1 : 0;
    
    // Initialize hot-swap detection state
    _flags.lastSDCardDetectState = checkSDCardPresence() ? 1 : 0;
    _lastSDCardCheckTime = millis();

    // Select initial storage type and active file system
    if (_preferredStorage.value == Common::StorageType::SD_CARD && _flags.sdAvailable) {
        _activeStorage = Common::StorageType::SD_CARD;
    } else if (_preferredStorage.value == Common::StorageType::EEPROM && _flags.eepromAvailable) {
        _activeStorage = Common::StorageType::EEPROM;
    } else if (_flags.sdAvailable) {
        _activeStorage = Common::StorageType::SD_CARD;
    } else if (_flags.eepromAvailable) {
        _activeStorage = Common::StorageType::EEPROM;
    } else {
        _activeStorage = Common::StorageType::SERIAL_TRANSFER;
        sendDisplayMessage(Common::DisplayMessage::ERROR, F("No Storage!"));
    }
    
    // Set active file system based on selected storage
    selectActiveFileSystem(_activeStorage);

    return _flags.sdAvailable || _flags.eepromAvailable || _activeFileSystem != nullptr;
}

void FileSystemManager::update(unsigned long currentTime) {
    // Check for SD card hot-swap every 1 second
    if (currentTime - _lastSDCardCheckTime >= 1000) {
        bool currentSDCardState = checkSDCardPresence();
        
        // SD card was inserted
        if (currentSDCardState && !_flags.lastSDCardDetectState) {
            handleSDCardInsertion();
        }
        // SD card was removed
        else if (!currentSDCardState && _flags.lastSDCardDetectState) {
            handleSDCardRemoval();
        }
        
        _flags.lastSDCardDetectState = currentSDCardState ? 1 : 0;
        _lastSDCardCheckTime = currentTime;
    }
}

void FileSystemManager::stop() { closeCurrentFile(); }

void FileSystemManager::processDataChunk(const Common::DataChunk &chunk) {
    // Debug logging for data chunk processing
    if (_cachedSystemManager->isParallelDebugEnabled()) {
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
        
        if (_cachedSystemManager->isParallelDebugEnabled()) {
            Serial.print(F("[DEBUG-FS] CREATING NEW FILE...\r\n"));
        }
        
        if (!createNewFile()) {
            if (_cachedSystemManager->isParallelDebugEnabled()) {
                Serial.print(F("[DEBUG-FS] FILE CREATION FAILED! Signaling error to TDS2024\r\n"));
            }
            
            // Signal error to TDS2024 to stop sending data
            // Use cached parallel port manager pointer
            _cachedParallelPortManager->setPrinterError(true);    // Set ERROR signal active
            _cachedParallelPortManager->setPrinterPaperOut(true); // Set PAPER_OUT to indicate problem
            _cachedParallelPortManager->clearBuffer();           // Clear any buffered data
            
            if (_cachedSystemManager->isParallelDebugEnabled()) {
                Serial.print(F("[DEBUG-FS] ERROR signals sent to TDS2024, buffer cleared\r\n"));
            }
            
            sendDisplayMessage(Common::DisplayMessage::ERROR, F("File Create Failed"));
            return;
        }
        
        if (_cachedSystemManager->isParallelDebugEnabled()) {
            Serial.print(F("[DEBUG-FS] FILE CREATED SUCCESSFULLY: "));
            Serial.print(_currentFilename);
            Serial.print(F("\r\n"));
        }
        
        // Clear any error signals to TDS2024 on successful file creation
        // Use cached parallel port manager pointer
        _cachedParallelPortManager->setPrinterError(false);   // Clear ERROR signal
        _cachedParallelPortManager->setPrinterPaperOut(false); // Clear PAPER_OUT signal
        
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
        
        if (_cachedSystemManager->isParallelDebugEnabled()) {
            Serial.print(F("[DEBUG-FS] WRITING DATA - "));
            Serial.print(chunk.length);
            Serial.print(F(" bytes, file open: "));
            Serial.print(_flags.isFileOpen ? F("YES") : F("NO"));
            Serial.print(F("\r\n"));
        }
        
        if (_flags.isFileOpen) {
            if (!writeDataChunk(chunk)) {
                _writeErrors++;
                if (_cachedSystemManager->isParallelDebugEnabled()) {
                    Serial.print(F("[DEBUG-FS] WRITE FAILED - Error count now: "));
                    Serial.print(_writeErrors);
                    Serial.print(F("\r\n"));
                }
                sendDisplayMessage(Common::DisplayMessage::ERROR, F("Write Failed"));
            } else {
                if (_cachedSystemManager->isParallelDebugEnabled()) {
                    Serial.print(F("[DEBUG-FS] WRITE SUCCESS - "));
                    Serial.print(chunk.length);
                    Serial.print(F(" bytes written\r\n"));
                }
            }
        } else {
            // File not open - don't spam errors, just count them
            _writeErrors++;
            if (_cachedSystemManager->isParallelDebugEnabled()) {
                Serial.print(F("[DEBUG-FS] WRITE ERROR - No file open! Error count: "));
                Serial.print(_writeErrors);
                Serial.print(F("\r\n"));
            }
            
            // Signal error to TDS2024 after multiple consecutive write errors
            if (_writeErrors >= 5) {  // After 5 errors, signal TDS2024 to stop
                // Use cached parallel port manager pointer
                _cachedParallelPortManager->setPrinterError(true);    // Set ERROR signal active
                _cachedParallelPortManager->setPrinterPaperOut(true); // Set PAPER_OUT to indicate problem
                
                if (_cachedSystemManager->isParallelDebugEnabled()) {
                    Serial.print(F("[DEBUG-FS] Multiple write errors - signaling TDS2024 to stop\r\n"));
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
        if (_cachedSystemManager->isParallelDebugEnabled()) {
            Serial.print(F("[DEBUG-FS] END OF FILE - Closing file: "));
            Serial.print(_currentFilename);
            Serial.print(F("\r\n"));
        }
        
        if (closeCurrentFile()) {
            char message[32];
            snprintf(message, sizeof(message), "Saved: %s", _currentFilename);
            sendDisplayMessage(Common::DisplayMessage::INFO, message);
            
            if (_cachedSystemManager->isParallelDebugEnabled()) {
                Serial.print(F("[DEBUG-FS] FILE CLOSED SUCCESSFULLY - "));
                Serial.print(_currentFilename);
                Serial.print(F("\r\n"));
            }
        } else {
            if (_cachedSystemManager->isParallelDebugEnabled()) {
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
    // Use cached display manager pointer
    _cachedDisplayManager->setStorageOperationActive(true);
    
    generateFilename(_currentFilename, sizeof(_currentFilename));

    switch (_activeStorage.value) {
    case Common::StorageType::SD_CARD:
        if (_flags.sdAvailable) {
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
            _cachedParallelPortManager->lockPort();
            
            _currentFile = SD.open(currentPath, FILE_WRITE);
            
            // Unlock LPT port immediately after SD operation
            _cachedParallelPortManager->unlockPort();
            
            _flags.isFileOpen = (_currentFile != 0);
            if (_flags.isFileOpen) {
                _currentFileBytesWritten = 0; // Reset counter for new file
            }
            
            if (_flags.isFileOpen) {
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
            
            return _flags.isFileOpen;
        }
        break;

    case Common::StorageType::EEPROM:
        // TODO: Implement EEPROM file creation
        _flags.isFileOpen = false;
        return false;

    case Common::StorageType::SERIAL_TRANSFER:
        // For serial transfer, we'll send data directly
        _flags.isFileOpen = true;
        _currentFileBytesWritten = 0; // Reset counter for new file
        _fileCounter++; // Increment counter for serial transfer files too
        return true;

    default:
        _flags.isFileOpen = false;
        return false;
    }

    _flags.isFileOpen = false;
    return false;
}

bool FileSystemManager::writeDataChunk(const Common::DataChunk &chunk) {
    if (!_flags.isFileOpen) {
        return false;
    }

    // Ensure storage operation mode is active during writes
    // Use cached display manager pointer
    _cachedDisplayManager->setStorageOperationActive(true);

    // Turn on write activity LED
    digitalWrite(Common::Pins::DATA_WRITE_LED, HIGH);

    bool success = false;

    switch (_activeStorage.value) {
    case Common::StorageType::SD_CARD:
        if (_currentFile) {

            // Lock LPT port during SPI operations to prevent interference
            _cachedParallelPortManager->lockPort();

            size_t written = _currentFile.write(chunk.data, chunk.length);
            _currentFile.flush(); // Ensure data is written

            // Unlock LPT port
            _cachedParallelPortManager->unlockPort();

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
    if (!_flags.isFileOpen) {
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

    _flags.isFileOpen = false;
    
    // Clear any error signals to TDS2024 on successful file closure
    // Use cached parallel port manager pointer
        _cachedParallelPortManager->setPrinterError(false);   // Clear ERROR signal
        _cachedParallelPortManager->setPrinterPaperOut(false); // Clear PAPER_OUT signal
    
    // Notify display manager that storage operation is ending
    // Use cached display manager pointer
    _cachedDisplayManager->setStorageOperationActive(false);
    
    return result;
}

void FileSystemManager::generateFilename(char *buffer, size_t bufferSize) {
    // Use timestamp-based filename format: yyyyMMddHHmmss.ext
    generateTimestampFilename(buffer, bufferSize);
}

void FileSystemManager::generateTimestampFilename(char *buffer, size_t bufferSize) {
    const char *extension = getFileExtension();

    // Use ServiceLocator to get TimeManager safely
    // Use cached time manager pointer

    if (_cachedTimeManager->isRTCAvailable()) {
        // Get formatted datetime and parse it for compact format
        auto rtc = _cachedTimeManager->getRTC();
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
    // Use cached display manager pointer
    _cachedDisplayManager->displayMessage(type, message);
}

void FileSystemManager::sendDisplayMessage(Common::DisplayMessage::Type type, const __FlashStringHelper *message) {
    // Use cached display manager pointer
    _cachedDisplayManager->displayMessage(type, message);
}

void FileSystemManager::setStorageType(Common::StorageType type) {
    if (_activeStorage.value != type.value) {
        // Use modular file system switching
        if (selectActiveFileSystem(type)) {
            Serial.print(F("Successfully switched to storage: "));
            Serial.print(_activeFileSystem->getStorageName());
            Serial.print(F("\r\n"));
        } else {
            // Fallback to legacy method for compatibility
            closeCurrentFile(); // Close any open file before switching
            _activeStorage.value = type.value;

            // Verify the new storage type is available
            switch (type.value) {
            case Common::StorageType::SD_CARD:
                if (!_flags.sdAvailable) {
                    sendDisplayMessage(Common::DisplayMessage::ERROR, F("SD Not Available"));
                    _activeStorage.value = Common::StorageType::SERIAL_TRANSFER;
                    selectActiveFileSystem(Common::StorageType(Common::StorageType::SERIAL_TRANSFER));
                }
                break;

            case Common::StorageType::EEPROM:
                if (!_flags.eepromAvailable) {
                    sendDisplayMessage(Common::DisplayMessage::ERROR, F("EEPROM Not Available"));
                    _activeStorage.value = Common::StorageType::SERIAL_TRANSFER;
                    selectActiveFileSystem(Common::StorageType(Common::StorageType::SERIAL_TRANSFER));
                }
                break;

            case Common::StorageType::SERIAL_TRANSFER:
                // Always available
                selectActiveFileSystem(Common::StorageType(Common::StorageType::SERIAL_TRANSFER));
                break;

            case Common::StorageType::AUTO_SELECT:
                if (_flags.sdAvailable) {
                    _activeStorage.value = Common::StorageType::SD_CARD;
                    selectActiveFileSystem(Common::StorageType(Common::StorageType::SD_CARD));
                } else if (_flags.eepromAvailable) {
                    _activeStorage.value = Common::StorageType::EEPROM;
                    selectActiveFileSystem(Common::StorageType(Common::StorageType::EEPROM));
                } else {
                    _activeStorage.value = Common::StorageType::SERIAL_TRANSFER;
                    selectActiveFileSystem(Common::StorageType(Common::StorageType::SERIAL_TRANSFER));
                }
                break;
            }
        }
    }
}

uint32_t FileSystemManager::getFilesStored() const {
    // Use modular file system if available
    if (_activeFileSystem && _activeFileSystem->isAvailable()) {
        return _activeFileSystem->getFileCount();
    }
    
    // Fallback to legacy method
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
    if (!_flags.sdAvailable) {
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
    // Use cached configuration service pointer
    
    // BMP files start with "BM"
    if (data[0] == _cachedConfigurationService->getBmpSignature1() && data[1] == _cachedConfigurationService->getBmpSignature2()) {
        return Common::FileType::BMP;
    }

    // PCX files start with 0x0A
    if (data[0] == _cachedConfigurationService->getPcxSignature()) {
        return Common::FileType::PCX;
    }

    // TIFF files start with "II" (little-endian) or "MM" (big-endian)
    if (_cachedConfigurationService->isTiffLittleEndian(data[0], data[1], data[2], data[3]) ||
        _cachedConfigurationService->isTiffBigEndian(data[0], data[1], data[2], data[3])) {
        return Common::FileType::TIFF;
    }

    // PostScript/EPS files start with "%!"
    if (data[0] == _cachedConfigurationService->getPsSignature1() && data[1] == _cachedConfigurationService->getPsSignature2()) {
        return Common::FileType::EPSIMAGE;
    }

    // Check for printer command sequences (HP PCL commands often start with ESC)
    if (data[0] == _cachedConfigurationService->getEscCharacter()) { // ESC character
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
    if (_flags.sdAvailable) {
        Serial.print(F("  SD Card: ✅ Available\r\n"));
    } else {
        Serial.print(F("  SD Card: ❌ Not Available\r\n"));
        result = false;
    }

    // Test EEPROM
    if (_flags.eepromAvailable) {
        Serial.print(F("  EEPROM: ✅ Available\r\n"));
    } else {
        Serial.print(F("  EEPROM: ⚠️  Not Available\r\n"));
        // EEPROM not critical for basic operation
    }

    // Dependencies validated by ServiceLocator at startup

    return result;
}

const char *FileSystemManager::getComponentName() const { 
    static char name_buffer[24];
    strcpy_P(name_buffer, component_name);
    return name_buffer;
}

bool FileSystemManager::validateDependencies() const {
    bool valid = true;

    // Use cached display manager pointer
    if (!_cachedDisplayManager) {
        Serial.print(F("  Missing DisplayManager dependency\r\n"));
        valid = false;
    }

    // Use cached time manager pointer
    if (!_cachedTimeManager) {
        Serial.print(F("  Missing TimeManager dependency\r\n"));
        valid = false;
    }

    return valid;
}

void FileSystemManager::printDependencyStatus() const {
    Serial.print(F("FileSystemManager Dependencies:\r\n"));

    // Use cached display manager pointer
    Serial.print(F("  DisplayManager: "));
    Serial.print(_cachedDisplayManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));

    // Use cached time manager pointer
    Serial.print(F("  TimeManager: "));
    Serial.print(_cachedTimeManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));
}

unsigned long FileSystemManager::getUpdateInterval() const {
    // Use cached configuration service pointer
    return _cachedConfigurationService->getFileSystemInterval(); // Default 10ms
}

// Hot-swap detection methods
bool FileSystemManager::checkSDCardPresence() {
    // Use SD Card Detect pin (active LOW)
    // If SD_CD pin is LOW, card is present
    return digitalRead(Common::Pins::SD_CD) == LOW;
}

void FileSystemManager::handleSDCardInsertion() {
    Serial.print(F("SD Card inserted - attempting re-initialization...\r\n"));
    
    // Attempt to re-initialize SD card
    bool initSuccess = initializeSD();
    
    if (initSuccess) {
        _flags.sdAvailable = true;
        Serial.print(F("SD Card re-initialization successful!\r\n"));
        sendDisplayMessage(Common::DisplayMessage::INFO, F("SD Card Ready"));
        
        // If we were previously using EEPROM due to SD failure,
        // and user prefers SD, switch back to SD
        if (_preferredStorage.value == Common::StorageType::SD_CARD && 
            _activeStorage.value != Common::StorageType::SD_CARD) {
            Serial.print(F("Switching back to preferred SD storage\r\n"));
            setStorageType(Common::StorageType(Common::StorageType::SD_CARD));
        }
    } else {
        _flags.sdAvailable = false;
        Serial.print(F("SD Card re-initialization failed\r\n"));
        sendDisplayMessage(Common::DisplayMessage::ERROR, F("SD Init Failed"));
    }
}

void FileSystemManager::handleSDCardRemoval() {
    Serial.print(F("SD Card removed\r\n"));
    
    // Close any open file on SD card
    if (_flags.isFileOpen && _activeStorage.value == Common::StorageType::SD_CARD) {
        closeCurrentFile();
        Serial.print(F("Closed file due to SD card removal\r\n"));
    }
    
    _flags.sdAvailable = false;
    sendDisplayMessage(Common::DisplayMessage::ERROR, F("SD Card Removed"));
    
    // If we were using SD card and it's removed, switch to fallback storage
    if (_activeStorage.value == Common::StorageType::SD_CARD) {
        if (_flags.eepromAvailable) {
            Serial.print(F("Switching to EEPROM storage\r\n"));
            setStorageType(Common::StorageType(Common::StorageType::EEPROM));
        } else {
            Serial.print(F("No fallback storage available\r\n"));
            setStorageType(Common::StorageType(Common::StorageType::SERIAL_TRANSFER));
        }
    }
}

// Modular file system implementation
bool FileSystemManager::initializeFileSystem() {
    bool sdInit = _sdCardFileSystem.initialize();
    bool eepromInit = _eepromFileSystem.initialize();
    bool serialInit = _serialTransferFileSystem.initialize();
    
    if (!sdInit) {
        Serial.print(F("SD Card file system initialization failed\r\n"));
    }
    if (!eepromInit) {
        Serial.print(F("EEPROM file system initialization failed\r\n"));
    }
    if (!serialInit) {
        Serial.print(F("Serial Transfer file system initialization failed\r\n"));
    }
    
    // At least one file system must be available
    return sdInit || eepromInit || serialInit;
}

bool FileSystemManager::selectActiveFileSystem(Common::StorageType storageType) {
    Storage::IFileSystem* newFileSystem = getFileSystemForType(storageType);
    
    if (!newFileSystem) {
        Serial.print(F("No file system available for storage type: "));
        Serial.print(storageType.value);
        Serial.print(F("\r\n"));
        return false;
    }
    
    if (!newFileSystem->isAvailable()) {
        Serial.print(F("File system not available for storage type: "));
        Serial.print(storageType.value);
        Serial.print(F("\r\n"));
        return false;
    }
    
    // Close any open file in the current file system
    if (_activeFileSystem && _activeFileSystem->hasActiveFile()) {
        _activeFileSystem->closeFile();
    }
    
    _activeFileSystem = newFileSystem;
    _activeStorage = storageType;
    
    Serial.print(F("Switched to file system: "));
    Serial.print(_activeFileSystem->getStorageName());
    Serial.print(F("\r\n"));
    
    return true;
}

Storage::IFileSystem* FileSystemManager::getFileSystemForType(Common::StorageType storageType) {
    switch (storageType.value) {
        case Common::StorageType::SD_CARD:
            return &_sdCardFileSystem;
        case Common::StorageType::EEPROM:
            return &_eepromFileSystem;
        case Common::StorageType::SERIAL_TRANSFER:
            return &_serialTransferFileSystem;
        case Common::StorageType::AUTO_SELECT:
            // Return the first available file system
            if (_sdCardFileSystem.isAvailable()) {
                return &_sdCardFileSystem;
            } else if (_eepromFileSystem.isAvailable()) {
                return &_eepromFileSystem;
            } else {
                return &_serialTransferFileSystem;
            }
        default:
            return nullptr;
    }
}

} // namespace DeviceBridge::Components