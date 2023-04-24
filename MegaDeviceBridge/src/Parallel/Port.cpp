#include <stdint.h>
#include <Arduino.h>
#include "Port.h"
#include "Control.h"
#include "Status.h"
#include "Data.h"
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
                   _buffer()
  {
  }

  void Port::handleInterrupt()
  {
    // TODO: should probaly have 2ms spin

    // if interrupt fell but strobe is high then it was a glitch so ignore
    if (_control.getStrobeValue())
      return;

    // TODO: job status events
    // TODO: set/push timeout for lpt read
    // TODO: set read status, clear on timeout
    // TODO: is it possible to detect new write?

    // TODO: if buffer is full do error instead
    //  if (_buffer.isFull()){
    //    _status.setError();
    //  }

    _status.setBusy();
    uint8_t value = _data.readValue();
    _status.setAck();

    // Note: write value to ring buffer
    _buffer.push(value);
  }

  Port *Port::_instance0;
  void Port::isr0()
  {
    _instance0->handleInterrupt();
  }

  void Port::initialize()
  {
    _control.initialize();
    _status.initialize();
    _data.initialize();

    _instance0 = this;
    attachInterrupt(digitalPinToInterrupt(_control.getStrobePin()), isr0, FALLING); // Attach to pin interrupt
  }

  bool Port::hasData()
  {
    return !_buffer.isEmpty();
  }

  bool Port::isAlmostFull()
  {
    return _buffer.size() > (_buffer.maxSize() / 4 * 3);
  }

  bool Port::isFull()
  {
    return _buffer.isFull();
  }

  uint16_t Port::readData(uint8_t buffer[], uint16_t index = 0, uint16_t lenght = 0)
  {
    uint16_t size = sizeof(buffer);
    if (lenght == 0) // if length is 0 use input buffer size
      lenght = size;

    if ((lenght + index) > size) // if index with length is greater than buffer than scale to fit
      lenght = size - index;

    if (lenght > size) // still invalid
      return 0;

    uint16_t cnt = 0;
    for (; index < lenght; index++)
    {
      uint8_t element;
      if (_buffer.lockedPop(element))
      {
        buffer[index] = element;
        cnt++;
      }
      else
      {
        break; // read buffer is empty
      }
    }
    return cnt;
  }
}