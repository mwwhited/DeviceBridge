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
  constexpr uint16_t BUFFER_WARNING_THRESHOLD = 384; // 75% of 512 - calculated dynamically in flow control
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

// Debug Configuration
namespace Debug {
  constexpr uint8_t HEADER_HEX_BYTES = 10;     // Number of bytes to show in hex dump for new files
}

// Hardware Pin Assignments (from Pinouts.md)
namespace Pins {  
  constexpr uint8_t HEARTBEAT = 13;

  // LCD Shield pins
  constexpr uint8_t LCD_RESET = 8;
  constexpr uint8_t LCD_ENABLE = 9;
  constexpr uint8_t LCD_D4 = 4;
  constexpr uint8_t LCD_D5 = 5;
  constexpr uint8_t LCD_D6 = 6;
  constexpr uint8_t LCD_D7 = 7;
  constexpr uint8_t LCD_BUTTONS = A0;  // OSEPP analog button input

  // Storage pins
  constexpr uint8_t SD_CS = 10;
  constexpr uint8_t EEPROM_CS = 3;
  constexpr uint8_t SD_CD = 36;         // Card Detect (active LOW)
  constexpr uint8_t SD_WP = 34;         // Write Protect (active HIGH)
  
  // Status LEDs
  constexpr uint8_t LPT_READ_LED = 30;  // L1 - Parallel port read activity
  constexpr uint8_t DATA_WRITE_LED = 32; // L2 - File write activity

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

// Timing Configuration (Microseconds and Milliseconds)
namespace Timing {
  // Main loop intervals (milliseconds)
  constexpr unsigned long PARALLEL_INTERVAL = 1;      // 1ms for real-time data capture
  constexpr unsigned long FILESYSTEM_INTERVAL = 10;   // 10ms for file operations
  constexpr unsigned long DISPLAY_INTERVAL = 100;     // 100ms for display updates
  constexpr unsigned long TIME_INTERVAL = 1000;       // 1s for time operations
  constexpr unsigned long SYSTEM_INTERVAL = 5000;     // 5s for system monitoring
  constexpr unsigned long HEARTBEAT_INTERVAL = 500;   // 500ms for blink heartbeat LED
  constexpr unsigned long CONFIGURATION_INTERVAL = 50; // 50ms for configuration/serial commands
  
  // Microsecond delays for hardware timing - OPTIMIZED FOR TDS2024
  constexpr uint16_t ACK_PULSE_US = 20;               // Extended pulse for TDS2024 recognition (was 15μs)
  constexpr uint16_t RECOVERY_DELAY_US = 2;           // Brief recovery time
  constexpr uint16_t HARDWARE_DELAY_US = 5;           // TDS2024 data stability delay (was 3μs)
  constexpr uint16_t TDS2024_TIMING_US = 2;           // Brief delay for TDS2024 timing stability
  constexpr uint16_t FLOW_CONTROL_DELAY_US = 5;       // Flow control timing
  constexpr uint16_t MODERATE_FLOW_DELAY_US = 25;     // Moderate delay to slow down sender
  constexpr uint16_t CRITICAL_FLOW_DELAY_US = 50;     // Extended delay in critical state
  
  // Millisecond delays for recovery operations
  constexpr uint16_t EMERGENCY_RECOVERY_MS = 100;     // Emergency recovery delay
  constexpr uint16_t GENERAL_DELAY_MS = 100;          // General operation delay
  constexpr uint16_t KEEP_BUSY_MS = 2000;             // Keep busy for operations
  constexpr uint16_t SHORT_DELAY_MS = 500;            // Short delay operations
}

// Buffer and Memory Configuration
namespace Buffer {
  constexpr uint16_t RING_BUFFER_SIZE = 512;          // Main parallel port ring buffer
  constexpr uint16_t DATA_CHUNK_SIZE = 512;           // Data chunk size (matches ring buffer for optimal transfer)
  constexpr uint16_t EEPROM_BUFFER_SIZE = 32;         // EEPROM write buffer (32 * 4 bytes = 128 bytes) - OPTIMIZED
  constexpr uint32_t CRITICAL_TIMEOUT_MS = 20000;     // 20 seconds emergency timeout
  constexpr uint32_t CHUNK_SEND_TIMEOUT_MS = 50;      // Send partial chunks after 50ms of data collection
  constexpr uint16_t MIN_CHUNK_SIZE = 64;             // Minimum chunk size to send (unless timeout or EOF)
  
  // Flow control thresholds (percentages as fractions) - OPTIMIZED FOR TDS2024
  constexpr uint8_t FLOW_CONTROL_50_PERCENT = 1;      // 1/2 = 50% moderate threshold (was 60%)
  constexpr uint8_t FLOW_CONTROL_50_DIVISOR = 2;
  constexpr uint8_t FLOW_CONTROL_70_PERCENT = 7;      // 7/10 = 70% critical threshold (was 80%)
  constexpr uint8_t FLOW_CONTROL_70_DIVISOR = 10;
  constexpr uint8_t FLOW_CONTROL_40_PERCENT = 2;      // 2/5 = 40% pre-warning threshold (NEW)
  constexpr uint8_t FLOW_CONTROL_40_DIVISOR = 5;
  // Legacy thresholds for compatibility
  constexpr uint8_t FLOW_CONTROL_60_PERCENT = 3;      // 3/5 = 60% threshold
  constexpr uint8_t FLOW_CONTROL_60_DIVISOR = 5;
  constexpr uint8_t FLOW_CONTROL_80_PERCENT = 4;      // 4/5 = 80% threshold  
  constexpr uint8_t FLOW_CONTROL_80_DIVISOR = 5;
}

// Button Configuration
namespace Buttons {
  // Analog threshold values for button detection
  constexpr uint16_t BUTTON_RIGHT_VALUE = 0;
  constexpr uint16_t BUTTON_UP_VALUE = 144;
  constexpr uint16_t BUTTON_DOWN_VALUE = 329;
  constexpr uint16_t BUTTON_LEFT_VALUE = 504;
  constexpr uint16_t BUTTON_SELECT_VALUE = 741;
  constexpr uint16_t BUTTON_NONE_VALUE = 1023;
  
  // Detection thresholds with tolerance
  constexpr uint16_t RIGHT_THRESHOLD = 50;
  constexpr uint16_t UP_THRESHOLD = 200;
  constexpr uint16_t DOWN_THRESHOLD = 400;
  constexpr uint16_t LEFT_THRESHOLD = 600;
  constexpr uint16_t SELECT_THRESHOLD = 800;
}

// File Format Detection Constants
namespace FileFormats {
  // BMP file signature
  constexpr uint8_t BMP_SIGNATURE_1 = 0x42;  // 'B'
  constexpr uint8_t BMP_SIGNATURE_2 = 0x4D;  // 'M'
  
  // PCX file signature
  constexpr uint8_t PCX_SIGNATURE = 0x0A;
  
  // TIFF file signatures (little-endian)
  constexpr uint8_t TIFF_LE_1 = 0x49;  // 'I'
  constexpr uint8_t TIFF_LE_2 = 0x49;  // 'I'
  constexpr uint8_t TIFF_LE_3 = 0x2A;
  constexpr uint8_t TIFF_LE_4 = 0x00;
  
  // TIFF file signatures (big-endian)
  constexpr uint8_t TIFF_BE_1 = 0x4D;  // 'M'
  constexpr uint8_t TIFF_BE_2 = 0x4D;  // 'M'
  constexpr uint8_t TIFF_BE_3 = 0x00;
  constexpr uint8_t TIFF_BE_4 = 0x2A;
  
  // PostScript file signature
  constexpr uint8_t PS_SIGNATURE_1 = 0x25;  // '%'
  constexpr uint8_t PS_SIGNATURE_2 = 0x21;  // '!'
  
  // ESC character for printer commands
  constexpr uint8_t ESC_CHARACTER = 0x1B;
}

// W25Q128 Flash Memory Configuration
namespace Flash {
  // Command constants
  constexpr uint8_t CMD_WRITE_ENABLE = 0x06;
  constexpr uint8_t CMD_WRITE_DISABLE = 0x04;
  constexpr uint8_t CMD_READ_STATUS = 0x05;
  constexpr uint8_t CMD_WRITE_STATUS = 0x01;
  constexpr uint8_t CMD_PAGE_PROGRAM = 0x02;
  constexpr uint8_t CMD_SECTOR_ERASE = 0x20;
  constexpr uint8_t CMD_BLOCK_ERASE_32K = 0x52;
  constexpr uint8_t CMD_BLOCK_ERASE_64K = 0xD8;
  constexpr uint8_t CMD_CHIP_ERASE = 0xC7;
  constexpr uint8_t CMD_READ_DATA = 0x03;
  constexpr uint8_t CMD_FAST_READ = 0x0B;
  constexpr uint8_t CMD_READ_JEDEC_ID = 0x9F;
  constexpr uint8_t CMD_POWER_DOWN = 0xB9;
  constexpr uint8_t CMD_RELEASE_POWER_DOWN = 0xAB;
  
  // Memory layout constants
  constexpr uint32_t PAGE_SIZE = 256;
  constexpr uint32_t SECTOR_SIZE = 4096;  // 4KB
  
  // JEDEC ID for W25Q128
  constexpr uint32_t W25Q128_JEDEC_ID = 0xEF4018;
}

// Display Refresh Configuration
namespace DisplayRefresh {
  constexpr uint32_t NORMAL_INTERVAL_MS = 100;        // Normal LCD refresh rate
  constexpr uint32_t STORAGE_INTERVAL_MS = 500;       // Throttled refresh during storage operations
  constexpr uint8_t LCD_WIDTH = 16;                   // LCD character width
  constexpr uint8_t LCD_HEIGHT = 2;                   // LCD character height
}

// Flow Control State Management - OPTIMIZED FOR TDS2024
namespace FlowControl {
  // Buffer level thresholds for adaptive flow control - TIGHTER THRESHOLDS
  constexpr uint8_t PRE_WARNING_THRESHOLD_PERCENT = 40;  // Early warning (NEW)
  constexpr uint8_t MODERATE_THRESHOLD_PERCENT = 50;     // Moderate flow control (was 60%)
  constexpr uint8_t CRITICAL_THRESHOLD_PERCENT = 70;     // Critical flow control (was 80%)
  constexpr uint8_t RECOVERY_THRESHOLD_PERCENT = 40;     // Recovery threshold (was 50%)
}

} // namespace DeviceBridge::Common
