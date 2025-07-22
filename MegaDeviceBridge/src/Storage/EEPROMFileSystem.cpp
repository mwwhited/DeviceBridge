#include "EEPROMFileSystem.h"
#include <string.h>

namespace DeviceBridge::Storage {

EEPROMFileSystem::EEPROMFileSystem() 
    : _eeprom(Common::Pins::EEPROM_CS), _initialized(false), _mounted(false),
      _currentFileAddress(0), _currentFileSize(0) {
    clearError();
    memset(_currentFilename, 0, sizeof(_currentFilename));
}

EEPROMFileSystem::~EEPROMFileSystem() {
    shutdown();
}

bool EEPROMFileSystem::initialize() {
    Serial.print(F("EEPROM: Minimal FS initialization...\r\n"));
    
    if (!_eeprom.initialize()) {
        Serial.print(F("EEPROM: W25Q128 not detected - disabled\r\n"));
        setError(FileSystemErrors::INIT_FAILED, "W25Q128 initialization failed");
        _initialized = false;
        _mounted = false;
        return false;
    }
    
    Serial.print(F("EEPROM: W25Q128 detected - minimal FS ready\r\n"));
    _initialized = true;
    _mounted = true;
    clearError();
    return true;
}

bool EEPROMFileSystem::isAvailable() const {
    return _initialized && _mounted;
}

void EEPROMFileSystem::shutdown() {
    if (_hasActiveFile) {
        closeFile();
    }
    _initialized = false;
    _mounted = false;
}

bool EEPROMFileSystem::createFile(const char* filename) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM not available");
        return false;
    }
    
    if (_hasActiveFile) {
        closeFile();
    }
    
    if (!isValidFilename(filename)) {
        setError(FileSystemErrors::INVALID_FILENAME, "Invalid filename format");
        return false;
    }
    
    // Check if file already exists by scanning EEPROM
    if (scanForFile(filename) >= 0) {
        setError(FileSystemErrors::FILE_EXISTS, "File already exists");
        return false;
    }
    
    // Find free directory slot
    int freeSlot = findFreeDirectorySlot();
    if (freeSlot < 0) {
        setError(FileSystemErrors::INSUFFICIENT_SPACE, "Directory full");
        return false;
    }
    
    // Find next free file address
    uint32_t fileAddress = findNextFreeFileAddress();
    if (fileAddress + 1024 > FLASH_SIZE) { // Ensure minimum space
        setError(FileSystemErrors::INSUFFICIENT_SPACE, "Not enough flash space");
        return false;
    }
    
    // Create directory entry
    DirectoryEntry entry;
    memset(&entry, 0, sizeof(entry));
    strncpy(entry.filename, filename, FILENAME_LENGTH - 1);
    entry.filename[FILENAME_LENGTH - 1] = '\0';
    entry.address = fileAddress;
    entry.size = 0;
    entry.crc32 = calculateCRC32(filename);
    entry.reserved = FLAG_USED;
    
    // Write directory entry to EEPROM
    if (!writeDirectoryEntry(freeSlot, entry)) {
        setError(FileSystemErrors::FILE_WRITE_FAILED, "Directory write failed");
        return false;
    }
    
    // Setup current file tracking
    _currentFileAddress = fileAddress;
    _currentFileSize = 0;
    strncpy(_currentFilename, filename, sizeof(_currentFilename) - 1);
    _currentFilename[sizeof(_currentFilename) - 1] = '\0';
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
    
    if (_hasActiveFile) {
        closeFile();
    }
    
    // Scan for file in EEPROM
    int fileSlot = scanForFile(filename);
    if (fileSlot < 0) {
        setError(FileSystemErrors::FILE_NOT_FOUND, "File not found");
        return false;
    }
    
    // Read directory entry
    DirectoryEntry entry;
    if (!readDirectoryEntry(fileSlot, entry)) {
        setError(FileSystemErrors::DIRECTORY_READ_FAILED, "Directory read failed");
        return false;
    }
    
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
        setError(FileSystemErrors::INVALID_PARAMETER, "Invalid data");
        return false;
    }
    
    uint32_t writeAddress = _currentFileAddress + _currentFileSize;
    if (writeAddress + length > FLASH_SIZE) {
        setError(FileSystemErrors::INSUFFICIENT_SPACE, "Not enough space");
        return false;
    }
    
    // Write data in page-aligned chunks
    uint32_t remaining = length;
    uint32_t dataOffset = 0;
    
    while (remaining > 0) {
        uint32_t pageSize = _eeprom.getPageSize();
        uint32_t pageOffset = writeAddress % pageSize;
        uint32_t bytesToWrite = (remaining < (pageSize - pageOffset)) ? remaining : (pageSize - pageOffset);
        
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
    clearError();
    return true;
}

bool EEPROMFileSystem::closeFile() {
    if (!_hasActiveFile) {
        return true;
    }
    
    // Find file entry and update size
    int fileSlot = scanForFile(_currentFilename);
    if (fileSlot >= 0) {
        DirectoryEntry entry;
        if (readDirectoryEntry(fileSlot, entry)) {
            entry.size = _currentFileSize;
            writeDirectoryEntry(fileSlot, entry);
        }
    }
    
    _hasActiveFile = false;
    _currentFileAddress = 0;
    _currentFileSize = 0;
    memset(_currentFilename, 0, sizeof(_currentFilename));
    
    clearError();
    return true;
}

bool EEPROMFileSystem::deleteFile(const char* filename) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM not available");
        return false;
    }
    
    int fileSlot = scanForFile(filename);
    if (fileSlot < 0) {
        setError(FileSystemErrors::FILE_NOT_FOUND, "File not found");
        return false;
    }
    
    // Mark entry as deleted
    DirectoryEntry entry;
    if (readDirectoryEntry(fileSlot, entry)) {
        entry.reserved = FLAG_DELETED;
        writeDirectoryEntry(fileSlot, entry);
    }
    
    clearError();
    return true;
}

bool EEPROMFileSystem::fileExists(const char* filename) {
    return isAvailable() && (scanForFile(filename) >= 0);
}

bool EEPROMFileSystem::listFiles(char* buffer, uint16_t bufferSize) {
    if (!isAvailable() || !buffer || bufferSize < 50) {
        setError(FileSystemErrors::INVALID_PARAMETER, "Invalid parameters");
        return false;
    }
    
    uint16_t offset = 0;
    uint32_t fileCount = 0;
    
    offset += snprintf(buffer + offset, bufferSize - offset, 
                      "EEPROM Minimal FS:\r\n");
    
    // Scan all directory entries
    for (int i = 0; i < MAX_FILES && offset < bufferSize - 50; i++) {
        DirectoryEntry entry;
        if (readDirectoryEntry(i, entry) && entry.reserved == FLAG_USED && entry.filename[0] != '\0') {
            offset += snprintf(buffer + offset, bufferSize - offset,
                             "  %s (%lu bytes)\r\n", entry.filename, entry.size);
            fileCount++;
        }
    }
    
    if (offset < bufferSize - 30) {
        offset += snprintf(buffer + offset, bufferSize - offset,
                          "Total: %lu files\r\n", fileCount);
    }
    
    clearError();
    return true;
}

uint32_t EEPROMFileSystem::getFileCount() {
    uint32_t count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        DirectoryEntry entry;
        if (readDirectoryEntry(i, entry) && entry.reserved == FLAG_USED && entry.filename[0] != '\0') {
            count++;
        }
    }
    return count;
}

uint32_t EEPROMFileSystem::getTotalSpace() {
    return FLASH_SIZE - FILE_DATA_START;
}

uint32_t EEPROMFileSystem::getFreeSpace() {
    uint32_t nextFree = findNextFreeFileAddress();
    return (nextFree < FLASH_SIZE) ? (FLASH_SIZE - nextFree) : 0;
}

bool EEPROMFileSystem::format() {
    if (!_initialized) {
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM not initialized");
        return false;
    }
    
    if (_hasActiveFile) {
        closeFile();
    }
    
    Serial.print(F("EEPROM: Formatting (erasing directory sectors)...\r\n"));
    
    // Erase first two sectors (directory)
    if (!_eeprom.eraseSector(0) || !_eeprom.eraseSector(SECTOR_SIZE)) {
        setError(FileSystemErrors::HARDWARE_ERROR, "Directory erase failed");
        return false;
    }
    
    _bytesWritten = 0;
    _filesCreated = 0;
    
    Serial.print(F("EEPROM: Format complete\r\n"));
    clearError();
    return true;
}

bool EEPROMFileSystem::flush() {
    return true; // No caching, nothing to flush
}

bool EEPROMFileSystem::sync() {
    return true; // No caching, nothing to sync
}

uint32_t EEPROMFileSystem::getFileSize(const char* filename) {
    int fileSlot = scanForFile(filename);
    if (fileSlot < 0) return 0;
    
    DirectoryEntry entry;
    if (readDirectoryEntry(fileSlot, entry)) {
        return entry.size;
    }
    return 0;
}

bool EEPROMFileSystem::readFileSegment(const char* filename, uint32_t offset, uint8_t* buffer, uint16_t length) {
    int fileSlot = scanForFile(filename);
    if (fileSlot < 0) {
        setError(FileSystemErrors::FILE_NOT_FOUND, "File not found");
        return false;
    }
    
    DirectoryEntry entry;
    if (!readDirectoryEntry(fileSlot, entry)) {
        setError(FileSystemErrors::DIRECTORY_READ_FAILED, "Directory read failed");
        return false;
    }
    
    if (offset >= entry.size || offset + length > entry.size) {
        setError(FileSystemErrors::INVALID_PARAMETER, "Read beyond file");
        return false;
    }
    
    return _eeprom.readData(entry.address + offset, buffer, length);
}

// Private methods
int EEPROMFileSystem::scanForFile(const char* filename) {
    if (!filename) return -1;
    
    uint32_t targetCrc = calculateCRC32(filename);
    
    // Scan directory entries in EEPROM
    for (int i = 0; i < MAX_FILES; i++) {
        DirectoryEntry entry;
        if (readDirectoryEntry(i, entry)) {
            if (entry.reserved == FLAG_USED && entry.crc32 == targetCrc && 
                strcmp(entry.filename, filename) == 0) {
                return i;
            }
        }
    }
    return -1;
}

int EEPROMFileSystem::findFreeDirectorySlot() {
    for (int i = 0; i < MAX_FILES; i++) {
        DirectoryEntry entry;
        if (!readDirectoryEntry(i, entry) || entry.reserved == FLAG_UNUSED || entry.reserved == FLAG_DELETED) {
            return i;
        }
    }
    return -1;
}

bool EEPROMFileSystem::readDirectoryEntry(int index, DirectoryEntry& entry) {
    if (index < 0 || index >= MAX_FILES) return false;
    
    uint32_t address = (index * sizeof(DirectoryEntry));
    return _eeprom.readData(address, (uint8_t*)&entry, sizeof(entry));
}

bool EEPROMFileSystem::writeDirectoryEntry(int index, const DirectoryEntry& entry) {
    if (index < 0 || index >= MAX_FILES) return false;
    
    uint32_t address = (index * sizeof(DirectoryEntry));
    
    // Check if we need to erase the sector
    uint32_t sectorAddress = (address / SECTOR_SIZE) * SECTOR_SIZE;
    DirectoryEntry testEntry;
    if (!_eeprom.readData(address, (uint8_t*)&testEntry, sizeof(testEntry)) ||
        testEntry.reserved != FLAG_UNUSED) {
        // Need to read entire sector, modify, erase, and rewrite
        uint8_t* sectorBuffer = new uint8_t[SECTOR_SIZE];
        if (!sectorBuffer) return false;
        
        // Read current sector
        if (_eeprom.readData(sectorAddress, sectorBuffer, SECTOR_SIZE)) {
            // Modify the entry
            memcpy(sectorBuffer + (address - sectorAddress), &entry, sizeof(entry));
            
            // Erase and rewrite
            if (_eeprom.eraseSector(sectorAddress)) {
                uint32_t pageSize = _eeprom.getPageSize();
                for (uint32_t offset = 0; offset < SECTOR_SIZE; offset += pageSize) {
                    _eeprom.writePage(sectorAddress + offset, sectorBuffer + offset, pageSize);
                }
            }
        }
        delete[] sectorBuffer;
        return true;
    }
    
    // Can write directly
    return _eeprom.writePage(address, (const uint8_t*)&entry, sizeof(entry));
}

bool EEPROMFileSystem::isValidFilename(const char* filename) {
    if (!filename) return false;
    
    size_t len = strlen(filename);
    if (len < 14 || len >= FILENAME_LENGTH) return false;
    
    // Check format: "00001122\\334455.EXT"
    // Should be 8 digits + backslash + 6 digits + dot + extension
    for (int i = 0; i < 8; i++) {
        if (!isdigit(filename[i])) return false;
    }
    
    if (filename[8] != '\\') return false;
    
    for (int i = 9; i < 15; i++) {
        if (!isdigit(filename[i])) return false;
    }
    
    if (filename[15] != '.') return false;
    
    // Extension should be 2-3 characters
    return (len == 18 || len == 19);
}

uint32_t EEPROMFileSystem::calculateCRC32(const char* filename) {
    // Simple CRC32 for fast filename lookup
    uint32_t crc = 0xffffffff;
    const char* data = filename;
    
    while (*data) {
        crc ^= *data++;
        for (int k = 0; k < 8; k++) {
            crc = (crc >> 1) ^ (0xedb88320 & -(crc & 1));
        }
    }
    
    return ~crc;
}

uint32_t EEPROMFileSystem::findNextFreeFileAddress() {
    uint32_t maxAddress = FILE_DATA_START;
    
    for (int i = 0; i < MAX_FILES; i++) {
        DirectoryEntry entry;
        if (readDirectoryEntry(i, entry) && entry.reserved == FLAG_USED) {
            uint32_t fileEnd = entry.address + entry.size;
            if (fileEnd > maxAddress) {
                maxAddress = fileEnd;
            }
        }
    }
    
    // Align to sector boundary
    return ((maxAddress + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
}

} // namespace DeviceBridge::Storage