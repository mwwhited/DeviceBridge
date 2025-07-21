#pragma once

#include <stdint.h>

namespace DeviceBridge::Parallel
{
  class Data
  {
  private:
    uint8_t _data[8]; 
    
    // Cached port configuration for atomic reading
    volatile uint8_t* _portRegister;
    uint8_t _portMask;
    uint8_t _portOffset;
    
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
    uint8_t readValue();            // Original non-atomic method (deprecated)
    uint8_t readValueAtomic();      // IEEE-1284 compliant atomic read
    
  private:
    void cachePortConfiguration();  // Cache port register info for atomic access
  };
}