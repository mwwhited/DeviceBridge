#pragma once

#include <stdint.h>
#include "ControlInterface.h"
#include "StatusInterface.h"
#include "DataInterface.h"
#include <RingBuf.h>

namespace DeviceBridge
{
  class PrinterInterface
  {
  private:
    ControlInterface _control;
    StatusInterface _status;
    DataInterface _data;

    void handleInterrupt();
    
    RingBuf<uint8_t, 512> _buffer;

    const byte _whichIsr;
    static byte _isrSeed;
    static void isr0();
    static PrinterInterface *_instance0;
    static void isr1();
    static PrinterInterface *_instance1;
    static void isr2();
    static PrinterInterface *_instance2;
    
  public:
    PrinterInterface(
        ControlInterface control,
        StatusInterface status,
        DataInterface data);

    void initialize();
    bool hasData();
    bool isAlmostFull();
    bool isFull();
    uint16_t readData(uint8_t buffer[], uint16_t index = 0, uint16_t lenght = 0);
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
