#pragma once

#include <stdint.h>
#include "Config.h"

namespace DeviceBridge::Common {

// Data structure for parallel port to file manager communication
struct __attribute__((packed)) DataChunk {
  uint8_t data[Buffer::DATA_CHUNK_SIZE];  // Match ring buffer size for optimal data transfer
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
struct StorageType {
    enum Value {
      SD_CARD,
      EEPROM,
      SERIAL_TRANSFER,
      AUTO_SELECT
    };

    static const int8_t Count = AUTO_SELECT+1;  // Number of storage types

    Value value;

    // Constructor
    constexpr StorageType(Value v) : value(v) {}

    // Get human-readable name
    const char* toString() const {
        switch (value) {
            case SD_CARD:          return "Storage: SD Card";
            case EEPROM:           return "Storage: EEPROM";
            case SERIAL_TRANSFER:  return "Storage: Serial Transfer";
            case AUTO_SELECT:      return "Storage: Auto Select";
            default:               return "Storage: Unknown";
        }
    }

    const char* toSimple() const {
        switch (value) {
            case SD_CARD:          return "SD Card";
            case EEPROM:           return "EEPROM";
            case SERIAL_TRANSFER:  return "Serial Transfer";
            case AUTO_SELECT:      return "Auto Select";
            default:               return "Unknown";
        }
    }
};

// File types for detection/override (TDS2024 Compatible)
struct FileType {
    enum Value {
        AUTO_DETECT,    // Detect from data headers (recommended)
        BMP,            // Bitmap images (most common from TDS2024)
        PCX,            // PC Paintbrush format
        TIFF,           // Tagged Image File Format
        RLE,            // Run-Length Encoded images  
        EPSIMAGE,       // Encapsulated PostScript images
        DPU411,         // Printer format
        DPU412,         // Printer format
        DPU3445,        // Printer format
        THINKJET,       // HP ThinkJet printer
        DESKJET,        // HP DeskJet printer
        LASERJET,       // HP LaserJet printer
        BUBBLEJET,      // Canon Bubble Jet printer
        EPSON_DOT,      // Epson Dot Matrix
        EPSON_C60,      // Epson Color C60
        EPSON_C80,      // Epson Color C80
        BINARY,         // Raw data capture
    };

    static const int8_t Count = BINARY+1;  // Number of storage types

    Value value;

    // Constructor
    constexpr FileType(Value v) : value(v) {}

    // Get human-readable name
    const char* toString() const {
        switch (value) {
            case AUTO_DETECT: return "Type: Auto Detect";
            case BINARY:      return "Type: Binary";
            case BMP:         return "Type: Bitmap";
            case PCX:         return "Type: PCX";
            case TIFF:        return "Type: TIFF";
            case RLE:         return "Type: RLE";
            case EPSIMAGE:    return "Type: EPS Image";
            case DPU411:      return "Type: DPU411";
            case DPU412:      return "Type: DPU412";
            case DPU3445:     return "Type: DPU3445";
            case THINKJET:    return "Type: ThinkJet";
            case DESKJET:     return "Type: DeskJet";
            case LASERJET:    return "Type: LaserJet";
            case BUBBLEJET:   return "Type: BubbleJet";
            case EPSON_DOT:   return "Type: Epson Dot Matrix";
            case EPSON_C60:   return "Type: Epson C60";
            case EPSON_C80:   return "Type: Epson C80";
            default:          return "Type: Unknown";
        }
    }
    
    // Get human-readable name
    const char* toSimple() const {
        switch (value) {
            case AUTO_DETECT: return "Auto Detect";
            case BINARY:      return "Binary";
            case BMP:         return "Bitmap";
            case PCX:         return "PCX";
            case TIFF:        return "TIFF";
            case RLE:         return "RLE";
            case EPSIMAGE:    return "EPS Image";
            case DPU411:      return "DPU411";
            case DPU412:      return "DPU412";
            case DPU3445:     return "DPU3445";
            case THINKJET:    return "ThinkJet";
            case DESKJET:     return "DeskJet";
            case LASERJET:    return "LaserJet";
            case BUBBLEJET:   return "BubbleJet";
            case EPSON_DOT:   return "Epson Dot Matrix";
            case EPSON_C60:   return "Epson C60";
            case EPSON_C80:   return "Epson C80";
            default:          return "Unknown";
        }
    }

    // Get file extension
    const char* getFileExtension() const {
        switch (value) {
            case BMP:         return ".bmp";
            case PCX:         return ".pcx";
            case TIFF:        return ".tiff";
            case RLE:         return ".rle";
            case EPSIMAGE:    return ".eps";

            case DPU411:
            case DPU412:
            case DPU3445:
            case BUBBLEJET:
            case EPSON_DOT:
            case EPSON_C60:
            case EPSON_C80:   return ".prn";  // default printer format

            case THINKJET:
            case DESKJET:
            case LASERJET:   return ".pcl";  // default printer format

            case AUTO_DETECT:
            case BINARY:      
            default:          return DeviceBridge::Common::FileSystem::DEFAULT_FILE_EXTENSION;;
        }
    }
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