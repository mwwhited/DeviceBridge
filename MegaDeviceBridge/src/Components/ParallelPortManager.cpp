#include "ParallelPortManager.h"
#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "SystemManager.h"
#include "../Common/ConfigurationService.h"
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

void ParallelPortManager::update() { 
    processData(); 
    
    // Check for critical buffer timeout
    if (checkCriticalTimeout()) {
        handleCriticalTimeout();
    }
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

        // Check for new file start - only set flag once per chunk
        if (detectNewFile()) {
            _currentChunk.isNewFile = 1;
            _fileInProgress = true;
            _currentFileBytes = 0;
            _filesReceived++;
            _chunkIndex = 0;
            
            // Debug logging for new file detection
            auto systemManager = getServices().getSystemManager();
            if (systemManager && systemManager->isParallelDebugEnabled()) {
                Serial.print(F("[DEBUG-LPT] NEW FILE DETECTED - File #"));
                Serial.print(_filesReceived);
                Serial.print(F(" started at "));
                Serial.print(millis());
                Serial.print(F("ms\r\n"));
            }
        }
        // NOTE: Don't reset isNewFile to 0 here - let it persist until chunk is sent

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
                
                // Debug logging for data reading
                auto systemManager = getServices().getSystemManager();
                if (systemManager && systemManager->isParallelDebugEnabled()) {
                    Serial.print(F("[DEBUG-LPT] Read "));
                    Serial.print(bytesRead);
                    Serial.print(F(" bytes, chunk: "));
                    Serial.print(_chunkIndex);
                    Serial.print(F("/"));
                    Serial.print(sizeof(_currentChunk.data));
                    Serial.print(F(", file: "));
                    Serial.print(_currentFileBytes);
                    Serial.print(F(" total"));
                    
                    // Show hex dump of first N bytes for new files once we have enough data
                    auto config = getServices().getConfigurationService();
                    uint8_t headerBytes = config->getHeaderHexBytes();
                    if (_currentFileBytes >= headerBytes && _currentChunk.isNewFile) {
                        Serial.print(F(" - HEADER HEX: "));
                        for (uint8_t i = 0; i < headerBytes; i++) {
                            if (_currentChunk.data[i] < 0x10) Serial.print(F("0"));
                            Serial.print(_currentChunk.data[i], HEX);
                            if (i < headerBytes - 1) Serial.print(F(" "));
                        }
                    }
                    Serial.print(F("\r\n"));
                }

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
            // Debug logging for end of file detection
            auto systemManager = getServices().getSystemManager();
            if (systemManager && systemManager->isParallelDebugEnabled()) {
                Serial.print(F("[DEBUG-LPT] END OF FILE DETECTED - File #"));
                Serial.print(_filesReceived);
                Serial.print(F(", total bytes: "));
                Serial.print(_currentFileBytes);
                Serial.print(F(", idle cycles: "));
                Serial.print(_idleCounter);
                Serial.print(F("\r\n"));
            }

            // Send final chunk with end-of-file marker
            _currentChunk.isEndOfFile = 1;
            _currentChunk.isNewFile = 0;
            _currentChunk.length = _chunkIndex;  // Use actual data length, not 0
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
    
    // Debug logging for chunk sending
    auto systemManager = getServices().getSystemManager();
    if (systemManager && systemManager->isParallelDebugEnabled()) {
        Serial.print(F("[DEBUG-LPT] SENDING CHUNK - Length: "));
        Serial.print(_chunkIndex);
        Serial.print(F(" bytes, new file: "));
        Serial.print(_currentChunk.isNewFile ? F("YES") : F("NO"));
        Serial.print(F(", timestamp: "));
        Serial.print(_currentChunk.timestamp);
        Serial.print(F("\r\n"));
    }

    auto fileSystemManager = getServices().getFileSystemManager();
    fileSystemManager->processDataChunk(_currentChunk);

    // Reset chunk for next data - ONLY reset isNewFile after processing
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
    uint32_t fileTimeoutMs = getServices().getConfigurationService()->getKeepBusyMs(); // Use configured timeout
    uint32_t currentTime = millis();
    uint32_t timeSinceLastData = currentTime - _lastDataTime;

    return timeSinceLastData >= fileTimeoutMs;
}

uint16_t ParallelPortManager::getBufferLevel() const {
    // Return exact buffer fill level using the new getBufferSize method
    return _port.getBufferSize();
}

bool ParallelPortManager::isBufferAlmostFull() const {
    return _port.isAlmostFull();
}

bool ParallelPortManager::isBufferCriticallyFull() const {
    return _port.isCriticallyFull();
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

bool ParallelPortManager::isCriticalFlowControlActive() const {
    return _port.isCriticalFlowControlActive();
}

bool ParallelPortManager::checkCriticalTimeout() const {
    return _port.checkCriticalTimeout();
}

void ParallelPortManager::resetCriticalState() {
    _port.resetCriticalState();
}

void ParallelPortManager::handleCriticalTimeout() {
    // Critical buffer hasn't cleared in 20 seconds - emergency recovery
    
    // 1. Signal error to TDS2024 via printer status
    _port.setError(true);    // Set ERROR signal active (goes LOW due to active-low nature)
    _port.setPaperOut(true); // Set PAPER_OUT signal to indicate problem
    
    // 2. Send error to LCD and Serial
    auto fileSystemManager = getServices().getFileSystemManager();
    auto displayManager = getServices().getDisplayManager();
    
    Serial.print(F("\r\n*** CRITICAL BUFFER TIMEOUT ***\r\n"));
    Serial.print(F("Buffer failed to clear in 20 seconds\r\n"));
    Serial.print(F("Emergency recovery: Closing file and clearing buffer\r\n"));
    
    displayManager->displayMessage(Common::DisplayMessage::ERROR, F("Buffer Timeout!"));
    displayManager->displayMessage(Common::DisplayMessage::ERROR, F("Emergency Clear"));
    
    // 3. Close current file if open
    if (_fileInProgress) {
        // Send end-of-file marker to properly close the file
        Common::DataChunk endChunk;
        memset(&endChunk, 0, sizeof(endChunk));
        endChunk.isEndOfFile = 1;
        endChunk.timestamp = millis();
        
        fileSystemManager->processDataChunk(endChunk);
        
        _fileInProgress = false;
        _currentFileBytes = 0;
        _chunkIndex = 0;
        
        Serial.print(F("Current file forcibly closed\r\n"));
    }
    
    // 4. Clear buffer and reset critical state
    _port.clearBuffer();
    _port.resetCriticalState();
    
    // 5. Reset error signals after brief delay
    delay(100);
    _port.setError(false);   // Clear ERROR signal
    _port.setPaperOut(false); // Clear PAPER_OUT signal
    
    Serial.print(F("Emergency recovery completed\r\n"));
    Serial.print(F("System ready for new data\r\n"));
    
    displayManager->displayMessage(Common::DisplayMessage::INFO, F("Recovery Done"));
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