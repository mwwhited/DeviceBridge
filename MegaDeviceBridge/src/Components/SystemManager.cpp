#include "SystemManager.h"
#include "ParallelPortManager.h"
#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "TimeManager.h"
#include <string.h>
#include <Arduino.h>

namespace DeviceBridge::Components {

SystemManager::SystemManager(QueueHandle_t commandQueue, QueueHandle_t displayQueue, SemaphoreHandle_t serialMutex)
    : _commandQueue(commandQueue)
    , _displayQueue(displayQueue)
    , _serialMutex(serialMutex)
    , _taskHandle(nullptr)
    , _parallelPortManager(nullptr)
    , _fileSystemManager(nullptr)
    , _displayManager(nullptr)
    , _timeManager(nullptr)
    , _systemStatus(Common::SystemStatus::INITIALIZING)
    , _lastError(Common::ErrorCode::NONE)
    , _uptimeSeconds(0)
    , _errorCount(0)
    , _commandsProcessed(0)
    , _monitoredTaskCount(0)
{
}

SystemManager::~SystemManager() {
    stop();
}

bool SystemManager::initialize() {
    _systemStatus = Common::SystemStatus::INITIALIZING;
    return true;
}

bool SystemManager::start() {
    if (_taskHandle != nullptr) {
        return false; // Already running
    }
    
    BaseType_t result = xTaskCreate(
        taskFunction,
        "SystemManager",
        Common::RTOS::SYSTEM_MONITOR_STACK,
        this,
        Common::RTOS::SYSTEM_MONITOR_PRIORITY,
        &_taskHandle
    );
    
    if (result == pdPASS) {
        // Add self to monitoring
        addTaskToMonitor(_taskHandle, "SystemManager");
    }
    
    return result == pdPASS;
}

void SystemManager::stop() {
    if (_taskHandle != nullptr) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
}

void SystemManager::taskFunction(void* pvParameters) {
    SystemManager* manager = static_cast<SystemManager*>(pvParameters);
    manager->runTask();
}

void SystemManager::runTask() {
    Common::SystemCommand cmd;
    TickType_t lastWakeTime = xTaskGetTickCount();
    uint32_t monitorCounter = 0;
    
    // System is now ready
    setSystemStatus(Common::SystemStatus::READY);
    
    for (;;) {
        // Process commands
        while (xQueueReceive(_commandQueue, &cmd, 0) == pdTRUE) {
            processCommand(cmd);
        }
        
        // Periodic system monitoring (every 5 cycles = 25 seconds)
        if (++monitorCounter >= 5) {
            monitorSystemHealth();
            monitorCounter = 0;
            _uptimeSeconds += (Common::RTOS::SYSTEM_MONITOR_MS * 5) / 1000;
        }
        
        // Maintain precise timing
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(Common::RTOS::SYSTEM_MONITOR_MS));
    }
}

void SystemManager::processCommand(const Common::SystemCommand& cmd) {
    _commandsProcessed++;
    
    switch (cmd.type) {
        case Common::SystemCommand::STORAGE_SELECT:
            processStorageSelectCommand(cmd.value);
            break;
            
        case Common::SystemCommand::FILE_TYPE:
            processFileTypeCommand(cmd.value);
            break;
            
        case Common::SystemCommand::TIME_SET:
            processTimeSetCommand(cmd.data);
            break;
            
        case Common::SystemCommand::CONFIG_SAVE:
            processConfigSaveCommand();
            break;
            
        default:
            // Unknown command
            break;
    }
}

void SystemManager::processStorageSelectCommand(uint8_t value) {
    if (_fileSystemManager == nullptr) return;
    
    Common::StorageType storageType;
    const char* message;
    
    switch (value) {
        case 0:
            storageType = Common::StorageType::SD_CARD;
            message = "Storage: SD";
            break;
        case 1:
            storageType = Common::StorageType::EEPROM;
            message = "Storage: EEPROM";
            break;
        case 2:
            storageType = Common::StorageType::SERIAL_TRANSFER;
            message = "Storage: Serial";
            break;
        case 3:
            storageType = Common::StorageType::AUTO_SELECT;
            message = "Storage: Auto";
            break;
        default:
            return;
    }
    
    _fileSystemManager->setPreferredStorage(storageType);
    sendDisplayMessage(Common::DisplayMessage::INFO, message);
}

void SystemManager::processFileTypeCommand(uint8_t value) {
    if (_fileSystemManager == nullptr) return;
    
    Common::FileType fileType;
    const char* message;
    
    switch (value) {
        case 0:
            fileType = Common::FileType::AUTO_DETECT;
            message = "Type: Auto";
            break;
        case 1:
            fileType = Common::FileType::BINARY;
            message = "Type: Binary";
            break;
        case 2:
            fileType = Common::FileType::BITMAP;
            message = "Type: Bitmap";
            break;
        case 3:
            fileType = Common::FileType::PNG;
            message = "Type: PNG";
            break;
        default:
            return;
    }
    
    _fileSystemManager->setFileType(fileType);
    sendDisplayMessage(Common::DisplayMessage::INFO, message);
}

void SystemManager::processTimeSetCommand(const char* data) {
    // TODO: Parse time string and set RTC
    // Format expected: "HH:MM:SS" or "DD/MM/YYYY HH:MM"
    sendDisplayMessage(Common::DisplayMessage::INFO, "Time Set (TODO)");
}

void SystemManager::processConfigSaveCommand() {
    // TODO: Save configuration to EEPROM
    sendDisplayMessage(Common::DisplayMessage::INFO, "Config Saved");
}

void SystemManager::monitorSystemHealth() {
    checkTaskStacks();
    checkQueueLevels();
    logSystemStatus();
}

void SystemManager::checkTaskStacks() {
    for (uint8_t i = 0; i < _monitoredTaskCount; i++) {
        checkTaskHealth(_monitoredTasks[i]);
    }
}

void SystemManager::checkTaskHealth(TaskHealth& task) {
    if (task.handle == nullptr) {
        task.healthy = false;
        return;
    }
    
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(task.handle);
    
    // Check for stack overflow (less than 64 bytes remaining)
    if (highWaterMark < 64) {
        task.healthy = false;
        handleError(Common::ErrorCode::HARDWARE_ERROR);
        
        if (xSemaphoreTake(_serialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            Serial.print("WARNING: Low stack on task ");
            Serial.print(task.name);
            Serial.print(" - ");
            Serial.print(highWaterMark);
            Serial.print(" bytes remaining\r\n");
            xSemaphoreGive(_serialMutex);
        }
    } else {
        task.healthy = true;
    }
    
    task.lastHighWaterMark = highWaterMark;
}

void SystemManager::checkQueueLevels() {
    // Check data queue level (most critical)
    UBaseType_t dataQueueLevel = uxQueueMessagesWaiting(_commandQueue); // TODO: Need reference to data queue
    
    if (dataQueueLevel >= Common::Limits::QUEUE_WARNING_THRESHOLD) {
        handleError(Common::ErrorCode::BUFFER_OVERFLOW);
        
        if (xSemaphoreTake(_serialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            Serial.print("WARNING: Queue nearly full - ");
            Serial.print(dataQueueLevel);
            Serial.print(" messages waiting\r\n");
            xSemaphoreGive(_serialMutex);
        }
    }
}

void SystemManager::logSystemStatus() {
    if (xSemaphoreTake(_serialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        Serial.print("Uptime: ");
        Serial.print(_uptimeSeconds);
        Serial.print("s, Errors: ");
        Serial.print(_errorCount);
        Serial.print(", Commands: ");
        Serial.print(_commandsProcessed); Serial.print("\r\n");
        xSemaphoreGive(_serialMutex);
    }
}

void SystemManager::setSystemStatus(Common::SystemStatus status) {
    _systemStatus = status;
    
    const char* statusMessage;
    switch (status) {
        case Common::SystemStatus::INITIALIZING:
            statusMessage = "Initializing...";
            break;
        case Common::SystemStatus::READY:
            statusMessage = "Ready";
            break;
        case Common::SystemStatus::RECEIVING:
            statusMessage = "Receiving...";
            break;
        case Common::SystemStatus::STORING:
            statusMessage = "Storing...";
            break;
        case Common::SystemStatus::ERROR:
            statusMessage = "System Error";
            break;
        case Common::SystemStatus::FULL:
            statusMessage = "Storage Full";
            break;
        default:
            statusMessage = "Unknown";
            break;
    }
    
    sendDisplayMessage(Common::DisplayMessage::STATUS, statusMessage);
}

void SystemManager::reportError(Common::ErrorCode error) {
    _lastError = error;
    _errorCount++;
    handleError(error);
}

void SystemManager::handleError(Common::ErrorCode error) {
    const char* errorMessage;
    
    switch (error) {
        case Common::ErrorCode::SD_INIT_FAILED:
            errorMessage = "SD Init Failed";
            break;
        case Common::ErrorCode::EEPROM_INIT_FAILED:
            errorMessage = "EEPROM Failed";
            break;
        case Common::ErrorCode::RTC_INIT_FAILED:
            errorMessage = "RTC Failed";
            break;
        case Common::ErrorCode::BUFFER_OVERFLOW:
            errorMessage = "Buffer Overflow";
            break;
        case Common::ErrorCode::FILE_WRITE_ERROR:
            errorMessage = "Write Error";
            break;
        case Common::ErrorCode::STORAGE_FULL:
            errorMessage = "Storage Full";
            setSystemStatus(Common::SystemStatus::FULL);
            break;
        case Common::ErrorCode::HARDWARE_ERROR:
            errorMessage = "Hardware Error";
            setSystemStatus(Common::SystemStatus::ERROR);
            break;
        default:
            errorMessage = "Unknown Error";
            break;
    }
    
    sendDisplayMessage(Common::DisplayMessage::ERROR, errorMessage);
}

void SystemManager::sendDisplayMessage(Common::DisplayMessage::Type type, const char* message) {
    Common::DisplayMessage msg;
    msg.type = type;
    strncpy(msg.message, message, sizeof(msg.message) - 1);
    msg.message[sizeof(msg.message) - 1] = '\0';
    msg.line2[0] = '\0';
    
    xQueueSend(_displayQueue, &msg, 0); // Non-blocking
}

void SystemManager::addTaskToMonitor(TaskHandle_t handle, const char* name) {
    if (_monitoredTaskCount < MAX_MONITORED_TASKS) {
        _monitoredTasks[_monitoredTaskCount].handle = handle;
        _monitoredTasks[_monitoredTaskCount].name = name;
        _monitoredTasks[_monitoredTaskCount].lastHighWaterMark = 0;
        _monitoredTasks[_monitoredTaskCount].healthy = true;
        _monitoredTaskCount++;
    }
}

void SystemManager::printSystemInfo() {
    if (xSemaphoreTake(_serialMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        Serial.print("=== Device Bridge System Info ===\r\n");
        Serial.print("Status: ");
        Serial.print((int)_systemStatus); Serial.print("\r\n");
        Serial.print("Uptime: ");
        Serial.print(_uptimeSeconds);
        Serial.print(" seconds\r\n");
        Serial.print("Total Errors: ");
        Serial.print(_errorCount); Serial.print("\r\n");
        Serial.print("Commands Processed: ");
        Serial.print(_commandsProcessed); Serial.print("\r\n");
        xSemaphoreGive(_serialMutex);
    }
}

void SystemManager::printTaskInfo() {
    if (xSemaphoreTake(_serialMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        Serial.print("=== Task Health Info ===\r\n");
        for (uint8_t i = 0; i < _monitoredTaskCount; i++) {
            Serial.print(i);
            Serial.print(": ");
            Serial.print(_monitoredTasks[i].name);
            Serial.print(" - Stack: ");
            Serial.print(_monitoredTasks[i].lastHighWaterMark);
            Serial.print(" bytes, Health: ");
            Serial.print(_monitoredTasks[i].healthy ? "OK\r\n" : "ERROR\r\n");
        }
        xSemaphoreGive(_serialMutex);
    }
}

void SystemManager::printMemoryInfo() {
    if (xSemaphoreTake(_serialMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        Serial.print("=== Memory Info ===\r\n");
        Serial.print("Free Heap: ");
        Serial.print(xPortGetFreeHeapSize());
        Serial.print(" bytes\r\n");
        Serial.print("Min Free Heap: ");
        Serial.print(xPortGetMinimumEverFreeHeapSize());
        Serial.print(" bytes\r\n");
        xSemaphoreGive(_serialMutex);
    }
}

} // namespace DeviceBridge::Components