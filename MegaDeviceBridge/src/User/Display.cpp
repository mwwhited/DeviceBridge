
#include <stdint.h>
#include <LiquidCrystal.h>
#include "Display.h"

namespace DeviceBridge::User
{
    Display::Display(
        uint8_t reset,
        uint8_t enable,
        uint8_t data0,
        uint8_t data1,
        uint8_t data2,
        uint8_t data3) : _lcd(reset, enable, data0, data1, data2, data3)
    {
    }

    void Display::initialize()
    {
        _lcd.begin(16, 2);
        _lcd.clear();
    }
}