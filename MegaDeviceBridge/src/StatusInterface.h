#pragma once

#include <stdint.h>

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
        uint8_t error);

    void initialize();
    void setBusy();
    void setAck();
  };
}
