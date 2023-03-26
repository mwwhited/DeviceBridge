#include <stdint.h>
#include <Arduino.h>
#include "PrinterInterface.h"
#include "ControlInterface.h"
#include "StatusInterface.h"
#include "DataInterface.h"

namespace DeviceBridge
{
  // https://forum.arduino.cc/t/use-attachinterrupt-with-a-class-method-function/301108/8

  PrinterInterface::PrinterInterface(
      ControlInterface control,
      StatusInterface status,
      DataInterface data) : _control(control), _status(status), _data(data), _whichIsr(_isrSeed++)
  {
  }

  void PrinterInterface::handleInterrupt()
  {
    //TODO: should probaly have 2ms spin 

    // if interrupt fell but strobe is high then it was a glitch so ignore
    if (_control.getStrobeValue()) return;

    _status.setBusy();
    uint8_t value = _data.readValue();
    _status.setAck();

    //TODO: write value to ring buffer
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