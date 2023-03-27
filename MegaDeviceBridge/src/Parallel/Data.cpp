#include <stdint.h>
#include <Arduino.h>
#include "Data.h"

namespace DeviceBridge::Parallel
{
  Data::Data(
      uint8_t data0,
      uint8_t data1,
      uint8_t data2,
      uint8_t data3,
      uint8_t data4,
      uint8_t data5,
      uint8_t data6,
      uint8_t data7)
  {
    _data[0] = data0;
    _data[1] = data1;
    _data[2] = data2;
    _data[3] = data3;
    _data[4] = data4;
    _data[5] = data5;
    _data[6] = data6;
    _data[7] = data7;
  }

  void Data::initialize()
  {
    pinMode(_data[0], INPUT_PULLUP); // D0
    pinMode(_data[1], INPUT_PULLUP); // D1
    pinMode(_data[2], INPUT_PULLUP); // D2
    pinMode(_data[3], INPUT_PULLUP); // D3
    pinMode(_data[4], INPUT_PULLUP); // D4
    pinMode(_data[5], INPUT_PULLUP); // D5
    pinMode(_data[6], INPUT_PULLUP); // D6
    pinMode(_data[7], INPUT_PULLUP); // D7
  }

  uint8_t Data::readValue()
  {
    return (digitalRead(_data[0]) << 0) |
           (digitalRead(_data[1]) << 1) |
           (digitalRead(_data[2]) << 2) |
           (digitalRead(_data[3]) << 3) |
           (digitalRead(_data[4]) << 4) |
           (digitalRead(_data[5]) << 5) |
           (digitalRead(_data[6]) << 6) |
           (digitalRead(_data[7]) << 7);
  }
}