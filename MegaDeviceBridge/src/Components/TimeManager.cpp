#include "TimeManager.h"
#include "DisplayManager.h"
#include "../Common/ConfigurationService.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

// PROGMEM component name for memory optimization
static const char component_name[] PROGMEM = "TimeManager";

namespace DeviceBridge::Components {

TimeManager::TimeManager() : _rtcAvailable(false), _lastTimeUpdate(0), _timeValid(false) {}

TimeManager::~TimeManager() { stop(); }

bool TimeManager::initialize() {
    _rtcAvailable = initializeRTC();
    return _rtcAvailable;
}

void TimeManager::update(unsigned long currentTime) {
    // Update time display periodically (called from main loop)
    if (currentTime - _lastTimeUpdate >= Common::RTOS::TIME_UPDATE_MS) {
        updateTimeDisplay();
        _lastTimeUpdate = currentTime;
    }
}

void TimeManager::stop() {
    // Nothing specific to stop for RTC
}

bool TimeManager::initializeRTC() {
    bool result = _rtc.begin();
    if (result) {
        // Check if RTC is running and has valid time
        if (_rtc.isrunning()) {
            DateTime now = _rtc.now();
            _timeValid = (now.year() >= 2020 && now.year() <= 2099);
        } else {
            _timeValid = false;
        }
    }
    return result;
}

void TimeManager::updateTimeDisplay() {
    if (!_rtcAvailable) {
        return;
    }

    char timeBuffer[32];
    formatTime(timeBuffer, sizeof(timeBuffer));
    auto displayManager = getServices().getDisplayManager();
    displayManager->displayMessage(Common::DisplayMessage::TIME, timeBuffer);
}

void TimeManager::formatTime(char *buffer, size_t bufferSize) {
    if (!_rtcAvailable || !_timeValid) {
        strncpy(buffer, "Time: --:--:--", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return;
    }

    DateTime now = _rtc.now();
    uint8_t hour = now.hour();
    uint8_t minute = now.minute();
    uint8_t second = now.second();

    snprintf(buffer, bufferSize, "Time: %02d:%02d:%02d", hour, minute, second);
}

void TimeManager::formatDateTime(char *buffer, size_t bufferSize) {
    if (!_rtcAvailable || !_timeValid) {
        strncpy(buffer, "yyyy-MM-dd hh:mm:ss", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return;
    }

    DateTime now = _rtc.now();
    uint8_t day = now.day();
    uint8_t month = now.month();
    uint16_t year = now.year();
    uint8_t hour = now.hour();
    uint8_t minute = now.minute();
    uint8_t second = now.second();

    snprintf(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
}

bool TimeManager::setTime(uint8_t hour, uint8_t minute, uint8_t second) {
    if (!_rtcAvailable) {
        return false;
    }

    DateTime now = _rtc.now();
    DateTime newTime(now.year(), now.month(), now.day(), hour, minute, second);
    _rtc.adjust(newTime);
    _timeValid = true;
    return true;
}

bool TimeManager::setDate(uint8_t day, uint8_t month, uint16_t year) {
    if (!_rtcAvailable) {
        return false;
    }

    DateTime now = _rtc.now();
    DateTime newDate(year, month, day, now.hour(), now.minute(), now.second());
    _rtc.adjust(newDate);
    _timeValid = true;
    return true;
}

uint32_t TimeManager::getTimestamp() {
    if (!_rtcAvailable || !_timeValid) {
        // Return system millis if RTC not available
        return millis();
    }

    // Return Unix timestamp if RTC is available
    DateTime now = _rtc.now();
    return now.unixtime();
}

void TimeManager::getFormattedTime(char *buffer, size_t bufferSize) { formatTime(buffer, bufferSize); }

void TimeManager::getFormattedDateTime(char *buffer, size_t bufferSize) { formatDateTime(buffer, bufferSize); }

bool TimeManager::setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    if (!_rtcAvailable) {
        return false;
    }

    DateTime newDateTime(year, month, day, hour, minute, second);
    _rtc.adjust(newDateTime);
    _timeValid = true;
    return true;
}

void TimeManager::padNumber(char *buffer, uint8_t number, uint8_t digits) {
    if (digits == 2) {
        sprintf(buffer, "%02d", number);
    } else if (digits == 4) {
        sprintf(buffer, "%04d", number);
    } else {
        sprintf(buffer, "%d", number);
    }
}
// IComponent interface implementation
bool TimeManager::selfTest() {
    Serial.print(F("TimeManager Self-Test:\r\n"));
    
    bool result = true;
    
    if (_rtcAvailable) {
        Serial.print(F("  RTC: ✅ Available\r\n"));
    } else {
        Serial.print(F("  RTC: ❌ Not Available\r\n"));
        result = false;
    }
    
    if (_timeValid) {
        Serial.print(F("  TIME: ✅ Valid\r\n"));
    } else {
        Serial.print(F("  TIME: ❌  Invalid\r\n"));
        result = false;
    }
    
    // Dependencies validated by ServiceLocator at startup
    
    return result;
}

const char* TimeManager::getComponentName() const {
    static char name_buffer[24];
    strcpy_P(name_buffer, component_name);
    return name_buffer;
}

bool TimeManager::validateDependencies() const {
    bool valid = true;
    
    auto displayManager = getServices().getDisplayManager();
    if (!displayManager) {
        Serial.print(F("  Missing DisplayManager dependency\r\n"));
        valid = false;
    }
    
    return valid;
}

void TimeManager::printDependencyStatus() const {
    Serial.print(F("TimeManager Dependencies:\r\n"));
    
    auto displayManager = getServices().getDisplayManager();
    Serial.print(F("  DisplayManager: "));
    Serial.print(displayManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));
}

unsigned long TimeManager::getUpdateInterval() const {
    auto configService = getServices().getConfigurationService();
    return configService ? configService->getTimeInterval() : 1000; // Default 1 second
}

} // namespace DeviceBridge::Components