#pragma once

#include <stdint.h>

namespace DeviceBridge::Parallel
{
  class Data
  {
  private:
    uint8_t _data[8]; 
    
  public:
    Data(
        uint8_t data0,
        uint8_t data1,
        uint8_t data2,
        uint8_t data3,
        uint8_t data4,
        uint8_t data5,
        uint8_t data6,
        uint8_t data7);

    void initialize();
    uint8_t readValue();
  };
}