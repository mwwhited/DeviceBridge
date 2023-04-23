#include <stdint.h>
#include <Arduino.h>
#include "Storage_SdCard.h"
#include "Storage_StorageInterface.h"

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