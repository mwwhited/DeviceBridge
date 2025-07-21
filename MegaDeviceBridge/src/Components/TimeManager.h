#pragma once

#include <Arduino.h>
#include <RTClib.h>
#include "../Common/Types.h"
#include "../Common/Config.h"
#include "../Common/ServiceLocator.h"

namespace DeviceBridge::Components {

// Forward declaration
class DisplayManager;

class TimeManager : public DeviceBridge::IComponent {
private:
    // Note: No longer storing direct references - using ServiceLocator
    
    // RTC instance
    RTC_DS1307 _rtc;
    bool _rtcAvailable;
    
    // Time tracking
    uint32_t _lastTimeUpdate;
    bool _timeValid;
    
    // Time management
    
    // Time management
    bool initializeRTC();
    void updateTimeDisplay();
    void formatTime(char* buffer, size_t bufferSize);
    void formatDateTime(char* buffer, size_t bufferSize);
    
public:
    TimeManager();
    ~TimeManager();
        
    // Lifecycle management (IComponent interface)
    bool initialize() override final;
    void update(unsigned long currentTime) override final;  // Called from main loop
    void stop() override final;
    
    // IComponent interface implementation
    bool selfTest() override final;
    const char* getComponentName() const override final;
    bool validateDependencies() const override final;
    void printDependencyStatus() const override final;
    unsigned long getUpdateInterval() const override final;
    
    // Time operations
    bool setTime(uint8_t hour, uint8_t minute, uint8_t second);
    bool setDate(uint8_t day, uint8_t month, uint16_t year);
    bool setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
    uint32_t getTimestamp();
    void getFormattedTime(char* buffer, size_t bufferSize);
    void getFormattedDateTime(char* buffer, size_t bufferSize);
    RTC_DS1307 getRTC() const { return _rtc; }
    
    // Status inquiry
    bool isRTCAvailable() const { return _rtcAvailable; }
    bool isTimeValid() const { return _timeValid; }
    
private:
    // Time formatting helpers
    static void padNumber(char* buffer, uint8_t number, uint8_t digits = 2);
};

} // namespace DeviceBridge::Components