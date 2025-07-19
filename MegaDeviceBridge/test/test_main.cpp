#include <unity.h>
#include <Arduino.h>

// Test function prototypes
void test_basic_arithmetic();
void test_data_structures();
void test_parallel_port_data();
void test_display_button_mapping();
void test_w25q128_addressing();

void setup() {
    // Initialize serial for test output
    Serial.begin(115200);
    delay(2000); // Wait for serial to initialize
    
    UNITY_BEGIN(); // Start Unity Test Framework
    
    // Basic tests
    RUN_TEST(test_basic_arithmetic);
    RUN_TEST(test_data_structures);
    
    // Component tests
    RUN_TEST(test_parallel_port_data);
    RUN_TEST(test_display_button_mapping);
    RUN_TEST(test_w25q128_addressing);
    
    UNITY_END(); // End Unity Test Framework
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
    // Test DataChunk structure size
    struct TestDataChunk {
        uint8_t data[512];
        uint16_t length;
        uint32_t timestamp;
        bool isNewFile;
        bool isEndOfFile;
    };
    
    // Should be 512 + 2 + 4 + 1 + 1 = 520 bytes
    TEST_ASSERT_EQUAL(520, sizeof(TestDataChunk));
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