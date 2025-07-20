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
                            _buffer(),
                            _whichIsr(_isrSeed++),
                            _interruptCount(0),
                            _dataCount(0),
                            _locked(false)
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
    
    // TDS2024 strobe pulses are very fast - on FALLING edge, capture data immediately
    // Don't check strobe state as it returns to HIGH before we can read it
    
    // Count valid data captures
    _dataCount++;
    
    // Set busy to indicate we're processing
    _status.setBusy();
    
    // Read the data byte from parallel port
    uint8_t value = _data.readValue();
    
    // Store data in ring buffer for processing
    _buffer.push(value);
    
    // Send acknowledge pulse to confirm data received
    sendAcknowledge();
    
    // Check if buffer is getting full and set busy accordingly
    if (isAlmostFull()) {
      setBusy(true);  // Hold busy high to slow down sender
    } else {
      setBusy(false); // Clear busy when buffer has space
    }
  }

  byte Port::_isrSeed = 0;
  Port *Port::_instance0;
  void Port::isr0()
  {
    _instance0->handleInterrupt();
  }
  Port *Port::_instance1;
  void Port::isr1()
  {
    _instance1->handleInterrupt();
  }
  Port *Port::_instance2;
  void Port::isr2()
  {
    _instance2->handleInterrupt();
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

  uint16_t Port::readData(uint8_t buffer[], uint16_t index, uint16_t length)
  {
    // Note: sizeof(buffer) gives size of pointer, not array
    // For Arduino, we'll use the length parameter properly
    if (length == 0) // if length is 0, assume we want to fill the buffer
      length = 512; // Default chunk size

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