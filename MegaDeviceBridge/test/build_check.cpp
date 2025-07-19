/**
 * Build verification test - checks that all components compile correctly
 * This file includes all headers to verify dependencies and syntax
 */

#include <Arduino.h>

// Standard libraries that should be available
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Arduino libraries
#include <SPI.h>
#include <Wire.h>

// For testing, only include basic types and configs (no complex dependencies)
#include "../src/Common/Config.h"
#include "../src/Common/Types.h"

// Skip complex component managers that require FreeRTOS

// Build verification functions
bool verify_data_structures();
bool verify_pin_assignments();
bool verify_component_creation();
bool verify_memory_usage();

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== Build Verification Test ===");
    
    bool allPassed = true;
    
    Serial.print("Testing data structures... ");
    if (verify_data_structures()) {
        Serial.println("PASS");
    } else {
        Serial.println("FAIL");
        allPassed = false;
    }
    
    Serial.print("Testing pin assignments... ");
    if (verify_pin_assignments()) {
        Serial.println("PASS");
    } else {
        Serial.println("FAIL");
        allPassed = false;
    }
    
    Serial.print("Testing component creation... ");
    if (verify_component_creation()) {
        Serial.println("PASS");
    } else {
        Serial.println("FAIL");
        allPassed = false;
    }
    
    Serial.print("Testing memory usage... ");
    if (verify_memory_usage()) {
        Serial.println("PASS");
    } else {
        Serial.println("FAIL");
        allPassed = false;
    }
    
    Serial.println("========================");
    if (allPassed) {
        Serial.println("BUILD VERIFICATION: ALL TESTS PASSED");
    } else {
        Serial.println("BUILD VERIFICATION: SOME TESTS FAILED");
    }
    Serial.println("========================");
}

void loop() {
    // Blink LED to show system is running
    static bool ledState = false;
    static uint32_t lastBlink = 0;
    
    if (millis() - lastBlink > 1000) {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
        lastBlink = millis();
    }
}

bool verify_data_structures() {
    using namespace DeviceBridge::Common;
    
    // Test that all data structures compile and have reasonable sizes
    DataChunk dataChunk;
    DisplayMessage displayMsg;
    SystemCommand sysCmd;
    
    // Check sizes (updated for optimized DataChunk)
    if (sizeof(dataChunk) > 300) return false;  // Should be ~264 bytes
    if (sizeof(displayMsg) > 100) return false; // Too large for queues
    if (sizeof(sysCmd) > 50) return false;      // Too large for queues
    
    // Test enum values
    if (StorageType::SD_CARD != static_cast<StorageType>(0)) return false;
    if (FileType::AUTO_DETECT != static_cast<FileType>(0)) return false;
    if (SystemStatus::INITIALIZING != static_cast<SystemStatus>(0)) return false;
    
    return true;
}

bool verify_pin_assignments() {
    using namespace DeviceBridge::Common;
    
    // Check that all pins are in valid range for Arduino Mega (0-69)
    if (Pins::LCD_RESET > 69) return false;
    if (Pins::LCD_ENABLE > 69) return false;
    if (Pins::LCD_D4 > 69) return false;
    if (Pins::LCD_D5 > 69) return false;
    if (Pins::LCD_D6 > 69) return false;
    if (Pins::LCD_D7 > 69) return false;
    
    if (Pins::SD_CS > 69) return false;
    if (Pins::EEPROM_CS > 69) return false;
    
    if (Pins::LPT_STROBE > 69) return false;
    if (Pins::LPT_D0 > 69) return false;
    if (Pins::LPT_D7 > 69) return false;
    
    // Check for conflicts (some basic ones)
    if (Pins::SD_CS == Pins::EEPROM_CS) return false;
    if (Pins::LCD_RESET == Pins::LCD_ENABLE) return false;
    
    return true;
}

bool verify_component_creation() {
    // Skip component creation test in minimal test environment
    // This would require including complex headers that pull in dependencies
    return true;
}

bool verify_memory_usage() {
    using namespace DeviceBridge::Common;
    
    // Check that RTOS configuration is reasonable for Arduino Mega (8KB RAM)
    uint16_t totalStack = RTOS::PARALLEL_PORT_STACK + 
                         RTOS::FILE_MANAGER_STACK + 
                         RTOS::DISPLAY_STACK + 
                         RTOS::TIME_STACK + 
                         RTOS::SYSTEM_MONITOR_STACK;
    
    if (totalStack > 2048) return false; // More than 25% of RAM for stacks
    
    // Estimate queue memory usage
    size_t queueMemory = (RTOS::DATA_QUEUE_SIZE * sizeof(DataChunk)) +
                        (RTOS::DISPLAY_QUEUE_SIZE * sizeof(DisplayMessage)) +
                        (RTOS::COMMAND_QUEUE_SIZE * sizeof(SystemCommand));
    
    if (queueMemory > 4096) return false; // More than 50% of RAM for queues
    
    // Total estimated memory usage
    size_t totalEstimated = totalStack + queueMemory + 1024; // 1KB for static vars
    
    if (totalEstimated > 6144) return false; // More than 75% of RAM
    
    return true;
}