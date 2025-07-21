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
    
    // Get configuration service once
    auto configService = ServiceLocator::getInstance().getConfigurationService();
    
    // Cache all timing values from configuration
    hardwareDelayUs = configService->getHardwareDelayUs();
    ackPulseUs = configService->getAckPulseUs();
    recoveryDelayUs = configService->getRecoveryDelayUs();
    criticalFlowDelayUs = configService->getCriticalFlowDelayUs();
    moderateFlowDelayUs = configService->getModerateFlowDelayUs();
    flowControlDelayUs = configService->getFlowControlDelayUs();
    
    // Cache emergency timeout values
    criticalTimeoutMs = configService->getCriticalTimeoutMs();
    chunkSendTimeoutMs = configService->getChunkSendTimeoutMs();
    
    // Pre-compute buffer thresholds to eliminate runtime calculations
    uint16_t bufferSize = configService->getRingBufferSize();
    
    // Use the optimized thresholds from Config.h
    preWarningThreshold = (bufferSize * Common::FlowControl::PRE_WARNING_THRESHOLD_PERCENT) / 100;
    moderateThreshold = (bufferSize * Common::FlowControl::MODERATE_THRESHOLD_PERCENT) / 100;
    criticalThreshold = (bufferSize * Common::FlowControl::CRITICAL_THRESHOLD_PERCENT) / 100;
    recoveryThreshold = (bufferSize * Common::FlowControl::RECOVERY_THRESHOLD_PERCENT) / 100;
    
    _initialized = true;
}

} // namespace DeviceBridge::Parallel