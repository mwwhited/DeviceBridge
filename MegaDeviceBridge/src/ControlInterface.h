#ifndef __H_DeviceBridge_ControlInterface_
#define __H_DeviceBridge_ControlInterface_

#include <stdint.h>

namespace DeviceBridge
{
  class ControlInterface
  {
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
#endif