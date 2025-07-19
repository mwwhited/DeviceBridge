#pragma once

#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include "../Parallel/Port.h"
#include "../Common/Types.h"
#include "../Common/Config.h"

namespace DeviceBridge::Components {

class ParallelPortManager {
private:
    Parallel::Port& _port;
    QueueHandle_t _dataQueue;
    TaskHandle_t _taskHandle;
    
    // File detection state
    bool _fileInProgress;
    uint32_t _idleCounter;
    uint32_t _lastDataTime;
    
    // Task function (static for FreeRTOS)
    static void taskFunction(void* pvParameters);
    
    // Instance handling for task function
    void runTask();
    
    // File boundary detection
    bool detectNewFile();
    bool detectEndOfFile();
    
public:
    ParallelPortManager(Parallel::Port& port, QueueHandle_t dataQueue);
    ~ParallelPortManager();
    
    // Lifecycle management
    bool initialize();
    bool start();
    void stop();
    
    // Status inquiry
    bool isReceiving() const { return _fileInProgress; }
    uint16_t getBufferLevel() const;
    TaskHandle_t getTaskHandle() const { return _taskHandle; }
    
    // Statistics
    uint32_t getTotalBytesReceived() const;
    uint32_t getFilesReceived() const;
    
private:
    // Statistics tracking
    uint32_t _totalBytesReceived;
    uint32_t _filesReceived;
    uint32_t _currentFileBytes;
};

} // namespace DeviceBridge::Components