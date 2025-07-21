#include "SDCardFileSystem.h"
#include <string.h>

namespace DeviceBridge::Storage {

SDCardFileSystem::SDCardFileSystem() 
    : _initialized(false), _writeProtected(false), _totalSpace(0), _freeSpace(0) {
    clearError();
}

SDCardFileSystem::~SDCardFileSystem() {
    shutdown();
}

bool SDCardFileSystem::initialize() {
    // Initialize hardware pins
    pinMode(Common::Pins::SD_CD, INPUT_PULLUP);  // Card Detect
    pinMode(Common::Pins::SD_WP, INPUT_PULLUP);  // Write Protect
    
    // Check card presence
    if (!checkCardPresence()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "SD card not inserted");
        return false;
    }
    
    // Check write protection
    _writeProtected = checkWriteProtection();
    
    // Initialize SD library
    if (!SD.begin(Common::Pins::SD_CS)) {
        setError(FileSystemErrors::INIT_FAILED, "SD.begin() failed");
        _initialized = false;
        return false;
    }
    
    _initialized = true;
    updateSpaceInfo();
    clearError();
    return true;
}

bool SDCardFileSystem::isAvailable() const {
    return _initialized && checkCardPresence();
}

void SDCardFileSystem::shutdown() {
    if (_currentFile) {
        _currentFile.close();
        _hasActiveFile = false;
    }
    _initialized = false;
}

bool SDCardFileSystem::createFile(const char* filename) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "SD card not available");
        return false;
    }
    
    if (_writeProtected) {
        setError(FileSystemErrors::WRITE_PROTECTED, "SD card is write protected");
        return false;
    }
    
    if (_hasActiveFile) {
        closeFile();
    }
    
    // Create directories if they don't exist
    String filepath(filename);
    int lastSlash = filepath.lastIndexOf('/');
    if (lastSlash > 0) {
        String dirPath = filepath.substring(0, lastSlash);
        if (!SD.exists(dirPath)) {
            // Note: SD library doesn't have mkdir, files create directories automatically
        }
    }
    
    _currentFile = SD.open(filename, FILE_WRITE);
    if (!_currentFile) {
        setError(FileSystemErrors::FILE_CREATE_FAILED, "Failed to create file");
        return false;
    }
    
    _hasActiveFile = true;
    _filesCreated++;
    clearError();
    return true;
}

bool SDCardFileSystem::openFile(const char* filename, bool append) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "SD card not available");
        return false;
    }
    
    if (_hasActiveFile) {
        closeFile();
    }
    
    _currentFile = SD.open(filename, append ? FILE_WRITE : FILE_READ);
    if (!_currentFile) {
        setError(FileSystemErrors::FILE_OPEN_FAILED, "Failed to open file");
        return false;
    }
    
    if (append) {
        _currentFile.seek(_currentFile.size());
    }
    
    _hasActiveFile = true;
    clearError();
    return true;
}

bool SDCardFileSystem::writeData(const uint8_t* data, uint16_t length) {
    if (!_hasActiveFile || !_currentFile) {
        setError(FileSystemErrors::FILE_WRITE_FAILED, "No active file");
        return false;
    }
    
    if (_writeProtected) {
        setError(FileSystemErrors::WRITE_PROTECTED, "SD card is write protected");
        return false;
    }
    
    size_t written = _currentFile.write(data, length);
    if (written != length) {
        setError(FileSystemErrors::FILE_WRITE_FAILED, "Write operation incomplete");
        return false;
    }
    
    _bytesWritten += written;
    clearError();
    return true;
}

bool SDCardFileSystem::closeFile() {
    if (!_hasActiveFile || !_currentFile) {
        return true; // Already closed
    }
    
    _currentFile.close();
    _hasActiveFile = false;
    clearError();
    return true;
}

bool SDCardFileSystem::deleteFile(const char* filename) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "SD card not available");
        return false;
    }
    
    if (_writeProtected) {
        setError(FileSystemErrors::WRITE_PROTECTED, "SD card is write protected");
        return false;
    }
    
    if (!SD.exists(filename)) {
        setError(FileSystemErrors::FILE_NOT_FOUND, "File does not exist");
        return false;
    }
    
    if (!SD.remove(filename)) {
        setError(FileSystemErrors::FILE_DELETE_FAILED, "Failed to delete file");
        return false;
    }
    
    clearError();
    return true;
}

bool SDCardFileSystem::fileExists(const char* filename) {
    if (!isAvailable()) {
        return false;
    }
    
    return SD.exists(filename);
}

bool SDCardFileSystem::listFiles(char* buffer, uint16_t bufferSize) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "SD card not available");
        return false;
    }
    
    File root = SD.open("/");
    if (!root) {
        setError(FileSystemErrors::DIRECTORY_READ_FAILED, "Failed to open root directory");
        return false;
    }
    
    uint16_t pos = 0;
    buffer[0] = '\0';
    
    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
            break;
        }
        
        const char* name = entry.name();
        uint16_t nameLen = strlen(name);
        
        // Check if we have space for filename + newline + null terminator
        if (pos + nameLen + 2 >= bufferSize) {
            entry.close();
            break;
        }
        
        strcat(buffer + pos, name);
        pos += nameLen;
        buffer[pos++] = '\n';
        buffer[pos] = '\0';
        
        entry.close();
    }
    
    root.close();
    clearError();
    return true;
}

uint32_t SDCardFileSystem::getFileCount() {
    if (!isAvailable()) {
        return 0;
    }
    
    File root = SD.open("/");
    if (!root) {
        return 0;
    }
    
    uint32_t count = 0;
    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
            break;
        }
        if (!entry.isDirectory()) {
            count++;
        }
        entry.close();
    }
    
    root.close();
    return count;
}

uint32_t SDCardFileSystem::getTotalSpace() {
    updateSpaceInfo();
    return _totalSpace;
}

uint32_t SDCardFileSystem::getFreeSpace() {
    updateSpaceInfo();
    return _freeSpace;
}

bool SDCardFileSystem::format() {
    // SD library doesn't provide format functionality
    setError(FileSystemErrors::HARDWARE_ERROR, "Format not supported by SD library");
    return false;
}

bool SDCardFileSystem::flush() {
    if (_hasActiveFile && _currentFile) {
        _currentFile.flush();
    }
    return true;
}

bool SDCardFileSystem::sync() {
    return flush(); // SD library doesn't distinguish between flush and sync
}

bool SDCardFileSystem::reinitialize() {
    shutdown();
    return initialize();
}

// Private methods
bool SDCardFileSystem::checkCardPresence() const {
    // SD_CD pin is active LOW (card present when LOW)
    return digitalRead(Common::Pins::SD_CD) == LOW;
}

bool SDCardFileSystem::checkWriteProtection() const {
    // SD_WP pin is active HIGH (write protected when HIGH)
    return digitalRead(Common::Pins::SD_WP) == HIGH;
}

void SDCardFileSystem::updateSpaceInfo() {
    // SD library doesn't provide direct space information
    // These would need to be estimated or calculated differently
    _totalSpace = 1024 * 1024 * 1024; // Default 1GB estimate
    _freeSpace = _totalSpace - _bytesWritten; // Simple estimation
}

} // namespace DeviceBridge::Storage