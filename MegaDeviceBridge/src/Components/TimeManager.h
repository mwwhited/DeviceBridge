#pragma once

#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <RTClib.h>
#include "../Common/Types.h"
#include "../Common/Config.h"

namespace DeviceBridge::Components {

class TimeManager {
private:
    QueueHandle_t _displayQueue;
    SemaphoreHandle_t _i2cMutex;
    TaskHandle_t _taskHandle;
    
    // RTC instance
    RTC_DS1307 _rtc;
    bool _rtcAvailable;
    
    // Time tracking
    uint32_t _lastTimeUpdate;
    bool _timeValid;
    
    // Task function (static for FreeRTOS)
    static void taskFunction(void* pvParameters);
    
    // Instance handling for task function
    void runTask();
    
    // Time management
    bool initializeRTC();
    void updateTimeDisplay();
    void formatTime(char* buffer, size_t bufferSize);
    void formatDateTime(char* buffer, size_t bufferSize);
    
public:
    TimeManager(QueueHandle_t displayQueue, SemaphoreHandle_t i2cMutex);
    ~TimeManager();
    
    // Lifecycle management
    bool initialize();
    bool start();
    void stop();
    
    // Time operations
    bool setTime(uint8_t hour, uint8_t minute, uint8_t second);
    bool setDate(uint8_t day, uint8_t month, uint16_t year);
    uint32_t getTimestamp();
    void getFormattedTime(char* buffer, size_t bufferSize);
    void getFormattedDateTime(char* buffer, size_t bufferSize);
    
    // Status inquiry
    bool isRTCAvailable() const { return _rtcAvailable; }
    bool isTimeValid() const { return _timeValid; }
    TaskHandle_t getTaskHandle() const { return _taskHandle; }
    
private:
    // Time formatting helpers
    static void padNumber(char* buffer, uint8_t number, uint8_t digits = 2);
};

} // namespace DeviceBridge::Components