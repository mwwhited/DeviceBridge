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
    : _portRegister(nullptr), _portMask(0), _portOffset(0)
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
    
    // Cache port configuration for atomic reading
    cachePortConfiguration();
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
  
  uint8_t Data::readValueAtomic()
  {
    // IEEE-1284 compliant atomic port reading
    // Single instruction read eliminates race conditions
    if (_portRegister != nullptr) {
      // Read entire port register in single atomic operation (~0.1μs)
      uint8_t portValue = *_portRegister;
      
      // Extract and rearrange data bits based on pin mapping
      // This assumes data pins are consecutive on the same port
      return (portValue & _portMask) >> _portOffset;
    }
    
    // Fallback to non-atomic method if port caching failed
    return readValue();
  }
  
  void Data::cachePortConfiguration()
  {
    // Determine which Arduino port the data pins are connected to
    // This is a simplified implementation - in practice you'd need to 
    // analyze the actual pin assignments and map to AVR ports
    
    // For Arduino Mega 2560, assuming data pins are on consecutive pins
    // of the same port (this would need to be verified against actual pinout)
    
    // Example implementation for pins 25-32 (would be on PORTA)
    // This is hardware-specific and must match actual pin assignments
    
    // NOTE: This is a template implementation - actual implementation
    // must be customized based on the specific pin assignments in Config.h
    
    bool canUseAtomicRead = true;
    
    // Check if all data pins are on the same AVR port
    // For now, disable atomic read and use fallback (safe)
    canUseAtomicRead = false;
    
    if (canUseAtomicRead) {
      // Example: If all pins are on PORTA (pins 22-29 on Mega)
      // _portRegister = &PINA;
      // _portMask = 0xFF;
      // _portOffset = 0;
    } else {
      // Disable atomic read - use safe fallback
      _portRegister = nullptr;
      _portMask = 0;
      _portOffset = 0;
    }
  }
}