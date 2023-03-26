
#include <stdint.h>
#include <Arduino.h>

namespace DeviceBridge
{
  class StatusInterface
  {
  private:
    uint8_t _acknowledge, _busy, _paperOut, _selected, _error;

  public:
    StatusInterface(
        uint8_t acknowledge,
        uint8_t busy,
        uint8_t paperOut,
        uint8_t selected,
        uint8_t error)
    {
      _acknowledge = acknowledge;
      _busy = busy;
      _paperOut = paperOut;
      _selected = selected;
      _error = error;
    }

    void initialize()
    {
      pinMode(_error, OUTPUT);  // Error - normally high
      digitalWrite(_error, true);

      pinMode(_selected, OUTPUT);  // Select - normally high
      digitalWrite(_selected, true);

      pinMode(_paperOut, OUTPUT);  // Paper out - normally low
      digitalWrite(_paperOut, false);

      pinMode(_busy, OUTPUT);  // Busy - normally low
      digitalWrite(_busy, false);

      pinMode(_acknowledge, OUTPUT);  // Ack - normally high
      digitalWrite(_acknowledge, true);
    }
  };
}