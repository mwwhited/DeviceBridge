#pragma once

#include <Arduino.h>
#include "../Common/Config.h"
#include "../Common/ServiceLocator.h"

namespace DeviceBridge::Components {

class HeartbeatLEDManager : public DeviceBridge::IComponent {
public:
    enum class HeartbeatMode {
        NORMAL,     // Regular heartbeat blink
        SOS,        // SOS pattern for errors
        OFF         // LED disabled
    };

private:
    HeartbeatMode _mode;
    uint8_t _pin;
    bool _ledState;
    uint32_t _lastUpdate;
    
    // SOS pattern state
    uint8_t _sosIndex;
    uint8_t _sosRepeat;
    bool _sosInPattern;
    uint32_t _sosLastTime;
    
    // SOS pattern: dot=150ms, dash=450ms, pause=150ms, letter_pause=450ms, word_pause=1050ms
    // Pattern: ... --- ... (pause) repeat every 5 seconds
    static constexpr uint16_t SOS_DOT_MS = 150;
    static constexpr uint16_t SOS_DASH_MS = 450;
    static constexpr uint16_t SOS_PAUSE_MS = 150;
    static constexpr uint16_t SOS_LETTER_PAUSE_MS = 450;
    static constexpr uint16_t SOS_REPEAT_PAUSE_MS = 1050;
    static constexpr uint8_t SOS_PATTERN_LENGTH = 11; // ... --- ...
    
    // SOS pattern definition: 1=dot, 3=dash, 0=pause
    static constexpr uint8_t SOS_PATTERN[SOS_PATTERN_LENGTH] = {1, 1, 1, 0, 3, 3, 3, 0, 1, 1, 1};
    
    void updateNormalHeartbeat();
    void updateSOSPattern();
    uint16_t getSOSElementDuration(uint8_t element);

public:
    HeartbeatLEDManager(uint8_t pin = Common::Pins::HEARTBEAT);
    ~HeartbeatLEDManager();
    
    // Lifecycle management (IComponent interface)
    bool initialize() override;
    void update() override;
    void stop() override;
    
    // IComponent interface implementation
    bool selfTest() override;
    const char* getComponentName() const override;
    bool validateDependencies() const override;
    void printDependencyStatus() const override;
    
    // Mode control
    void setMode(HeartbeatMode mode);
    HeartbeatMode getMode() const { return _mode; }
    
    // Manual LED control
    void setLEDState(bool state);
    bool getLEDState() const { return _ledState; }
    
    // Pin configuration
    void setPin(uint8_t pin);
    uint8_t getPin() const { return _pin; }
};

} // namespace DeviceBridge::Components