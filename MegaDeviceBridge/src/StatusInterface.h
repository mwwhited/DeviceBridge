#ifndef __H_DeviceBridge_StatusInterface_
#define __H_DeviceBridge_StatusInterface_

#include <stdint.h>

namespace DeviceBridge
{
  class StatusInterface
  {
  public:
    StatusInterface(
        uint8_t acknowledge,
        uint8_t busy,
        uint8_t paperOut,
        uint8_t selected,
        uint8_t error);

    void initialize();
  };
}

#endif