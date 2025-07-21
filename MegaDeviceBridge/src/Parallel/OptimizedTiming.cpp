#include <Arduino.h>
#include "OptimizedTiming.h"
#include "../Common/ServiceLocator.h"
#include "../Common/ConfigurationService.h"

namespace DeviceBridge::Parallel {

// Static member initialization
bool OptimizedTiming::_initialized = false;

uint16_t OptimizedTiming::hardwareDelayUs = 0;
uint16_t OptimizedTiming::ackPulseUs = 0; 
uint16_t OptimizedTiming::recoveryDelayUs = 0;
uint16_t OptimizedTiming::criticalFlowDelayUs = 0;
uint16_t OptimizedTiming::moderateFlowDelayUs = 0;
uint16_t OptimizedTiming::flowControlDelayUs = 0;

uint16_t OptimizedTiming::moderateThreshold = 0;
uint16_t OptimizedTiming::criticalThreshold = 0;
uint16_t OptimizedTiming::preWarningThreshold = 0;
uint16_t OptimizedTiming::recoveryThreshold = 0;

uint32_t OptimizedTiming::criticalTimeoutMs = 0;
uint32_t OptimizedTiming::chunkSendTimeoutMs = 0;

// Pin assignments accessed through Common::Pins namespace

void OptimizedTiming::initialize() {
    if (_initialized) {
        return; // Already initialized
    }
    
    // Cache all timing values from configuration constants
    hardwareDelayUs = Common::Timing::HARDWARE_DELAY_US;
    ackPulseUs = Common::Timing::ACK_PULSE_US;
    recoveryDelayUs = Common::Timing::RECOVERY_DELAY_US;
    criticalFlowDelayUs = Common::Timing::CRITICAL_FLOW_DELAY_US;
    moderateFlowDelayUs = Common::Timing::MODERATE_FLOW_DELAY_US;
    flowControlDelayUs = Common::Timing::FLOW_CONTROL_DELAY_US;
    
    // Cache emergency timeout values from constants
    criticalTimeoutMs = Common::Buffer::CRITICAL_TIMEOUT_MS;
    chunkSendTimeoutMs = Common::Buffer::CHUNK_SEND_TIMEOUT_MS;
    
    // Use the pre-computed thresholds from Config.h
    preWarningThreshold = Common::FlowControl::PRE_WARNING_THRESHOLD;
    moderateThreshold = Common::FlowControl::MODERATE_THRESHOLD;
    criticalThreshold = Common::FlowControl::CRITICAL_THRESHOLD;
    recoveryThreshold = Common::FlowControl::RECOVERY_THRESHOLD;
    
    _initialized = true;
}

} // namespace DeviceBridge::Parallel