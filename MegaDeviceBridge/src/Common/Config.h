#pragma once

#include <stdint.h>

namespace DeviceBridge::Common {

// Loop-based Architecture Configuration (formerly FreeRTOS)
namespace RTOS {
  // Timing constants (in milliseconds) for cooperative multitasking
  constexpr uint32_t PARALLEL_PORT_POLL_MS = 1;
  constexpr uint32_t DISPLAY_UPDATE_MS = 100;
  constexpr uint32_t TIME_UPDATE_MS = 1000;
  constexpr uint32_t SYSTEM_MONITOR_MS = 5000;
  constexpr uint32_t FILE_TIMEOUT_MS = 2000;
  
  // Legacy constants for compatibility with existing tests
  constexpr uint8_t DATA_QUEUE_SIZE = 8;      // No longer used (direct callbacks)
  constexpr uint8_t DISPLAY_QUEUE_SIZE = 4;   // No longer used (direct calls)
  constexpr uint8_t COMMAND_QUEUE_SIZE = 4;   // No longer used (direct calls)
}

// System Limits and Thresholds
namespace Limits {
  constexpr uint16_t MAX_FILENAME_LENGTH = 32;
  constexpr uint16_t MAX_MESSAGE_LENGTH = 32;
  constexpr uint32_t MAX_FILE_SIZE = 16777216UL; // 16MB
  constexpr uint8_t QUEUE_WARNING_THRESHOLD = 6; // 75% of 8
  constexpr uint16_t BUFFER_WARNING_THRESHOLD = 384; // 75% of 512
}

// File System Configuration
namespace FileSystem {
  constexpr char DEFAULT_FILE_PREFIX[] = "capture";
  constexpr char DEFAULT_FILE_EXTENSION[] = ".bin";
  constexpr uint32_t SD_TIMEOUT_MS = 1000;
  constexpr uint32_t EEPROM_TIMEOUT_MS = 500;
  constexpr uint8_t MAX_RETRIES = 3;
}

// Display Configuration
namespace Display {
  constexpr uint8_t SCREEN_WIDTH = 16;
  constexpr uint8_t SCREEN_HEIGHT = 2;
  constexpr uint32_t MESSAGE_TIMEOUT_MS = 3000;
  constexpr uint32_t IDLE_TIME_MS = 10000; // Show time after 10s idle
}

// Serial Communication
namespace Serial {
  constexpr uint32_t BAUD_RATE = 115200;
  constexpr uint32_t TIMEOUT_MS = 1000;
  constexpr uint8_t BUFFER_SIZE = 64;
}

} // namespace DeviceBridge::Common


// #if defined(ARDUINO_AVR_MEGA2560)
// Mega2560 Pin Definitions
// Hardware Pin Assignments (from Pinouts.md)
namespace Pins {
  // LCD Shield pins
  constexpr uint8_t LCD_RESET = 8;
  constexpr uint8_t LCD_ENABLE = 9;
  constexpr uint8_t LCD_D4 = 4;
  constexpr uint8_t LCD_D5 = 5;
  constexpr uint8_t LCD_D6 = 6;
  constexpr uint8_t LCD_D7 = 7;

  // Storage pins
  constexpr uint8_t SD_CS = 10;
  constexpr uint8_t EEPROM_CS = 3;

  // Parallel port control pins
  constexpr uint8_t LPT_STROBE = 18;
  constexpr uint8_t LPT_AUTO_FEED = 22;
  constexpr uint8_t LPT_INITIALIZE = 26;
  constexpr uint8_t LPT_SELECT_IN = 28;

  // Parallel port status pins  
  constexpr uint8_t LPT_ACK = 41;
  constexpr uint8_t LPT_BUSY = 43;
  constexpr uint8_t LPT_PAPER_OUT = 45;
  constexpr uint8_t LPT_SELECT = 47;
  constexpr uint8_t LPT_ERROR = 24;

  // Parallel port data pins
  constexpr uint8_t LPT_D0 = 25;
  constexpr uint8_t LPT_D1 = 27;
  constexpr uint8_t LPT_D2 = 29;
  constexpr uint8_t LPT_D3 = 31;
  constexpr uint8_t LPT_D4 = 33;
  constexpr uint8_t LPT_D5 = 35;
  constexpr uint8_t LPT_D6 = 37;
  constexpr uint8_t LPT_D7 = 39;
}

// #elif defined(ARDUINO_ESP32_DEV)
// // ESP32 Dev Module Pin Definitions
// namespace Pins {
//   // LCD Shield pins (outputs)
//   constexpr uint8_t LCD_RESET = 5;    // Output (was 4, moved to free GPIO5)
//   constexpr uint8_t LCD_ENABLE = 19;  // Output (was 5, moved to GPIO19)
//   constexpr uint8_t LCD_D4 = 16;      // Output
//   constexpr uint8_t LCD_D5 = 17;      // Output
//   constexpr uint8_t LCD_D6 = 18;      // Output
//   constexpr uint8_t LCD_D7 = 21;      // Output (was free GPIO21)

//   // Storage pins (outputs)
//   constexpr uint8_t SD_CS = 22;       // Output (SPI CS)
//   constexpr uint8_t EEPROM_CS = 23;   // Output (SPI CS)

//   // Parallel port control pins (inputs from host)
//   constexpr uint8_t LPT_STROBE = 25;     // Input
//   constexpr uint8_t LPT_AUTO_FEED = 26;  // Input
//   constexpr uint8_t LPT_INITIALIZE = 27; // Input
//   constexpr uint8_t LPT_SELECT_IN = 32;  // Input (GPIO32 is input capable)

//   // Parallel port status pins (outputs to host)
//   constexpr uint8_t LPT_ACK = 2;        // Output (GPIO2)
//   constexpr uint8_t LPT_BUSY = 15;      // Output (GPIO15)
//   constexpr uint8_t LPT_PAPER_OUT = 33; // Output (GPIO33)
//   constexpr uint8_t LPT_SELECT = 14;    // Output (GPIO14)
//   constexpr uint8_t LPT_ERROR = 12;     // Output (GPIO12)

//   // Parallel port data pins (inputs from host)
//   constexpr uint8_t LPT_D0 = 34;  // Input-only ADC pin
//   constexpr uint8_t LPT_D1 = 35;  // Input-only ADC pin
//   constexpr uint8_t LPT_D2 = 36;  // Input-only ADC pin (VP)
//   constexpr uint8_t LPT_D3 = 39;  // Input-only ADC pin (VN)
//   constexpr uint8_t LPT_D4 = 0;   // Input (boot pin, caution)
//   constexpr uint8_t LPT_D5 = 4;   // Input (GPIO4, now free)
//   constexpr uint8_t LPT_D6 = 13;  // Input
//   constexpr uint8_t LPT_D7 = 21;  // Input (GPIO21 was LCD_D7, swapped to LPT_D7)
// }

// #elif defined(ARDUINO_BLUEPILL_F103C8)
// // STM32 Bluepill Pin Definitions
// namespace Pins {
//   // LCD Shield pins (outputs)
//   constexpr uint8_t LCD_RESET  = PA8;   // Output
//   constexpr uint8_t LCD_ENABLE = PA9;   // Output
//   constexpr uint8_t LCD_D4     = PA10;  // Output
//   constexpr uint8_t LCD_D5     = PA11;  // Output
//   constexpr uint8_t LCD_D6     = PA12;  // Output
//   constexpr uint8_t LCD_D7     = PA15;  // Output

//   // Storage pins (outputs)
//   constexpr uint8_t SD_CS      = PB2;   // Output (SPI2 CS for SD)
//   constexpr uint8_t EEPROM_CS  = PB0;   // Output (SPI1 CS for EEPROM)

//   // Parallel port control pins (inputs from host)
//   constexpr uint8_t LPT_STROBE     = PB10;  // Input
//   constexpr uint8_t LPT_AUTO_FEED  = PB11;  // Input
//   constexpr uint8_t LPT_INITIALIZE = PB12;  // Input
//   constexpr uint8_t LPT_SELECT_IN  = PB13;  // Input

//   // Parallel port status pins (outputs to host)
//   constexpr uint8_t LPT_ACK        = PB14;  // Output
//   constexpr uint8_t LPT_BUSY       = PB15;  // Output
//   constexpr uint8_t LPT_PAPER_OUT  = PA0;   // Output
//   constexpr uint8_t LPT_SELECT     = PA1;   // Output
//   constexpr uint8_t LPT_ERROR      = PA2;   // Output

//   // Parallel port data pins (inputs from host)
//   constexpr uint8_t LPT_D0 = PA3;   // Input
//   constexpr uint8_t LPT_D1 = PA4;   // Input
//   constexpr uint8_t LPT_D2 = PA5;   // Input
//   constexpr uint8_t LPT_D3 = PA6;   // Input
//   constexpr uint8_t LPT_D4 = PA7;   // Input
//   constexpr uint8_t LPT_D5 = PB8;   // Input
//   constexpr uint8_t LPT_D6 = PB9;   // Input
//   constexpr uint8_t LPT_D7 = PB7;   // Input
// }

// #else
// #error "Unsupported board - define pin mapping"
// #endif
