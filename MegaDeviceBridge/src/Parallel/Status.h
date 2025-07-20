#pragma once

#include <stdint.h>

namespace DeviceBridge::Parallel
{
  class Status
  {
  private:
    uint8_t _acknowledge, _busy, _paperOut, _selected, _error;

  public:
    Status(
        uint8_t acknowledge,
        uint8_t busy,
        uint8_t paperOut,
        uint8_t selected,
        uint8_t error);

    void initialize();
    void setBusy();
    void setBusy(bool busy);
    void setAck();
    void sendAcknowledgePulse();
    void setError(bool error);
    void setPaperOut(bool paperOut);
    void setSelect(bool select);
  };
}
