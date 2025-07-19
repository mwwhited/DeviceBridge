#include "W25Q128Manager.h"

namespace DeviceBridge::Components {

W25Q128Manager::W25Q128Manager(uint8_t csPin, SemaphoreHandle_t spiMutex)
    : _spiMutex(spiMutex)
    , _csPin(csPin)
    , _initialized(false)
{
}

W25Q128Manager::~W25Q128Manager() {
    // Nothing to clean up
}

bool W25Q128Manager::initialize() {
    if (xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }
    
    // Configure CS pin
    pinMode(_csPin, OUTPUT);
    chipSelect(false); // Deselect initially
    
    // Initialize SPI (should already be done by main)
    SPI.begin();
    
    // Read JEDEC ID to verify chip presence
    uint32_t jedecId = readJedecId();
    
    xSemaphoreGive(_spiMutex);
    
    // W25Q128 JEDEC ID should be 0xEF4018 (Winbond, 128Mbit)
    if ((jedecId & 0xFFFFFF) == 0xEF4018) {
        _initialized = true;
        return true;
    }
    
    return false;
}

void W25Q128Manager::chipSelect(bool select) {
    digitalWrite(_csPin, select ? LOW : HIGH);
    if (select) {
        delayMicroseconds(1); // Small delay after CS assertion
    }
}

uint8_t W25Q128Manager::readStatus() {
    chipSelect(true);
    SPI.transfer(CMD_READ_STATUS1);
    uint8_t status = SPI.transfer(0x00);
    chipSelect(false);
    return status;
}

void W25Q128Manager::waitForReady() {
    while (readStatus() & STATUS_BUSY) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void W25Q128Manager::writeEnable() {
    chipSelect(true);
    SPI.transfer(CMD_WRITE_ENABLE);
    chipSelect(false);
}

void W25Q128Manager::writeDisable() {
    chipSelect(true);
    SPI.transfer(CMD_WRITE_DISABLE);
    chipSelect(false);
}

uint32_t W25Q128Manager::readJedecId() {
    chipSelect(true);
    SPI.transfer(CMD_JEDEC_ID);
    uint32_t id = 0;
    id |= (uint32_t)SPI.transfer(0x00) << 16; // Manufacturer ID
    id |= (uint32_t)SPI.transfer(0x00) << 8;  // Device ID 1
    id |= (uint32_t)SPI.transfer(0x00);       // Device ID 2
    chipSelect(false);
    return id;
}

bool W25Q128Manager::readData(uint32_t address, uint8_t* buffer, uint32_t length) {
    if (!_initialized || !isAddressValid(address) || !buffer || length == 0) {
        return false;
    }
    
    if (address + length > FLASH_SIZE) {
        length = FLASH_SIZE - address; // Clamp to valid range
    }
    
    if (xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }
    
    waitForReady();
    
    chipSelect(true);
    SPI.transfer(CMD_READ_DATA);
    SPI.transfer((address >> 16) & 0xFF); // Address bits 23-16
    SPI.transfer((address >> 8) & 0xFF);  // Address bits 15-8
    SPI.transfer(address & 0xFF);         // Address bits 7-0
    
    for (uint32_t i = 0; i < length; i++) {
        buffer[i] = SPI.transfer(0x00);
    }
    
    chipSelect(false);
    xSemaphoreGive(_spiMutex);
    
    return true;
}

bool W25Q128Manager::writePage(uint32_t address, const uint8_t* buffer, uint32_t length) {
    if (!_initialized || !isAddressValid(address) || !buffer || length == 0) {
        return false;
    }
    
    // Ensure we don't cross page boundaries
    uint32_t pageOffset = address % PAGE_SIZE;
    if (pageOffset + length > PAGE_SIZE) {
        length = PAGE_SIZE - pageOffset;
    }
    
    if (xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }
    
    waitForReady();
    writeEnable();
    
    chipSelect(true);
    SPI.transfer(CMD_PAGE_PROGRAM);
    SPI.transfer((address >> 16) & 0xFF); // Address bits 23-16
    SPI.transfer((address >> 8) & 0xFF);  // Address bits 15-8
    SPI.transfer(address & 0xFF);         // Address bits 7-0
    
    for (uint32_t i = 0; i < length; i++) {
        SPI.transfer(buffer[i]);
    }
    
    chipSelect(false);
    waitForReady();
    
    xSemaphoreGive(_spiMutex);
    
    return true;
}

bool W25Q128Manager::eraseSector(uint32_t address) {
    if (!_initialized || !isAddressValid(address)) {
        return false;
    }
    
    // Align to sector boundary
    address = getSectorAddress(address);
    
    if (xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        return false;
    }
    
    waitForReady();
    writeEnable();
    
    chipSelect(true);
    SPI.transfer(CMD_SECTOR_ERASE_4KB);
    SPI.transfer((address >> 16) & 0xFF); // Address bits 23-16
    SPI.transfer((address >> 8) & 0xFF);  // Address bits 15-8
    SPI.transfer(address & 0xFF);         // Address bits 7-0
    chipSelect(false);
    
    waitForReady(); // Sector erase can take up to 400ms
    
    xSemaphoreGive(_spiMutex);
    
    return true;
}

bool W25Q128Manager::eraseBlock32K(uint32_t address) {
    if (!_initialized || !isAddressValid(address)) {
        return false;
    }
    
    // Align to 32K boundary
    address = address & ~(BLOCK_32K_SIZE - 1);
    
    if (xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(10000)) != pdTRUE) {
        return false;
    }
    
    waitForReady();
    writeEnable();
    
    chipSelect(true);
    SPI.transfer(CMD_BLOCK_ERASE_32KB);
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    chipSelect(false);
    
    waitForReady(); // Block erase can take up to 1.6s
    
    xSemaphoreGive(_spiMutex);
    
    return true;
}

bool W25Q128Manager::eraseBlock64K(uint32_t address) {
    if (!_initialized || !isAddressValid(address)) {
        return false;
    }
    
    // Align to 64K boundary
    address = address & ~(BLOCK_64K_SIZE - 1);
    
    if (xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(15000)) != pdTRUE) {
        return false;
    }
    
    waitForReady();
    writeEnable();
    
    chipSelect(true);
    SPI.transfer(CMD_BLOCK_ERASE_64KB);
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    chipSelect(false);
    
    waitForReady(); // Block erase can take up to 2s
    
    xSemaphoreGive(_spiMutex);
    
    return true;
}

bool W25Q128Manager::eraseChip() {
    if (!_initialized) {
        return false;
    }
    
    if (xSemaphoreTake(_spiMutex, pdMS_TO_TICKS(60000)) != pdTRUE) {
        return false;
    }
    
    waitForReady();
    writeEnable();
    
    chipSelect(true);
    SPI.transfer(CMD_CHIP_ERASE);
    chipSelect(false);
    
    waitForReady(); // Chip erase can take up to 50s
    
    xSemaphoreGive(_spiMutex);
    
    return true;
}

} // namespace DeviceBridge::Components