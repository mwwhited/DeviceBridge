#include <stdint.h>
#include <Arduino.h>
#include "SdCard.h"
#include "StorageInterface.h"

namespace DeviceBridge::Storage
{
    SdCard::SdCard(
        uint8_t chipSelect
    ):_chipSelect(chipSelect)
    {
    }

    void SdCard::initialize() 
    {
        pinMode(_chipSelect, OUTPUT);
    }
}