#pragma once

#include <stdint.h>

namespace DeviceBridge
{
  class ControlInterface
  {
  private:
    uint8_t _strobe, _autoFeed, _initialize, _select;

  public:
    ControlInterface(
        uint8_t strobe,
        uint8_t autoFeed,
        uint8_t initialize,
        uint8_t select);

    void initialize();
    uint8_t getStrobe();
  };
}
