
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
        _lcd.print(F("Device Bridge"));
        _lcd.setCursor(0, 1);
        _lcd.print(F("Initializing..."));
    }

    void Display::updateStatus(const char* message)
    {
        _lcd.clear();
        _lcd.setCursor(0, 0);
        _lcd.print(F("Device Bridge"));
        _lcd.setCursor(0, 1);
        _lcd.print(message);
    }

    void Display::showTime(const char* timeStr)
    {
        _lcd.clear();
        _lcd.setCursor(0, 0);
        _lcd.print(F("Device Bridge"));
        _lcd.setCursor(0, 1);
        _lcd.print(timeStr);
    }

    void Display::showMenu(const char* line1, const char* line2)
    {
        _lcd.clear();
        _lcd.setCursor(0, 0);
        _lcd.print(line1);
        _lcd.setCursor(0, 1);
        _lcd.print(line2);
    }
    
    void Display::clear()
    {
        _lcd.clear();
    }
    
    void Display::setCursor(uint8_t col, uint8_t row)
    {
        _lcd.setCursor(col, row);
    }
    
    void Display::print(const char* text)
    {
        _lcd.print(text);
    }
    
    void Display::print(const __FlashStringHelper* text)
    {
        _lcd.print(text);
    }
}