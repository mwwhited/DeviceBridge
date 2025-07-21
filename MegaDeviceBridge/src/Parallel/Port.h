#pragma once

#include <stdint.h>
#include "Control.h"
#include "Status.h"
#include "Data.h"
#include "OptimizedTiming.h"
#include "../Common/Config.h"
#include <RingBuf.h>

namespace DeviceBridge::Parallel
{
  class Port
  {
  private:
    Control _control;
    Status _status;
    Data _data;

    void handleInterrupt();               // Original ISR (deprecated)
    void handleInterruptOptimized();      // IEEE-1284 compliant ISR
    
    RingBuf<uint8_t, DeviceBridge::Common::Buffer::RING_BUFFER_SIZE> _buffer;

    const byte _whichIsr;
    static byte _isrSeed;
    static void isr0();
    static Port *_instance0;
    static void isr1();
    static Port *_instance1;
    static void isr2();
    static Port *_instance2;
    
    // Debug counters
    volatile uint32_t _interruptCount;
    volatile uint32_t _dataCount;
    
    // Printer protocol state
    volatile bool _locked;
    
    // Critical buffer management
    volatile bool _criticalFlowControl;
    volatile uint32_t _criticalStartTime;
    static const uint32_t CRITICAL_TIMEOUT_MS = DeviceBridge::Common::Buffer::CRITICAL_TIMEOUT_MS; // 20 seconds
    
    // Deferred processing for minimal ISR
    volatile bool _pendingAck;
    volatile bool _pendingFlowControl;
    volatile uint8_t _lastFlowControlLevel;
    
  public:
    Port(
        Control control,
        Status status,
        Data data);

    void initialize();
    void initializeOptimized();           // Initialize with cached timing values
    bool hasData();
    bool isAlmostFull();    // 60% threshold - moderate flow control
    bool isCriticallyFull(); // 80% threshold - extended flow control  
    uint16_t getBufferCapacity() const { return DeviceBridge::Common::Buffer::RING_BUFFER_SIZE; }
    uint16_t getBufferFreeSpace() const;
    bool isFull();
    uint16_t readData(uint8_t buffer[], uint16_t index = 0, uint16_t length = 0);
    
    // Printer protocol methods
    void setBusy(bool busy);
    void setError(bool error);  
    void setPaperOut(bool paperOut);
    void setSelect(bool select);
    void sendAcknowledge();
    bool isLocked() const { return _locked; }
    void lock() { _locked = true; setBusy(true); }
    void unlock() { _locked = false; setBusy(false); }
    
    // Debug methods
    uint32_t getInterruptCount() const { return _interruptCount; }
    uint32_t getDataCount() const { return _dataCount; }
    
    // Deferred processing for optimized ISR
    void processPendingOperations();
    
    // Control signal debugging
    bool isStrobeLow() { return _control.isStrobeLow(); }
    bool isAutoFeedLow() { return _control.isAutoFeedLow(); }
    bool isInitializeLow() { return _control.isInitializeLow(); }
    bool isSelectInLow() { return _control.isSelectInLow(); }
    
    // Buffer management methods
    void clearBuffer();
    uint16_t getBufferSize() const;
    
    // Critical buffer state management
    bool isCriticalFlowControlActive() const { return _criticalFlowControl; }
    bool checkCriticalTimeout() const;
    void resetCriticalState();
  };

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
