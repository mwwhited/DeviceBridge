#include "ParallelPortManager.h"
#include <string.h>

namespace DeviceBridge::Components {

ParallelPortManager::ParallelPortManager(Parallel::Port& port, QueueHandle_t dataQueue)
    : _port(port)
    , _dataQueue(dataQueue)
    , _taskHandle(nullptr)
    , _fileInProgress(false)
    , _idleCounter(0)
    , _lastDataTime(0)
    , _totalBytesReceived(0)
    , _filesReceived(0)
    , _currentFileBytes(0)
{
}

ParallelPortManager::~ParallelPortManager() {
    stop();
}

bool ParallelPortManager::initialize() {
    // Port should already be initialized by main
    return true;
}

bool ParallelPortManager::start() {
    if (_taskHandle != nullptr) {
        return false; // Already running
    }
    
    BaseType_t result = xTaskCreate(
        taskFunction,
        "ParallelPort",
        Common::RTOS::PARALLEL_PORT_STACK,
        this,
        Common::RTOS::PARALLEL_PORT_PRIORITY,
        &_taskHandle
    );
    
    return result == pdPASS;
}

void ParallelPortManager::stop() {
    if (_taskHandle != nullptr) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
}

void ParallelPortManager::taskFunction(void* pvParameters) {
    ParallelPortManager* manager = static_cast<ParallelPortManager*>(pvParameters);
    manager->runTask();
}

void ParallelPortManager::runTask() {
    Common::DataChunk chunk;
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for (;;) {
        bool hasData = _port.hasData();
        
        if (hasData) {
            _idleCounter = 0;
            _lastDataTime = xTaskGetTickCount();
            
            // Check for new file start
            if (detectNewFile()) {
                chunk.isNewFile = true;
                _fileInProgress = true;
                _currentFileBytes = 0;
                _filesReceived++;
            } else {
                chunk.isNewFile = false;
            }
            
            // Read available data
            chunk.length = _port.readData(chunk.data, 0, sizeof(chunk.data));
            chunk.timestamp = xTaskGetTickCount();
            chunk.isEndOfFile = false;
            
            if (chunk.length > 0) {
                _totalBytesReceived += chunk.length;
                _currentFileBytes += chunk.length;
                
                // Send data to file manager
                if (xQueueSend(_dataQueue, &chunk, 0) != pdTRUE) {
                    // Queue full - data loss! This is critical
                    // TODO: Implement overflow handling
                }
            }
        } else {
            _idleCounter++;
            
            // Check for end of file
            if (detectEndOfFile()) {
                chunk.isEndOfFile = true;
                chunk.isNewFile = false;
                chunk.length = 0;
                chunk.timestamp = xTaskGetTickCount();
                
                xQueueSend(_dataQueue, &chunk, portMAX_DELAY);
                
                _fileInProgress = false;
                _idleCounter = 0;
                _currentFileBytes = 0;
            }
        }
        
        // Maintain precise timing
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(Common::RTOS::PARALLEL_PORT_POLL_MS));
    }
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
    
    uint32_t idleTime = _idleCounter * Common::RTOS::PARALLEL_PORT_POLL_MS;
    return idleTime >= Common::RTOS::FILE_TIMEOUT_MS;
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