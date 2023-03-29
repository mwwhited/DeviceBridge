#include <stdint.h>
#include <Arduino.h>
#include "SpiEeprom.h"
#include "StorageInterface.h"

namespace DeviceBridge::Storage
{
    SpiEeprom::SpiEeprom(
        uint8_t chipSelect
    ):_chipSelect(chipSelect)
    {
    }

    void SpiEeprom::initialize()
    {
        pinMode(_chipSelect, OUTPUT);
    }
}