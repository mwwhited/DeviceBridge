#include "EEPROMFileSystem.h"
#include "../Components/SystemManager.h"
#include "../Common/ServiceLocator.h"
#include <string.h>

namespace DeviceBridge::Storage {

// Helper function to check if EEPROM debug is enabled
static bool isEEPROMDebugEnabled() {
    auto& services = DeviceBridge::ServiceLocator::getInstance();
    auto* systemManager = services.getSystemManager();
    return systemManager && systemManager->isEEPROMDebugEnabled();
}

// Macro for conditional debug printing - support multiple args
#define EEPROM_DEBUG_PRINT(...) do { if (isEEPROMDebugEnabled()) { Serial.print(__VA_ARGS__); } } while(0)
#define EEPROM_DEBUG_PRINTLN(...) do { if (isEEPROMDebugEnabled()) { Serial.println(__VA_ARGS__); } } while(0)

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
    
    _initialized = true;
    
    // Check if filesystem is formatted by reading first directory entry
    DirectoryEntry firstEntry;
    if (!_eeprom.readData(0, (uint8_t*)&firstEntry, sizeof(firstEntry))) {
        Serial.println(F("EEPROM: ⚠️ Cannot read directory - formatting..."));
        if (!format()) {
            Serial.println(F("EEPROM: ❌ Format failed"));
            _mounted = false;
            return false;
        }
    }
    
    Serial.print(F("EEPROM: W25Q128 detected - minimal FS ready\r\n"));
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
    EEPROM_DEBUG_PRINT(F("EEPROM: Creating file: "));
    EEPROM_DEBUG_PRINTLN(filename);
    
    if (!isAvailable()) {
        Serial.println(F("EEPROM: ❌ Not available"));
        setError(FileSystemErrors::NOT_AVAILABLE, "EEPROM not available");
        return false;
    }
    
    if (_hasActiveFile) {
        Serial.println(F("EEPROM: Closing existing file"));
        closeFile();
    }
    
    if (!isValidFilename(filename)) {
        Serial.println(F("EEPROM: ❌ Invalid filename"));
        setError(FileSystemErrors::INVALID_FILENAME, "Invalid filename format");
        return false;
    }
    
    // Check if file already exists by scanning EEPROM
    if (scanForFile(filename) >= 0) {
        Serial.println(F("EEPROM: ❌ File exists"));
        setError(FileSystemErrors::FILE_EXISTS, "File already exists");
        return false;
    }
    
    // Find free directory slot
    int freeSlot = findFreeDirectorySlot();
    EEPROM_DEBUG_PRINT(F("EEPROM: Free slot: "));
    EEPROM_DEBUG_PRINTLN(freeSlot);
    if (freeSlot < 0) {
        Serial.println(F("EEPROM: ❌ Directory full"));
        setError(FileSystemErrors::INSUFFICIENT_SPACE, "Directory full");
        return false;
    }
    
    // Find next free file address
    uint32_t fileAddress = findNextFreeFileAddress();
    EEPROM_DEBUG_PRINT(F("EEPROM: File address: 0x"));
    EEPROM_DEBUG_PRINTLN(fileAddress, HEX);
    EEPROM_DEBUG_PRINT(F("EEPROM: FLASH_SIZE: "));
    EEPROM_DEBUG_PRINTLN(FLASH_SIZE);
    EEPROM_DEBUG_PRINT(F("EEPROM: Space check: "));
    EEPROM_DEBUG_PRINT(fileAddress + 1024);
    EEPROM_DEBUG_PRINT(F(" > "));
    EEPROM_DEBUG_PRINT(FLASH_SIZE);
    EEPROM_DEBUG_PRINT(F(" = "));
    EEPROM_DEBUG_PRINTLN((fileAddress + 1024 > FLASH_SIZE) ? F("TRUE") : F("FALSE"));
    
    if (fileAddress + 1024 > FLASH_SIZE) { // Ensure minimum space
        Serial.println(F("EEPROM: ❌ Not enough space"));
        setError(FileSystemErrors::INSUFFICIENT_SPACE, "Not enough flash space");
        return false;
    }
    
    // Create directory entry
    DirectoryEntry entry;
    memset(&entry, 0, sizeof(entry));
    strncpy(entry.filename, filename, FILENAME_LENGTH - 1);
    entry.filename[FILENAME_LENGTH - 1] = '\0';
    entry.address = fileAddress;
    entry.size = 0xFFFFFFFF;  // Pre-allocate maximum size for Flash memory constraints
    entry.crc32 = calculateCRC32(filename);
    entry.reserved = FLAG_USED;
    
    EEPROM_DEBUG_PRINTLN(F("EEPROM: Writing directory entry..."));
    // Write directory entry to EEPROM
    if (!writeDirectoryEntry(freeSlot, entry)) {
        EEPROM_DEBUG_PRINTLN(F("EEPROM: ❌ Directory write failed"));
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
    _currentFileSize = (entry.size == 0xFFFFFFFF) ? 0 : ~entry.size;
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
    
    EEPROM_DEBUG_PRINT(F("EEPROM: Closing file: "));
    EEPROM_DEBUG_PRINTLN(_currentFilename);
    EEPROM_DEBUG_PRINT(F("EEPROM: Final size: "));
    EEPROM_DEBUG_PRINTLN(_currentFileSize);
    
    // Find file entry and update size
    int fileSlot = scanForFile(_currentFilename);
    EEPROM_DEBUG_PRINT(F("EEPROM: File slot: "));
    EEPROM_DEBUG_PRINTLN(fileSlot);
    
    if (fileSlot >= 0) {
        DirectoryEntry entry;
        if (readDirectoryEntry(fileSlot, entry)) {
            EEPROM_DEBUG_PRINT(F("EEPROM: Current entry size: "));
            EEPROM_DEBUG_PRINTLN(entry.size);
            // For Flash memory: change 0xFFFFFFFF (all 1s) to ~actualSize (complement)
            // This allows us to only change bits from 1→0, which Flash can do
            entry.size = ~_currentFileSize;  // Store bitwise complement
            EEPROM_DEBUG_PRINTLN(F("EEPROM: Updating directory entry size..."));
            EEPROM_DEBUG_PRINT(F("EEPROM: Setting size to complement: "));
            EEPROM_DEBUG_PRINTLN(entry.size);
            if (writeDirectoryEntry(fileSlot, entry, true)) { // Allow update
                EEPROM_DEBUG_PRINTLN(F("EEPROM: ✅ Directory entry updated"));
            } else {
                EEPROM_DEBUG_PRINTLN(F("EEPROM: ❌ Directory entry update failed"));
            }
        } else {
            Serial.println(F("EEPROM: ❌ Failed to read directory entry"));
        }
    } else {
        Serial.println(F("EEPROM: ❌ File not found in directory"));
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
            // Decode size: if 0xFFFFFFFF, file is still open; otherwise decode complement
            uint32_t actualSize = (entry.size == 0xFFFFFFFF) ? 0 : ~entry.size;
            offset += snprintf(buffer + offset, bufferSize - offset,
                             "  %s (%lu bytes) [DEBUG: reserved=0x%08lx]\r\n", 
                             entry.filename, actualSize, entry.reserved);
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
        // Decode size: if 0xFFFFFFFF, file is still open; otherwise decode complement
        return (entry.size == 0xFFFFFFFF) ? 0 : ~entry.size;
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
    
    uint32_t actualSize = (entry.size == 0xFFFFFFFF) ? 0 : ~entry.size;
    if (offset >= actualSize || offset + length > actualSize) {
        setError(FileSystemErrors::INVALID_PARAMETER, "Read beyond file");
        return false;
    }
    
    return _eeprom.readData(entry.address + offset, buffer, length);
}

bool EEPROMFileSystem::readFile(const char* filename, char* buffer, uint16_t bufferSize) {
    if (!filename || !buffer || bufferSize < 50) {
        setError(FileSystemErrors::INVALID_PARAMETER, "Invalid read parameters");
        return false;
    }
    
    // Find file size
    uint32_t fileSize = getFileSize(filename);
    if (fileSize == 0) {
        setError(FileSystemErrors::FILE_NOT_FOUND, "File not found");
        return false;
    }
    
    uint16_t offset = 0;
    offset += snprintf(buffer + offset, bufferSize - offset, 
                      "File: %s (%lu bytes)\r\n", filename, fileSize);
    
    // Read file in chunks - return raw binary data for EEPROM
    uint8_t readBuffer[64];
    uint32_t bytesRead = 0;
    uint32_t maxBytesToShow = (bufferSize - offset - 50) / 2; // Space for hex representation
    
    if (fileSize > maxBytesToShow) {
        offset += snprintf(buffer + offset, bufferSize - offset,
                          "(Showing first %lu bytes)\r\n", maxBytesToShow);
        fileSize = maxBytesToShow;
    }
    
    while (bytesRead < fileSize && offset < bufferSize - 100) {
        uint16_t chunkSize = (fileSize - bytesRead > 64) ? 64 : (fileSize - bytesRead);
        
        if (readFileSegment(filename, bytesRead, readBuffer, chunkSize)) {
            // Add the raw data (will be interpreted by receiving storage)
            for (uint16_t i = 0; i < chunkSize && offset < bufferSize - 10; i++) {
                buffer[offset++] = readBuffer[i];
            }
            bytesRead += chunkSize;
        } else {
            break;
        }
    }
    
    buffer[offset] = '\0'; // Null terminate
    clearError();
    return true;
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
        if (!readDirectoryEntry(i, entry) || entry.reserved == FLAG_UNUSED || entry.reserved == FLAG_DELETED || entry.reserved == 0xFFFFFFFF) {
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

bool EEPROMFileSystem::writeDirectoryEntry(int index, const DirectoryEntry& entry, bool allowUpdate) {
    EEPROM_DEBUG_PRINT(F("EEPROM: writeDirectoryEntry index="));
    EEPROM_DEBUG_PRINTLN(index);
    
    if (index < 0 || index >= MAX_FILES) {
        Serial.println(F("EEPROM: ❌ Invalid index"));
        return false;
    }
    
    uint32_t address = (index * sizeof(DirectoryEntry));
    EEPROM_DEBUG_PRINT(F("EEPROM: Directory entry address: 0x"));
    EEPROM_DEBUG_PRINTLN(address, HEX);
    
    // Check if we need to erase the sector
    uint32_t sectorAddress = (address / SECTOR_SIZE) * SECTOR_SIZE;
    DirectoryEntry testEntry;
    bool needsErase = false;
    
    if (!_eeprom.readData(address, (uint8_t*)&testEntry, sizeof(testEntry))) {
        Serial.println(F("EEPROM: ⚠️ Read failed, assuming needs erase"));
        needsErase = true;
    } else if (testEntry.reserved != FLAG_UNUSED && testEntry.reserved != 0xFFFFFFFF && testEntry.reserved != FLAG_USED) {
        Serial.print(F("EEPROM: Entry not unused ("));
        Serial.print(testEntry.reserved);
        Serial.println(F("), needs erase"));
        needsErase = true;
    } else if (testEntry.reserved == FLAG_USED && !allowUpdate) {
        Serial.print(F("EEPROM: Entry is used ("));
        Serial.print(testEntry.reserved);
        Serial.println(F("), but update not allowed"));
        needsErase = true;
    }
    
    // Special case: if updating an existing entry, we can write if we're not changing bits from 0 to 1
    if (allowUpdate && testEntry.reserved == FLAG_USED) {
        // For flash memory updates, we can only change 1->0, not 0->1
        // Since we're typically updating size from 0 to a value, this should work
        Serial.println(F("EEPROM: Allowing update of existing entry"));
        needsErase = false;
    }
    
    if (needsErase) {
        Serial.println(F("EEPROM: ❌ Sector needs erase - filesystem requires formatting"));
        Serial.println(F("EEPROM: Use 'format eeprom' command first"));
        return false;
    }
    
    // Can write directly
    Serial.println(F("EEPROM: Direct write"));
    bool result = _eeprom.writePage(address, (const uint8_t*)&entry, sizeof(entry));
    Serial.print(F("EEPROM: Direct write result: "));
    Serial.println(result ? F("✅") : F("❌"));
    return result;
}

bool EEPROMFileSystem::isValidFilename(const char* filename) {
    if (!filename) return false;
    
    size_t len = strlen(filename);
    // Just check that filename fits in our buffer and isn't empty
    if (len == 0 || len >= FILENAME_LENGTH) return false;
    
    // Accept any filename that fits - no pattern restrictions
    return true;
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
            uint32_t actualSize = (entry.size == 0xFFFFFFFF) ? 0 : ~entry.size;
            uint32_t fileEnd = entry.address + actualSize;
            if (fileEnd > maxAddress) {
                maxAddress = fileEnd;
            }
        }
    }
    
    // Align to sector boundary
    return ((maxAddress + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
}

} // namespace DeviceBridge::Storage