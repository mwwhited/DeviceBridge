
#include <stdint.h>
#include <Arduino.h>
#include "ControlInterface.h"

namespace DeviceBridge::Printer
{
    ControlInterface::ControlInterface(
        uint8_t strobe,
        uint8_t autoFeed,
        uint8_t initialize,
        uint8_t select)
    {
        _strobe = strobe;
        _autoFeed = autoFeed;
        _initialize = initialize;
        _select = select;
    }

    void ControlInterface::initialize()
    {
        pinMode(_strobe, INPUT_PULLUP); // Strobe - normally high

        pinMode(_autoFeed, INPUT_PULLUP);
        pinMode(_initialize, INPUT_PULLUP);
        pinMode(_select, INPUT_PULLUP);
    }

    uint8_t ControlInterface::getStrobePin()
    {
        return _strobe;
    }
    
    int ControlInterface::getStrobeValue(){
        return digitalRead(_strobe);
    }
    
    uint8_t ControlInterface::readValue(){
        uint8_t val =   
            (digitalRead(_strobe) << 0)    | 
            (digitalRead(_autoFeed) << 1)  |
            (digitalRead(_initialize) << 2)| 
            (digitalRead(_select) << 3);
        return val;
    }
}