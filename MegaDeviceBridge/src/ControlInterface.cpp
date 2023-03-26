
#include <stdint.h>
#include <Arduino.h>
#include "PrinterInterface.h"

namespace DeviceBridge
{
    class ControlInterface
    {
    private:
        uint8_t _strobe, _autoFeed, _initialize, _select;

    public:
        ControlInterface(
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

        void initialize()
        {         
            pinMode(_strobe, INPUT_PULLUP);  // Strobe - normally high

            pinMode(_autoFeed, INPUT_PULLUP);
            pinMode(_initialize, INPUT_PULLUP);
            pinMode(_select, INPUT_PULLUP);
        }

        uint8_t getStrobe() {
            return _strobe;
        }
    };
}