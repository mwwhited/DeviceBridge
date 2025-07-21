#pragma once

#include <stdint.h>
#include <Arduino.h>
#include "../Common/Config.h"
#include "OptimizedTiming.h"

namespace DeviceBridge::Parallel {

/**
 * @brief Hardware-assisted flow control for IEEE-1284 parallel port communication
 * 
 * This class provides hardware-level flow control using direct pin manipulation
 * and interrupt-driven state management. It eliminates software delays in the 
 * ISR by using hardware signals to manage data flow.
 */
class HardwareFlowControl {
public:
    /**
     * @brief Flow control states for hardware signaling
     */
    enum class FlowState : uint8_t {
        NORMAL = 0,      // Normal operation - ready for data
        WARNING = 1,     // Buffer approaching full - slow down
        CRITICAL = 2,    // Buffer nearly full - hold transmission
        EMERGENCY = 3    // Buffer overflow imminent - stop immediately
    };
    
    /**
     * @brief Hardware flow control configuration
     */
    struct Config {
        uint8_t busyPin;          // LPT_BUSY pin for flow control
        uint8_t errorPin;         // LPT_ERROR pin for error signaling
        uint8_t paperOutPin;      // LPT_PAPER_OUT pin for additional signaling
        uint8_t selectPin;        // LPT_SELECT pin for status
        
        // Threshold levels (buffer percentages)
        uint16_t warningThreshold;    // Switch to WARNING state
        uint16_t criticalThreshold;   // Switch to CRITICAL state
        uint16_t emergencyThreshold;  // Switch to EMERGENCY state
        uint16_t recoveryThreshold;   // Return to NORMAL state
        
        // Hardware timing (microseconds)
        uint16_t signalSetupTime;     // Time for host to recognize signal changes
        uint16_t signalHoldTime;      // Time to hold signals for reliable detection
    };
    
private:
    Config _config;
    volatile FlowState _currentState;
    volatile FlowState _previousState;
    volatile uint32_t _stateChangeTime;
    volatile uint32_t _stateHoldTime;
    
    // State transition tracking
    volatile uint16_t _lastBufferLevel;
    volatile bool _emergencyMode;
    volatile uint32_t _emergencyStartTime;
    
    // Performance counters
    volatile uint32_t _stateTransitions;
    volatile uint32_t _emergencyCount;
    volatile uint32_t _recoveryCount;
    
    // Pin state cache for fast access
    volatile uint8_t _pinStates;
    
public:
    /**
     * @brief Constructor with default configuration
     */
    HardwareFlowControl();
    
    /**
     * @brief Constructor with custom configuration
     * @param config Hardware flow control configuration
     */
    explicit HardwareFlowControl(const Config& config);
    
    /**
     * @brief Initialize hardware flow control
     * Must be called during system setup
     */
    void initialize();
    
    /**
     * @brief Update flow control based on buffer level
     * Call from ISR context for immediate hardware response
     * @param bufferLevel Current buffer utilization (0-100%)
     * @param bufferSize Total buffer capacity
     * @return true if state changed, false if no change
     */
    bool updateFlowControl(uint16_t bufferLevel, uint16_t bufferSize);
    
    /**
     * @brief Force specific flow control state
     * Used for emergency conditions or testing
     * @param state Desired flow control state
     */
    void setFlowState(FlowState state);
    
    /**
     * @brief Get current flow control state
     * @return Current state
     */
    FlowState getCurrentState() const { return _currentState; }
    
    /**
     * @brief Check if in emergency mode
     * @return true if emergency flow control is active
     */
    bool isEmergencyMode() const { return _emergencyMode; }
    
    /**
     * @brief Reset emergency mode and return to normal operation
     */
    void resetEmergency();
    
    /**
     * @brief Get performance statistics
     */
    struct Statistics {
        uint32_t stateTransitions;
        uint32_t emergencyActivations;
        uint32_t recoveryOperations;
        FlowState currentState;
        uint32_t timeInCurrentState;
    };
    
    Statistics getStatistics() const;
    
    /**
     * @brief Process deferred flow control operations
     * Call from main loop context for non-critical operations
     */
    void processDeferred();
    
    /**
     * @brief Get readable state name
     * @param state Flow control state
     * @return String representation of state
     */
    static const char* getStateName(FlowState state);
    
private:
    /**
     * @brief Apply hardware signals for current state
     * Fast pin manipulation for ISR context
     */
    void applyHardwareSignals();
    
    /**
     * @brief Calculate optimal state based on buffer level
     * @param bufferLevel Current buffer utilization
     * @return Recommended flow state
     */
    FlowState calculateOptimalState(uint16_t bufferLevel) const;
    
    /**
     * @brief Check if state transition is allowed
     * Prevents rapid state oscillation
     * @param newState Proposed new state
     * @return true if transition is allowed
     */
    bool isStateTransitionAllowed(FlowState newState) const;
    
    /**
     * @brief Update pin states cache for fast access
     */
    void updatePinStateCache();
};

} // namespace DeviceBridge::Parallel