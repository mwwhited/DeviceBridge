#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include "../Common/Config.h"

namespace DeviceBridge::Components {

class W25Q128Manager {
private:
    SemaphoreHandle_t _spiMutex;
    uint8_t _csPin;
    bool _initialized;
    
    // W25Q128 Commands
    static constexpr uint8_t CMD_WRITE_ENABLE = 0x06;
    static constexpr uint8_t CMD_WRITE_DISABLE = 0x04;
    static constexpr uint8_t CMD_READ_STATUS1 = 0x05;
    static constexpr uint8_t CMD_READ_STATUS2 = 0x35;
    static constexpr uint8_t CMD_WRITE_STATUS = 0x01;
    static constexpr uint8_t CMD_PAGE_PROGRAM = 0x02;
    static constexpr uint8_t CMD_QUAD_PAGE_PROGRAM = 0x32;
    static constexpr uint8_t CMD_BLOCK_ERASE_64KB = 0xD8;
    static constexpr uint8_t CMD_BLOCK_ERASE_32KB = 0x52;
    static constexpr uint8_t CMD_SECTOR_ERASE_4KB = 0x20;
    static constexpr uint8_t CMD_CHIP_ERASE = 0xC7;
    static constexpr uint8_t CMD_ERASE_SUSPEND = 0x75;
    static constexpr uint8_t CMD_ERASE_RESUME = 0x7A;
    static constexpr uint8_t CMD_POWER_DOWN = 0xB9;
    static constexpr uint8_t CMD_HIGH_PERFORMANCE_MODE = 0xA3;
    static constexpr uint8_t CMD_CONTINUOUS_READ_MODE_RESET = 0xFF;
    static constexpr uint8_t CMD_RELEASE_POWERDOWN_ID = 0xAB;
    static constexpr uint8_t CMD_MANUFACTURER_ID = 0x90;
    static constexpr uint8_t CMD_JEDEC_ID = 0x9F;
    static constexpr uint8_t CMD_READ_DATA = 0x03;
    static constexpr uint8_t CMD_FAST_READ = 0x0B;
    static constexpr uint8_t CMD_READ_DUAL_OUTPUT = 0x3B;
    static constexpr uint8_t CMD_READ_DUAL_IO = 0xBB;
    static constexpr uint8_t CMD_READ_QUAD_OUTPUT = 0x6B;
    static constexpr uint8_t CMD_READ_QUAD_IO = 0xEB;
    static constexpr uint8_t CMD_WORD_READ = 0xE3;
    
    // Status register bits
    static constexpr uint8_t STATUS_BUSY = 0x01;
    static constexpr uint8_t STATUS_WEL = 0x02;
    
    // Flash specifications
    static constexpr uint32_t FLASH_SIZE = 16777216UL; // 16MB
    static constexpr uint32_t PAGE_SIZE = 256;
    static constexpr uint32_t SECTOR_SIZE = 4096; // 4KB
    static constexpr uint32_t BLOCK_32K_SIZE = 32768UL; // 32KB
    static constexpr uint32_t BLOCK_64K_SIZE = 65536UL; // 64KB
    
    // Helper functions
    void chipSelect(bool select);
    uint8_t readStatus();
    void waitForReady();
    void writeEnable();
    void writeDisable();
    uint32_t readJedecId();
    
public:
    W25Q128Manager(uint8_t csPin, SemaphoreHandle_t spiMutex);
    ~W25Q128Manager();
    
    // Lifecycle
    bool initialize();
    bool isInitialized() const { return _initialized; }
    
    // Basic operations
    bool readData(uint32_t address, uint8_t* buffer, uint32_t length);
    bool writePage(uint32_t address, const uint8_t* buffer, uint32_t length);
    bool eraseSector(uint32_t address);
    bool eraseBlock32K(uint32_t address);
    bool eraseBlock64K(uint32_t address);
    bool eraseChip();
    
    // Information
    uint32_t getSize() const { return FLASH_SIZE; }
    uint32_t getPageSize() const { return PAGE_SIZE; }
    uint32_t getSectorSize() const { return SECTOR_SIZE; }
    
    // Utility
    bool isAddressValid(uint32_t address) const { return address < FLASH_SIZE; }
    uint32_t getPageAddress(uint32_t address) const { return address & ~(PAGE_SIZE - 1); }
    uint32_t getSectorAddress(uint32_t address) const { return address & ~(SECTOR_SIZE - 1); }
};

} // namespace DeviceBridge::Components