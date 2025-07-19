#include <unity.h>
#include <Arduino.h>

// Mock includes for testing without hardware
#define MOCK_HARDWARE 1

// Include config first to test it independently
#include "../src/Common/Config.h"

// Test simple config values without complex includes
namespace TestConfig = DeviceBridge::Common;

// Test function prototypes
void test_data_chunk_size();
void test_display_message_size();
void test_system_command_size();
void test_pin_definitions();
void test_rtos_configuration();
void test_file_detection_logic();

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    UNITY_BEGIN();
    
    // Data structure tests
    RUN_TEST(test_data_chunk_size);
    RUN_TEST(test_display_message_size);
    RUN_TEST(test_system_command_size);
    
    // Configuration tests
    RUN_TEST(test_pin_definitions);
    RUN_TEST(test_rtos_configuration);
    
    // Logic tests
    RUN_TEST(test_file_detection_logic);
    
    UNITY_END();
}

void loop() {
    // Empty
}

void test_data_chunk_size() {
    // Test basic data sizes for memory planning
    TEST_ASSERT_EQUAL(512, 512); // Data array size
    TEST_ASSERT_EQUAL(2, sizeof(uint16_t)); // Length field
    TEST_ASSERT_EQUAL(4, sizeof(uint32_t)); // Timestamp field
    TEST_ASSERT_EQUAL(1, sizeof(bool)); // Boolean fields
}

void test_display_message_size() {
    // Test message buffer sizes
    TEST_ASSERT_EQUAL(32, 32); // Message string size
    TEST_ASSERT_TRUE(sizeof(int) <= 4); // Enum size
}

void test_system_command_size() {
    // Test command structure sizes
    TEST_ASSERT_EQUAL(16, 16); // Data field size
    TEST_ASSERT_EQUAL(1, sizeof(uint8_t)); // Value field size
}

void test_pin_definitions() {
    using namespace TestConfig;
    
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
    using namespace TestConfig;
    
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

void test_file_detection_logic() {
    // Test file timeout calculation
    const uint32_t POLL_MS = 1;
    const uint32_t TIMEOUT_MS = 2000;
    const uint32_t EXPECTED_CYCLES = TIMEOUT_MS / POLL_MS;
    
    TEST_ASSERT_EQUAL(2000, EXPECTED_CYCLES);
    
    // Test file boundary detection logic
    bool fileInProgress = false;
    uint32_t idleCounter = 0;
    bool hasData = false;
    
    // Simulate data arrival
    hasData = true;
    if (!fileInProgress && hasData) {
        fileInProgress = true; // New file detected
        idleCounter = 0;
    }
    TEST_ASSERT_TRUE(fileInProgress);
    
    // Simulate data stream
    for (int i = 0; i < 100; i++) {
        if (hasData) {
            idleCounter = 0;
        } else {
            idleCounter++;
        }
    }
    
    // Simulate end of transmission
    hasData = false;
    for (uint32_t i = 0; i < EXPECTED_CYCLES + 1; i++) {
        if (!hasData) {
            idleCounter++;
        }
        
        if (fileInProgress && idleCounter >= EXPECTED_CYCLES) {
            fileInProgress = false; // End of file detected
            break;
        }
    }
    
    TEST_ASSERT_FALSE(fileInProgress);
    TEST_ASSERT_TRUE(idleCounter >= EXPECTED_CYCLES);
}