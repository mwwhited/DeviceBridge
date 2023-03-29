#pragma once

#include <stdint.h>
#include "StorageInterface.h"

namespace DeviceBridge::Storage
{
  class SdCard : public StorageInterface
  {
  private:
    const uint8_t _chipSelect;

  public:
    SdCard(uint8_t chipSelect);
    virtual void initialize() override;
  };
}