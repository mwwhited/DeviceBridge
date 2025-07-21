#include <stdint.h>
#include <Arduino.h>
#include "Port.h"
#include "Control.h"
#include "Status.h"
#include "Data.h"
#include "OptimizedTiming.h"
#include "HardwareFlowControl.h"
#include "../Common/ServiceLocator.h"
#include "../Common/ConfigurationService.h"
#include <RingBuf.h>

namespace DeviceBridge::Parallel
{
  // https://forum.arduino.cc/t/use-attachinterrupt-with-a-class-method-function/301108/8

  Port::Port(
      Control control,
      Status status,
      Data data) : _control(control),
                            _status(status),
                            _data(data),
                            _buffer(),
                            _whichIsr(_isrSeed++),
                            _interruptCount(0),
                            _dataCount(0),
                            _locked(false),
                            _criticalFlowControl(false),
                            _criticalStartTime(0),
                            _pendingAck(false),
                            _pendingFlowControl(false),
                            _lastFlowControlLevel(0),
                            _hardwareFlowEnabled(false)
  {
  }

  void Port::handleInterrupt()
  {
    // Count all interrupt calls for debugging
    _interruptCount++;
    
    // Check if port is locked (SPI/Serial operations in progress)
    if (_locked) {
      return; // Ignore data while locked
    }
    
    // Check for buffer overflow BEFORE capturing data
    if (_buffer.isFull()) {
      // Critical: Buffer overflow! Drop this byte and signal error
      setBusy(true);  // Hold busy to prevent more data
      return;
    }
    
    // TDS2024 strobe pulses are very fast - on FALLING edge, capture data immediately
    // Don't check strobe state as it returns to HIGH before we can read it
    
    // Set busy to indicate we're processing (critical for TDS2024 timing)
    _status.setBusy();
    
    // Brief delay to ensure TDS2024 sees the busy signal
    delayMicroseconds(ServiceLocator::getInstance().getConfigurationService()->getHardwareDelayUs());
    
    // Read the data byte from parallel port with timing critical section
    uint8_t value = _data.readValue();
    
    // Count valid data captures
    _dataCount++;
    
    // Store data in ring buffer for processing - this MUST succeed
    // since we checked for full buffer above
    bool pushResult = _buffer.push(value);
    
    // Send acknowledge pulse to confirm data received
    // This MUST happen before clearing busy for proper protocol
    sendAcknowledge();
    
    // State-based adaptive flow control
    if (isCriticallyFull()) {
      // Entering or staying in critical state
      if (!_criticalFlowControl) {
        _criticalFlowControl = true;
        _criticalStartTime = millis();
      }
      setBusy(true);
      delayMicroseconds(ServiceLocator::getInstance().getConfigurationService()->getCriticalFlowDelayUs()); // Extended delay in critical state
    } else if (_criticalFlowControl) {
      // In critical recovery - stay busy until below warning level
      if (!isAlmostFull()) {
        // Buffer drained below warning level - exit critical state
        _criticalFlowControl = false;
        setBusy(false);
        delayMicroseconds(ServiceLocator::getInstance().getConfigurationService()->getTds2024TimingUs());
      } else {
        // Still above warning level - maintain critical flow control
        setBusy(true);
        delayMicroseconds(ServiceLocator::getInstance().getConfigurationService()->getCriticalFlowDelayUs());
      }
    } else if (isAlmostFull()) {
      // 60%+ full - WARNING: Hold busy with moderate delay
      setBusy(true);
      delayMicroseconds(ServiceLocator::getInstance().getConfigurationService()->getModerateFlowDelayUs()); // Moderate delay to slow down sender
    } else {
      // <60% full - Normal operation
      setBusy(false);
      delayMicroseconds(ServiceLocator::getInstance().getConfigurationService()->getTds2024TimingUs()); // Brief delay for TDS2024 timing stability
    }
    
    // Memory barrier to ensure all operations complete
    __asm__ __volatile__("" ::: "memory");
  }
  
  void Port::handleInterruptOptimized()
  {
    // IEEE-1284 COMPLIANT MINIMAL ISR - Target execution time: ≤2μs
    
    // CRITICAL: Immediate data capture with atomic read
    uint8_t data = _data.readValueAtomic();
    
    // CRITICAL: Check buffer space and store data (≤1μs)
    if (!_buffer.isFull()) {
      _buffer.push(data);
      _dataCount++;
      
      // CRITICAL: Immediate ACK response (IEEE-1284 requires ≤10μs)
      _status.sendAcknowledgePulseOptimized();
      
      // Flag deferred processing for main loop
      _pendingFlowControl = true;
      
      // Fast flow control decision using cached thresholds
      uint16_t bufferSize = _buffer.size();
      
      // Use hardware flow control if enabled, otherwise use basic flow control
      if (_hardwareFlowEnabled) {
        _hardwareFlowControl.updateFlowControl(bufferSize, _buffer.maxSize());
      } else {
        // Basic flow control using status pins
        if (bufferSize >= OptimizedTiming::criticalThreshold) {
          _lastFlowControlLevel = 3; // Critical
          _status.setBusy(true);
        } else if (bufferSize >= OptimizedTiming::moderateThreshold) {
          _lastFlowControlLevel = 2; // Moderate  
          _status.setBusy(true);
        } else {
          _lastFlowControlLevel = 1; // Normal
          _status.setBusy(false);
        }
      }
    } else {
      // Buffer overflow - signal error immediately
      if (_hardwareFlowEnabled) {
        _hardwareFlowControl.setFlowState(HardwareFlowControl::FlowState::EMERGENCY);
      } else {
        _status.setBusy(true);
        _status.setError(true);
      }
    }
    
    _interruptCount++;
    
    // TOTAL ISR TIME: ≤2μs (IEEE-1284 compliant!)
  }
  
  // Hardware flow control is now integrated into handleInterruptOptimized()
  // This eliminates the need for a separate ISR and improves maintainability

  byte Port::_isrSeed = 0;
  Port *Port::_instance0;
  void Port::isr0()
  {
    // Use optimized ISR by default (hardware flow control is integrated)
    if (OptimizedTiming::isInitialized()) {
      _instance0->handleInterruptOptimized();
    } else {
      _instance0->handleInterrupt();
    }
  }
  Port *Port::_instance1;
  void Port::isr1()
  {
    // Use optimized ISR by default (hardware flow control is integrated)
    if (OptimizedTiming::isInitialized()) {
      _instance1->handleInterruptOptimized();
    } else {
      _instance1->handleInterrupt();
    }
  }
  Port *Port::_instance2;
  void Port::isr2()
  {
    // Use optimized ISR by default (hardware flow control is integrated)  
    if (OptimizedTiming::isInitialized()) {
      _instance2->handleInterruptOptimized();
    } else {
      _instance2->handleInterrupt();
    }
  }

  void Port::initialize()
  {
    _control.initialize();
    _status.initialize();
    _data.initialize();

    switch (_whichIsr)
    {
    case 0:
      _instance0 = this;
      attachInterrupt(digitalPinToInterrupt(_control.getStrobePin()), isr0, FALLING); // Attach to pin interrupt
      break;
    case 1:
      _instance1 = this;
      attachInterrupt(digitalPinToInterrupt(_control.getStrobePin()), isr1, FALLING); // Attach to pin interrupt
      break;
    case 2:
      _instance2 = this;
      attachInterrupt(digitalPinToInterrupt(_control.getStrobePin()), isr2, FALLING); // Attach to pin interrupt
      break;
    }
  }
  
  void Port::initializeOptimized()
  {
    // Initialize cached timing values first
    OptimizedTiming::initialize();
    
    // Initialize hardware components
    _control.initialize();
    _status.initialize();
    _data.initialize();

    // Replace ISR handlers with optimized versions
    switch (_whichIsr)
    {
    case 0:
      _instance0 = this;
      attachInterrupt(digitalPinToInterrupt(_control.getStrobePin()), isr0, FALLING);
      break;
    case 1:
      _instance1 = this;
      attachInterrupt(digitalPinToInterrupt(_control.getStrobePin()), isr1, FALLING);
      break;
    case 2:
      _instance2 = this;
      attachInterrupt(digitalPinToInterrupt(_control.getStrobePin()), isr2, FALLING);
      break;
    }
  }
  
  // Hardware flow control is now integrated - use initializeOptimized() and enable via setHardwareFlowControlEnabled()

  bool Port::hasData()
  {
    return !_buffer.isEmpty();
  }

  bool Port::isAlmostFull()
  {
    // 60% threshold for moderate flow control
    return _buffer.size() >= ServiceLocator::getInstance().getConfigurationService()->getModerateFlowThreshold(_buffer.maxSize());
  }

  bool Port::isCriticallyFull()
  {
    // 80% threshold for extended flow control
    return _buffer.size() >= ServiceLocator::getInstance().getConfigurationService()->getCriticalFlowThreshold(_buffer.maxSize());
  }

  bool Port::isFull()
  {
    return _buffer.isFull();
  }

  uint16_t Port::readData(uint8_t buffer[], uint16_t index, uint16_t length)
  {
    // Note: sizeof(buffer) gives size of pointer, not array
    // For Arduino, we'll use the length parameter properly
    if (length == 0) // if length is 0, assume we want to fill the buffer
      length = ServiceLocator::getInstance().getConfigurationService()->getDataChunkSize(); // Default chunk size from configuration

    // Disable interrupts during buffer operations to prevent corruption
    noInterrupts();
    
    uint16_t cnt = 0;
    for (uint16_t i = 0; i < length; i++)
    {
      uint8_t element;
      if (_buffer.lockedPop(element))
      {
        buffer[index + i] = element;
        cnt++;
      }
      else
      {
        break; // read buffer is empty
      }
    }
    
    // Re-enable interrupts
    interrupts();
    
    // Aggressive flow control update based on buffer level after read
    uint16_t bufferLevelAfterRead = _buffer.size();
    uint16_t bufferCapacity = _buffer.maxSize();
    
    if (cnt > 0) { // Only update if we actually read data
      if (bufferLevelAfterRead < ServiceLocator::getInstance().getConfigurationService()->getRecoveryFlowThreshold(bufferCapacity)) {
        // Less than 50% full - clear busy immediately
        setBusy(false);
      } else if (bufferLevelAfterRead < ServiceLocator::getInstance().getConfigurationService()->getModerateFlowThreshold(bufferCapacity)) {
        // 50-60% full - clear busy but with brief delay
        setBusy(false);
        delayMicroseconds(ServiceLocator::getInstance().getConfigurationService()->getFlowControlDelayUs());
      }
      // If still >60% full, keep busy active until next interrupt
    }
    
    return cnt;
  }

  void Port::setBusy(bool busy) {
    _status.setBusy(busy);
  }

  void Port::setError(bool error) {
    _status.setError(error);
  }

  void Port::setPaperOut(bool paperOut) {
    _status.setPaperOut(paperOut);
  }

  void Port::setSelect(bool select) {
    _status.setSelect(select);
  }

  void Port::sendAcknowledge() {
    _status.sendAcknowledgePulse();
  }

  void Port::clearBuffer() {
    // Clear the ring buffer and reset flow control
    noInterrupts();
    while (!_buffer.isEmpty()) {
      uint8_t dummy;
      _buffer.lockedPop(dummy);
    }
    setBusy(false); // Clear busy signal since buffer is empty
    interrupts();
  }

  uint16_t Port::getBufferSize() const {
    return _buffer.size();
  }

  uint16_t Port::getBufferFreeSpace() const {
    return _buffer.maxSize() - _buffer.size();
  }

  bool Port::checkCriticalTimeout() const {
    if (!_criticalFlowControl) {
      return false;
    }
    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - _criticalStartTime;
    return elapsed >= CRITICAL_TIMEOUT_MS;
  }

  void Port::resetCriticalState() {
    _criticalFlowControl = false;
    _criticalStartTime = 0;
    setBusy(false);
  }

  void Port::processPendingOperations()
  {
    // Process deferred operations from optimized ISR
    // This runs in main loop context, not interrupt context
    
    if (_pendingFlowControl) {
      _pendingFlowControl = false;
      
      // Apply detailed flow control timing based on level
      switch (_lastFlowControlLevel) {
        case 3: // Critical
          if (!_criticalFlowControl) {
            _criticalFlowControl = true;
            _criticalStartTime = millis();
          }
          delayMicroseconds(OptimizedTiming::criticalFlowDelayUs);
          break;
          
        case 2: // Moderate
          delayMicroseconds(OptimizedTiming::moderateFlowDelayUs);
          break;
          
        case 1: // Normal
        default:
          // Clear critical state if we were in it
          if (_criticalFlowControl && _buffer.size() < OptimizedTiming::recoveryThreshold) {
            _criticalFlowControl = false;
          }
          delayMicroseconds(OptimizedTiming::flowControlDelayUs);
          break;
      }
    }
    
    // Check for critical timeout recovery
    if (_criticalFlowControl && checkCriticalTimeout()) {
      // Emergency timeout - clear buffer and reset state
      clearBuffer();
      resetCriticalState();
    }
    
    // Process hardware flow control deferred operations
    if (_hardwareFlowEnabled) {
      _hardwareFlowControl.processDeferred();
    }
  }
  
  void Port::setHardwareFlowControlEnabled(bool enabled)
  {
    _hardwareFlowEnabled = enabled;
    if (enabled) {
      _hardwareFlowControl.initialize();
    }
  }
  
  HardwareFlowControl::Statistics Port::getFlowControlStatistics() const
  {
    return _hardwareFlowControl.getStatistics();
  }
  
  /*
  | Name         | DB25  |  Direction | Register |
  |--------------|-------|------------|----------|
  | /Strobe      | 1     |  Input     | Control  |
  | D0           | 2     |  Input     | Data     |
  | D1           | 3     |  Input     | Data     |
  | D2           | 4     |  Input     | Data     |
  | D3           | 5     |  Input     | Data     |
  | D4           | 6     |  Input     | Data     |
  | D5           | 7     |  Input     | Data     |
  | D6           | 8     |  Input     | Data     |
  | D7           | 9     |  Input     | Data     |
  | /Acknowledge | 10    |  Output    | Status   |
  | Busy         | 11    |  Output    | Status   |
  | Paper Out    | 12    |  Output    | Status   |
  | Select       | 13    |  Output    | Status   |
  | /Auto Feed   | 14    |  Input     | Control  |
  | /Error       | 15    |  Output    | Status   |
  | /Initialize  | 16    |  Input     | Control  |
  | /Select In   | 17    |  Input     | Control  |
  | Ground       | 18-25 |  Power     |          |
  */
}