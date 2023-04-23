#pragma once

#include <stdint.h>

namespace DeviceBridge::Parallel
{
  class Control
  {
  private:
    uint8_t _strobe, _autoFeed, _initialize, _select;

  public:
    Control(
        uint8_t strobe,
        uint8_t autoFeed,
        uint8_t initialize,
        uint8_t select);

    void initialize();
    uint8_t getStrobePin();
    int getStrobeValue();
    uint8_t readValue();
  };
}
