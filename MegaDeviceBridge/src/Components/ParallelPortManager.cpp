#include "ParallelPortManager.h"
#include "FileSystemManager.h"
#include <string.h>

namespace DeviceBridge::Components {

ParallelPortManager::ParallelPortManager(Parallel::Port &port)
    : _port(port), _fileInProgress(false), _idleCounter(0), _lastDataTime(0), _chunkIndex(0), _totalBytesReceived(0),
      _filesReceived(0), _currentFileBytes(0) {
    memset(&_currentChunk, 0, sizeof(_currentChunk));
}

ParallelPortManager::~ParallelPortManager() { stop(); }

bool ParallelPortManager::initialize() {
    // Initialize LPT read activity LED
    pinMode(Common::Pins::LPT_READ_LED, OUTPUT);
    digitalWrite(Common::Pins::LPT_READ_LED, LOW);

    // Port should already be initialized by main
    return true;
}

void ParallelPortManager::update() { processData(); }

void ParallelPortManager::stop() {
    _fileInProgress = false;
    _idleCounter = 0;
    _chunkIndex = 0;
}

void ParallelPortManager::processData() {
    bool hasData = _port.hasData();

    if (hasData) {
        _idleCounter = 0;
        _lastDataTime = millis();

        // Check for new file start
        if (detectNewFile()) {
            _currentChunk.isNewFile = 1;
            _fileInProgress = true;
            _currentFileBytes = 0;
            _filesReceived++;
            _chunkIndex = 0;
        } else {
            _currentChunk.isNewFile = 0;
        }

        // Read available data into current chunk
        if (_chunkIndex < sizeof(_currentChunk.data)) {
            // Turn on LPT read activity LED
            digitalWrite(Common::Pins::LPT_READ_LED, HIGH);

            uint16_t bytesToRead = sizeof(_currentChunk.data) - _chunkIndex;
            uint16_t bytesRead = _port.readData(_currentChunk.data, _chunkIndex, bytesToRead);

            if (bytesRead > 0) {
                _chunkIndex += bytesRead;
                _totalBytesReceived += bytesRead;
                _currentFileBytes += bytesRead;

                // If chunk is full, send it
                if (_chunkIndex >= sizeof(_currentChunk.data)) {
                    sendChunk();
                }
            }

            // Turn off LPT read activity LED
            digitalWrite(Common::Pins::LPT_READ_LED, LOW);
        }
    } else {
        _idleCounter++;

        // Check for end of file
        if (detectEndOfFile()) {
            // Send partial chunk if any data remains
            if (_chunkIndex > 0) {
                sendChunk();
            }

            // Send end-of-file marker
            _currentChunk.isEndOfFile = 1;
            _currentChunk.isNewFile = 0;
            _currentChunk.length = 0;
            _currentChunk.timestamp = millis();

            auto fileSystemManager = getServices().getFileSystemManager();
            fileSystemManager->processDataChunk(_currentChunk);

            _fileInProgress = false;
            _idleCounter = 0;
            _currentFileBytes = 0;
            _chunkIndex = 0;
            
            // Clear any remaining data in buffer to prevent corruption of next file
            _port.clearBuffer();
        }
    }
}

void ParallelPortManager::sendChunk() {
    _currentChunk.length = _chunkIndex;
    _currentChunk.timestamp = millis();
    _currentChunk.isEndOfFile = 0;

    auto fileSystemManager = getServices().getFileSystemManager();
    fileSystemManager->processDataChunk(_currentChunk);

    // Reset chunk for next data
    _chunkIndex = 0;
    _currentChunk.isNewFile = 0;
}

bool ParallelPortManager::detectNewFile() {
    // New file detected if we weren't in progress and now have data
    return !_fileInProgress && _port.hasData();
}

bool ParallelPortManager::detectEndOfFile() {
    // End of file after timeout period with no data
    if (!_fileInProgress) {
        return false;
    }

    // Use millis() based timing for loop-based architecture
    const uint32_t FILE_TIMEOUT_MS = 2000; // 2 second timeout
    uint32_t currentTime = millis();
    uint32_t timeSinceLastData = currentTime - _lastDataTime;

    return timeSinceLastData >= FILE_TIMEOUT_MS;
}

uint16_t ParallelPortManager::getBufferLevel() const {
    // Return exact buffer fill level using the new getBufferSize method
    return _port.getBufferSize();
}

uint32_t ParallelPortManager::getTotalBytesReceived() const { return _totalBytesReceived; }

uint32_t ParallelPortManager::getFilesReceived() const { return _filesReceived; }

uint32_t ParallelPortManager::getInterruptCount() const { return _port.getInterruptCount(); }

uint32_t ParallelPortManager::getDataCount() const { return _port.getDataCount(); }

void ParallelPortManager::lockPort() { _port.lock(); }

void ParallelPortManager::unlockPort() { _port.unlock(); }

bool ParallelPortManager::isPortLocked() const { return _port.isLocked(); }

void ParallelPortManager::setPrinterBusy(bool busy) { _port.setBusy(busy); }

void ParallelPortManager::setPrinterError(bool error) { _port.setError(error); }

void ParallelPortManager::setPrinterPaperOut(bool paperOut) { _port.setPaperOut(paperOut); }

void ParallelPortManager::setPrinterSelect(bool select) { _port.setSelect(select); }

void ParallelPortManager::sendPrinterAcknowledge() { _port.sendAcknowledge(); }

void ParallelPortManager::clearBuffer() { 
    _port.clearBuffer(); 
    // Also reset internal state
    _chunkIndex = 0;
    _fileInProgress = false;
    _idleCounter = 0;
}

uint16_t ParallelPortManager::getBufferSize() const { 
    return _port.getBufferSize(); 
}

// IComponent interface implementation
bool ParallelPortManager::selfTest() {
    Serial.print(F("ParallelPortManager Self-Test:\r\n"));

    bool result = true;

    // Test dependencies
    if (!validateDependencies()) {
        result = false;
    }

    return result;
}

const char *ParallelPortManager::getComponentName() const { return "ParallelPortManager"; }

bool ParallelPortManager::validateDependencies() const {
    bool valid = true;

    auto fileSystemManager = getServices().getFileSystemManager();
    if (!fileSystemManager) {
        Serial.print(F("  Missing FileSystemManager dependency\r\n"));
        valid = false;
    }

    return valid;
}

void ParallelPortManager::printDependencyStatus() const {
    Serial.print(F("ParallelPortManager Dependencies:\r\n"));

    auto fileSystemManager = getServices().getFileSystemManager();
    Serial.print(F("  FileSystemManager: "));
    Serial.print(fileSystemManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));
}

} // namespace DeviceBridge::Components