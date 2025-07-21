#include "SerialTransferFileSystem.h"
#include <string.h>

namespace DeviceBridge::Storage {

SerialTransferFileSystem::SerialTransferFileSystem() 
    : _initialized(false), _transferInProgress(false), _currentFileSize(0), _transferredBytes(0) {
    memset(_currentFilename, 0, sizeof(_currentFilename));
    clearError();
}

SerialTransferFileSystem::~SerialTransferFileSystem() {
    shutdown();
}

bool SerialTransferFileSystem::initialize() {
    // Serial transfer is always available if Serial is initialized
    _initialized = Serial;
    
    if (!_initialized) {
        setError(FileSystemErrors::INIT_FAILED, "Serial not initialized");
        return false;
    }
    
    clearError();
    return true;
}

bool SerialTransferFileSystem::isAvailable() const {
    return _initialized && Serial;
}

void SerialTransferFileSystem::shutdown() {
    if (_transferInProgress) {
        closeFile();
    }
    _initialized = false;
}

bool SerialTransferFileSystem::createFile(const char* filename) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "Serial not available");
        return false;
    }
    
    if (_hasActiveFile) {
        closeFile();
    }
    
    // Check filename length
    size_t filenameLen = strlen(filename);
    if (filenameLen == 0 || filenameLen >= sizeof(_currentFilename)) {
        setError(FileSystemErrors::INVALID_FILENAME, "Invalid filename length");
        return false;
    }
    
    // Store filename for transfer
    strncpy(_currentFilename, filename, sizeof(_currentFilename) - 1);
    _currentFilename[sizeof(_currentFilename) - 1] = '\0';
    
    // Reset transfer state
    _currentFileSize = 0;
    _transferredBytes = 0;
    _transferInProgress = false;
    
    _hasActiveFile = true;
    _filesCreated++;
    clearError();
    return true;
}

bool SerialTransferFileSystem::openFile(const char* filename, bool append) {
    // Serial transfer doesn't support opening existing files
    setError(FileSystemErrors::FILE_OPEN_FAILED, "Serial transfer only supports new files");
    return false;
}

bool SerialTransferFileSystem::writeData(const uint8_t* data, uint16_t length) {
    if (!_hasActiveFile) {
        setError(FileSystemErrors::FILE_WRITE_FAILED, "No active file");
        return false;
    }
    
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "Serial not available");
        return false;
    }
    
    // Start transfer if this is the first write
    if (!_transferInProgress) {
        if (!sendTransferHeader(_currentFilename, 0)) { // Size unknown at start
            setError(FileSystemErrors::FILE_WRITE_FAILED, "Failed to send transfer header");
            return false;
        }
        _transferInProgress = true;
    }
    
    // Send data chunk
    if (!sendDataChunk(data, length)) {
        setError(FileSystemErrors::FILE_WRITE_FAILED, "Failed to send data chunk");
        return false;
    }
    
    _transferredBytes += length;
    _bytesWritten += length;
    
    // Send progress update every 1KB
    if (_transferredBytes % 1024 == 0) {
        sendProgressUpdate();
    }
    
    clearError();
    return true;
}

bool SerialTransferFileSystem::closeFile() {
    if (!_hasActiveFile) {
        return true; // Already closed
    }
    
    // Send transfer end packet
    if (_transferInProgress) {
        if (!sendTransferEnd()) {
            setError(FileSystemErrors::FILE_CLOSE_FAILED, "Failed to send transfer end");
            return false;
        }
        _transferInProgress = false;
    }
    
    _hasActiveFile = false;
    clearError();
    return true;
}

bool SerialTransferFileSystem::deleteFile(const char* filename) {
    // Serial transfer doesn't support file deletion
    setError(FileSystemErrors::FILE_DELETE_FAILED, "Serial transfer doesn't support deletion");
    return false;
}

bool SerialTransferFileSystem::fileExists(const char* filename) {
    // Serial transfer doesn't maintain file records
    return false;
}

bool SerialTransferFileSystem::listFiles(char* buffer, uint16_t bufferSize) {
    if (!isAvailable()) {
        setError(FileSystemErrors::NOT_AVAILABLE, "Serial not available");
        return false;
    }
    
    // Show transfer statistics
    snprintf(buffer, bufferSize, "Serial Transfer: %lu files, %lu bytes\n", 
             getFilesCreated(), getBytesWritten());
    clearError();
    return true;
}

uint32_t SerialTransferFileSystem::getFileCount() {
    return _filesCreated;
}

uint32_t SerialTransferFileSystem::getTotalSpace() {
    // Serial transfer has unlimited space (limited by receiver)
    return 0xFFFFFFFF;
}

uint32_t SerialTransferFileSystem::getFreeSpace() {
    // Serial transfer has unlimited space
    return 0xFFFFFFFF;
}

uint32_t SerialTransferFileSystem::getTransferProgress() const {
    if (_currentFileSize == 0 || !_transferInProgress) {
        return 0;
    }
    
    return (_transferredBytes * 100) / _currentFileSize;
}

void SerialTransferFileSystem::setProgressCallback(void (*callback)(uint32_t percent)) {
    _progressCallback = callback;
}

bool SerialTransferFileSystem::enableBinaryMode(bool enable) {
    _binaryMode = enable;
    return true;
}

bool SerialTransferFileSystem::setTransferSpeed(uint32_t baudRate) {
    _transferBaudRate = baudRate;
    Serial.begin(baudRate);
    return true;
}

// Private methods
bool SerialTransferFileSystem::sendTransferHeader(const char* filename, uint32_t fileSize) {
    if (!Serial) {
        return false;
    }
    
    // Send file start notification
    Serial.print(F(">>> FILE_START "));
    Serial.print(filename);
    Serial.print(F(" SIZE:"));
    Serial.print(fileSize);
    Serial.print(F(" <<<\r\n"));
    
    // In binary mode, send structured header
    if (_binaryMode) {
        TransferHeader header;
        header.startMarker = START_MARKER;
        header.version = PROTOCOL_VERSION;
        header.packetType = PACKET_FILE_HEADER;
        header.reserved = 0;
        header.fileSize = fileSize;
        header.sequenceNumber = 0;
        header.dataLength = strlen(filename);
        header.checksum = calculatePacketChecksum(header, (const uint8_t*)filename);
        
        Serial.write((uint8_t*)&header, sizeof(header));
        Serial.write((const uint8_t*)filename, strlen(filename));
        Serial.write(END_MARKER);
    }
    
    return true;
}

bool SerialTransferFileSystem::sendDataChunk(const uint8_t* data, uint16_t length) {
    if (!Serial) {
        return false;
    }
    
    if (_binaryMode) {
        // Send binary data with protocol header
        TransferHeader header;
        header.startMarker = START_MARKER;
        header.version = PROTOCOL_VERSION;
        header.packetType = PACKET_DATA;
        header.reserved = 0;
        header.fileSize = _currentFileSize;
        header.sequenceNumber = _transferredBytes / MAX_CHUNK_SIZE;
        header.dataLength = length;
        header.checksum = calculatePacketChecksum(header, data);
        
        Serial.write((uint8_t*)&header, sizeof(header));
        Serial.write(data, length);
        Serial.write(END_MARKER);
    } else {
        // Send as hex-encoded text
        Serial.print(F(">>> DATA "));
        for (uint16_t i = 0; i < length; i++) {
            if (data[i] < 0x10) Serial.print('0');
            Serial.print(data[i], HEX);
        }
        Serial.print(F(" <<<\r\n"));
    }
    
    return true;
}

bool SerialTransferFileSystem::sendTransferEnd() {
    if (!Serial) {
        return false;
    }
    
    // Send file end notification
    Serial.print(F(">>> FILE_END "));
    Serial.print(_currentFilename);
    Serial.print(F(" BYTES:"));
    Serial.print(_transferredBytes);
    Serial.print(F(" <<<\r\n"));
    
    // In binary mode, send structured end packet
    if (_binaryMode) {
        TransferHeader header;
        header.startMarker = START_MARKER;
        header.version = PROTOCOL_VERSION;
        header.packetType = PACKET_FILE_END;
        header.reserved = 0;
        header.fileSize = _transferredBytes;
        header.sequenceNumber = 0xFFFFFFFF; // End marker
        header.dataLength = 0;
        header.checksum = calculatePacketChecksum(header, nullptr);
        
        Serial.write((uint8_t*)&header, sizeof(header));
        Serial.write(END_MARKER);
    }
    
    return true;
}

uint16_t SerialTransferFileSystem::calculatePacketChecksum(const TransferHeader& header, const uint8_t* data) {
    uint16_t checksum = 0;
    
    // Checksum header (excluding checksum field)
    const uint8_t* headerBytes = (const uint8_t*)&header;
    for (uint8_t i = 0; i < sizeof(TransferHeader) - 2; i++) {
        checksum ^= headerBytes[i];
    }
    
    // Checksum data
    if (data && header.dataLength > 0) {
        for (uint16_t i = 0; i < header.dataLength; i++) {
            checksum ^= data[i];
        }
    }
    
    return checksum;
}

void SerialTransferFileSystem::sendProgressUpdate() {
    if (!Serial) {
        return;
    }
    
    uint32_t percent = getTransferProgress();
    Serial.print(F(">>> PROGRESS "));
    Serial.print(percent);
    Serial.print(F("% BYTES:"));
    Serial.print(_transferredBytes);
    Serial.print(F(" <<<\r\n"));
    
    // Call progress callback if set
    if (_progressCallback) {
        _progressCallback(percent);
    }
}

bool SerialTransferFileSystem::waitForAcknowledgment(uint32_t timeoutMs) {
    // Simple implementation - just add delay for transmission
    delay(timeoutMs / 100); // Short delay to allow transmission
    return true;
}

} // namespace DeviceBridge::Storage