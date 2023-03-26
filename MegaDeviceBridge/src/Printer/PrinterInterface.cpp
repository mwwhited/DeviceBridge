#include <stdint.h>
#include <Arduino.h>
#include "PrinterInterface.h"
#include "ControlInterface.h"
#include "StatusInterface.h"
#include "DataInterface.h"
#include <RingBuf.h>

namespace DeviceBridge::Printer
{
  // https://forum.arduino.cc/t/use-attachinterrupt-with-a-class-method-function/301108/8

  PrinterInterface::PrinterInterface(
      ControlInterface control,
      StatusInterface status,
      DataInterface data) : _control(control),
                            _status(status),
                            _data(data),
                            _whichIsr(_isrSeed++),
                            _buffer()
  {
  }

  void PrinterInterface::handleInterrupt()
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

  byte PrinterInterface::_isrSeed = 0;
  PrinterInterface *PrinterInterface::_instance0;
  void PrinterInterface::isr0()
  {
    _instance0->handleInterrupt();
  }
  PrinterInterface *PrinterInterface::_instance1;
  void PrinterInterface::isr1()
  {
    _instance1->handleInterrupt();
  }
  PrinterInterface *PrinterInterface::_instance2;
  void PrinterInterface::isr2()
  {
    _instance2->handleInterrupt();
  }

  void PrinterInterface::initialize()
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

  bool PrinterInterface::hasData()
  {
    return !_buffer.isEmpty();
  }

  bool PrinterInterface::isAlmostFull()
  {
    return _buffer.size() > (_buffer.maxSize() / 4 * 3);
  }

  bool PrinterInterface::isFull()
  {
    return _buffer.isFull();
  }

  uint16_t PrinterInterface::readData(uint8_t buffer[], uint16_t index = 0, uint16_t lenght = 0)
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