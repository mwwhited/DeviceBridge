#include "ParallelPortManager.h"
#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "SystemManager.h"
#include "../Common/ConfigurationService.h"
#include <string.h>

// PROGMEM component name for memory optimization
static const char component_name[] PROGMEM = "ParallelPortManager";

// Performance-critical configuration constants cached for maximum speed
// These values are accessed in interrupt-critical paths and must be compile-time constants
namespace PerformanceConstants {
    static constexpr uint8_t HEADER_HEX_BYTES = DeviceBridge::Common::Debug::HEADER_HEX_BYTES;
    static constexpr uint32_t CHUNK_SEND_TIMEOUT_MS = DeviceBridge::Common::Buffer::CHUNK_SEND_TIMEOUT_MS;
    static constexpr uint16_t MIN_CHUNK_SIZE = DeviceBridge::Common::Buffer::MIN_CHUNK_SIZE;
    static constexpr uint16_t DATA_CHUNK_SIZE = DeviceBridge::Common::Buffer::DATA_CHUNK_SIZE;
    static constexpr uint16_t KEEP_BUSY_MS = DeviceBridge::Common::Timing::KEEP_BUSY_MS;
}

namespace DeviceBridge::Components {

ParallelPortManager::ParallelPortManager(Parallel::Port &port)
    : _port(port), _fileInProgress(false), _idleCounter(0), _lastDataTime(0), _chunkIndex(0), _chunkStartTime(0),
      _totalBytesReceived(0), _filesReceived(0), _currentFileBytes(0) {
    memset(&_currentChunk, 0, sizeof(_currentChunk));
}

ParallelPortManager::~ParallelPortManager() { stop(); }

bool ParallelPortManager::initialize() {
    // Cache service dependencies first (performance optimization)
    cacheServiceDependencies();
    
    // Initialize LPT read activity LED
    pinMode(Common::Pins::LPT_READ_LED, OUTPUT);
    digitalWrite(Common::Pins::LPT_READ_LED, LOW);

    // Port should already be initialized by main
    return true;
}

void ParallelPortManager::update(unsigned long currentTime) { 
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
    _chunkStartTime = 0;
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
            _chunkStartTime = millis(); // Start timing for first chunk
            
            // Debug logging for new file detection
            if (_cachedSystemManager->isParallelDebugEnabled()) {
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
            // Start timing if this is the first data in a new chunk
            if (_chunkIndex == 0 && _chunkStartTime == 0) {
                _chunkStartTime = millis();
            }
            
            // Turn on LPT read activity LED
            digitalWrite(Common::Pins::LPT_READ_LED, HIGH);

            uint16_t bytesToRead = sizeof(_currentChunk.data) - _chunkIndex;
            uint16_t bytesRead = _port.readData(_currentChunk.data, _chunkIndex, bytesToRead);

            if (bytesRead > 0) {
                _chunkIndex += bytesRead;
                _totalBytesReceived += bytesRead;
                _currentFileBytes += bytesRead;
                
                // Debug logging for data reading
                if (_cachedSystemManager->isParallelDebugEnabled()) {
                    Serial.print(F("[DEBUG-LPT] Read "));
                    Serial.print(bytesRead);
                    Serial.print(F(" bytes, chunk: "));
                    Serial.print(_chunkIndex);
                    Serial.print(F("/"));
                    Serial.print(sizeof(_currentChunk.data));
                    Serial.print(F(", file: "));
                    Serial.print(_currentFileBytes);
                    Serial.print(F(" total"));
                    
                    // Control signal status
                    Serial.print(F(" | Signals: /STR="));
                    Serial.print(_port.isStrobeLow() ? F("ACT") : F("INA"));
                    Serial.print(F(" /AF="));
                    Serial.print(_port.isAutoFeedLow() ? F("ACT") : F("INA"));
                    Serial.print(F(" /INI="));
                    Serial.print(_port.isInitializeLow() ? F("ACT") : F("INA"));
                    Serial.print(F(" /SEL="));
                    Serial.print(_port.isSelectInLow() ? F("ACT") : F("INA"));
                    
                    // Byte tracking comparison (read vs written)
                    Serial.print(F(" | Bytes: Read="));
                    Serial.print(_currentFileBytes);
                    Serial.print(F(" Written="));
                    Serial.print(_cachedFileSystemManager->getCurrentFileBytesWritten());
                    uint32_t difference = (_currentFileBytes > _cachedFileSystemManager->getCurrentFileBytesWritten()) ? 
                        (_currentFileBytes - _cachedFileSystemManager->getCurrentFileBytesWritten()) : 
                        (_cachedFileSystemManager->getCurrentFileBytesWritten() - _currentFileBytes);
                    if (difference > 0) {
                        Serial.print(F(" DIFF="));
                        Serial.print(difference);
                    }
                    
                    // Show hex dump of first N bytes for new files once we have enough data
                    constexpr uint8_t headerBytes = PerformanceConstants::HEADER_HEX_BYTES;
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

                // If chunk is full, send it immediately
                if (_chunkIndex >= sizeof(_currentChunk.data)) {
                    sendChunk();
                } else if (shouldSendPartialChunk()) {
                    // Send partial chunk if timeout reached and minimum size met
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
            // Send final chunk with end-of-file marker FIRST
            _currentChunk.isEndOfFile = 1;
            _currentChunk.isNewFile = 0;
            _currentChunk.length = _chunkIndex;  // Use actual data length, not 0
            _currentChunk.timestamp = millis();

            _cachedFileSystemManager->processDataChunk(_currentChunk);

            // Debug logging for end of file detection AFTER final chunk is written
            if (_cachedSystemManager->isParallelDebugEnabled()) {
                Serial.print(F("[DEBUG-LPT] END OF FILE DETECTED - File #"));
                Serial.print(_filesReceived);
                Serial.print(F(", bytes read: "));
                Serial.print(_currentFileBytes);
                Serial.print(F(", bytes written: "));
                Serial.print(_cachedFileSystemManager->getCurrentFileBytesWritten());
                Serial.print(F(", idle cycles: "));
                Serial.print(_idleCounter);
                
                // Check for data loss AFTER final write
                if (_currentFileBytes != _cachedFileSystemManager->getCurrentFileBytesWritten()) {
                    Serial.print(F(" **DATA MISMATCH**"));
                }
                Serial.print(F("\r\n"));
            }

            _fileInProgress = false;
            _idleCounter = 0;
            _currentFileBytes = 0;
            _chunkIndex = 0;
            _chunkStartTime = 0;
            
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
    if (_cachedSystemManager->isParallelDebugEnabled()) {
        Serial.print(F("[DEBUG-LPT] SENDING CHUNK - Length: "));
        Serial.print(_chunkIndex);
        Serial.print(F(" bytes, new file: "));
        Serial.print(_currentChunk.isNewFile ? F("YES") : F("NO"));
        Serial.print(F(", timestamp: "));
        Serial.print(_currentChunk.timestamp);
        Serial.print(F("\r\n"));
    }

    _cachedFileSystemManager->processDataChunk(_currentChunk);

    // Reset chunk for next data - ONLY reset isNewFile after processing
    _chunkIndex = 0;
    _currentChunk.isNewFile = 0;
    _chunkStartTime = millis(); // Reset timing for next chunk
}

bool ParallelPortManager::shouldSendPartialChunk() const {
    // Don't send if no data collected yet
    if (_chunkIndex == 0) {
        return false;
    }
    
    uint32_t currentTime = millis();
    uint32_t chunkAge = currentTime - _chunkStartTime;
    
    // Send if timeout reached and we have minimum data, or if we have significant data
    return (chunkAge >= PerformanceConstants::CHUNK_SEND_TIMEOUT_MS && _chunkIndex >= PerformanceConstants::MIN_CHUNK_SIZE) ||
           (_chunkIndex >= PerformanceConstants::DATA_CHUNK_SIZE / 2); // Send when half full regardless of time
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
    constexpr uint32_t fileTimeoutMs = PerformanceConstants::KEEP_BUSY_MS; // Use configured timeout
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

bool ParallelPortManager::isStrobeLow() { return _port.isStrobeLow(); }

bool ParallelPortManager::isAutoFeedLow() { return _port.isAutoFeedLow(); }

bool ParallelPortManager::isInitializeLow() { return _port.isInitializeLow(); }

bool ParallelPortManager::isSelectInLow() { return _port.isSelectInLow(); }

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
    _chunkStartTime = 0;
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
    
    Serial.print(F("\r\n*** CRITICAL BUFFER TIMEOUT ***\r\n"));
    Serial.print(F("Buffer failed to clear in 20 seconds\r\n"));
    Serial.print(F("Emergency recovery: Closing file and clearing buffer\r\n"));
    
    _cachedDisplayManager->displayMessage(Common::DisplayMessage::ERROR, F("Buffer Timeout!"));
    _cachedDisplayManager->displayMessage(Common::DisplayMessage::ERROR, F("Emergency Clear"));
    
    // 3. Close current file if open
    if (_fileInProgress) {
        // Send end-of-file marker to properly close the file
        Common::DataChunk endChunk;
        memset(&endChunk, 0, sizeof(endChunk));
        endChunk.isEndOfFile = 1;
        endChunk.timestamp = millis();
        
        _cachedFileSystemManager->processDataChunk(endChunk);
        
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
    
    _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("Recovery Done"));
}

// IComponent interface implementation
bool ParallelPortManager::selfTest() {
    Serial.print(F("ParallelPortManager Self-Test:\r\n"));

    bool result = true;

    // Test parallel port hardware
    Serial.print(F("  Testing parallel port pins... "));
    
    // Test control signals (available through Port interface)
    bool strobeState = _port.isStrobeLow();
    bool autoFeedState = _port.isAutoFeedLow();
    bool initState = _port.isInitializeLow();
    bool selectState = _port.isSelectInLow();
    
    Serial.print(F("✅ OK\r\n"));
    
    // Test ring buffer through Port interface
    Serial.print(F("  Testing ring buffer... "));
    uint16_t bufferCapacity = _port.getBufferCapacity();
    uint16_t bufferFreeSpace = _port.getBufferFreeSpace();
    uint16_t bufferSize = _port.getBufferSize();
    
    if (bufferCapacity > 0 && bufferFreeSpace <= bufferCapacity) {
        Serial.print(F("✅ OK (capacity: "));
        Serial.print(bufferCapacity);
        Serial.print(F(", used: "));
        Serial.print(bufferSize);
        Serial.print(F(", free: "));
        Serial.print(bufferFreeSpace);
        Serial.print(F(")\r\n"));
    } else {
        Serial.print(F("❌ FAIL\r\n"));
        result = false;
    }
    
    // Test interrupt counting
    Serial.print(F("  Testing interrupt system... "));
    uint32_t interruptCount = _port.getInterruptCount();
    uint32_t dataCount = _port.getDataCount();
    
    Serial.print(F("✅ OK (interrupts: "));
    Serial.print(interruptCount);
    Serial.print(F(", data: "));
    Serial.print(dataCount);
    Serial.print(F(")\r\n"));
    
    // Dependencies validated by ServiceLocator at startup

    return result;
}

const char *ParallelPortManager::getComponentName() const { 
    static char name_buffer[24];
    strcpy_P(name_buffer, component_name);
    return name_buffer;
}

bool ParallelPortManager::validateDependencies() const {
    bool valid = true;

    if (!_cachedFileSystemManager) {
        Serial.print(F("  Missing FileSystemManager dependency\r\n"));
        valid = false;
    }

    return valid;
}

void ParallelPortManager::printDependencyStatus() const {
    Serial.print(F("ParallelPortManager Dependencies:\r\n"));

    Serial.print(F("  FileSystemManager: "));
    Serial.print(_cachedFileSystemManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));
}

void ParallelPortManager::setHardwareFlowControlEnabled(bool enabled) {
    _port.setHardwareFlowControlEnabled(enabled);
}

bool ParallelPortManager::isHardwareFlowControlEnabled() const {
    return _port.isHardwareFlowControlEnabled();
}

DeviceBridge::Parallel::HardwareFlowControl::Statistics ParallelPortManager::getFlowControlStatistics() const {
    return _port.getFlowControlStatistics();
}

unsigned long ParallelPortManager::getUpdateInterval() const {
    return _cachedConfigurationService->getParallelInterval(); // Default 1ms for real-time
}

} // namespace DeviceBridge::Components