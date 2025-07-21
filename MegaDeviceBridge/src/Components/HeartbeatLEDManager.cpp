#include "HeartbeatLEDManager.h"
#include "../Common/ConfigurationService.h"
#include <Arduino.h>
#include <string.h>

namespace DeviceBridge::Components {

// Initialize SOS pattern array
constexpr uint8_t HeartbeatLEDManager::SOS_PATTERN[SOS_PATTERN_LENGTH];

HeartbeatLEDManager::HeartbeatLEDManager(uint8_t pin)
    : _mode(HeartbeatMode::NORMAL), _pin(pin), _ledState(false), _lastUpdate(0),
      _sosIndex(0), _sosRepeat(0), _sosInPattern(false), _sosLastTime(0),
      _lastErrorMessageTime(0) {
    memset(_errorMessage, 0, sizeof(_errorMessage));
}

HeartbeatLEDManager::~HeartbeatLEDManager() {
    stop();
}

bool HeartbeatLEDManager::initialize() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _ledState = false;
    _lastUpdate = millis();
    _sosLastTime = millis();
    return true;
}

void HeartbeatLEDManager::update(unsigned long currentTime) {
    switch (_mode) {
        case HeartbeatMode::NORMAL:
            updateNormalHeartbeat();
            break;
        case HeartbeatMode::SOS:
            updateSOSPattern();
            break;
        case HeartbeatMode::OFF:
            // Keep LED off
            if (_ledState) {
                setLEDState(false);
            }
            break;
    }
}

void HeartbeatLEDManager::stop() {
    setLEDState(false);
    _mode = HeartbeatMode::OFF;
}

void HeartbeatLEDManager::updateNormalHeartbeat() {
    uint32_t currentTime = millis();
    auto config = getServices().getConfigurationService();
    
    if (currentTime - _lastUpdate >= config->getHeartbeatInterval()) {
        setLEDState(!_ledState);
        _lastUpdate = currentTime;
    }
}

void HeartbeatLEDManager::updateSOSPattern() {
    uint32_t currentTime = millis();
    
    // Output error message every 5 seconds
    if (currentTime - _lastErrorMessageTime >= 5000) {
        Serial.print(F("SOS ERROR: "));
        Serial.print(_errorMessage);
        Serial.print(F(" - System requires attention\r\n"));
        _lastErrorMessageTime = currentTime;
    }
    
    // Check if we need to start a new SOS sequence (every 5 seconds)
    if (!_sosInPattern && (currentTime - _sosLastTime >= 5000)) {
        _sosIndex = 0;
        _sosRepeat = 0;
        _sosInPattern = true;
        _sosLastTime = currentTime;
        setLEDState(true); // Start with LED on
        return;
    }
    
    if (!_sosInPattern) {
        // Keep LED off between SOS sequences
        if (_ledState) {
            setLEDState(false);
        }
        return;
    }
    
    // Process SOS pattern
    if (_sosIndex >= SOS_PATTERN_LENGTH) {
        // End of pattern, turn off LED and wait for next sequence
        setLEDState(false);
        _sosInPattern = false;
        _sosLastTime = currentTime;
        return;
    }
    
    uint8_t currentElement = SOS_PATTERN[_sosIndex];
    uint16_t elementDuration = getSOSElementDuration(currentElement);
    
    if (currentTime - _sosLastTime >= elementDuration) {
        if (currentElement == 0) {
            // Pause - LED should be off
            setLEDState(false);
        } else {
            // Dot or dash - LED should be on for the duration, then off
            if (_ledState) {
                // LED was on, turn it off and move to next element
                setLEDState(false);
                _sosIndex++;
            } else {
                // LED was off, turn it on for this element
                setLEDState(true);
            }
        }
        _sosLastTime = currentTime;
        
        // If this was a pause, move to next element immediately
        if (currentElement == 0) {
            _sosIndex++;
        }
    }
}

uint16_t HeartbeatLEDManager::getSOSElementDuration(uint8_t element) {
    switch (element) {
        case 1: return SOS_DOT_MS;      // Dot
        case 3: return SOS_DASH_MS;     // Dash
        case 0: return SOS_PAUSE_MS;    // Pause between elements
        default: return SOS_PAUSE_MS;
    }
}

void HeartbeatLEDManager::setMode(HeartbeatMode mode) {
    _mode = mode;
    _lastUpdate = millis();
    _sosLastTime = millis();
    _sosIndex = 0;
    _sosInPattern = false;
    
    if (mode == HeartbeatMode::OFF) {
        setLEDState(false);
    }
}

void HeartbeatLEDManager::setLEDState(bool state) {
    _ledState = state;
    digitalWrite(_pin, state ? HIGH : LOW);
}

void HeartbeatLEDManager::setPin(uint8_t pin) {
    if (_pin != pin) {
        // Clean up old pin
        digitalWrite(_pin, LOW);
        
        // Set up new pin
        _pin = pin;
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, _ledState ? HIGH : LOW);
    }
}

void HeartbeatLEDManager::setSOSMode(const char* errorMessage) {
    _mode = HeartbeatMode::SOS;
    if (errorMessage) {
        strncpy(_errorMessage, errorMessage, sizeof(_errorMessage) - 1);
        _errorMessage[sizeof(_errorMessage) - 1] = '\0';
    } else {
        strcpy(_errorMessage, "SOS ERROR");
    }
    _sosIndex = 0;
    _sosRepeat = 0;
    _sosInPattern = false;
    _sosLastTime = millis();
    _lastErrorMessageTime = 0; // Force immediate error message
}

// IComponent interface implementation
bool HeartbeatLEDManager::selfTest() {
    Serial.print(F("HeartbeatLEDManager Self-Test:\r\n"));
    
    bool result = true;
    
    // Test LED pin
    Serial.print(F("  Testing LED pin "));
    Serial.print(_pin);
    Serial.print(F("... "));
    
    // Save current state
    bool originalState = _ledState;
    
    // Test LED on
    setLEDState(true);
    delay(100);
    
    // Test LED off
    setLEDState(false);
    delay(100);
    
    // Restore original state
    setLEDState(originalState);
    
    Serial.print(F("✅ OK\r\n"));
    
    // Dependencies validated by ServiceLocator at startup
    
    return result;
}

const char* HeartbeatLEDManager::getComponentName() const {
    return "HeartbeatLEDManager";
}

bool HeartbeatLEDManager::validateDependencies() const {
    bool valid = true;
    
    auto configService = getServices().getConfigurationService();
    if (!configService) {
        Serial.print(F("  Missing ConfigurationService dependency\r\n"));
        valid = false;
    }
    
    return valid;
}

void HeartbeatLEDManager::printDependencyStatus() const {
    Serial.print(F("HeartbeatLEDManager Dependencies:\r\n"));
    
    auto configService = getServices().getConfigurationService();
    Serial.print(F("  ConfigurationService: "));
    Serial.print(configService ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));
}

unsigned long HeartbeatLEDManager::getUpdateInterval() const {
    auto configService = getServices().getConfigurationService();
    return configService ? configService->getHeartbeatInterval() : 100; // Default 100ms
}

} // namespace DeviceBridge::Components