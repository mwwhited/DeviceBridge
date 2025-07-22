#include "EEPROMFileSystem.h"
#include <string.h>

namespace DeviceBridge::Storage {

EEPROMFileSystem::EEPROMFileSystem() 
    : _eeprom(Common::Pins::EEPROM_CS), _initialized(false), _mounted(false),
      _currentFileAddress(0), _currentFileSize(0), _totalFiles(0), 
      _totalSpace(FLASH_SIZE), _usedSpace(0), _nextFreeAddress(FILE_DATA_START),
      _directoryLoaded(false), _directoryModified(false) {
    clearError();
    memset(_currentFilename, 0, sizeof(_currentFilename));
    memset(_directory, 0, sizeof(_directory));
}

EEPROMFileSystem::~EEPROMFileSystem() {
    shutdown();
}

bool EEPROMFileSystem::initialize() {
    Serial.print(F("EEPROM: Starting initialization...\r\n"));
    Serial.flush();
    
    // Try to initialize W25Q128, but don't fail completely if it doesn't work
    Serial.print(F("EEPROM: Attempting W25Q128 detection...\r\n"));
    Serial.flush();
    
    if (!_eeprom.initialize()) {
        Serial.print(F("EEPROM: W25Q128 flash chip not detected - EEPROM storage disabled\r\n"));
        Serial.flush();
        setError(FileSystemErrors::INIT_FAILED, "W25Q128 EEPROM initialization failed");
        _initialized = false;
        _mounted = false;
        return false; // Return false but don't hang the system
    }
    
    Serial.print(F("EEPROM: W25Q128 detected successfully\r\n"));
    Serial.flush();
    
    // Try to load directory, but create a minimal one if it fails
    Serial.print(F("EEPROM: Loading directory...\r\n"));
    Serial.flush();
    
    if (!loadDirectory()) {
        Serial.print(F("EEPROM: Creating new directory...\r\n"));
        Serial.flush();
        
        // Create minimal directory without writing to flash initially
        memset(_directory, 0, sizeof(_directory));
        _directoryLoaded = true;
        _directoryModified = false;
        _nextFreeAddress = FILE_DATA_START;
    }
    
    _initialized = true;
    _mounted = true;
    
    clearError();
    Serial.print(F("EEPROM: Initialization complete\r\n"));
    Serial.flush();
    
    return true;
}

bool EEPROMFileSystem::isAvailable() const {
    return _initialized && _mounted;
}

void EEPROMFileSystem::shutdown() {
    if (_hasActiveFile) {
        closeFile();
    }
    
    if (_directoryModified) {
        saveDirectory();
    }
    
    _initialized = false;
    _mounted = false;
}

bool EEPROMFileSystem::createFile(const char* filename) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM file system not available");
        return false;
    }
    
    if (_hasActiveFile) {
        closeFile();
    }
    
    // Validate filename
    if (!filename || strlen(filename) == 0 || strlen(filename) >= FILENAME_LENGTH) {
        setError(FileSystemErrors::INVALID_FILENAME, "Invalid filename");
        return false;
    }
    
    // Check if file already exists
    if (findFileEntry(filename) >= 0) {
        setError(FileSystemErrors::FILE_EXISTS, "File already exists");
        return false;
    }
    
    // Find free directory entry
    int entryIndex = findFreeEntry();
    if (entryIndex < 0) {
        setError(FileSystemErrors::INSUFFICIENT_SPACE, "Directory full");
        return false;
    }
    
    // Setup directory entry
    DirectoryEntry& entry = _directory[entryIndex];
    strncpy(entry.filename, filename, FILENAME_LENGTH - 1);
    entry.filename[FILENAME_LENGTH - 1] = '\0';
    entry.address = _nextFreeAddress;
    entry.size = 0;
    entry.flags = FLAG_USED;
    
    // Setup current file tracking
    _currentFileAddress = entry.address;
    _currentFileSize = 0;
    strncpy(_currentFilename, filename, sizeof(_currentFilename) - 1);
    _currentFilename[sizeof(_currentFilename) - 1] = '\0';
    _hasActiveFile = true;
    _filesCreated++;
    
    _directoryModified = true;
    clearError();
    return true;
}

bool EEPROMFileSystem::openFile(const char* filename, bool append) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM file system not available");
        return false;
    }
    
    if (_hasActiveFile) {
        closeFile();
    }
    
    // Validate filename
    if (!filename || strlen(filename) == 0) {
        setError(FileSystemErrors::INVALID_FILENAME, "Invalid filename");
        return false;
    }
    
    // Find file entry
    int entryIndex = findFileEntry(filename);
    if (entryIndex < 0) {
        setError(FileSystemErrors::FILE_NOT_FOUND, "File not found");
        return false;
    }
    
    DirectoryEntry& entry = _directory[entryIndex];
    
    // Setup current file tracking
    _currentFileAddress = entry.address;
    _currentFileSize = entry.size;
    strncpy(_currentFilename, filename, sizeof(_currentFilename) - 1);
    _currentFilename[sizeof(_currentFilename) - 1] = '\0';
    _hasActiveFile = true;
    
    clearError();
    return true;
}

bool EEPROMFileSystem::writeData(const uint8_t* data, uint16_t length) {
    if (!_hasActiveFile) {
        setError(FileSystemErrors::FILE_WRITE_FAILED, "No active file");
        return false;
    }
    
    if (!data || length == 0) {
        setError(FileSystemErrors::INVALID_PARAMETER, "Invalid data or length");
        return false;
    }
    
    // Check if we have enough space
    uint32_t writeAddress = _currentFileAddress + _currentFileSize;
    if (writeAddress + length > FLASH_SIZE) {
        setError(FileSystemErrors::INSUFFICIENT_SPACE, "Not enough flash space");
        return false;
    }
    
    // Write data to flash
    uint32_t remaining = length;
    uint32_t dataOffset = 0;
    
    while (remaining > 0) {
        uint32_t pageSize = _eeprom.getPageSize();
        uint32_t pageOffset = writeAddress % pageSize;
        uint32_t bytesToWrite = min(remaining, pageSize - pageOffset);
        
        if (!_eeprom.writePage(writeAddress, data + dataOffset, bytesToWrite)) {
            setError(FileSystemErrors::FILE_WRITE_FAILED, "Flash write failed");
            return false;
        }
        
        writeAddress += bytesToWrite;
        dataOffset += bytesToWrite;
        remaining -= bytesToWrite;
    }
    
    _currentFileSize += length;
    _bytesWritten += length;
    _directoryModified = true;
    
    clearError();
    return true;
}

bool EEPROMFileSystem::closeFile() {
    if (!_hasActiveFile) {
        return true; // Already closed
    }
    
    // Update directory entry with final file size
    int entryIndex = findFileEntry(_currentFilename);
    if (entryIndex >= 0) {
        _directory[entryIndex].size = _currentFileSize;
        _directoryModified = true;
    }
    
    // Update next free address
    _nextFreeAddress = max(_nextFreeAddress, _currentFileAddress + _currentFileSize);
    
    _hasActiveFile = false;
    _currentFileAddress = 0;
    _currentFileSize = 0;
    memset(_currentFilename, 0, sizeof(_currentFilename));
    
    // Save directory changes
    if (_directoryModified) {
        saveDirectory();
        _directoryModified = false;
    }
    
    clearError();
    return true;
}

bool EEPROMFileSystem::deleteFile(const char* filename) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM file system not available");
        return false;
    }
    
    if (!filename) {
        setError(FileSystemErrors::INVALID_FILENAME, "Invalid filename");
        return false;
    }
    
    // Find file entry
    int entryIndex = findFileEntry(filename);
    if (entryIndex < 0) {
        setError(FileSystemErrors::FILE_NOT_FOUND, "File not found");
        return false;
    }
    
    // Mark entry as deleted
    _directory[entryIndex].flags = FLAG_DELETED;
    _directoryModified = true;
    _totalFiles--;
    
    // Save directory immediately
    saveDirectory();
    _directoryModified = false;
    
    clearError();
    return true;
}

bool EEPROMFileSystem::fileExists(const char* filename) {
    if (!isAvailable() || !filename) {
        return false;
    }
    
    return findFileEntry(filename) >= 0;
}

bool EEPROMFileSystem::listFiles(char* buffer, uint16_t bufferSize) {
    if (!isAvailable() || !buffer || bufferSize == 0) {
        setError(FileSystemErrors::INVALID_PARAMETER, "Invalid parameters");
        return false;
    }
    
    uint16_t offset = 0;
    uint32_t fileCount = 0;
    
    // Add header
    offset += snprintf(buffer + offset, bufferSize - offset, 
                      "EEPROM Simple FS Files:\r\n");
    
    // List all files
    for (int i = 0; i < MAX_FILES && offset < bufferSize - 50; i++) {
        DirectoryEntry& entry = _directory[i];
        if (entry.flags == FLAG_USED && entry.filename[0] != '\0') {
            offset += snprintf(buffer + offset, bufferSize - offset,
                             "  %s (%u bytes)\r\n", entry.filename, entry.size);
            fileCount++;
        }
    }
    
    // Add summary
    if (offset < bufferSize - 30) {
        offset += snprintf(buffer + offset, bufferSize - offset,
                          "Total: %u files\r\n", fileCount);
    }
    
    _totalFiles = fileCount;
    clearError();
    return true;
}

uint32_t EEPROMFileSystem::getFileCount() {
    uint32_t count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (_directory[i].flags == FLAG_USED && _directory[i].filename[0] != '\0') {
            count++;
        }
    }
    _totalFiles = count;
    return count;
}

uint32_t EEPROMFileSystem::getTotalSpace() {
    return FLASH_SIZE - DIRECTORY_SIZE; // Usable space
}

uint32_t EEPROMFileSystem::getFreeSpace() {
    return FLASH_SIZE - _nextFreeAddress;
}

bool EEPROMFileSystem::format() {
    if (!_initialized) {
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM not initialized");
        return false;
    }
    
    // Close any active file
    if (_hasActiveFile) {
        closeFile();
    }
    
    // Erase entire flash (this will take a while)
    Serial.print(F("EEPROM: Formatting flash memory (this may take a moment)...\r\n"));
    
    // Erase directory sector
    if (!_eeprom.eraseSector(DIRECTORY_ADDRESS)) {
        setError(FileSystemErrors::HARDWARE_ERROR, "Directory erase failed");
        return false;
    }
    
    // Initialize clean directory
    if (!initializeDirectory()) {
        setError(FileSystemErrors::HARDWARE_ERROR, "Directory initialization failed");
        return false;
    }
    
    // Reset statistics
    _bytesWritten = 0;
    _filesCreated = 0;
    _totalFiles = 0;
    _nextFreeAddress = FILE_DATA_START;
    
    Serial.print(F("EEPROM: Format completed successfully\r\n"));
    clearError();
    return true;
}

bool EEPROMFileSystem::flush() {
    if (_directoryModified) {
        return saveDirectory();
    }
    return true;
}

bool EEPROMFileSystem::sync() {
    return flush();
}

uint32_t EEPROMFileSystem::getFileSize(const char* filename) {
    if (!isAvailable() || !filename) {
        return 0;
    }
    
    int entryIndex = findFileEntry(filename);
    if (entryIndex < 0) {
        return 0;
    }
    
    return _directory[entryIndex].size;
}

bool EEPROMFileSystem::defragment() {
    // Simple defragmentation would require significant implementation
    // For now, return true as a no-op
    return true;
}

// Private methods
bool EEPROMFileSystem::initializeDirectory() {
    // Clear directory in memory
    memset(_directory, 0, sizeof(_directory));
    
    // Save empty directory to flash
    if (!saveDirectory()) {
        return false;
    }
    
    _directoryLoaded = true;
    _directoryModified = false;
    _nextFreeAddress = FILE_DATA_START;
    
    return true;
}

bool EEPROMFileSystem::loadDirectory() {
    // Read directory from flash
    if (!_eeprom.readData(DIRECTORY_ADDRESS, (uint8_t*)_directory, sizeof(_directory))) {
        return false;
    }
    
    // Basic validation - check if first entry looks reasonable
    bool validDirectory = true;
    for (int i = 0; i < 4; i++) { // Check first few entries
        DirectoryEntry& entry = _directory[i];
        if (entry.flags > FLAG_DELETED || entry.address < FILE_DATA_START || 
            entry.address >= FLASH_SIZE || entry.size > FLASH_SIZE) {
            validDirectory = false;
            break;
        }
    }
    
    if (!validDirectory) {
        return false;
    }
    
    _directoryLoaded = true;
    _directoryModified = false;
    
    return true;
}

bool EEPROMFileSystem::saveDirectory() {
    // Erase directory sector first
    if (!_eeprom.eraseSector(DIRECTORY_ADDRESS)) {
        return false;
    }
    
    // Write directory in pages
    uint32_t remaining = sizeof(_directory);
    uint32_t dataOffset = 0;
    uint32_t writeAddress = DIRECTORY_ADDRESS;
    
    while (remaining > 0) {
        uint32_t pageSize = _eeprom.getPageSize();
        uint32_t bytesToWrite = min(remaining, pageSize);
        
        if (!_eeprom.writePage(writeAddress, ((uint8_t*)_directory) + dataOffset, bytesToWrite)) {
            return false;
        }
        
        writeAddress += bytesToWrite;
        dataOffset += bytesToWrite;
        remaining -= bytesToWrite;
    }
    
    _directoryModified = false;
    return true;
}

int EEPROMFileSystem::findFileEntry(const char* filename) {
    if (!filename) {
        return -1;
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        DirectoryEntry& entry = _directory[i];
        if (entry.flags == FLAG_USED && strcmp(entry.filename, filename) == 0) {
            return i;
        }
    }
    
    return -1;
}

int EEPROMFileSystem::findFreeEntry() {
    for (int i = 0; i < MAX_FILES; i++) {
        DirectoryEntry& entry = _directory[i];
        if (entry.flags == FLAG_UNUSED || entry.flags == FLAG_DELETED) {
            return i;
        }
    }
    
    return -1;
}

uint32_t EEPROMFileSystem::calculateNextFreeAddress() {
    uint32_t maxAddress = FILE_DATA_START;
    
    for (int i = 0; i < MAX_FILES; i++) {
        DirectoryEntry& entry = _directory[i];
        if (entry.flags == FLAG_USED) {
            uint32_t fileEnd = entry.address + entry.size;
            if (fileEnd > maxAddress) {
                maxAddress = fileEnd;
            }
        }
    }
    
    return maxAddress;
}

bool EEPROMFileSystem::eraseFile(uint32_t address, uint32_t size) {
    // Erase sectors containing the file
    uint32_t startSector = address / SECTOR_SIZE;
    uint32_t endSector = (address + size - 1) / SECTOR_SIZE;
    
    for (uint32_t sector = startSector; sector <= endSector; sector++) {
        if (!_eeprom.eraseSector(sector * SECTOR_SIZE)) {
            return false;
        }
    }
    
    return true;
}

} // namespace DeviceBridge::Storage