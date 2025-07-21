#include "HardwareFlowControl.h"
#include "../Common/ServiceLocator.h"

namespace DeviceBridge::Parallel {

HardwareFlowControl::HardwareFlowControl() 
    : _currentState(FlowState::NORMAL)
    , _previousState(FlowState::NORMAL)
    , _stateChangeTime(0)
    , _stateHoldTime(0)
    , _lastBufferLevel(0)
    , _emergencyMode(false)
    , _emergencyStartTime(0)
    , _stateTransitions(0)
    , _emergencyCount(0)
    , _recoveryCount(0)
    , _pinStates(0)
{
    // Initialize with default configuration using cached timing values
    _config.busyPin = Common::Pins::LPT_BUSY;
    _config.errorPin = Common::Pins::LPT_ERROR;
    _config.paperOutPin = Common::Pins::LPT_PAPER_OUT;
    _config.selectPin = Common::Pins::LPT_SELECT;
    
    // Use pre-computed thresholds from Config.h constants
    _config.warningThreshold = Common::FlowControl::PRE_WARNING_THRESHOLD;
    _config.criticalThreshold = Common::FlowControl::CRITICAL_THRESHOLD;
    _config.emergencyThreshold = Common::FlowControl::CRITICAL_THRESHOLD + 10; // 10 bytes above critical
    _config.recoveryThreshold = Common::FlowControl::RECOVERY_THRESHOLD;
    
    // Hardware timing for reliable signal recognition
    _config.signalSetupTime = 2;  // 2μs setup time for host recognition
    _config.signalHoldTime = 5;   // 5μs hold time for signal stability
}

HardwareFlowControl::HardwareFlowControl(const Config& config) 
    : _config(config)
    , _currentState(FlowState::NORMAL)
    , _previousState(FlowState::NORMAL)
    , _stateChangeTime(0)
    , _stateHoldTime(0)
    , _lastBufferLevel(0)
    , _emergencyMode(false)
    , _emergencyStartTime(0)
    , _stateTransitions(0)
    , _emergencyCount(0)
    , _recoveryCount(0)
    , _pinStates(0)
{
}

void HardwareFlowControl::initialize() {
    // Initialize all control pins as outputs with correct initial states
    pinMode(_config.busyPin, OUTPUT);
    digitalWrite(_config.busyPin, LOW);      // Not busy initially
    
    pinMode(_config.errorPin, OUTPUT);
    digitalWrite(_config.errorPin, HIGH);    // No error initially (active LOW)
    
    pinMode(_config.paperOutPin, OUTPUT);
    digitalWrite(_config.paperOutPin, LOW);  // Paper available initially
    
    pinMode(_config.selectPin, OUTPUT);
    digitalWrite(_config.selectPin, HIGH);   // Selected initially
    
    // Initialize state
    _currentState = FlowState::NORMAL;
    _previousState = FlowState::NORMAL;
    _stateChangeTime = millis();
    _stateHoldTime = 0;
    _emergencyMode = false;
    
    // Update pin state cache
    updatePinStateCache();
    
    // Apply initial hardware signals
    applyHardwareSignals();
}

bool HardwareFlowControl::updateFlowControl(uint16_t bufferLevel, uint16_t bufferSize) {
    // Calculate buffer utilization percentage
    uint16_t bufferPercentage = (bufferLevel * 100) / bufferSize;
    
    // Determine optimal state based on buffer level
    FlowState optimalState = calculateOptimalState(bufferLevel);
    
    // Check if state transition is allowed (prevent oscillation)
    if (!isStateTransitionAllowed(optimalState)) {
        return false; // No state change
    }
    
    // Emergency override - immediate response to buffer overflow risk
    if (bufferLevel >= _config.emergencyThreshold && !_emergencyMode) {
        _emergencyMode = true;
        _emergencyStartTime = millis();
        _emergencyCount++;
        optimalState = FlowState::EMERGENCY;
    }
    
    // Update state if changed
    if (optimalState != _currentState) {
        _previousState = _currentState;
        _currentState = optimalState;
        _stateChangeTime = millis();
        _stateTransitions++;
        
        // Apply hardware signals immediately for ISR context
        applyHardwareSignals();
        
        _lastBufferLevel = bufferLevel;
        return true; // State changed
    }
    
    _lastBufferLevel = bufferLevel;
    return false; // No state change
}

void HardwareFlowControl::setFlowState(FlowState state) {
    if (state != _currentState) {
        _previousState = _currentState;
        _currentState = state;
        _stateChangeTime = millis();
        _stateTransitions++;
        
        // Handle emergency mode flag
        if (state == FlowState::EMERGENCY) {
            _emergencyMode = true;
            _emergencyStartTime = millis();
            _emergencyCount++;
        } else if (_emergencyMode && state == FlowState::NORMAL) {
            _emergencyMode = false;
            _recoveryCount++;
        }
        
        applyHardwareSignals();
    }
}

void HardwareFlowControl::resetEmergency() {
    if (_emergencyMode) {
        _emergencyMode = false;
        _recoveryCount++;
        setFlowState(FlowState::NORMAL);
    }
}

HardwareFlowControl::Statistics HardwareFlowControl::getStatistics() const {
    Statistics stats;
    stats.stateTransitions = _stateTransitions;
    stats.emergencyActivations = _emergencyCount;
    stats.recoveryOperations = _recoveryCount;
    stats.currentState = _currentState;
    stats.timeInCurrentState = millis() - _stateChangeTime;
    return stats;
}

void HardwareFlowControl::processDeferred() {
    // Check for emergency timeout (20 seconds max)
    if (_emergencyMode) {
        uint32_t emergencyDuration = millis() - _emergencyStartTime;
        if (emergencyDuration > 20000) { // 20 second timeout
            // Force reset emergency mode after timeout
            resetEmergency();
        }
    }
    
    // Additional deferred processing can be added here
    // (logging, statistics updates, etc.)
}

const char* HardwareFlowControl::getStateName(FlowState state) {
    switch (state) {
        case FlowState::NORMAL:    return "NORMAL";
        case FlowState::WARNING:   return "WARNING";
        case FlowState::CRITICAL:  return "CRITICAL";
        case FlowState::EMERGENCY: return "EMERGENCY";
        default:                   return "UNKNOWN";
    }
}

void HardwareFlowControl::applyHardwareSignals() {
    // Apply hardware signals based on current state
    // Use direct pin manipulation for maximum speed in ISR context
    
    switch (_currentState) {
        case FlowState::NORMAL:
            // Normal operation - ready to receive data
            digitalWrite(_config.busyPin, LOW);      // Not busy
            digitalWrite(_config.errorPin, HIGH);    // No error (active LOW)
            digitalWrite(_config.paperOutPin, LOW);  // Paper available
            digitalWrite(_config.selectPin, HIGH);   // Selected
            break;
            
        case FlowState::WARNING:
            // Buffer getting full - signal host to slow down
            digitalWrite(_config.busyPin, HIGH);     // Busy signal
            digitalWrite(_config.errorPin, HIGH);    // No error yet
            digitalWrite(_config.paperOutPin, LOW);  // Paper still available
            digitalWrite(_config.selectPin, HIGH);   // Still selected
            break;
            
        case FlowState::CRITICAL:
            // Buffer nearly full - strong signal to slow down
            digitalWrite(_config.busyPin, HIGH);     // Busy signal
            digitalWrite(_config.errorPin, HIGH);    // No error yet
            digitalWrite(_config.paperOutPin, HIGH); // Paper out signal (additional warning)
            digitalWrite(_config.selectPin, HIGH);   // Still selected
            break;
            
        case FlowState::EMERGENCY:
            // Buffer overflow imminent - signal to stop immediately
            digitalWrite(_config.busyPin, HIGH);     // Busy signal
            digitalWrite(_config.errorPin, LOW);     // Error signal (active LOW)
            digitalWrite(_config.paperOutPin, HIGH); // Paper out signal
            digitalWrite(_config.selectPin, LOW);    // Not selected (stop transmission)
            break;
    }
    
    // Brief delay for signal setup time (hardware recognition)
    delayMicroseconds(_config.signalSetupTime);
    
    // Update cached pin states
    updatePinStateCache();
}

HardwareFlowControl::FlowState HardwareFlowControl::calculateOptimalState(uint16_t bufferLevel) const {
    // Determine optimal state based on buffer level and thresholds
    
    if (bufferLevel >= _config.emergencyThreshold) {
        return FlowState::EMERGENCY;
    } else if (bufferLevel >= _config.criticalThreshold) {
        return FlowState::CRITICAL;
    } else if (bufferLevel >= _config.warningThreshold) {
        return FlowState::WARNING;
    } else if (bufferLevel <= _config.recoveryThreshold) {
        return FlowState::NORMAL;
    }
    
    // Hysteresis - maintain current state if between thresholds
    return _currentState;
}

bool HardwareFlowControl::isStateTransitionAllowed(FlowState newState) const {
    // Prevent rapid state oscillation by enforcing minimum hold time
    uint32_t currentTime = millis();
    uint32_t timeSinceLastChange = currentTime - _stateChangeTime;
    
    // Minimum hold times for state stability (milliseconds)
    uint32_t minHoldTime = 0;
    switch (_currentState) {
        case FlowState::NORMAL:    minHoldTime = 10;  break;  // 10ms minimum
        case FlowState::WARNING:   minHoldTime = 20;  break;  // 20ms minimum  
        case FlowState::CRITICAL:  minHoldTime = 50;  break;  // 50ms minimum
        case FlowState::EMERGENCY: minHoldTime = 100; break;  // 100ms minimum
    }
    
    // Always allow emergency transitions immediately
    if (newState == FlowState::EMERGENCY) {
        return true;
    }
    
    // Always allow recovery from emergency
    if (_currentState == FlowState::EMERGENCY && newState == FlowState::NORMAL) {
        return true;
    }
    
    // Check minimum hold time for other transitions
    return timeSinceLastChange >= minHoldTime;
}

void HardwareFlowControl::updatePinStateCache() {
    // Cache current pin states for fast access
    _pinStates = 0;
    _pinStates |= (digitalRead(_config.busyPin) ? 0x01 : 0);
    _pinStates |= (digitalRead(_config.errorPin) ? 0x02 : 0);
    _pinStates |= (digitalRead(_config.paperOutPin) ? 0x04 : 0);
    _pinStates |= (digitalRead(_config.selectPin) ? 0x08 : 0);
}

} // namespace DeviceBridge::Parallel