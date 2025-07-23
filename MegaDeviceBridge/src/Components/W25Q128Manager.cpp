#include "W25Q128Manager.h"
#include "../Common/ServiceLocator.h"
#include "../Common/ConfigurationService.h"
#include "SystemManager.h"

// Debug helper function
static bool isEEPROMDebugEnabled() {
    auto& services = DeviceBridge::ServiceLocator::getInstance();
    auto* systemManager = services.getSystemManager();
    return systemManager && systemManager->isEEPROMDebugEnabled();
}

// Debug macros for conditional output
#define W25Q128_DEBUG_PRINT(...) do { if (isEEPROMDebugEnabled()) { Serial.print(__VA_ARGS__); } } while(0)
#define W25Q128_DEBUG_PRINTLN(...) do { if (isEEPROMDebugEnabled()) { Serial.println(__VA_ARGS__); } } while(0)

namespace DeviceBridge::Components {

W25Q128Manager::W25Q128Manager(uint8_t csPin)
    : _csPin(csPin)
    , _initialized(false)
{
}

W25Q128Manager::~W25Q128Manager() {
    // Nothing to clean up
}

bool W25Q128Manager::initialize() {
    W25Q128_DEBUG_PRINTLN(F("W25Q128: Starting initialization..."));
    W25Q128_DEBUG_PRINT(F("W25Q128: CS pin: "));
    W25Q128_DEBUG_PRINTLN(_csPin);
    
    // Configure CS pin
    W25Q128_DEBUG_PRINTLN(F("W25Q128: Configuring CS pin as OUTPUT..."));
    pinMode(_csPin, OUTPUT);
    chipSelect(false); // Deselect initially
    W25Q128_DEBUG_PRINTLN(F("W25Q128: CS pin configured and deselected"));
    
    // Initialize SPI (should already be done by main)
    W25Q128_DEBUG_PRINTLN(F("W25Q128: Initializing SPI..."));
    SPI.begin();
    W25Q128_DEBUG_PRINTLN(F("W25Q128: SPI initialized"));
    
    // Small delay to ensure SPI is stable
    delay(10);
    W25Q128_DEBUG_PRINTLN(F("W25Q128: SPI stabilization delay complete"));
    
    // Read JEDEC ID to verify chip presence
    W25Q128_DEBUG_PRINTLN(F("W25Q128: Reading JEDEC ID to detect chip..."));
    uint32_t jedecId = readJedecId();
    
    W25Q128_DEBUG_PRINT(F("W25Q128: JEDEC ID read: 0x"));
    W25Q128_DEBUG_PRINT(jedecId, HEX);
    W25Q128_DEBUG_PRINTLN(F(" (raw 32-bit)"));
    
    W25Q128_DEBUG_PRINT(F("W25Q128: JEDEC ID masked: 0x"));
    W25Q128_DEBUG_PRINT(jedecId & 0xFFFFFF, HEX);
    W25Q128_DEBUG_PRINTLN(F(" (24-bit)"));
    
    W25Q128_DEBUG_PRINT(F("W25Q128: Expected JEDEC ID: 0x"));
    Serial.print(Common::Flash::W25Q128_JEDEC_ID, HEX);
    Serial.print(F("\r\n"));
    
    // Analyze individual bytes
    uint8_t manufacturerId = (jedecId >> 16) & 0xFF;
    uint8_t deviceId1 = (jedecId >> 8) & 0xFF;
    uint8_t deviceId2 = jedecId & 0xFF;
    
    Serial.print(F("W25Q128: Manufacturer ID: 0x"));
    Serial.print(manufacturerId, HEX);
    Serial.print(F(" (expected: 0xEF for Winbond)\r\n"));
    
    Serial.print(F("W25Q128: Device ID 1: 0x"));
    Serial.print(deviceId1, HEX);
    Serial.print(F(" (expected: 0x40)\r\n"));
    
    Serial.print(F("W25Q128: Device ID 2: 0x"));
    Serial.print(deviceId2, HEX);
    Serial.print(F(" (expected: 0x18)\r\n"));
    
    // W25Q128 JEDEC ID should be 0xEF4018 (Winbond, 128Mbit)
    if ((jedecId & 0xFFFFFF) == Common::Flash::W25Q128_JEDEC_ID) {
        W25Q128_DEBUG_PRINTLN(F("W25Q128: ✅ Chip identified successfully as W25Q128"));
        _initialized = true;
        return true;
    }
    
    W25Q128_DEBUG_PRINTLN(F("W25Q128: ❌ Chip not detected or JEDEC ID mismatch"));
    
    // Provide diagnostic information
    if (jedecId == 0x000000 || jedecId == 0xFFFFFF) {
        W25Q128_DEBUG_PRINTLN(F("W25Q128: ❌ No response from chip (likely not present or wrong CS pin)"));
    } else if (manufacturerId == 0xEF) {
        W25Q128_DEBUG_PRINTLN(F("W25Q128: ⚠️  Winbond chip detected but wrong capacity"));
    } else {
        W25Q128_DEBUG_PRINTLN(F("W25Q128: ⚠️  Different flash chip detected"));
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
        delay(1);
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
    
    // Note: No mutex needed in loop-based architecture
    
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
    // Note: No mutex needed in loop-based architecture
    
    return true;
}

bool W25Q128Manager::writePage(uint32_t address, const uint8_t* buffer, uint32_t length) {
    if (!_initialized) {
        Serial.print(F("W25Q128: ❌ Write failed - not initialized\r\n"));
        return false;
    }
    if (!isAddressValid(address)) {
        Serial.print(F("W25Q128: ❌ Write failed - invalid address 0x"));
        Serial.print(address, HEX);
        Serial.print(F("\r\n"));
        return false;
    }
    if (!buffer || length == 0) {
        Serial.print(F("W25Q128: ❌ Write failed - invalid buffer or length\r\n"));
        return false;
    }
    
    // Ensure we don't cross page boundaries
    uint32_t pageOffset = address % PAGE_SIZE;
    if (pageOffset + length > PAGE_SIZE) {
        length = PAGE_SIZE - pageOffset;
        Serial.print(F("W25Q128: ⚠️  Length adjusted to avoid page boundary crossing\r\n"));
    }
    
    // Note: No mutex needed in loop-based architecture
    
    waitForReady();
    writeEnable();
    
    // Verify write enable was successful
    uint8_t status = readStatus();
    if (!(status & STATUS_WEL)) {
        return false;
    }
    
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
    
    // Verify write was successful by checking status
    status = readStatus();
    if (status & STATUS_WEL) {
        return false;
    }
    
    // Note: No mutex needed in loop-based architecture
    
    return true;
}

bool W25Q128Manager::eraseSector(uint32_t address) {
    if (!_initialized) {
        Serial.print(F("W25Q128: ❌ Erase failed - not initialized\r\n"));
        return false;
    }
    if (!isAddressValid(address)) {
        Serial.print(F("W25Q128: ❌ Erase failed - invalid address 0x"));
        Serial.print(address, HEX);
        Serial.print(F("\r\n"));
        return false;
    }
    
    // Align to sector boundary
    uint32_t originalAddress = address;
    address = getSectorAddress(address);
    
    // Minimal logging to prevent lockups
    
    // Note: No mutex needed in loop-based architecture
    
    waitForReady();
    writeEnable();
    
    // Verify write enable was successful
    uint8_t status = readStatus();
    if (!(status & STATUS_WEL)) {
        return false;
    }
    
    chipSelect(true);
    SPI.transfer(CMD_SECTOR_ERASE_4KB);
    SPI.transfer((address >> 16) & 0xFF); // Address bits 23-16
    SPI.transfer((address >> 8) & 0xFF);  // Address bits 15-8
    SPI.transfer(address & 0xFF);         // Address bits 7-0
    chipSelect(false);
    
    waitForReady(); // Sector erase can take up to 400ms
    
    // Verify erase was successful
    status = readStatus();
    if (status & STATUS_WEL) {
        return false;
    }
    
    // Note: No mutex needed in loop-based architecture
    
    return true;
}

bool W25Q128Manager::eraseBlock32K(uint32_t address) {
    if (!_initialized || !isAddressValid(address)) {
        return false;
    }
    
    // Align to 32K boundary
    address = address & ~(BLOCK_32K_SIZE - 1);
    
    // Note: No mutex needed in loop-based architecture
    
    waitForReady();
    writeEnable();
    
    chipSelect(true);
    SPI.transfer(CMD_BLOCK_ERASE_32KB);
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    chipSelect(false);
    
    waitForReady(); // Block erase can take up to 1.6s
    
    // Note: No mutex needed in loop-based architecture
    
    return true;
}

bool W25Q128Manager::eraseBlock64K(uint32_t address) {
    if (!_initialized || !isAddressValid(address)) {
        return false;
    }
    
    // Align to 64K boundary
    address = address & ~(BLOCK_64K_SIZE - 1);
    
    // Note: No mutex needed in loop-based architecture
    
    waitForReady();
    writeEnable();
    
    chipSelect(true);
    SPI.transfer(CMD_BLOCK_ERASE_64KB);
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    chipSelect(false);
    
    waitForReady(); // Block erase can take up to 2s
    
    // Note: No mutex needed in loop-based architecture
    
    return true;
}

bool W25Q128Manager::eraseChip() {
    if (!_initialized) {
        return false;
    }
    
    // Note: No mutex needed in loop-based architecture
    
    waitForReady();
    writeEnable();
    
    chipSelect(true);
    SPI.transfer(CMD_CHIP_ERASE);
    chipSelect(false);
    
    waitForReady(); // Chip erase can take up to 50s
    
    // Note: No mutex needed in loop-based architecture
    
    return true;
}

} // namespace DeviceBridge::Components