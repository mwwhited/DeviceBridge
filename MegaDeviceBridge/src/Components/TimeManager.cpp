#include "TimeManager.h"
#include <string.h>
#include <stdio.h>

namespace DeviceBridge::Components {

TimeManager::TimeManager(QueueHandle_t displayQueue, SemaphoreHandle_t i2cMutex)
    : _displayQueue(displayQueue)
    , _i2cMutex(i2cMutex)
    , _taskHandle(nullptr)
    , _rtcAvailable(false)
    , _lastTimeUpdate(0)
    , _timeValid(false)
{
}

TimeManager::~TimeManager() {
    stop();
}

bool TimeManager::initialize() {
    _rtcAvailable = initializeRTC();
    return _rtcAvailable;
}

bool TimeManager::start() {
    if (_taskHandle != nullptr) {
        return false; // Already running
    }
    
    BaseType_t result = xTaskCreate(
        taskFunction,
        "TimeManager",
        Common::RTOS::TIME_STACK,
        this,
        Common::RTOS::TIME_PRIORITY,
        &_taskHandle
    );
    
    return result == pdPASS;
}

void TimeManager::stop() {
    if (_taskHandle != nullptr) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
}

void TimeManager::taskFunction(void* pvParameters) {
    TimeManager* manager = static_cast<TimeManager*>(pvParameters);
    manager->runTask();
}

void TimeManager::runTask() {
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for (;;) {
        // Update time display periodically
        uint32_t currentTime = xTaskGetTickCount();
        if (currentTime - _lastTimeUpdate >= pdMS_TO_TICKS(Common::RTOS::TIME_UPDATE_MS)) {
            updateTimeDisplay();
            _lastTimeUpdate = currentTime;
        }
        
        // Maintain precise timing
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(Common::RTOS::TIME_UPDATE_MS));
    }
}

bool TimeManager::initializeRTC() {
    if (xSemaphoreTake(_i2cMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        bool result = _rtc.begin();
        if (result) {
            // Check if RTC is running and has valid time
            // Simple validation - check if year is reasonable
            uint16_t year = _rtc.getYear();
            _timeValid = (year >= 2020 && year <= 2099);
        }
        xSemaphoreGive(_i2cMutex);
        return result;
    }
    return false;
}

void TimeManager::updateTimeDisplay() {
    if (!_rtcAvailable) {
        return;
    }
    
    char timeBuffer[32];
    formatTime(timeBuffer, sizeof(timeBuffer));
    
    Common::DisplayMessage msg;
    msg.type = Common::DisplayMessage::TIME;
    strncpy(msg.message, timeBuffer, sizeof(msg.message) - 1);
    msg.message[sizeof(msg.message) - 1] = '\0';
    msg.line2[0] = '\0';
    
    xQueueSend(_displayQueue, &msg, 0); // Non-blocking
}

void TimeManager::formatTime(char* buffer, size_t bufferSize) {
    if (!_rtcAvailable || !_timeValid) {
        strncpy(buffer, "Time: --:--:--", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return;
    }
    
    if (xSemaphoreTake(_i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        uint8_t hour = _rtc.getHours();
        uint8_t minute = _rtc.getMinutes();
        uint8_t second = _rtc.getSeconds();
        
        snprintf(buffer, bufferSize, "Time: %02d:%02d:%02d", hour, minute, second);
        xSemaphoreGive(_i2cMutex);
    } else {
        strncpy(buffer, "Time: I2C Busy", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
}

void TimeManager::formatDateTime(char* buffer, size_t bufferSize) {
    if (!_rtcAvailable || !_timeValid) {
        strncpy(buffer, "--/--/---- --:--", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return;
    }
    
    if (xSemaphoreTake(_i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        uint8_t day = _rtc.getDay();
        uint8_t month = _rtc.getMonth();
        uint16_t year = _rtc.getYear();
        uint8_t hour = _rtc.getHours();
        uint8_t minute = _rtc.getMinutes();
        
        snprintf(buffer, bufferSize, "%02d/%02d/%04d %02d:%02d", 
                day, month, year, hour, minute);
        xSemaphoreGive(_i2cMutex);
    } else {
        strncpy(buffer, "DateTime: I2C Busy", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
}

bool TimeManager::setTime(uint8_t hour, uint8_t minute, uint8_t second) {
    if (!_rtcAvailable) {
        return false;
    }
    
    if (xSemaphoreTake(_i2cMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        bool result = _rtc.setTime(hour, minute, second);
        if (result) {
            _timeValid = true;
        }
        xSemaphoreGive(_i2cMutex);
        return result;
    }
    return false;
}

bool TimeManager::setDate(uint8_t day, uint8_t month, uint16_t year) {
    if (!_rtcAvailable) {
        return false;
    }
    
    if (xSemaphoreTake(_i2cMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        bool result = _rtc.setDate(day, month, year);
        if (result) {
            _timeValid = true;
        }
        xSemaphoreGive(_i2cMutex);
        return result;
    }
    return false;
}

uint32_t TimeManager::getTimestamp() {
    if (!_rtcAvailable || !_timeValid) {
        // Return system tick count if RTC not available
        return xTaskGetTickCount();
    }
    
    // Return Unix timestamp if RTC is available
    // This is a simplified implementation
    return xTaskGetTickCount(); // TODO: Implement proper Unix timestamp
}

void TimeManager::getFormattedTime(char* buffer, size_t bufferSize) {
    formatTime(buffer, bufferSize);
}

void TimeManager::getFormattedDateTime(char* buffer, size_t bufferSize) {
    formatDateTime(buffer, bufferSize);
}

void TimeManager::padNumber(char* buffer, uint8_t number, uint8_t digits) {
    if (digits == 2) {
        sprintf(buffer, "%02d", number);
    } else if (digits == 4) {
        sprintf(buffer, "%04d", number);
    } else {
        sprintf(buffer, "%d", number);
    }
}

} // namespace DeviceBridge::Components