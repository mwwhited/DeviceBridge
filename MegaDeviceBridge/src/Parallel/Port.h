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

    static void isr0();
    static Port *_instance0;
    
  public:
    Port(
        Control control,
        Status status,
        Data data);

    void initialize();
    bool hasData();
    bool isAlmostFull();
    bool isFull();
    uint16_t readData(uint8_t buffer[], uint16_t index = 0, uint16_t lenght = 0);
  };
}
