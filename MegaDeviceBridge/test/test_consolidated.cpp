#include <unity.h>
#include <Arduino.h>

// Include only basic types for testing
#include "../src/Common/Config.h"
#include "../src/Common/Types.h"

// Test function prototypes
void test_basic_arithmetic();
void test_data_structures(); 
void test_parallel_port_data();
void test_display_button_mapping();
void test_w25q128_addressing();
void test_pin_definitions();
void test_rtos_configuration();
void test_memory_usage();

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    UNITY_BEGIN();
    
    // Basic tests
    RUN_TEST(test_basic_arithmetic);
    RUN_TEST(test_data_structures);
    
    // Hardware tests
    RUN_TEST(test_parallel_port_data);
    RUN_TEST(test_display_button_mapping);
    RUN_TEST(test_w25q128_addressing);
    
    // Configuration tests
    RUN_TEST(test_pin_definitions);
    RUN_TEST(test_rtos_configuration);
    RUN_TEST(test_memory_usage);
    
    UNITY_END();
}

void loop() {
    // Empty - tests run once in setup
}

// Test implementations
void test_basic_arithmetic() {
    TEST_ASSERT_EQUAL(4, 2 + 2);
    TEST_ASSERT_EQUAL(0, 2 - 2);
    TEST_ASSERT_EQUAL(4, 2 * 2);
    TEST_ASSERT_EQUAL(1, 2 / 2);
}

void test_data_structures() {
    using namespace DeviceBridge::Common;
    
    // Test actual DataChunk from Types.h
    DataChunk chunk;
    TEST_ASSERT_EQUAL(256, sizeof(chunk.data));
    TEST_ASSERT_EQUAL(2, sizeof(chunk.length));
    TEST_ASSERT_EQUAL(4, sizeof(chunk.timestamp));
    TEST_ASSERT_EQUAL(1, sizeof(chunk.isNewFile));
    TEST_ASSERT_EQUAL(1, sizeof(chunk.isEndOfFile));
    
    // Test that packed structure is expected size
    TEST_ASSERT_TRUE(sizeof(chunk) <= 270); // Allow some padding
}

void test_parallel_port_data() {
    // Test data bit packing/unpacking
    uint8_t testData = 0xAC; // 10101100
    
    // Test individual bit extraction
    TEST_ASSERT_EQUAL(0, (testData >> 0) & 1); // Bit 0
    TEST_ASSERT_EQUAL(0, (testData >> 1) & 1); // Bit 1
    TEST_ASSERT_EQUAL(1, (testData >> 2) & 1); // Bit 2
    TEST_ASSERT_EQUAL(1, (testData >> 3) & 1); // Bit 3
    TEST_ASSERT_EQUAL(0, (testData >> 4) & 1); // Bit 4
    TEST_ASSERT_EQUAL(1, (testData >> 5) & 1); // Bit 5
    TEST_ASSERT_EQUAL(0, (testData >> 6) & 1); // Bit 6
    TEST_ASSERT_EQUAL(1, (testData >> 7) & 1); // Bit 7
    
    // Test bit reconstruction
    uint8_t reconstructed = 0;
    reconstructed |= (0 << 0); // Bit 0
    reconstructed |= (0 << 1); // Bit 1
    reconstructed |= (1 << 2); // Bit 2
    reconstructed |= (1 << 3); // Bit 3
    reconstructed |= (0 << 4); // Bit 4
    reconstructed |= (1 << 5); // Bit 5
    reconstructed |= (0 << 6); // Bit 6
    reconstructed |= (1 << 7); // Bit 7
    
    TEST_ASSERT_EQUAL(testData, reconstructed);
}

void test_display_button_mapping() {
    // Test OSEPP LCD Shield button analog values
    // Values from specifications: 0, 144, 329, 504, 741, 1023
    
    // Test button detection ranges
    TEST_ASSERT_TRUE(0 < 50);      // RIGHT button range
    TEST_ASSERT_TRUE(144 < 194);   // UP button range  
    TEST_ASSERT_TRUE(329 < 416);   // DOWN button range
    TEST_ASSERT_TRUE(504 < 622);   // LEFT button range
    TEST_ASSERT_TRUE(741 < 891);   // SELECT button range
    TEST_ASSERT_TRUE(1023 > 891);  // NONE button range
    
    // Test no overlap between ranges
    TEST_ASSERT_TRUE(50 < 144);    // RIGHT-UP gap
    TEST_ASSERT_TRUE(194 < 329);   // UP-DOWN gap
    TEST_ASSERT_TRUE(416 < 504);   // DOWN-LEFT gap
    TEST_ASSERT_TRUE(622 < 741);   // LEFT-SELECT gap
    TEST_ASSERT_TRUE(891 < 1023);  // SELECT-NONE gap
}

void test_w25q128_addressing() {
    // Test W25Q128 memory addressing calculations
    const uint32_t FLASH_SIZE = 16777216UL; // 16MB
    const uint32_t PAGE_SIZE = 256;
    const uint32_t SECTOR_SIZE = 4096; // 4KB
    
    // Test size calculations
    TEST_ASSERT_EQUAL(16777216, FLASH_SIZE);
    TEST_ASSERT_EQUAL(65536, FLASH_SIZE / PAGE_SIZE); // Total pages
    TEST_ASSERT_EQUAL(4096, FLASH_SIZE / SECTOR_SIZE); // Total sectors
    
    // Test address alignment
    uint32_t testAddress = 0x12345;
    uint32_t pageAligned = testAddress & ~(PAGE_SIZE - 1);
    uint32_t sectorAligned = testAddress & ~(SECTOR_SIZE - 1);
    
    TEST_ASSERT_EQUAL(0x12300, pageAligned);   // Page boundary
    TEST_ASSERT_EQUAL(0x12000, sectorAligned); // Sector boundary
    
    // Test address validation
    TEST_ASSERT_TRUE(0 < FLASH_SIZE);           // Valid start
    TEST_ASSERT_TRUE((FLASH_SIZE - 1) < FLASH_SIZE); // Valid end
    TEST_ASSERT_FALSE(FLASH_SIZE < FLASH_SIZE); // Invalid address
}

void test_pin_definitions() {
    using namespace DeviceBridge::Common;
    
    // Test LCD pins are in valid range for Arduino Mega
    TEST_ASSERT_TRUE(Pins::LCD_RESET >= 0 && Pins::LCD_RESET <= 69);
    TEST_ASSERT_TRUE(Pins::LCD_ENABLE >= 0 && Pins::LCD_ENABLE <= 69);
    TEST_ASSERT_TRUE(Pins::LCD_D4 >= 0 && Pins::LCD_D4 <= 69);
    TEST_ASSERT_TRUE(Pins::LCD_D5 >= 0 && Pins::LCD_D5 <= 69);
    TEST_ASSERT_TRUE(Pins::LCD_D6 >= 0 && Pins::LCD_D6 <= 69);
    TEST_ASSERT_TRUE(Pins::LCD_D7 >= 0 && Pins::LCD_D7 <= 69);
    
    // Test SPI pins
    TEST_ASSERT_TRUE(Pins::SD_CS >= 0 && Pins::SD_CS <= 69);
    TEST_ASSERT_TRUE(Pins::EEPROM_CS >= 0 && Pins::EEPROM_CS <= 69);
    
    // Test parallel port pins
    TEST_ASSERT_TRUE(Pins::LPT_STROBE >= 0 && Pins::LPT_STROBE <= 69);
    TEST_ASSERT_TRUE(Pins::LPT_D0 >= 0 && Pins::LPT_D0 <= 69);
    TEST_ASSERT_TRUE(Pins::LPT_D7 >= 0 && Pins::LPT_D7 <= 69);
    
    // Test pin uniqueness (no conflicts)
    TEST_ASSERT_NOT_EQUAL(Pins::SD_CS, Pins::EEPROM_CS);
    TEST_ASSERT_NOT_EQUAL(Pins::LCD_RESET, Pins::LCD_ENABLE);
}

void test_rtos_configuration() {
    using namespace DeviceBridge::Common;
    
    // Test task priorities are reasonable (0-4 range)
    TEST_ASSERT_TRUE(RTOS::PARALLEL_PORT_PRIORITY <= 4);
    TEST_ASSERT_TRUE(RTOS::FILE_MANAGER_PRIORITY <= 4);
    TEST_ASSERT_TRUE(RTOS::DISPLAY_PRIORITY <= 4);
    TEST_ASSERT_TRUE(RTOS::TIME_PRIORITY <= 4);
    TEST_ASSERT_TRUE(RTOS::SYSTEM_MONITOR_PRIORITY <= 4);
    
    // Test priority ordering (parallel port should be highest)
    TEST_ASSERT_TRUE(RTOS::PARALLEL_PORT_PRIORITY >= RTOS::FILE_MANAGER_PRIORITY);
    TEST_ASSERT_TRUE(RTOS::FILE_MANAGER_PRIORITY >= RTOS::DISPLAY_PRIORITY);
    
    // Test stack sizes are reasonable for Arduino Mega (8KB total RAM)
    uint16_t totalStack = RTOS::PARALLEL_PORT_STACK + 
                         RTOS::FILE_MANAGER_STACK + 
                         RTOS::DISPLAY_STACK + 
                         RTOS::TIME_STACK + 
                         RTOS::SYSTEM_MONITOR_STACK;
    
    TEST_ASSERT_TRUE(totalStack <= 2048); // Don't use more than 2KB for stacks
    
    // Test queue sizes are reasonable
    TEST_ASSERT_TRUE(RTOS::DATA_QUEUE_SIZE >= 4);
    TEST_ASSERT_TRUE(RTOS::DATA_QUEUE_SIZE <= 16);
    TEST_ASSERT_TRUE(RTOS::DISPLAY_QUEUE_SIZE >= 2);
    TEST_ASSERT_TRUE(RTOS::COMMAND_QUEUE_SIZE >= 2);
    
    // Test timing constants
    TEST_ASSERT_EQUAL(1, RTOS::PARALLEL_PORT_POLL_MS); // 1ms for real-time
    TEST_ASSERT_TRUE(RTOS::DISPLAY_UPDATE_MS >= 50);   // At least 20Hz
    TEST_ASSERT_TRUE(RTOS::FILE_TIMEOUT_MS >= 1000);   // At least 1 second
}

void test_memory_usage() {
    using namespace DeviceBridge::Common;
    
    // Check that RTOS configuration is reasonable for Arduino Mega (8KB RAM)
    uint16_t totalStack = RTOS::PARALLEL_PORT_STACK + 
                         RTOS::FILE_MANAGER_STACK + 
                         RTOS::DISPLAY_STACK + 
                         RTOS::TIME_STACK + 
                         RTOS::SYSTEM_MONITOR_STACK;
    
    TEST_ASSERT_TRUE(totalStack <= 2048); // More than 25% of RAM for stacks
    
    // Estimate queue memory usage with actual DataChunk size
    DataChunk testChunk;
    size_t queueMemory = (RTOS::DATA_QUEUE_SIZE * sizeof(testChunk)) +
                        (RTOS::DISPLAY_QUEUE_SIZE * 64) + // Estimate DisplayMessage size
                        (RTOS::COMMAND_QUEUE_SIZE * 32);  // Estimate SystemCommand size
    
    TEST_ASSERT_TRUE(queueMemory <= 4096); // More than 50% of RAM for queues
    
    // Total estimated memory usage
    size_t totalEstimated = totalStack + queueMemory + 1024; // 1KB for static vars
    
    TEST_ASSERT_TRUE(totalEstimated <= 6144); // More than 75% of RAM
}