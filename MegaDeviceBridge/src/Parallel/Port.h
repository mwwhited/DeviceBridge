#pragma once

#include <stdint.h>
#include "Control.h"
#include "Status.h"
#include "Data.h"
#include <RingBuf.h>

namespace DeviceBridge::Parallel
{
  class Port
  {
  private:
    Control _control;
    Status _status;
    Data _data;

    void handleInterrupt();
    
    RingBuf<uint8_t, 512> _buffer;

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
    
  public:
    Port(
        Control control,
        Status status,
        Data data);

    void initialize();
    bool hasData();
    bool isAlmostFull();
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
