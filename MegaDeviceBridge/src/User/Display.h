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
        void updateStatus(const char* message);
        void showTime(const char* timeStr);
        void showMenu(const char* line1, const char* line2);
        
        // LCD control methods
        void clear();
        void setCursor(uint8_t col, uint8_t row);
        void print(const char* text);
        void print(const __FlashStringHelper* text);
    };
}