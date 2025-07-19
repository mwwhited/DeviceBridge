#include "ParallelPortManager.h"
#include "FileSystemManager.h"
#include <string.h>

namespace DeviceBridge::Components {

ParallelPortManager::ParallelPortManager(Parallel::Port& port)
    : _port(port)
    , _fileSystemManager(nullptr)
    , _fileInProgress(false)
    , _idleCounter(0)
    , _lastDataTime(0)
    , _chunkIndex(0)
    , _totalBytesReceived(0)
    , _filesReceived(0)
    , _currentFileBytes(0)
{
    memset(&_currentChunk, 0, sizeof(_currentChunk));
}

ParallelPortManager::~ParallelPortManager() {
    stop();
}

bool ParallelPortManager::initialize() {
    // Port should already be initialized by main
    return true;
}

void ParallelPortManager::update() {
    processData();
}

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
            
            if (_fileSystemManager) {
                _fileSystemManager->processDataChunk(_currentChunk);
            }
            
            _fileInProgress = false;
            _idleCounter = 0;
            _currentFileBytes = 0;
            _chunkIndex = 0;
        }
    }
}

void ParallelPortManager::sendChunk() {
    _currentChunk.length = _chunkIndex;
    _currentChunk.timestamp = millis();
    _currentChunk.isEndOfFile = 0;
    
    if (_fileSystemManager) {
        _fileSystemManager->processDataChunk(_currentChunk);
    }
    
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
    const uint32_t FILE_TIMEOUT_MS = 2000;  // 2 second timeout
    uint32_t currentTime = millis();
    uint32_t timeSinceLastData = currentTime - _lastDataTime;
    
    return timeSinceLastData >= FILE_TIMEOUT_MS;
}

uint16_t ParallelPortManager::getBufferLevel() const {
    // Return approximate buffer fill level
    if (_port.isFull()) {
        return 512;
    } else if (_port.isAlmostFull()) {
        return 384; // 75%
    } else if (_port.hasData()) {
        return 128; // Estimate for "some data"
    }
    return 0;
}

uint32_t ParallelPortManager::getTotalBytesReceived() const {
    return _totalBytesReceived;
}

uint32_t ParallelPortManager::getFilesReceived() const {
    return _filesReceived;
}

} // namespace DeviceBridge::Components