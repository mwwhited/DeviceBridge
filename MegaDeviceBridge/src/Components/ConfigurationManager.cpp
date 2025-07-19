#include "ConfigurationManager.h"
#include "ParallelPortManager.h"
#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "TimeManager.h"
#include "SystemManager.h"
#include <string.h>
#include <Arduino.h>

namespace DeviceBridge::Components {

ConfigurationManager::ConfigurationManager()
    : _parallelPortManager(nullptr)
    , _fileSystemManager(nullptr)
    , _displayManager(nullptr)
    , _timeManager(nullptr)
    , _systemManager(nullptr)
    , _lastCommandCheck(0)
{
}

ConfigurationManager::~ConfigurationManager() {
    stop();
}

bool ConfigurationManager::initialize() {
    return true;
}

void ConfigurationManager::update() {
    // Check for serial commands periodically
    uint32_t currentTime = millis();
    if (currentTime - _lastCommandCheck >= 50) {  // Check every 50ms
        checkSerialCommands();
        _lastCommandCheck = currentTime;
    }
}

void ConfigurationManager::stop() {
    // Nothing specific to stop
}

void ConfigurationManager::checkSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
            processCommand(command);
        }
    }
}

void ConfigurationManager::processCommand(const String& command) {
    if (command.equalsIgnoreCase(F("validate")) || command.equalsIgnoreCase(F("test"))) {
        if (_systemManager) {
            _systemManager->validateHardware();
        }
        
    } else if (command.equalsIgnoreCase(F("info"))) {
        if (_systemManager) {
            _systemManager->printSystemInfo();
            _systemManager->printMemoryInfo();
        }
        
    } else if (command.equalsIgnoreCase(F("status"))) {
        printDetailedStatus();
        
    } else if (command.startsWith(F("time set "))) {
        handleTimeSetCommand(command);
        
    } else if (command.startsWith(F("storage "))) {
        handleStorageCommand(command);
        
    } else if (command.equalsIgnoreCase(F("storage"))) {
        printStorageStatus();
        
    } else if (command.equalsIgnoreCase(F("testwrite")) || command.startsWith(F("testwrite "))) {
        handleTestWriteCommand(command);
        
    } else if (command.startsWith(F("heartbeat "))) {
        handleHeartbeatCommand(command);
        
    } else if (command.equalsIgnoreCase(F("time"))) {
        printCurrentTime();
        
    } else if (command.equalsIgnoreCase(F("buttons"))) {
        printButtonStatus();
        
    } else if (command.equalsIgnoreCase(F("parallel")) || command.equalsIgnoreCase(F("lpt"))) {
        printParallelPortStatus();
        
    } else if (command.equalsIgnoreCase(F("testint")) || command.equalsIgnoreCase(F("testinterrupt"))) {
        testInterruptPin();
        
    } else if (command.equalsIgnoreCase(F("files")) || command.equalsIgnoreCase(F("lastfile"))) {
        printLastFileInfo();
        
    } else if (command.equalsIgnoreCase(F("restart")) || command.equalsIgnoreCase(F("reset"))) {
        Serial.print(F("Restarting system...\r\n"));
        delay(100);
        asm volatile ("  jmp 0");  // Software reset
        
    } else if (command.equalsIgnoreCase(F("help"))) {
        printHelpMenu();
        
    } else {
        Serial.print(F("Unknown command: "));
        Serial.print(command);
        Serial.print(F("\r\nType 'help' for available commands.\r\n"));
    }
}

void ConfigurationManager::printHelpMenu() {
    Serial.print(F("\r\n=== Device Bridge Serial Interface ===\r\n"));
    Serial.print(F("Hardware Commands:\r\n"));
    Serial.print(F("  validate/test     - Run hardware validation\r\n"));
    Serial.print(F("  info              - Show system information\r\n"));
    Serial.print(F("  status            - Show detailed component status\r\n"));
    Serial.print(F("\r\nTime Commands:\r\n"));
    Serial.print(F("  time              - Show current time\r\n"));
    Serial.print(F("  time set YYYY-MM-DD HH:MM - Set RTC time\r\n"));
    Serial.print(F("\r\nDebug Commands:\r\n"));
    Serial.print(F("  buttons           - Show button analog values\r\n"));
    Serial.print(F("  parallel/lpt      - Show parallel port status\r\n"));
    Serial.print(F("  testint           - Test interrupt pin response\r\n"));
    Serial.print(F("  files/lastfile    - Show last saved file info\r\n"));
    Serial.print(F("\r\nStorage Commands:\r\n"));
    Serial.print(F("  storage           - Show storage device status\r\n"));
    Serial.print(F("  storage sd        - Use SD card storage\r\n"));
    Serial.print(F("  storage eeprom    - Use EEPROM storage\r\n"));
    Serial.print(F("  storage serial    - Use serial transfer\r\n"));
    Serial.print(F("  storage auto      - Auto-select storage\r\n"));
    Serial.print(F("  testwrite         - Write test file to current storage\r\n"));
    Serial.print(F("\r\nSystem Commands:\r\n"));
    Serial.print(F("  heartbeat on/off  - Enable/disable serial heartbeat\r\n"));
    Serial.print(F("  restart/reset     - Restart the system\r\n"));
    Serial.print(F("  help              - Show this help\r\n"));
    Serial.print(F("=====================================\r\n\r\n"));
}

void ConfigurationManager::printDetailedStatus() {
    Serial.print(F("\r\n=== Detailed System Status ===\r\n"));
    
    // System info
    if (_systemManager) {
        _systemManager->printSystemInfo();
        _systemManager->printMemoryInfo();
    }
    
    // Component status
    Serial.print(F("\r\n=== Component Status ===\r\n"));
    
    if (_fileSystemManager) {
        Serial.print(F("SD Card: "));
        Serial.print(_fileSystemManager->isSDAvailable() ? F("Available") : F("Not Available"));
        Serial.print(F("\r\n"));
        
        Serial.print(F("EEPROM: "));
        Serial.print(_fileSystemManager->isEEPROMAvailable() ? F("Available") : F("Not Available"));
        Serial.print(F("\r\n"));
        
        Serial.print(F("Active Storage: "));
        Serial.print(_fileSystemManager->getCurrentStorageType().toString());
        Serial.print(F("\r\n"));
    }
    
    if (_timeManager) {
        Serial.print(F("RTC: "));
        Serial.print(_timeManager->isRTCAvailable() ? F("Available") : F("Not Available"));
        Serial.print(F("\r\n"));
    }
    
    if (_systemManager) {
        Serial.print(F("Serial Heartbeat: "));
        Serial.print(_systemManager->isSerialHeartbeatEnabled() ? F("Enabled") : F("Disabled"));
        Serial.print(F("\r\n"));
    }
    
    Serial.print(F("===========================\r\n\r\n"));
}

void ConfigurationManager::printCurrentTime() {
    if (_timeManager && _timeManager->isRTCAvailable()) {
        char timeBuffer[32];
        _timeManager->getFormattedDateTime(timeBuffer, sizeof(timeBuffer));
        Serial.print(F("Current Time: "));
        Serial.print(timeBuffer);
        Serial.print(F("\r\n"));
    } else {
        Serial.print(F("RTC not available\r\n"));
    }
}

void ConfigurationManager::handleTimeSetCommand(const String& command) {
    // Expected format: "time set 2025-07-19 19:30"
    String timeStr = command.substring(9); // Remove "time set "
    timeStr.trim();
    
    if (timeStr.length() < 16) {
        Serial.print(F("Invalid time format. Use: time set YYYY-MM-DD HH:MM\r\n"));
        return;
    }
    
    // Parse date and time
    int year = timeStr.substring(0, 4).toInt();
    int month = timeStr.substring(5, 7).toInt();
    int day = timeStr.substring(8, 10).toInt();
    int hour = timeStr.substring(11, 13).toInt();
    int minute = timeStr.substring(14, 16).toInt();
    
    // Validate ranges
    if (year < 2020 || year > 2099 || month < 1 || month > 12 || 
        day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        Serial.print(F("Invalid date/time values. Use: time set YYYY-MM-DD HH:MM\r\n"));
        return;
    }
    
    // Set time via TimeManager
    if (_timeManager && _timeManager->setDateTime(year, month, day, hour, minute, 0)) {
        Serial.print(F("Time set successfully to: "));
        printCurrentTime();
        if (_displayManager) {
            _displayManager->displayMessage(Common::DisplayMessage::INFO, F("Time Updated"));
        }
    } else {
        Serial.print(F("Failed to set time - RTC not available\r\n"));
        if (_displayManager) {
            _displayManager->displayMessage(Common::DisplayMessage::ERROR, F("Time Set Failed"));
        }
    }
}

void ConfigurationManager::handleStorageCommand(const String& command) {
    String storageType = command.substring(8); // Remove "storage "
    storageType.trim();
    storageType.toLowerCase();
    
    Common::StorageType newStorage(Common::StorageType::AUTO_SELECT); // Initialize with default
    
    if (storageType == F("sd")) {
        newStorage = Common::StorageType(Common::StorageType::SD_CARD);
    } else if (storageType == F("eeprom")) {
        newStorage = Common::StorageType(Common::StorageType::EEPROM);
    } else if (storageType == F("serial")) {
        newStorage = Common::StorageType(Common::StorageType::SERIAL_TRANSFER);
    } else if (storageType == F("auto")) {
        newStorage = Common::StorageType(Common::StorageType::AUTO_SELECT);
    } else {
        Serial.print(F("Invalid storage type. Use: sd, eeprom, serial, or auto\r\n"));
        return;
    }
    
    // Set storage type via FileSystemManager
    if (_fileSystemManager) {
        _fileSystemManager->setStorageType(newStorage);
        
        Serial.print(F("Storage type set to: "));
        Serial.print(newStorage.toString());
        Serial.print(F("\r\n"));
        
        if (_displayManager) {
            _displayManager->displayMessage(Common::DisplayMessage::INFO, newStorage.toString());
        }
    } else {
        Serial.print(F("FileSystemManager not available\r\n"));
    }
}

void ConfigurationManager::printButtonStatus() {
    int16_t analogValue = analogRead(Common::Pins::LCD_BUTTONS);
    
    Serial.print(F("Button Analog Value: "));
    Serial.print(analogValue);
    Serial.print(F(" ("));
    
    // Interpret button based on expected OSEPP values
    if (analogValue < 50) {
        Serial.print(F("RIGHT"));
    } else if (analogValue < 200) {
        Serial.print(F("UP"));
    } else if (analogValue < 400) {
        Serial.print(F("DOWN"));
    } else if (analogValue < 600) {
        Serial.print(F("LEFT"));
    } else if (analogValue < 850) {
        Serial.print(F("SELECT"));
    } else {
        Serial.print(F("NONE"));
    }
    
    Serial.print(F(")\r\n"));
    Serial.print(F("Expected values: RIGHT(~0), UP(~144), DOWN(~329), LEFT(~504), SELECT(~741), NONE(~1023)\r\n"));
}

void ConfigurationManager::printParallelPortStatus() {
    Serial.print(F("\r\n=== Parallel Port Status ===\r\n"));
    
    if (_parallelPortManager) {
        Serial.print(F("Total Bytes Received: "));
        Serial.print(_parallelPortManager->getTotalBytesReceived());
        Serial.print(F("\r\n"));
        
        Serial.print(F("Files Received: "));
        Serial.print(_parallelPortManager->getFilesReceived());
        Serial.print(F("\r\n"));
        
        Serial.print(F("Buffer Level: "));
        Serial.print(_parallelPortManager->getBufferLevel());
        Serial.print(F(" bytes\r\n"));
        
        Serial.print(F("Interrupt Count: "));
        Serial.print(_parallelPortManager->getInterruptCount());
        Serial.print(F("\r\n"));
        
        Serial.print(F("Data Count: "));
        Serial.print(_parallelPortManager->getDataCount());
        Serial.print(F("\r\n"));
    }
    
    // Read raw parallel port pin states
    Serial.print(F("\r\nPin States:\r\n"));
    Serial.print(F("  Strobe (pin 18): "));
    Serial.print(digitalRead(Common::Pins::LPT_STROBE));
    Serial.print(F("\r\n"));
    
    Serial.print(F("  Data pins (D0-D7): "));
    for (int i = 0; i < 8; i++) {
        uint8_t dataPins[] = {Common::Pins::LPT_D0, Common::Pins::LPT_D1, Common::Pins::LPT_D2, 
                              Common::Pins::LPT_D3, Common::Pins::LPT_D4, Common::Pins::LPT_D5, 
                              Common::Pins::LPT_D6, Common::Pins::LPT_D7};
        Serial.print(digitalRead(dataPins[i]));
    }
    Serial.print(F("\r\n"));
    
    Serial.print(F("\r\nStatus pins:\r\n"));
    Serial.print(F("  Ack (pin 41): "));
    Serial.print(digitalRead(Common::Pins::LPT_ACK));
    Serial.print(F("\r\n"));
    Serial.print(F("  Busy (pin 43): "));
    Serial.print(digitalRead(Common::Pins::LPT_BUSY));
    Serial.print(F("\r\n"));
    Serial.print(F("  Error (pin 24): "));
    Serial.print(digitalRead(Common::Pins::LPT_ERROR));
    Serial.print(F("\r\n"));
    
    Serial.print(F("============================\r\n\r\n"));
}

void ConfigurationManager::testInterruptPin() {
    Serial.print(F("\r\n=== Testing Interrupt Pin ===\r\n"));
    Serial.print(F("Monitoring strobe pin (18) for 10 seconds...\r\n"));
    Serial.print(F("Press PRINT on TDS2024 to test interrupt response.\r\n"));
    
    uint32_t startTime = millis();
    uint32_t lastCheck = startTime;
    int lastStrobeState = digitalRead(Common::Pins::LPT_STROBE);
    int strobeChanges = 0;
    
    while (millis() - startTime < 10000) {  // 10 second test
        int currentStrobeState = digitalRead(Common::Pins::LPT_STROBE);
        
        // Check for strobe state changes
        if (currentStrobeState != lastStrobeState) {
            strobeChanges++;
            Serial.print(F("Strobe changed to: "));
            Serial.print(currentStrobeState);
            Serial.print(F(" (count: "));
            Serial.print(strobeChanges);
            Serial.print(F(")\r\n"));
            lastStrobeState = currentStrobeState;
        }
        
        // Update every second
        if (millis() - lastCheck >= 1000) {
            Serial.print(F("."));
            lastCheck = millis();
        }
    }
    
    Serial.print(F("\r\n"));
    Serial.print(F("Test complete. Strobe changes detected: "));
    Serial.print(strobeChanges);
    Serial.print(F("\r\n"));
    
    if (_parallelPortManager) {
        Serial.print(F("Buffer level after test: "));
        Serial.print(_parallelPortManager->getBufferLevel());
        Serial.print(F(" bytes\r\n"));
    }
    
    Serial.print(F("==============================\r\n\r\n"));
}

void ConfigurationManager::printStorageStatus() {
    Serial.print(F("\r\n=== Storage Device Status ===\r\n"));
    
    if (_fileSystemManager) {
        Serial.print(F("SD Card: "));
        Serial.print(_fileSystemManager->isSDAvailable() ? F("Available") : F("Not Available"));
        Serial.print(F("\r\n"));
        
        Serial.print(F("EEPROM: "));
        Serial.print(_fileSystemManager->isEEPROMAvailable() ? F("Available") : F("Not Available"));
        Serial.print(F("\r\n"));
        
        Serial.print(F("Active Storage: "));
        Serial.print(_fileSystemManager->getActiveStorage().toSimple());
        Serial.print(F("\r\n"));
        
        Serial.print(F("Files Stored: "));
        Serial.print(_fileSystemManager->getFilesStored());
        Serial.print(F("\r\n"));
        
        Serial.print(F("Total Bytes Written: "));
        Serial.print(_fileSystemManager->getTotalBytesWritten());
        Serial.print(F(" bytes\r\n"));
        
        Serial.print(F("Write Errors: "));
        Serial.print(_fileSystemManager->getWriteErrors());
        Serial.print(F("\r\n"));
    } else {
        Serial.print(F("FileSystemManager not available\r\n"));
    }
    
    if (_systemManager) {
        Serial.print(F("Free Memory: "));
        Serial.print(_systemManager->getFreeMemory());
        Serial.print(F(" bytes\r\n"));
    }
    
    Serial.print(F("=============================\r\n\r\n"));
}

void ConfigurationManager::handleTestWriteCommand(const String& command) {
    Serial.print(F("\r\n=== Test File Write ===\r\n"));
    
    if (!_fileSystemManager) {
        Serial.print(F("FileSystemManager not available\r\n"));
        return;
    }
    
    // Create test data with timestamp
    char testData[64];
    if (_timeManager && _timeManager->isRTCAvailable()) {
        char timeBuffer[32];
        _timeManager->getFormattedDateTime(timeBuffer, sizeof(timeBuffer));
        snprintf(testData, sizeof(testData), "TEST %s - Memory: %d bytes free", 
                timeBuffer, _systemManager ? _systemManager->getFreeMemory() : 0);
    } else {
        snprintf(testData, sizeof(testData), "TEST %lu - Memory: %d bytes free", 
                millis(), _systemManager ? _systemManager->getFreeMemory() : 0);
    }
    
    Serial.print(F("Test Data: "));
    Serial.print(testData);
    Serial.print(F("\r\n"));
    
    Serial.print(F("Current Storage: "));
    Serial.print(_fileSystemManager->getActiveStorage().toSimple());
    Serial.print(F("\r\n"));
    
    // Create a test data chunk to simulate file write
    Common::DataChunk testChunk;
    memset(&testChunk, 0, sizeof(testChunk));
    
    // Set up test chunk as new file
    testChunk.isNewFile = 1;
    testChunk.isEndOfFile = 0;
    testChunk.length = strlen(testData);
    testChunk.timestamp = millis();
    
    // Copy test data to chunk
    strncpy((char*)testChunk.data, testData, sizeof(testChunk.data) - 1);
    
    Serial.print(F("Writing test file...\r\n"));
    
    // Process the chunk (creates new file)
    _fileSystemManager->processDataChunk(testChunk);
    
    // Check write status after first chunk
    Serial.print(F("Write errors after data chunk: "));
    Serial.print(_fileSystemManager->getWriteErrors());
    Serial.print(F("\r\n"));
    
    // Create end-of-file chunk
    Common::DataChunk endChunk;
    memset(&endChunk, 0, sizeof(endChunk));
    endChunk.isNewFile = 0;
    endChunk.isEndOfFile = 1;
    endChunk.length = 0;
    endChunk.timestamp = millis();
    
    // Process end chunk (closes file)
    _fileSystemManager->processDataChunk(endChunk);
    
    // Check final status
    Serial.print(F("Write errors after close: "));
    Serial.print(_fileSystemManager->getWriteErrors());
    Serial.print(F("\r\n"));
    
    Serial.print(F("Test write completed.\r\n"));
    Serial.print(F("=======================\r\n\r\n"));
}

void ConfigurationManager::printLastFileInfo() {
    Serial.print(F("\r\n=== Last Saved File Information ===\r\n"));
    
    if (_fileSystemManager) {
        Serial.print(F("Files Stored: "));
        Serial.print(_fileSystemManager->getFilesStored());
        Serial.print(F("\r\n"));
        
        if (_fileSystemManager->getFilesStored() > 0) {
            Serial.print(F("Last Filename: "));
            Serial.print(_fileSystemManager->getCurrentFilename());
            Serial.print(F("\r\n"));
            
            Serial.print(F("Storage Device: "));
            Serial.print(_fileSystemManager->getActiveStorage().toString());
            Serial.print(F("\r\n"));
            
            Serial.print(F("File Type (Requested): "));
            Serial.print(_fileSystemManager->getFileType().toSimple());
            Serial.print(F("\r\n"));
            
            Serial.print(F("File Type (Detected): "));
            Serial.print(_fileSystemManager->getDetectedFileType().toSimple());
            Serial.print(F("\r\n"));
            
            Serial.print(F("Total Bytes Written: "));
            Serial.print(_fileSystemManager->getTotalBytesWritten());
            Serial.print(F(" bytes\r\n"));
            
            Serial.print(F("Write Errors: "));
            Serial.print(_fileSystemManager->getWriteErrors());
            Serial.print(F("\r\n"));
        } else {
            Serial.print(F("No files saved yet.\r\n"));
        }
    } else {
        Serial.print(F("FileSystemManager not available\r\n"));
    }
    
    Serial.print(F("===================================\r\n\r\n"));
}

void ConfigurationManager::handleHeartbeatCommand(const String& command) {
    String setting = command.substring(10); // Remove "heartbeat "
    setting.trim();
    setting.toLowerCase();
    
    if (setting == F("on") || setting == F("enable") || setting == F("true") || setting == F("1")) {
        if (_systemManager) {
            _systemManager->setSerialHeartbeatEnabled(true);
            Serial.print(F("Serial heartbeat enabled\r\n"));
            if (_displayManager) {
                _displayManager->displayMessage(Common::DisplayMessage::INFO, F("Heartbeat ON"));
            }
        }
    } else if (setting == F("off") || setting == F("disable") || setting == F("false") || setting == F("0")) {
        if (_systemManager) {
            _systemManager->setSerialHeartbeatEnabled(false);
            Serial.print(F("Serial heartbeat disabled\r\n"));
            if (_displayManager) {
                _displayManager->displayMessage(Common::DisplayMessage::INFO, F("Heartbeat OFF"));
            }
        }
    } else if (setting == F("status")) {
        if (_systemManager) {
            bool enabled = _systemManager->isSerialHeartbeatEnabled();
            Serial.print(F("Serial heartbeat is "));
            Serial.print(enabled ? F("enabled") : F("disabled"));
            Serial.print(F("\r\n"));
        }
    } else {
        Serial.print(F("Usage: heartbeat on/off/status\r\n"));
        Serial.print(F("  on/enable/true/1  - Enable serial heartbeat\r\n"));
        Serial.print(F("  off/disable/false/0 - Disable serial heartbeat\r\n"));
        Serial.print(F("  status - Show current status\r\n"));
    }
}

void ConfigurationManager::setComponentManagers(
    ParallelPortManager* parallelPort,
    FileSystemManager* fileSystem,
    DisplayManager* display,
    TimeManager* time,
    SystemManager* system
) {
    _parallelPortManager = parallelPort;
    _fileSystemManager = fileSystem;
    _displayManager = display;
    _timeManager = time;
    _systemManager = system;
}

} // namespace DeviceBridge::Components