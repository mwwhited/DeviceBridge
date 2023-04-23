#pragma once

#include <stdint.h>
#include "StorageInterface.h"

#include <common_types.h>
// #include <eefs_filesys.h>
// #include <eefstool_driver.h>
// #include <eefstool_opts.h>

namespace DeviceBridge::Storage
{
  class SpiEeprom : public StorageInterface
  {
  private:
    const uint8_t _chipSelect;

  public:
    SpiEeprom(uint8_t chipSelect);
    virtual void initialize() override;
  };
}