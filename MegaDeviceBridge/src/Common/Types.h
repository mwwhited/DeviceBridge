#pragma once

#include <stdint.h>
#include <Arduino.h>  // For PROGMEM and pgm_read_* functions
#include "Config.h"

namespace DeviceBridge::Common {

// PROGMEM string tables for memory optimization
// All enum string constants are stored in Flash memory instead of RAM

// StorageType PROGMEM strings
static const char storage_sd_card[] PROGMEM = "Storage: SD Card";
static const char storage_eeprom[] PROGMEM = "Storage: EEPROM";
static const char storage_serial_transfer[] PROGMEM = "Storage: Serial Transfer";
static const char storage_auto_select[] PROGMEM = "Storage: Auto Select";
static const char storage_unknown[] PROGMEM = "Storage: Unknown";

static const char storage_sd_card_simple[] PROGMEM = "SD Card";
static const char storage_eeprom_simple[] PROGMEM = "EEPROM";
static const char storage_serial_transfer_simple[] PROGMEM = "Serial Transfer";
static const char storage_auto_select_simple[] PROGMEM = "Auto Select";
static const char storage_unknown_simple[] PROGMEM = "Unknown";

// StorageType string pointer tables in PROGMEM
static const char* const storage_strings[] PROGMEM = {
    storage_sd_card,
    storage_eeprom,
    storage_serial_transfer,
    storage_auto_select,
    storage_unknown
};

static const char* const storage_strings_simple[] PROGMEM = {
    storage_sd_card_simple,
    storage_eeprom_simple,
    storage_serial_transfer_simple,
    storage_auto_select_simple,
    storage_unknown_simple
};

// FileType PROGMEM strings
static const char file_auto_detect[] PROGMEM = "Type: Auto Detect";
static const char file_binary[] PROGMEM = "Type: Binary";
static const char file_bmp[] PROGMEM = "Type: Bitmap";
static const char file_pcx[] PROGMEM = "Type: PCX";
static const char file_tiff[] PROGMEM = "Type: TIFF";
static const char file_rle[] PROGMEM = "Type: RLE";
static const char file_epsimage[] PROGMEM = "Type: EPS Image";
static const char file_dpu411[] PROGMEM = "Type: DPU411";
static const char file_dpu412[] PROGMEM = "Type: DPU412";
static const char file_dpu3445[] PROGMEM = "Type: DPU3445";
static const char file_thinkjet[] PROGMEM = "Type: ThinkJet";
static const char file_deskjet[] PROGMEM = "Type: DeskJet";
static const char file_laserjet[] PROGMEM = "Type: LaserJet";
static const char file_bubblejet[] PROGMEM = "Type: BubbleJet";
static const char file_epson_dot[] PROGMEM = "Type: Epson Dot Matrix";
static const char file_epson_c60[] PROGMEM = "Type: Epson C60";
static const char file_epson_c80[] PROGMEM = "Type: Epson C80";
static const char file_type_unknown[] PROGMEM = "Type: Unknown";

// FileType simple PROGMEM strings
static const char file_auto_detect_simple[] PROGMEM = "Auto Detect";
static const char file_binary_simple[] PROGMEM = "Binary";
static const char file_bmp_simple[] PROGMEM = "Bitmap";
static const char file_pcx_simple[] PROGMEM = "PCX";
static const char file_tiff_simple[] PROGMEM = "TIFF";
static const char file_rle_simple[] PROGMEM = "RLE";
static const char file_epsimage_simple[] PROGMEM = "EPS Image";
static const char file_dpu411_simple[] PROGMEM = "DPU411";
static const char file_dpu412_simple[] PROGMEM = "DPU412";
static const char file_dpu3445_simple[] PROGMEM = "DPU3445";
static const char file_thinkjet_simple[] PROGMEM = "ThinkJet";
static const char file_deskjet_simple[] PROGMEM = "DeskJet";
static const char file_laserjet_simple[] PROGMEM = "LaserJet";
static const char file_bubblejet_simple[] PROGMEM = "BubbleJet";
static const char file_epson_dot_simple[] PROGMEM = "Epson Dot Matrix";
static const char file_epson_c60_simple[] PROGMEM = "Epson C60";
static const char file_epson_c80_simple[] PROGMEM = "Epson C80";
static const char file_unknown_simple[] PROGMEM = "Unknown";

// FileType extension PROGMEM strings
static const char ext_bmp[] PROGMEM = ".bmp";
static const char ext_pcx[] PROGMEM = ".pcx";
static const char ext_tiff[] PROGMEM = ".tiff";
static const char ext_rle[] PROGMEM = ".rle";
static const char ext_eps[] PROGMEM = ".eps";
static const char ext_prn[] PROGMEM = ".prn";
static const char ext_pcl[] PROGMEM = ".pcl";

// FileType string pointer tables in PROGMEM
static const char* const filetype_strings[] PROGMEM = {
    file_auto_detect,    // AUTO_DETECT = 0
    file_bmp,            // BMP = 1
    file_pcx,            // PCX = 2
    file_tiff,           // TIFF = 3
    file_rle,            // RLE = 4
    file_epsimage,       // EPSIMAGE = 5
    file_dpu411,         // DPU411 = 6
    file_dpu412,         // DPU412 = 7
    file_dpu3445,        // DPU3445 = 8
    file_thinkjet,       // THINKJET = 9
    file_deskjet,        // DESKJET = 10
    file_laserjet,       // LASERJET = 11
    file_bubblejet,      // BUBBLEJET = 12
    file_epson_dot,      // EPSON_DOT = 13
    file_epson_c60,      // EPSON_C60 = 14
    file_epson_c80,      // EPSON_C80 = 15
    file_binary          // BINARY = 16
};

static const char* const filetype_strings_simple[] PROGMEM = {
    file_auto_detect_simple,    // AUTO_DETECT = 0
    file_bmp_simple,            // BMP = 1
    file_pcx_simple,            // PCX = 2
    file_tiff_simple,           // TIFF = 3
    file_rle_simple,            // RLE = 4
    file_epsimage_simple,       // EPSIMAGE = 5
    file_dpu411_simple,         // DPU411 = 6
    file_dpu412_simple,         // DPU412 = 7
    file_dpu3445_simple,        // DPU3445 = 8
    file_thinkjet_simple,       // THINKJET = 9
    file_deskjet_simple,        // DESKJET = 10
    file_laserjet_simple,       // LASERJET = 11
    file_bubblejet_simple,      // BUBBLEJET = 12
    file_epson_dot_simple,      // EPSON_DOT = 13
    file_epson_c60_simple,      // EPSON_C60 = 14
    file_epson_c80_simple,      // EPSON_C80 = 15
    file_binary_simple          // BINARY = 16
};

// Shared string buffer for PROGMEM reads (saves significant RAM)
static char progmem_string_buffer[32];

// Helper function to safely read string from PROGMEM
inline const char* getProgmemString(const char* const* table, uint8_t index, uint8_t max_index) {
    if (index > max_index) {
        index = max_index; // Use last entry as "Unknown"
    }
    strcpy_P(progmem_string_buffer, (char*)pgm_read_word(&table[index]));
    return progmem_string_buffer;
}

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

    // Get human-readable name (PROGMEM optimized)
    const char* toString() const {
        return getProgmemString(storage_strings, (uint8_t)value, 4);
    }

    const char* toSimple() const {
        return getProgmemString(storage_strings_simple, (uint8_t)value, 4);
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

    // Get human-readable name (PROGMEM optimized)
    const char* toString() const {
        return getProgmemString(filetype_strings, (uint8_t)value, 16);
    }
    
    // Get human-readable name (PROGMEM optimized)
    const char* toSimple() const {
        return getProgmemString(filetype_strings_simple, (uint8_t)value, 16);
    }

    // Get file extension (PROGMEM optimized for specific types, default for others)
    const char* getFileExtension() const {
        static char ext_buffer[8]; // Small buffer for extension strings
        
        switch (value) {
            case BMP:
                strcpy_P(ext_buffer, ext_bmp);
                return ext_buffer;
            case PCX:
                strcpy_P(ext_buffer, ext_pcx);
                return ext_buffer;
            case TIFF:
                strcpy_P(ext_buffer, ext_tiff);
                return ext_buffer;
            case RLE:
                strcpy_P(ext_buffer, ext_rle);
                return ext_buffer;
            case EPSIMAGE:
                strcpy_P(ext_buffer, ext_eps);
                return ext_buffer;

            case DPU411:
            case DPU412:
            case DPU3445:
            case BUBBLEJET:
            case EPSON_DOT:
            case EPSON_C60:
            case EPSON_C80:
                strcpy_P(ext_buffer, ext_prn);
                return ext_buffer;

            case THINKJET:
            case DESKJET:
            case LASERJET:
                strcpy_P(ext_buffer, ext_pcl);
                return ext_buffer;

            case AUTO_DETECT:
            case BINARY:      
            default:
                return DeviceBridge::Common::FileSystem::DEFAULT_FILE_EXTENSION;
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