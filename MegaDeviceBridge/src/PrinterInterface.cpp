
#include <stdint.h>
#include <Arduino.h>
#include "ControlInterface.h"
#include "StatusInterface.h"
#include "DataInterface.h"

namespace DeviceBridge
{
  void strobeHandler()
  {
  }

  class PrinterInterface
  {
  private:
    ControlInterface _control;
    StatusInterface _status;
    DataInterface _data;

    void StrobeHandler()
    {
    }

  public:
    PrinterInterface(
        const ControlInterface &control,
        const StatusInterface &status,
        const DataInterface &data) : _control(control), _status(status), _data(data)
    {
    }

    void initialize()
    {
      _control.initialize();
      _status.initialize();
      _data.initialize();

      attachInterrupt(digitalPinToInterrupt(_control.getStrobe()), strobeHandler, FALLING); // Attach to pin interrupt
    }
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