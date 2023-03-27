#pragma once

#include <stdint.h>
#include <LiquidCrystal.h>

namespace DeviceBridge::User
{
    class Display
    {
    private:
        LiquidCrystal _lcd;

    public:
        Display(
            uint8_t reset,
            uint8_t enable,
            uint8_t data0,
            uint8_t data1,
            uint8_t data2,
            uint8_t data3);
        void initialize();
    };
}