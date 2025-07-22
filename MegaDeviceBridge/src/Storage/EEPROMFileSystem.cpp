#include "EEPROMFileSystem.h"
#include <string.h>

namespace DeviceBridge::Storage {

EEPROMFileSystem::EEPROMFileSystem() 
    : _eeprom(Common::Pins::EEPROM_CS), _blockDevice(&_eeprom), _initialized(false), 
      _mounted(false), _totalFiles(0), _totalSpace(0), _usedSpace(0) {
    clearError();
    memset(_currentFilename, 0, sizeof(_currentFilename));
    memset(&_lfs, 0, sizeof(_lfs));
    memset(&_lfsConfig, 0, sizeof(_lfsConfig));
    memset(&_currentFile, 0, sizeof(_currentFile));
}

EEPROMFileSystem::~EEPROMFileSystem() {
    shutdown();
}

bool EEPROMFileSystem::initialize() {
    // TEMPORARY: Disable EEPROM LittleFS due to system lockup
    Serial.print(F("EEPROM: LittleFS temporarily disabled to prevent system lockup\r\n"));
    Serial.print(F("EEPROM: W25Q128 chip detected but LittleFS integration needs debugging\r\n"));
    
    setError(FileSystemErrors::NOT_AVAILABLE, "LittleFS temporarily disabled");
    return false;
}

bool EEPROMFileSystem::isAvailable() const {
    return _initialized && _mounted;
}

void EEPROMFileSystem::shutdown() {
    if (_hasActiveFile) {
        closeFile();
    }
    
    if (_mounted) {
        unmountFileSystem();
    }
    
    _initialized = false;
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
    if (!filename || strlen(filename) == 0 || strlen(filename) >= LFS_NAME_MAX) {
        setError(FileSystemErrors::INVALID_FILENAME, "Invalid filename");
        return false;
    }
    
    // Open file for writing (create if doesn't exist)
    int result = lfs_file_open(&_lfs, &_currentFile, filename, 
                               LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    
    if (result < 0) {
        setError(convertLfsError(result), getLfsErrorMessage(result));
        return false;
    }
    
    strncpy(_currentFilename, filename, sizeof(_currentFilename) - 1);
    _currentFilename[sizeof(_currentFilename) - 1] = '\\0';
    _hasActiveFile = true;
    _filesCreated++;
    
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
    
    // Open file with appropriate flags
    int flags = append ? (LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND) : LFS_O_RDONLY;
    int result = lfs_file_open(&_lfs, &_currentFile, filename, flags);
    
    if (result < 0) {
        setError(convertLfsError(result), getLfsErrorMessage(result));
        return false;
    }
    
    strncpy(_currentFilename, filename, sizeof(_currentFilename) - 1);
    _currentFilename[sizeof(_currentFilename) - 1] = '\\0';
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
    
    lfs_ssize_t result = lfs_file_write(&_lfs, &_currentFile, data, length);
    
    if (result < 0) {
        setError(convertLfsError(result), getLfsErrorMessage(result));
        return false;
    }
    
    if ((uint16_t)result != length) {
        setError(FileSystemErrors::FILE_WRITE_FAILED, "Incomplete write");
        return false;
    }
    
    _bytesWritten += length;
    clearError();
    return true;
}

bool EEPROMFileSystem::closeFile() {
    if (!_hasActiveFile) {
        return true; // Already closed
    }
    
    int result = lfs_file_close(&_lfs, &_currentFile);
    
    _hasActiveFile = false;
    memset(_currentFilename, 0, sizeof(_currentFilename));
    
    if (result < 0) {
        setError(convertLfsError(result), getLfsErrorMessage(result));
        return false;
    }
    
    // Sync file system to ensure data is written
    lfs_fs_gc(&_lfs);
    
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
    
    int result = lfs_remove(&_lfs, filename);
    
    if (result < 0) {
        setError(convertLfsError(result), getLfsErrorMessage(result));
        return false;
    }
    
    updateStatistics();
    clearError();
    return true;
}

bool EEPROMFileSystem::fileExists(const char* filename) {
    if (!isAvailable() || !filename) {
        return false;
    }
    
    struct lfs_info info;
    int result = lfs_stat(&_lfs, filename, &info);
    
    return (result >= 0 && info.type == LFS_TYPE_REG);
}

bool EEPROMFileSystem::listFiles(char* buffer, uint16_t bufferSize) {
    if (!isAvailable() || !buffer || bufferSize == 0) {
        setError(FileSystemErrors::INVALID_PARAMETER, "Invalid parameters");
        return false;
    }
    
    lfs_dir_t dir;
    struct lfs_info info;
    int result;
    uint16_t offset = 0;
    uint32_t fileCount = 0;
    
    // Open root directory
    result = lfs_dir_open(&_lfs, &dir, "/");
    if (result < 0) {
        setError(convertLfsError(result), "Failed to open root directory");
        return false;
    }
    
    // Add header
    offset += snprintf(buffer + offset, bufferSize - offset, 
                      "EEPROM LittleFS Files:\\r\\n");
    
    // List all files
    while (true) {
        result = lfs_dir_read(&_lfs, &dir, &info);
        if (result < 0) {
            lfs_dir_close(&_lfs, &dir);
            setError(convertLfsError(result), "Directory read failed");
            return false;
        }
        
        if (result == 0) {
            break; // End of directory
        }
        
        // Skip "." and ".." entries
        if (info.name[0] == '.' && (info.name[1] == '\\0' || 
            (info.name[1] == '.' && info.name[2] == '\\0'))) {
            continue;
        }
        
        if (offset < bufferSize - 50) { // Leave room for entry
            if (info.type == LFS_TYPE_REG) {
                offset += snprintf(buffer + offset, bufferSize - offset,
                                 "  %s (%lu bytes)\\r\\n", info.name, info.size);
                fileCount++;
            } else if (info.type == LFS_TYPE_DIR) {
                offset += snprintf(buffer + offset, bufferSize - offset,
                                 "  %s/ (directory)\\r\\n", info.name);
            }
        }
    }
    
    // Add summary
    if (offset < bufferSize - 30) {
        offset += snprintf(buffer + offset, bufferSize - offset,
                          "Total: %lu files\\r\\n", fileCount);
    }
    
    lfs_dir_close(&_lfs, &dir);
    _totalFiles = fileCount;
    
    clearError();
    return true;
}

uint32_t EEPROMFileSystem::getFileCount() {
    updateStatistics();
    return _totalFiles;
}

uint32_t EEPROMFileSystem::getTotalSpace() {
    if (!isAvailable()) {
        return 0;
    }
    
    return _blockDevice.getBlockCount() * _blockDevice.getBlockSize();
}

uint32_t EEPROMFileSystem::getFreeSpace() {
    if (!isAvailable()) {
        return 0;
    }
    
    lfs_ssize_t usedBlocks = lfs_fs_size(&_lfs);
    if (usedBlocks < 0) {
        return 0;
    }
    
    uint32_t totalBlocks = _blockDevice.getBlockCount();
    
    if ((uint32_t)usedBlocks >= totalBlocks) {
        return 0;
    }
    
    return (totalBlocks - (uint32_t)usedBlocks) * _blockDevice.getBlockSize();
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
    
    // Unmount file system
    if (_mounted) {
        unmountFileSystem();
    }
    
    // Format file system
    if (!formatFileSystem()) {
        setError(FileSystemErrors::HARDWARE_ERROR, "Format failed");
        return false;
    }
    
    // Remount file system
    if (!mountFileSystem()) {
        setError(FileSystemErrors::HARDWARE_ERROR, "Mount after format failed");
        return false;
    }
    
    // Reset statistics
    _bytesWritten = 0;
    _filesCreated = 0;
    _totalFiles = 0;
    updateStatistics();
    
    clearError();
    return true;
}

bool EEPROMFileSystem::flush() {
    if (!isAvailable()) {
        return false;
    }
    
    int result = lfs_fs_gc(&_lfs);
    return (result >= 0);
}

bool EEPROMFileSystem::sync() {
    return flush();
}

// LittleFS-specific methods
bool EEPROMFileSystem::createDirectory(const char* path) {
    if (!isAvailable() || !path) {
        return false;
    }
    
    int result = lfs_mkdir(&_lfs, path);
    return (result >= 0);
}

bool EEPROMFileSystem::removeDirectory(const char* path) {
    if (!isAvailable() || !path) {
        return false;
    }
    
    int result = lfs_remove(&_lfs, path);
    return (result >= 0);
}

bool EEPROMFileSystem::isDirectory(const char* path) {
    if (!isAvailable() || !path) {
        return false;
    }
    
    struct lfs_info info;
    int result = lfs_stat(&_lfs, path, &info);
    
    return (result >= 0 && info.type == LFS_TYPE_DIR);
}

uint32_t EEPROMFileSystem::getFileSize(const char* filename) {
    if (!isAvailable() || !filename) {
        return 0;
    }
    
    struct lfs_info info;
    int result = lfs_stat(&_lfs, filename, &info);
    
    return (result >= 0) ? info.size : 0;
}

bool EEPROMFileSystem::renameFile(const char* oldname, const char* newname) {
    if (!isAvailable() || !oldname || !newname) {
        return false;
    }
    
    int result = lfs_rename(&_lfs, oldname, newname);
    return (result >= 0);
}

bool EEPROMFileSystem::copyFile(const char* src, const char* dst) {
    if (!isAvailable() || !src || !dst) {
        return false;
    }
    
    // Read source file
    lfs_file_t srcFile, dstFile;
    if (lfs_file_open(&_lfs, &srcFile, src, LFS_O_RDONLY) < 0) {
        return false;
    }
    
    // Create destination file
    if (lfs_file_open(&_lfs, &dstFile, dst, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) < 0) {
        lfs_file_close(&_lfs, &srcFile);
        return false;
    }
    
    // Copy data in chunks
    uint8_t buffer[256];
    lfs_ssize_t bytesRead;
    bool success = true;
    
    while ((bytesRead = lfs_file_read(&_lfs, &srcFile, buffer, sizeof(buffer))) > 0) {
        if (lfs_file_write(&_lfs, &dstFile, buffer, bytesRead) != bytesRead) {
            success = false;
            break;
        }
    }
    
    lfs_file_close(&_lfs, &srcFile);
    lfs_file_close(&_lfs, &dstFile);
    
    return success && (bytesRead == 0); // bytesRead == 0 means we reached EOF
}

// Advanced file operations
int EEPROMFileSystem::readFile(const char* filename, uint8_t* buffer, uint32_t bufferSize) {
    if (!isAvailable() || !filename || !buffer || bufferSize == 0) {
        return -1;
    }
    
    lfs_file_t file;
    int result = lfs_file_open(&_lfs, &file, filename, LFS_O_RDONLY);
    if (result < 0) {
        return -1;
    }
    
    lfs_ssize_t bytesRead = lfs_file_read(&_lfs, &file, buffer, bufferSize);
    lfs_file_close(&_lfs, &file);
    
    return (int)bytesRead;
}

bool EEPROMFileSystem::writeFile(const char* filename, const uint8_t* data, uint32_t length) {
    if (!isAvailable() || !filename || !data || length == 0) {
        return false;
    }
    
    lfs_file_t file;
    int result = lfs_file_open(&_lfs, &file, filename, 
                               LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (result < 0) {
        return false;
    }
    
    lfs_ssize_t bytesWritten = lfs_file_write(&_lfs, &file, data, length);
    lfs_file_close(&_lfs, &file);
    
    return (bytesWritten == (lfs_ssize_t)length);
}

bool EEPROMFileSystem::appendToFile(const char* filename, const uint8_t* data, uint32_t length) {
    if (!isAvailable() || !filename || !data || length == 0) {
        return false;
    }
    
    lfs_file_t file;
    int result = lfs_file_open(&_lfs, &file, filename, 
                               LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
    if (result < 0) {
        return false;
    }
    
    lfs_ssize_t bytesWritten = lfs_file_write(&_lfs, &file, data, length);
    lfs_file_close(&_lfs, &file);
    
    return (bytesWritten == (lfs_ssize_t)length);
}

// File system maintenance
bool EEPROMFileSystem::checkFileSystem() {
    if (!isAvailable()) {
        return false;
    }
    
    // LittleFS doesn't have a built-in fsck, but we can verify basic operations
    lfs_dir_t dir;
    int result = lfs_dir_open(&_lfs, &dir, "/");
    if (result < 0) {
        return false;
    }
    
    lfs_dir_close(&_lfs, &dir);
    return true;
}

uint32_t EEPROMFileSystem::getBlockSize() const {
    return _blockDevice.getBlockSize();
}

uint32_t EEPROMFileSystem::getTotalBlocks() const {
    return _blockDevice.getBlockCount();
}

uint32_t EEPROMFileSystem::getFreeBlocks() const {
    if (!isAvailable()) {
        return 0;
    }
    
    lfs_ssize_t usedBlocks = lfs_fs_size(&_lfs);
    if (usedBlocks < 0) {
        return 0;
    }
    
    uint32_t totalBlocks = getTotalBlocks();
    return (totalBlocks > (uint32_t)usedBlocks) ? (totalBlocks - (uint32_t)usedBlocks) : 0;
}

uint32_t EEPROMFileSystem::getBadBlocks() const {
    // W25Q128 is NOR flash with very low bad block rates
    // LittleFS handles bad block management internally
    return _blockDevice.getBadBlockCount();
}

float EEPROMFileSystem::getFragmentation() const {
    if (!isAvailable()) {
        return 0.0f;
    }
    
    uint32_t totalBlocks = getTotalBlocks();
    uint32_t freeBlocks = getFreeBlocks();
    
    if (totalBlocks == 0) {
        return 0.0f;
    }
    
    // Simple fragmentation calculation based on free space distribution
    // More sophisticated analysis would require scanning the filesystem structure
    float usedRatio = (float)(totalBlocks - freeBlocks) / (float)totalBlocks;
    return usedRatio * 100.0f;
}

// Private methods
bool EEPROMFileSystem::initializeLittleFS() {
    Serial.print(F("EEPROM: Configuring LittleFS parameters...\r\n"));
    
    // Configure LittleFS
    memset(&_lfsConfig, 0, sizeof(_lfsConfig));
    
    _lfsConfig.context = &_blockDevice;
    _lfsConfig.read = w25q128_read;
    _lfsConfig.prog = w25q128_prog;
    _lfsConfig.erase = w25q128_erase;
    _lfsConfig.sync = w25q128_sync;
    
    _lfsConfig.read_size = _blockDevice.getReadSize();
    _lfsConfig.prog_size = _blockDevice.getProgSize();
    _lfsConfig.block_size = _blockDevice.getBlockSize();
    _lfsConfig.block_count = _blockDevice.getBlockCount();
    _lfsConfig.cache_size = _blockDevice.getCacheSize();
    _lfsConfig.lookahead_size = _blockDevice.getLookaheadSize();
    _lfsConfig.block_cycles = LFS_BLOCK_CYCLES;
    
    // Debug configuration values
    Serial.print(F("LittleFS Config:\r\n"));
    Serial.print(F("  - read_size: "));
    Serial.print(_lfsConfig.read_size);
    Serial.print(F("\r\n"));
    Serial.print(F("  - prog_size: "));
    Serial.print(_lfsConfig.prog_size);
    Serial.print(F("\r\n"));
    Serial.print(F("  - block_size: "));
    Serial.print(_lfsConfig.block_size);
    Serial.print(F("\r\n"));
    Serial.print(F("  - block_count: "));
    Serial.print(_lfsConfig.block_count);
    Serial.print(F("\r\n"));
    Serial.print(F("  - cache_size: "));
    Serial.print(_lfsConfig.cache_size);
    Serial.print(F("\r\n"));
    Serial.print(F("  - lookahead_size: "));
    Serial.print(_lfsConfig.lookahead_size);
    Serial.print(F("\r\n"));
    Serial.print(F("  - block_cycles: "));
    Serial.print(_lfsConfig.block_cycles);
    Serial.print(F("\r\n"));
    
    // Assign buffers
    _lfsConfig.read_buffer = _blockDevice.getCacheBuffer();
    _lfsConfig.prog_buffer = _blockDevice.getCacheBuffer();
    _lfsConfig.lookahead_buffer = _blockDevice.getLookaheadBuffer();
    
    // Basic validation
    if (!_lfsConfig.read_buffer || !_lfsConfig.lookahead_buffer || 
        _lfsConfig.cache_size == 0 || _lfsConfig.lookahead_size == 0) {
        Serial.print(F("EEPROM: ❌ Buffer validation failed\r\n"));
        return false;
    }
    
    Serial.print(F("EEPROM: ✅ LittleFS configuration completed\r\n"));
    return true;
}

bool EEPROMFileSystem::mountFileSystem() {
    int result = lfs_mount(&_lfs, &_lfsConfig);
    
    if (result == 0) {
        _mounted = true;
        return true;
    }
    
    return false;
}

void EEPROMFileSystem::unmountFileSystem() {
    if (_mounted) {
        lfs_unmount(&_lfs);
        _mounted = false;
    }
}

bool EEPROMFileSystem::formatFileSystem() {
    int result = lfs_format(&_lfs, &_lfsConfig);
    return (result == 0);
}

void EEPROMFileSystem::updateStatistics() {
    if (!isAvailable()) {
        return;
    }
    
    _totalSpace = getTotalSpace();
    _usedSpace = _totalSpace - getFreeSpace();
    
    // Count files in root directory
    lfs_dir_t dir;
    struct lfs_info info;
    uint32_t fileCount = 0;
    
    if (lfs_dir_open(&_lfs, &dir, "/") >= 0) {
        while (lfs_dir_read(&_lfs, &dir, &info) > 0) {
            if (info.type == LFS_TYPE_REG && info.name[0] != '.') {
                fileCount++;
            }
        }
        lfs_dir_close(&_lfs, &dir);
    }
    
    _totalFiles = fileCount;
}

int EEPROMFileSystem::convertLfsError(int lfsError) {
    switch (lfsError) {
        case LFS_ERR_OK: return 0;
        case LFS_ERR_IO: return FileSystemErrors::HARDWARE_ERROR;
        case LFS_ERR_CORRUPT: return FileSystemErrors::CORRUPTION_DETECTED;
        case LFS_ERR_NOENT: return FileSystemErrors::FILE_NOT_FOUND;
        case LFS_ERR_EXIST: return FileSystemErrors::FILE_EXISTS;
        case LFS_ERR_NOTDIR: return FileSystemErrors::INVALID_PATH;
        case LFS_ERR_ISDIR: return FileSystemErrors::INVALID_PATH;
        case LFS_ERR_NOTEMPTY: return FileSystemErrors::DIRECTORY_NOT_EMPTY;
        case LFS_ERR_BADF: return FileSystemErrors::FILE_OPEN_FAILED;
        case LFS_ERR_FBIG: return FileSystemErrors::FILE_TOO_LARGE;
        case LFS_ERR_INVAL: return FileSystemErrors::INVALID_PARAMETER;
        case LFS_ERR_NOSPC: return FileSystemErrors::INSUFFICIENT_SPACE;
        case LFS_ERR_NOMEM: return FileSystemErrors::OUT_OF_MEMORY;
        default: return FileSystemErrors::UNKNOWN_ERROR;
    }
}

const char* EEPROMFileSystem::getLfsErrorMessage(int lfsError) {
    switch (lfsError) {
        case LFS_ERR_OK: return "Success";
        case LFS_ERR_IO: return "I/O error";
        case LFS_ERR_CORRUPT: return "Corruption detected";
        case LFS_ERR_NOENT: return "No such file or directory";
        case LFS_ERR_EXIST: return "File exists";
        case LFS_ERR_NOTDIR: return "Not a directory";
        case LFS_ERR_ISDIR: return "Is a directory";
        case LFS_ERR_NOTEMPTY: return "Directory not empty";
        case LFS_ERR_BADF: return "Bad file number";
        case LFS_ERR_FBIG: return "File too large";
        case LFS_ERR_INVAL: return "Invalid argument";
        case LFS_ERR_NOSPC: return "No space left on device";
        case LFS_ERR_NOMEM: return "Out of memory";
        default: return "Unknown error";
    }
}

} // namespace DeviceBridge::Storage