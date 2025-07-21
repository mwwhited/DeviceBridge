#include "EEPROMFileSystem.h"
#include <string.h>

namespace DeviceBridge::Storage {

EEPROMFileSystem::EEPROMFileSystem() 
    : _eeprom(Common::Pins::EEPROM_CS), _initialized(false), _currentAddress(0), 
      _fileStartAddress(0), _availableSpace(0) {
    clearError();
}

EEPROMFileSystem::~EEPROMFileSystem() {
    shutdown();
}

bool EEPROMFileSystem::initialize() {
    if (!_eeprom.initialize()) {
        setError(FileSystemErrors::INIT_FAILED, "EEPROM initialization failed");
        return false;
    }
    
    _initialized = true;
    _availableSpace = calculateAvailableSpace();
    clearError();
    return true;
}

bool EEPROMFileSystem::isAvailable() const {
    return _initialized;
}

void EEPROMFileSystem::shutdown() {
    if (_hasActiveFile) {
        closeFile();
    }
    _initialized = false;
}

bool EEPROMFileSystem::createFile(const char* filename) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM not available");
        return false;
    }
    
    if (_hasActiveFile) {
        closeFile();
    }
    
    // Check filename length
    uint8_t filenameLen = strlen(filename);
    if (filenameLen == 0 || filenameLen >= MAX_FILENAME_SIZE) {
        setError(FileSystemErrors::INVALID_FILENAME, "Invalid filename length");
        return false;
    }
    
    // Find next available write address
    if (!findNextWriteAddress()) {
        setError(FileSystemErrors::INSUFFICIENT_SPACE, "No space for new file");
        return false;
    }
    
    // Reserve space for file header
    _fileStartAddress = _currentAddress;
    _currentAddress += FILE_ENTRY_OVERHEAD;
    
    _hasActiveFile = true;
    _filesCreated++;
    clearError();
    return true;
}

bool EEPROMFileSystem::openFile(const char* filename, bool append) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM not available");
        return false;
    }
    
    // EEPROM file system is append-only for simplicity
    // Opening existing files for modification not supported
    setError(FileSystemErrors::FILE_OPEN_FAILED, "EEPROM only supports new file creation");
    return false;
}

bool EEPROMFileSystem::writeData(const uint8_t* data, uint16_t length) {
    if (!_hasActiveFile) {
        setError(FileSystemErrors::FILE_WRITE_FAILED, "No active file");
        return false;
    }
    
    // Check available space
    if (_currentAddress + length >= _eeprom.getSize()) {
        setError(FileSystemErrors::INSUFFICIENT_SPACE, "EEPROM full");
        return false;
    }
    
    // Write data to EEPROM using page write
    if (!_eeprom.writePage(_currentAddress, data, length)) {
        setError(FileSystemErrors::FILE_WRITE_FAILED, "EEPROM write failed");
        return false;
    }
    
    _currentAddress += length;
    _bytesWritten += length;
    clearError();
    return true;
}

bool EEPROMFileSystem::closeFile() {
    if (!_hasActiveFile) {
        return true; // Already closed
    }
    
    // Calculate file size
    uint16_t dataSize = _currentAddress - _fileStartAddress - FILE_ENTRY_OVERHEAD;
    
    // Write file header at the beginning of the file
    FileHeader header;
    header.magic = 0xFEED;  // File validity marker
    header.fileSize = dataSize;
    header.filenameLen = 0; // Will be set when we know the filename
    header.fileType = 0;    // Generic binary for now
    header.checksum = 0;    // TODO: Calculate checksum
    
    // Write header to EEPROM
    uint8_t* headerBytes = (uint8_t*)&header;
    _eeprom.writePage(_fileStartAddress, headerBytes, sizeof(FileHeader));
    
    _hasActiveFile = false;
    clearError();
    return true;
}

bool EEPROMFileSystem::deleteFile(const char* filename) {
    // EEPROM file system doesn't support individual file deletion
    // Use format() to clear all files
    setError(FileSystemErrors::FILE_DELETE_FAILED, "Use format() to clear EEPROM");
    return false;
}

bool EEPROMFileSystem::fileExists(const char* filename) {
    // Simple implementation - scan through EEPROM to find file
    // Not implemented for basic functionality
    return false;
}

bool EEPROMFileSystem::listFiles(char* buffer, uint16_t bufferSize) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM not available");
        return false;
    }
    
    // Simple implementation - just show file count
    snprintf(buffer, bufferSize, "EEPROM: %lu files\n", getFilesCreated());
    clearError();
    return true;
}

uint32_t EEPROMFileSystem::getFileCount() {
    // Return number of files created (approximate)
    return _filesCreated;
}

uint32_t EEPROMFileSystem::getTotalSpace() {
    return _eeprom.getSize();
}

uint32_t EEPROMFileSystem::getFreeSpace() {
    if (!isAvailable()) {
        return 0;
    }
    
    return _eeprom.getSize() - _bytesWritten;
}

bool EEPROMFileSystem::format() {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM not available");
        return false;
    }
    
    // Clear entire EEPROM by erasing the chip
    if (!_eeprom.eraseChip()) {
        setError(FileSystemErrors::HARDWARE_ERROR, "EEPROM erase failed");
        return false;
    }
    
    // Reset counters
    _currentAddress = 0;
    _fileStartAddress = 0;
    _bytesWritten = 0;
    _filesCreated = 0;
    _hasActiveFile = false;
    
    clearError();
    return true;
}

bool EEPROMFileSystem::defragment() {
    // TODO: Implement defragmentation
    return true;
}

uint16_t EEPROMFileSystem::getWearLevel() {
    // TODO: Implement wear level tracking
    return 0;
}

bool EEPROMFileSystem::verifyAllFiles() {
    // TODO: Implement file verification
    return true;
}

// Private methods
uint16_t EEPROMFileSystem::calculateAvailableSpace() {
    if (!_initialized) {
        return 0;
    }
    
    return _eeprom.getSize() - _bytesWritten;
}

bool EEPROMFileSystem::writeFileHeader(const char* filename, uint16_t dataSize) {
    // TODO: Implement file header writing
    return true;
}

bool EEPROMFileSystem::findNextWriteAddress() {
    // For basic implementation, files are written sequentially
    // More sophisticated implementation would scan for deleted files
    
    if (_currentAddress >= _eeprom.getSize()) {
        return false; // No more space
    }
    
    return true;
}

uint16_t EEPROMFileSystem::calculateChecksum(const uint8_t* data, uint16_t length) {
    uint16_t checksum = 0;
    for (uint16_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

bool EEPROMFileSystem::verifyIntegrity(uint16_t address, uint16_t expectedSize) {
    // TODO: Implement integrity verification
    return true;
}

} // namespace DeviceBridge::Storage