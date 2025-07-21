#pragma once

#include <stdint.h>
#include <Arduino.h>
#include "../Common/Config.h"

namespace DeviceBridge::Parallel {

/**
 * @brief Pre-computed timing constants for IEEE-1284 compliant ISR performance
 * 
 * This class caches all timing values at initialization to eliminate
 * ServiceLocator overhead in time-critical interrupt service routines.
 * All values are computed once and stored as static constants for
 * maximum performance.
 */
class OptimizedTiming {
public:
    // Cached timing values (computed at initialization)
    static uint16_t hardwareDelayUs;
    static uint16_t ackPulseUs;
    static uint16_t recoveryDelayUs;
    static uint16_t criticalFlowDelayUs;
    static uint16_t moderateFlowDelayUs;
    static uint16_t flowControlDelayUs;
    
    // Buffer thresholds (pre-computed percentages)
    static uint16_t moderateThreshold;      // 50% of buffer size
    static uint16_t criticalThreshold;      // 70% of buffer size  
    static uint16_t preWarningThreshold;    // 40% of buffer size
    static uint16_t recoveryThreshold;      // 40% of buffer size
    
    // Emergency timeout values
    static uint32_t criticalTimeoutMs;
    static uint32_t chunkSendTimeoutMs;
    
    // Pin assignments available through Common::Pins namespace
    
    /**
     * @brief Initialize all cached timing values
     * 
     * Call this ONCE during system initialization to cache all timing
     * values from ConfigurationService. This eliminates the need for
     * ServiceLocator calls in the ISR.
     */
    static void initialize();
    
    /**
     * @brief Check if timing values have been initialized
     * @return true if initialize() has been called
     */
    static bool isInitialized() { return _initialized; }
    
private:
    static bool _initialized;
};

} // namespace DeviceBridge::Parallel