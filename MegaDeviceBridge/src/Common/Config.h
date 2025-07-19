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
