#pragma once

#include <stdint.h>

namespace DeviceBridge::Common {

// Data structure for parallel port to file manager communication
struct __attribute__((packed)) DataChunk {
  uint8_t data[256];  // Reduced from 512 to 256 bytes to fit in RAM
  uint16_t length;
  uint32_t timestamp;
  uint8_t isNewFile;    // Use uint8_t instead of bool for consistent size
  uint8_t isEndOfFile;  // Use uint8_t instead of bool for consistent size
};

// Display message types for user feedback
struct DisplayMessage {
  enum Type { 
    STATUS,     // General status updates
    ERROR,      // Error messages  
    INFO,       // Information messages
    TIME,       // Time display
    MENU        // Menu display
  } type;
  char message[32];
  char line2[32];  // Second line for menus
};

// System commands for configuration and control
struct SystemCommand {
  enum Type { 
    STORAGE_SELECT,   // Select storage type (SD/EEPROM/Serial)
    FILE_TYPE,        // Set file type override
    TRANSFER_MODE,    // Set transfer mode
    TIME_SET,         // Set RTC time
    CONFIG_SAVE       // Save configuration
  } type;
  uint8_t value;
  char data[16];    // Additional command data
};

// Storage types for file operations
enum class StorageType {
  SD_CARD,
  EEPROM,
  SERIAL_TRANSFER,
  AUTO_SELECT
};

// File types for detection/override (TDS2024 Compatible)
enum class FileType {
  AUTO_DETECT,    // Detect from data headers (recommended)
  BINARY,         // Raw data capture
  BMP,           // Bitmap images (most common from TDS2024)
  PCX,           // PC Paintbrush format
  TIFF,          // Tagged Image File Format
  RLE,           // Run-Length Encoded images  
  EPSIMAGE,      // Encapsulated PostScript images
  DPU411,        // Printer format
  DPU412,        // Printer format
  DPU3445,       // Printer format
  THINKJET,      // HP ThinkJet printer
  DESKJET,       // HP DeskJet printer
  LASERJET,      // HP LaserJet printer
  BUBBLEJET,     // Canon Bubble Jet printer
  EPSON_DOT,     // Epson Dot Matrix
  EPSON_C60,     // Epson Color C60
  EPSON_C80      // Epson Color C80
};

// System status enumeration
enum class SystemStatus {
  INITIALIZING,
  READY,
  RECEIVING,
  STORING,
  ERROR,
  FULL
};

// Error codes for system monitoring
enum class ErrorCode {
  NONE,
  SD_INIT_FAILED,
  EEPROM_INIT_FAILED,
  RTC_INIT_FAILED,
  BUFFER_OVERFLOW,
  FILE_WRITE_ERROR,
  STORAGE_FULL,
  HARDWARE_ERROR
};

} // namespace DeviceBridge::Common