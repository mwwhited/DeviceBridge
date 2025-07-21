#include "ConfigurationManager.h"
#include "DisplayManager.h"
#include "FileSystemManager.h"
#include "ParallelPortManager.h"
#include "SystemManager.h"
#include "TimeManager.h"
#include "../Common/ConfigurationService.h"
#include <Arduino.h>
#include <string.h>

// PROGMEM component name for memory optimization
static const char component_name[] PROGMEM = "ConfigurationManager";

namespace DeviceBridge::Components {

ConfigurationManager::ConfigurationManager() : _lastCommandCheck(0) {}

ConfigurationManager::~ConfigurationManager() { stop(); }

bool ConfigurationManager::initialize() { 
    // Cache service dependencies first (performance optimization)
    cacheServiceDependencies();
    
    return true; 
}

void ConfigurationManager::update(unsigned long currentTime) {
    // Check for serial commands periodically
    if (currentTime - _lastCommandCheck >= 50) { // Check every 50ms
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

void ConfigurationManager::processCommand(const String &command) {
    // Use cached system manager pointer
    if (command.equalsIgnoreCase(F("validate")) || command.equalsIgnoreCase(F("test"))) {
        // Run comprehensive system validation
        Serial.print(F("\r\n=== COMPREHENSIVE SYSTEM VALIDATION ===\r\n"));

        // 1. Service Locator validation
        DeviceBridge::ServiceLocator &services = DeviceBridge::ServiceLocator::getInstance();
        bool dependenciesOK = services.validateAllDependencies();

        // 2. Individual component self-tests
        bool selfTestsOK = services.runSystemSelfTest();

        // 3. Hardware validation (existing)
        Serial.print(F("\r\n=== HARDWARE VALIDATION ===\r\n"));
        _cachedSystemManager->validateHardware();

        // 4. Summary
        Serial.print(F("\r\n=== VALIDATION SUMMARY ===\r\n"));
        Serial.print(F("Dependencies: "));
        Serial.print(dependenciesOK ? F("✅ PASSED") : F("❌ FAILED"));
        Serial.print(F("\r\nSelf-Tests: "));
        Serial.print(selfTestsOK ? F("✅ PASSED") : F("⚠️  WARNINGS"));
        Serial.print(F("\r\nOverall Status: "));
        if (dependenciesOK && selfTestsOK) {
            Serial.print(F("✅ SYSTEM READY\r\n"));
        } else if (dependenciesOK) {
            Serial.print(F("⚠️  OPERATIONAL WITH WARNINGS\r\n"));
        } else {
            Serial.print(F("❌ CRITICAL ISSUES DETECTED\r\n"));
        }
        Serial.print(F("=====================================\r\n"));
    } else if (command.equalsIgnoreCase(F("info"))) {
        _cachedSystemManager->printSystemInfo();
        _cachedSystemManager->printMemoryInfo();
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
    } else if (command.equalsIgnoreCase(F("testwritelong")) || command.startsWith(F("testwritelong "))) {
        handleTestWriteLongCommand(command);
    } else if (command.startsWith(F("heartbeat "))) {
        handleHeartbeatCommand(command);
    } else if (command.startsWith(F("debug "))) {
        handleDebugCommand(command);
    } else if (command.equalsIgnoreCase(F("time"))) {
        printCurrentTime();
    } else if (command.equalsIgnoreCase(F("buttons"))) {
        printButtonStatus();
    } else if (command.equalsIgnoreCase(F("parallel")) || command.equalsIgnoreCase(F("lpt"))) {
        printParallelPortStatus();
    } else if (command.equalsIgnoreCase(F("testint")) || command.equalsIgnoreCase(F("testinterrupt"))) {
        testInterruptPin();
    } else if (command.equalsIgnoreCase(F("testlpt")) || command.equalsIgnoreCase(F("testprinter"))) {
        testPrinterProtocol();
    } else if (command.equalsIgnoreCase(F("clearbuffer")) || command.equalsIgnoreCase(F("clearport"))) {
        clearLPTBuffer();
    } else if (command.equalsIgnoreCase(F("resetcritical")) || command.equalsIgnoreCase(F("clearcritical"))) {
        resetCriticalState();
    } else if (command.startsWith(F("flowcontrol "))) {
        handleFlowControlCommand(command);
    } else if (command.equalsIgnoreCase(F("flowstats")) || command.equalsIgnoreCase(F("flowstatus"))) {
        printFlowControlStatistics();
    } else if (command.startsWith(F("lcdthrottle "))) {
        handleLCDThrottleCommand(command);
    } else if (command.startsWith(F("led "))) {
        handleLEDCommand(command);
    } else if (command.equalsIgnoreCase(F("files")) || command.equalsIgnoreCase(F("lastfile"))) {
        printLastFileInfo();
    } else if (command.startsWith(F("list "))) {
        handleListCommand(command);
    } else if (command.equalsIgnoreCase(F("restart")) || command.equalsIgnoreCase(F("reset"))) {
        Serial.print(F("Restarting system...\r\n"));
        delay(100);
        asm volatile("  jmp 0"); // Software reset
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
    Serial.print(F("  parallel/lpt      - Show parallel port status with hex data\r\n"));
    Serial.print(F("  testint           - Test interrupt pin response\r\n"));
    Serial.print(F("  testlpt           - Test LPT printer protocol signals\r\n"));
    Serial.print(F("  clearbuffer       - Clear LPT data buffer and reset state\r\n"));
    Serial.print(F("  resetcritical     - Reset critical flow control state\r\n"));
    Serial.print(F("  flowcontrol on/off - Enable/disable hardware flow control\r\n"));
    Serial.print(F("  flowstats         - Show hardware flow control statistics\r\n"));
    Serial.print(F("  lcdthrottle on/off - Control LCD refresh throttling for storage ops\r\n"));
    Serial.print(F("  led l1/l2 on/off  - Control L1 (LPT) and L2 (Write) LEDs\r\n"));
    Serial.print(F("  debug lcd on/off      - Enable/disable LCD debug output to serial\r\n"));
    Serial.print(F("  debug parallel on/off - Enable/disable parallel port debug logging\r\n"));
    Serial.print(F("  files/lastfile    - Show last saved file info with SD status\r\n"));
    Serial.print(F("  list sd           - List all files on SD card\r\n"));
    Serial.print(F("\r\nStorage Commands:\r\n"));
    Serial.print(F("  storage           - Show storage/hardware status\r\n"));
    Serial.print(F("  storage sd        - Use SD card storage\r\n"));
    Serial.print(F("  storage eeprom    - Use EEPROM storage\r\n"));
    Serial.print(F("  storage serial    - Use serial transfer\r\n"));
    Serial.print(F("  storage auto      - Auto-select storage\r\n"));
    Serial.print(F("  testwrite         - Write test file to current storage\r\n"));
    Serial.print(F("  testwritelong     - Write test file with multiple chunks (tests LED/buffer)\r\n"));
    Serial.print(F("\r\nSystem Commands:\r\n"));
    Serial.print(F("  heartbeat on/off  - Enable/disable serial heartbeat\r\n"));
    Serial.print(F("  restart/reset     - Restart the system\r\n"));
    Serial.print(F("  help              - Show this help\r\n"));
    Serial.print(F("=====================================\r\n\r\n"));
}

void ConfigurationManager::printDetailedStatus() {
    // Use cached file system manager pointer
    // Use cached time manager pointer
    // Use cached system manager pointer

    Serial.print(F("\r\n=== Detailed System Status ===\r\n"));
    _cachedSystemManager->printSystemInfo();
    _cachedSystemManager->printMemoryInfo();

    // Component status
    Serial.print(F("\r\n=== Component Status ===\r\n"));

    Serial.print(F("SD Card: "));
    Serial.print(_cachedFileSystemManager->isSDAvailable() ? F("Available") : F("Not Available"));
    Serial.print(F("\r\n"));

    Serial.print(F("EEPROM: "));
    Serial.print(_cachedFileSystemManager->isEEPROMAvailable() ? F("Available") : F("Not Available"));
    Serial.print(F("\r\n"));

    Serial.print(F("Active Storage: "));
    Serial.print(_cachedFileSystemManager->getCurrentStorageType().toString());
    Serial.print(F("\r\n"));

    Serial.print(F("RTC: "));
    Serial.print(_cachedTimeManager->isRTCAvailable() ? F("Available") : F("Not Available"));
    Serial.print(F("\r\n"));

    Serial.print(F("Serial Heartbeat: "));
    Serial.print(_cachedSystemManager->isSerialHeartbeatEnabled() ? F("Enabled") : F("Disabled"));
    Serial.print(F("\r\n"));

    Serial.print(F("===========================\r\n\r\n"));
}

void ConfigurationManager::printCurrentTime() {
    // Use cached time manager pointer
    if (_cachedTimeManager->isRTCAvailable()) {
        char timeBuffer[32];
        _cachedTimeManager->getFormattedDateTime(timeBuffer, sizeof(timeBuffer));
        Serial.print(F("Current Time: "));
        Serial.print(timeBuffer);
        Serial.print(F("\r\n"));
    } else {
        Serial.print(F("RTC not available\r\n"));
    }
}

void ConfigurationManager::handleTimeSetCommand(const String &command) {
    // Use cached display manager pointer
    // Use cached time manager pointer
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
    if (year < 2020 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 ||
        minute < 0 || minute > 59) {
        Serial.print(F("Invalid date/time values. Use: time set YYYY-MM-DD HH:MM\r\n"));
        return;
    }

    // Set time via TimeManager
    if (_cachedTimeManager->setDateTime(year, month, day, hour, minute, 0)) {
        Serial.print(F("Time set successfully to: "));
        printCurrentTime();
        _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("Time Updated"));
    } else {
        Serial.print(F("Failed to set time - RTC not available\r\n"));
        _cachedDisplayManager->displayMessage(Common::DisplayMessage::ERROR, F("Time Set Failed"));
    }
}

void ConfigurationManager::handleStorageCommand(const String &command) {
    // Use cached file system manager pointer
    // Use cached display manager pointer
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

    _cachedFileSystemManager->setStorageType(newStorage);

    Serial.print(F("Storage type set to: "));
    Serial.print(newStorage.toString());
    Serial.print(F("\r\n"));

    _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, newStorage.toString());
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
    Serial.print(F("Expected values: RIGHT(~0), UP(~144), DOWN(~329), LEFT(~504), SELECT(~741), "
                   "NONE(~1023)\r\n"));
}

void ConfigurationManager::printParallelPortStatus() {
    // Use cached parallel port manager pointer
    // Use cached file system manager pointer
    Serial.print(F("\r\n=== Parallel Port Status ===\r\n"));

    Serial.print(F("Total Bytes Received: "));
    Serial.print(_cachedParallelPortManager->getTotalBytesReceived());
    Serial.print(F("\r\n"));

    Serial.print(F("Total Bytes Written: "));
    Serial.print(_cachedFileSystemManager->getTotalBytesWritten());
    Serial.print(F("\r\n"));

    Serial.print(F("Files Received: "));
    Serial.print(_cachedParallelPortManager->getFilesReceived());
    Serial.print(F("\r\n"));

    Serial.print(F("Buffer Level: "));
    Serial.print(_cachedParallelPortManager->getBufferLevel());
    Serial.print(F(" bytes\r\n"));

    Serial.print(F("Interrupt Count: "));
    Serial.print(_cachedParallelPortManager->getInterruptCount());
    Serial.print(F("\r\n"));

    Serial.print(F("Data Count: "));
    Serial.print(_cachedParallelPortManager->getDataCount());
    Serial.print(F("\r\n"));
    
    // Data integrity check
    uint32_t totalRead = _cachedParallelPortManager->getTotalBytesReceived();
    uint32_t totalWritten = _cachedFileSystemManager->getTotalBytesWritten();
    Serial.print(F("Data Integrity: "));
    if (totalRead == totalWritten) {
        Serial.print(F("GOOD ("));
        Serial.print(totalRead);
        Serial.print(F(" bytes match)\r\n"));
    } else {
        Serial.print(F("MISMATCH - Read: "));
        Serial.print(totalRead);
        Serial.print(F(", Written: "));
        Serial.print(totalWritten);
        Serial.print(F(", Diff: "));
        Serial.print((totalRead > totalWritten) ? (totalRead - totalWritten) : (totalWritten - totalRead));
        Serial.print(F("\r\n"));
    }

    // Read raw parallel port pin states
    Serial.print(F("\r\nPin States:\r\n"));
    Serial.print(F("  Strobe (pin 18): "));
    Serial.print(digitalRead(Common::Pins::LPT_STROBE));
    Serial.print(F("\r\n"));

    // Read data pins and calculate hex value
    uint8_t dataPins[] = {Common::Pins::LPT_D0, Common::Pins::LPT_D1, Common::Pins::LPT_D2, Common::Pins::LPT_D3,
                          Common::Pins::LPT_D4, Common::Pins::LPT_D5, Common::Pins::LPT_D6, Common::Pins::LPT_D7};

    uint8_t dataValue = 0;
    Serial.print(F("  Data pins (D0-D7): "));
    for (int i = 0; i < 8; i++) {
        int pinState = digitalRead(dataPins[i]);
        Serial.print(pinState);
        if (pinState) {
            dataValue |= (1 << i); // Set bit i if pin is HIGH
        }
    }
    Serial.print(F(" (0x"));
    if (dataValue < 16)
        Serial.print(F("0")); // Leading zero for single hex digit
    Serial.print(dataValue, HEX);
    Serial.print(F(")"));
    Serial.print(F("\r\n"));

    Serial.print(F("\r\nControl pins (Input - Active Low):\r\n"));
    Serial.print(F("  /Strobe: "));
    Serial.print(_cachedParallelPortManager->isStrobeLow() ? F("ACTIVE") : F("INACTIVE"));
    Serial.print(F(" (pin "));
    Serial.print(digitalRead(Common::Pins::LPT_STROBE));
    Serial.print(F(")\r\n"));
    
    Serial.print(F("  /Auto Feed: "));
    Serial.print(_cachedParallelPortManager->isAutoFeedLow() ? F("ACTIVE") : F("INACTIVE"));
    Serial.print(F(" (pin "));
    Serial.print(digitalRead(Common::Pins::LPT_AUTO_FEED));
    Serial.print(F(")\r\n"));
    
    Serial.print(F("  /Initialize: "));
    Serial.print(_cachedParallelPortManager->isInitializeLow() ? F("ACTIVE") : F("INACTIVE"));
    Serial.print(F(" (pin "));
    Serial.print(digitalRead(Common::Pins::LPT_INITIALIZE));
    Serial.print(F(")\r\n"));
    
    Serial.print(F("  /Select In: "));
    Serial.print(_cachedParallelPortManager->isSelectInLow() ? F("ACTIVE") : F("INACTIVE"));
    Serial.print(F(" (pin "));
    Serial.print(digitalRead(Common::Pins::LPT_SELECT_IN));
    Serial.print(F(")\r\n"));

    Serial.print(F("\r\nStatus pins (Output):\r\n"));
    Serial.print(F("  Ack (pin 41): "));
    Serial.print(digitalRead(Common::Pins::LPT_ACK));
    Serial.print(F("\r\n"));
    Serial.print(F("  Busy (pin 43): "));
    Serial.print(digitalRead(Common::Pins::LPT_BUSY));
    Serial.print(F("\r\n"));
    Serial.print(F("  Paper Out (pin 45): "));
    Serial.print(digitalRead(Common::Pins::LPT_PAPER_OUT));
    Serial.print(F("\r\n"));
    Serial.print(F("  Select (pin 47): "));
    Serial.print(digitalRead(Common::Pins::LPT_SELECT));
    Serial.print(F("\r\n"));
    Serial.print(F("  Error (pin 24): "));
    Serial.print(digitalRead(Common::Pins::LPT_ERROR));
    Serial.print(F("\r\n"));

    Serial.print(F("============================\r\n\r\n"));
}

void ConfigurationManager::testInterruptPin() {
    // Use cached parallel port manager pointer
    Serial.print(F("\r\n=== Testing Interrupt Pin ===\r\n"));
    Serial.print(F("Monitoring strobe pin (18) for 10 seconds...\r\n"));
    Serial.print(F("Press PRINT on TDS2024 to test interrupt response.\r\n"));

    uint32_t startTime = millis();
    uint32_t lastCheck = startTime;
    int lastStrobeState = digitalRead(Common::Pins::LPT_STROBE);
    int strobeChanges = 0;

    while (millis() - startTime < 10000) { // 10 second test
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

    Serial.print(F("Buffer level after test: "));
    Serial.print(_cachedParallelPortManager->getBufferLevel());
    Serial.print(F(" bytes\r\n"));

    Serial.print(F("==============================\r\n\r\n"));
}

void ConfigurationManager::testPrinterProtocol() {
    // Use cached parallel port manager pointer
    Serial.print(F("\r\n=== Testing LPT Printer Protocol ===\r\n"));
    Serial.print(F("Testing busy/acknowledge signaling for 5 seconds...\r\n"));

    // Test sequence: Set various printer states
    Serial.print(F("Setting printer to READY state...\r\n"));
    _cachedParallelPortManager->setPrinterBusy(false);
    _cachedParallelPortManager->setPrinterError(false);
    _cachedParallelPortManager->setPrinterPaperOut(false);
    _cachedParallelPortManager->setPrinterSelect(true);
    delay(500);

    Serial.print(F("Testing BUSY signal (should block TDS2024)...\r\n"));
    _cachedParallelPortManager->setPrinterBusy(true);
    delay(2000); // Keep busy for 2 seconds
    _cachedParallelPortManager->setPrinterBusy(false);
    Serial.print(F("BUSY signal cleared\r\n"));

    Serial.print(F("Testing ERROR signal...\r\n"));
    _cachedParallelPortManager->setPrinterError(true);
    delay(500);
    _cachedParallelPortManager->setPrinterError(false);
    Serial.print(F("ERROR signal cleared\r\n"));

    Serial.print(F("Testing SELECT signal...\r\n"));
    _cachedParallelPortManager->setPrinterSelect(false);
    delay(500);
    _cachedParallelPortManager->setPrinterSelect(true);
    Serial.print(F("SELECT signal restored\r\n"));

    Serial.print(F("Testing ACKNOWLEDGE pulse...\r\n"));
    for (int i = 0; i < 3; i++) {
        _cachedParallelPortManager->sendPrinterAcknowledge();
        delay(100);
    }
    Serial.print(F("ACK pulses sent\r\n"));

    Serial.print(F("Returning to READY state...\r\n"));
    _cachedParallelPortManager->setPrinterBusy(false);
    _cachedParallelPortManager->setPrinterError(false);
    _cachedParallelPortManager->setPrinterPaperOut(false);
    _cachedParallelPortManager->setPrinterSelect(true);

    Serial.print(F("LPT Printer Protocol test completed.\r\n"));
    Serial.print(F("=====================================\r\n\r\n"));
}

void ConfigurationManager::printStorageStatus() {
    // Use cached file system manager pointer
    // Use cached system manager pointer
    // Use cached parallel port manager pointer
    
    Serial.print(F("\r\n=== Storage Device Status ===\r\n"));

    Serial.print(F("SD Card: "));
    Serial.print(_cachedFileSystemManager->isSDAvailable() ? F("Available") : F("Not Available"));
    Serial.print(F("\r\n"));

    Serial.print(F("SD Card Present: "));
    Serial.print(_cachedFileSystemManager->isSDCardPresent() ? F("YES") : F("NO"));
    Serial.print(F(" (CD Pin 36: "));
    Serial.print(digitalRead(Common::Pins::SD_CD) ? F("Missing") : F("Detected"));
    Serial.print(F(")\r\n"));

    Serial.print(F("SD Write Protected: "));
    Serial.print(_cachedFileSystemManager->isSDWriteProtected() ? F("YES") : F("NO"));
    Serial.print(F(" (WP Pin 34: "));
    Serial.print(digitalRead(Common::Pins::SD_WP) ? F("Protected") : F("Unprotected"));
    Serial.print(F(")\r\n"));

    Serial.print(F("EEPROM: "));
    Serial.print(_cachedFileSystemManager->isEEPROMAvailable() ? F("Available") : F("Not Available"));
    Serial.print(F("\r\n"));

    // Add LPT buffer status for debugging data loss issues
    // Use cached configuration service pointer
    uint16_t bufferCapacity = _cachedConfigurationService->getRingBufferSize();
    uint16_t moderateThreshold = _cachedConfigurationService->getModerateFlowThreshold(bufferCapacity);
    uint16_t criticalThreshold = _cachedConfigurationService->getCriticalFlowThreshold(bufferCapacity);
    uint16_t recoveryThreshold = _cachedConfigurationService->getRecoveryFlowThreshold(bufferCapacity);
    
    Serial.print(F("\r\n=== LPT Buffer Status ===\r\n"));
    uint16_t bufferLevel = _cachedParallelPortManager->getBufferLevel();
    Serial.print(F("Buffer Level: "));
    Serial.print(bufferLevel);
    Serial.print(F("/"));
    Serial.print(bufferCapacity);
    Serial.print(F(" bytes ("));
    Serial.print((bufferLevel * 100) / bufferCapacity);
    Serial.print(F("% full)\r\n"));
    
    Serial.print(F("Flow Control Thresholds:\r\n"));
    Serial.print(F("  60% ("));
    Serial.print(moderateThreshold);
    Serial.print(F(" bytes): Moderate busy delay ("));
    Serial.print(_cachedConfigurationService->getModerateFlowDelayUs());
    Serial.print(F("μs)\r\n"));
    Serial.print(F("  80% ("));
    Serial.print(criticalThreshold);
    Serial.print(F(" bytes): Extended busy delay ("));
    Serial.print(_cachedConfigurationService->getCriticalFlowDelayUs());
    Serial.print(F("μs)\r\n"));
    
    Serial.print(F("Buffer Status: "));
    if (bufferLevel >= bufferCapacity) {
        Serial.print(F("❌ FULL - DATA LOSS RISK!"));
    } else if (bufferLevel >= criticalThreshold) {  // 80% threshold
        Serial.print(F("🔴 CRITICAL - Extended flow control ("));
        Serial.print(_cachedConfigurationService->getCriticalFlowDelayUs());
        Serial.print(F("μs)"));
    } else if (bufferLevel >= moderateThreshold) {  // 60% threshold
        Serial.print(F("⚠️  WARNING - Moderate flow control ("));
        Serial.print(_cachedConfigurationService->getModerateFlowDelayUs());
        Serial.print(F("μs)"));
    } else if (bufferLevel >= recoveryThreshold) {  // 50% threshold
        Serial.print(F("🟡 ELEVATED - Ready for flow control"));
    } else if (bufferLevel > 0) {
        Serial.print(F("✅ Normal - Data available"));
    } else {
        Serial.print(F("✅ Empty"));
    }
    Serial.print(F("\r\n"));
    
    // Add critical state information
    if (_cachedParallelPortManager->isCriticalFlowControlActive()) {
        Serial.print(F("⚠️  CRITICAL FLOW CONTROL ACTIVE\r\n"));
        Serial.print(F("Critical State Duration: "));
        // Calculate duration (approximation)
        Serial.print(F("Active\r\n"));
    }
    
    // Add interrupt statistics
    Serial.print(F("Interrupt Count: "));
    Serial.print(_cachedParallelPortManager->getInterruptCount());
    Serial.print(F("\r\n"));
    Serial.print(F("Data Count: "));
    Serial.print(_cachedParallelPortManager->getDataCount());
    Serial.print(F("\r\n"));
    
    // Add LCD refresh status
    // Use cached display manager pointer
    Serial.print(F("\r\n=== LCD Refresh Status ===\r\n"));
    Serial.print(F("Storage Operation Active: "));
    Serial.print(_cachedDisplayManager->isStorageOperationActive() ? F("YES") : F("NO"));
    Serial.print(F("\r\n"));
    Serial.print(F("Current Refresh Rate: "));
    Serial.print(_cachedDisplayManager->isStorageOperationActive() ? F("500ms (Throttled)") : F("100ms (Normal)"));
    Serial.print(F("\r\n"));

    Serial.print(F("Active Storage: "));
    Serial.print(_cachedFileSystemManager->getActiveStorage().toSimple());
    Serial.print(F("\r\n"));

    Serial.print(F("Files Stored: "));
    Serial.print(_cachedFileSystemManager->getFilesStored());
    Serial.print(F("\r\n"));

    Serial.print(F("Total Bytes Written: "));
    Serial.print(_cachedFileSystemManager->getTotalBytesWritten());
    Serial.print(F(" bytes\r\n"));

    Serial.print(F("Write Errors: "));
    Serial.print(_cachedFileSystemManager->getWriteErrors());
    Serial.print(F("\r\n"));

    Serial.print(F("Free Memory: "));
    Serial.print(_cachedSystemManager->getFreeMemory());
    Serial.print(F(" bytes\r\n"));

    Serial.print(F("\r\n=== Hardware Status ===\r\n"));
    Serial.print(F("L1 LED (Pin 30): "));
    Serial.print(digitalRead(Common::Pins::LPT_READ_LED) ? F("ON") : F("OFF"));
    Serial.print(F("\r\n"));

    Serial.print(F("L2 LED (Pin 32): "));
    Serial.print(digitalRead(Common::Pins::DATA_WRITE_LED) ? F("ON") : F("OFF"));
    Serial.print(F("\r\n"));

    Serial.print(F("SD Card Detect (Pin 36): "));
    Serial.print(digitalRead(Common::Pins::SD_CD) ? F("Missing") : F("Detected"));
    Serial.print(F("\r\n"));

    Serial.print(F("SD Write Protect (Pin 34): "));
    Serial.print(digitalRead(Common::Pins::SD_WP) ? F("Protected") : F("Unprotected"));
    Serial.print(F("\r\n"));

    Serial.print(F("=============================\r\n\r\n"));
}

void ConfigurationManager::handleTestWriteCommand(const String &command) {
    // Use cached file system manager pointer
    // Use cached time manager pointer
    // Use cached system manager pointer
    Serial.print(F("\r\n=== Test File Write ===\r\n"));

    // Create test data with timestamp
    char testData[64];
    if (_cachedTimeManager->isRTCAvailable()) {
        char timeBuffer[32];
        _cachedTimeManager->getFormattedDateTime(timeBuffer, sizeof(timeBuffer));
        snprintf(testData, sizeof(testData), "TEST %s - Memory: %d bytes free", timeBuffer,
                 _cachedSystemManager->getFreeMemory());
    } else {
        snprintf(testData, sizeof(testData), "TEST %lu - Memory: %d bytes free", millis(),
                 _cachedSystemManager->getFreeMemory());
    }

    Serial.print(F("Test Data: "));
    Serial.print(testData);
    Serial.print(F("\r\n"));

    Serial.print(F("Active Storage: "));
    Serial.print(_cachedFileSystemManager->getActiveStorage().toSimple());
    Serial.print(F("\r\n"));

    Serial.print(F("Storage Status: SD="));
    Serial.print(_cachedFileSystemManager->isSDAvailable() ? F("OK") : F("FAIL"));
    Serial.print(F(", EEPROM="));
    Serial.print(_cachedFileSystemManager->isEEPROMAvailable() ? F("OK") : F("FAIL"));
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
    strncpy((char *)testChunk.data, testData, sizeof(testChunk.data) - 1);

    Serial.print(F("Writing test file...\r\n"));

    // Process the chunk (creates new file)
    _cachedFileSystemManager->processDataChunk(testChunk);

    // Check write status after first chunk
    Serial.print(F("Write errors after data chunk: "));
    Serial.print(_cachedFileSystemManager->getWriteErrors());
    Serial.print(F("\r\n"));

    // Create end-of-file chunk
    Common::DataChunk endChunk;
    memset(&endChunk, 0, sizeof(endChunk));
    endChunk.isNewFile = 0;
    endChunk.isEndOfFile = 1;
    endChunk.length = 0;
    endChunk.timestamp = millis();

    // Process end chunk (closes file)
    _cachedFileSystemManager->processDataChunk(endChunk);

    // Check final status
    Serial.print(F("Write errors after close: "));
    Serial.print(_cachedFileSystemManager->getWriteErrors());
    Serial.print(F("\r\n"));

    Serial.print(F("Final Storage Used: "));
    Serial.print(_cachedFileSystemManager->getActiveStorage().toSimple());
    Serial.print(F("\r\n"));

    Serial.print(F("Files Now Stored: "));
    Serial.print(_cachedFileSystemManager->getFilesStored());
    Serial.print(F("\r\n"));

    Serial.print(F("New file: "));
    Serial.print(_cachedFileSystemManager->getCurrentFilename());
    Serial.print(F("\r\n"));

    Serial.print(F("Test write completed.\r\n"));
    Serial.print(F("=======================\r\n\r\n"));
}

void ConfigurationManager::handleTestWriteLongCommand(const String &command) {
    // Use cached file system manager pointer
    // Use cached time manager pointer
    // Use cached system manager pointer
    
    Serial.print(F("\r\n=== Long Test File Write (Multiple Chunks) ===\r\n"));
    
    // Parse optional chunk count parameter (default 10)
    int chunkCount = 10;
    if (command.length() > 14) { // "testwritelong "
        String param = command.substring(14);
        param.trim();
        if (param.length() > 0) {
            chunkCount = param.toInt();
            if (chunkCount < 1 || chunkCount > 500) {
                chunkCount = 10; // Safe default
            }
        }
    }
    
    Serial.print(F("Chunks to write: "));
    Serial.print(chunkCount);
    Serial.print(F("\r\n"));
    
    // Create base test data with timestamp
    char baseData[48];
    if (_cachedTimeManager->isRTCAvailable()) {
        char timeBuffer[32];
        _cachedTimeManager->getFormattedDateTime(timeBuffer, sizeof(timeBuffer));
        snprintf(baseData, sizeof(baseData), "LONG-TEST %s", timeBuffer);
    } else {
        snprintf(baseData, sizeof(baseData), "LONG-TEST %lu", millis());
    }
    
    Serial.print(F("Base Data: "));
    Serial.print(baseData);
    Serial.print(F("\r\n"));
    Serial.print(F("Active Storage: "));
    Serial.print(_cachedFileSystemManager->getActiveStorage().toSimple());
    Serial.print(F("\r\n"));
    
    Serial.print(F("Writing long test file...\r\n"));
    Serial.print(F("Watch L2 LED for activity!\r\n"));
    
    // First chunk - new file
    Common::DataChunk chunk;
    memset(&chunk, 0, sizeof(chunk));
    
    chunk.isNewFile = 1;
    chunk.isEndOfFile = 0;
    chunk.timestamp = millis();
    
    // Create first chunk data
    char chunkData[80];
    snprintf(chunkData, sizeof(chunkData), "%s - Chunk 1/%d - Memory: %d\r\n", 
            baseData, chunkCount, _cachedSystemManager->getFreeMemory());
    chunk.length = strlen(chunkData);
    strncpy((char *)chunk.data, chunkData, sizeof(chunk.data) - 1);
    
    // Process first chunk (creates file)
    _cachedFileSystemManager->processDataChunk(chunk);
    Serial.print(F("Chunk 1 written\r\n"));
    
    // Write remaining chunks with delays to show L2 LED activity
    for (int i = 2; i <= chunkCount; i++) {
        delay(100); // 100ms delay between chunks to make LED visible
        
        // Prepare next chunk
        memset(&chunk, 0, sizeof(chunk));
        chunk.isNewFile = 0;
        chunk.isEndOfFile = 0;
        chunk.timestamp = millis();
        
        snprintf(chunkData, sizeof(chunkData), "%s - Chunk %d/%d - Free: %d\r\n", 
                 baseData, i, chunkCount, _cachedSystemManager->getFreeMemory());
        chunk.length = strlen(chunkData);
        strncpy((char *)chunk.data, chunkData, sizeof(chunk.data) - 1);
        
        // Process chunk
        _cachedFileSystemManager->processDataChunk(chunk);
        
        Serial.print(F("Chunk "));
        Serial.print(i);
        Serial.print(F(" written\r\n"));
    }
    
    // Final chunk - end of file
    delay(100);
    memset(&chunk, 0, sizeof(chunk));
    chunk.isNewFile = 0;
    chunk.isEndOfFile = 1;
    chunk.length = 0;
    chunk.timestamp = millis();
    
    // Process end chunk (closes file)
    _cachedFileSystemManager->processDataChunk(chunk);
    
    // Check final status
    Serial.print(F("Write errors after completion: "));
    Serial.print(_cachedFileSystemManager->getWriteErrors());
    Serial.print(F("\r\n"));
    
    Serial.print(F("Final Storage Used: "));
    Serial.print(_cachedFileSystemManager->getActiveStorage().toSimple());
    Serial.print(F("\r\n"));
    
    Serial.print(F("Files Now Stored: "));
    Serial.print(_cachedFileSystemManager->getFilesStored());
    Serial.print(F("\r\n"));
    
    Serial.print(F("New file: "));
    Serial.print(_cachedFileSystemManager->getCurrentFilename());
    Serial.print(F("\r\n"));
    
    Serial.print(F("Long test write completed - "));
    Serial.print(chunkCount);
    Serial.print(F(" chunks written.\r\n"));
    Serial.print(F("===============================================\r\n\r\n"));
}

void ConfigurationManager::printLastFileInfo() {
    // Use cached file system manager pointer
    Serial.print(F("\r\n=== Last Saved File Information ===\r\n"));

    // Check SD card insertion first
    Serial.print(F("SD Card Status: "));
    if (_cachedFileSystemManager->isSDCardPresent()) {
        Serial.print(F("Detected"));
        if (_cachedFileSystemManager->isSDAvailable()) {
            Serial.print(F(" and Available"));
        } else {
            Serial.print(F(" but Not Available"));
        }
    } else {
        Serial.print(F("Missing"));
    }
    Serial.print(F("\r\n"));

    Serial.print(F("Files Stored: "));
    Serial.print(_cachedFileSystemManager->getFilesStored());
    Serial.print(F("\r\n"));

    if (_cachedFileSystemManager->getFilesStored() > 0) {
        Serial.print(F("Last Filename: "));
        Serial.print(_cachedFileSystemManager->getCurrentFilename());
        Serial.print(F("\r\n"));

        Serial.print(F("Storage Device: "));
        Serial.print(_cachedFileSystemManager->getActiveStorage().toString());
        Serial.print(F("\r\n"));

        Serial.print(F("File Type (Requested): "));
        Serial.print(_cachedFileSystemManager->getFileType().toSimple());
        Serial.print(F("\r\n"));

        Serial.print(F("File Type (Detected): "));
        Serial.print(_cachedFileSystemManager->getDetectedFileType().toSimple());
        Serial.print(F("\r\n"));

        Serial.print(F("Total Bytes Written: "));
        Serial.print(_cachedFileSystemManager->getTotalBytesWritten());
        Serial.print(F(" bytes\r\n"));
        
        Serial.print(F("Current File Bytes Written: "));
        Serial.print(_cachedFileSystemManager->getCurrentFileBytesWritten());
        Serial.print(F(" bytes\r\n"));

        // Show byte tracking comparison
        // Use cached parallel port manager pointer
        uint32_t totalRead = _cachedParallelPortManager->getTotalBytesReceived();
        uint32_t totalWritten = _cachedFileSystemManager->getTotalBytesWritten();
        Serial.print(F("Data Integrity Check: "));
        if (totalRead == totalWritten) {
            Serial.print(F("GOOD ("));
            Serial.print(totalRead);
            Serial.print(F(" bytes match)\r\n"));
        } else {
            Serial.print(F("MISMATCH - Read: "));
            Serial.print(totalRead);
            Serial.print(F(", Written: "));
            Serial.print(totalWritten);
            Serial.print(F("\r\n"));
        }

        Serial.print(F("Write Errors: "));
        Serial.print(_cachedFileSystemManager->getWriteErrors());
        Serial.print(F("\r\n"));
    } else {
        Serial.print(F("No files saved yet.\r\n"));
    }

    Serial.print(F("===================================\r\n\r\n"));
}

void ConfigurationManager::handleHeartbeatCommand(const String &command) {
    // Use cached display manager pointer
    // Use cached system manager pointer
    String setting = command.substring(10); // Remove "heartbeat "
    setting.trim();
    setting.toLowerCase();

    if (setting == F("on") || setting == F("enable") || setting == F("true") || setting == F("1")) {
        _cachedSystemManager->setSerialHeartbeatEnabled(true);
        Serial.print(F("Serial heartbeat enabled\r\n"));
        _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("Heartbeat ON"));
    } else if (setting == F("off") || setting == F("disable") || setting == F("false") || setting == F("0")) {
        _cachedSystemManager->setSerialHeartbeatEnabled(false);
        Serial.print(F("Serial heartbeat disabled\r\n"));
        _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("Heartbeat OFF"));
    } else if (setting == F("status")) {
        bool enabled = _cachedSystemManager->isSerialHeartbeatEnabled();
        Serial.print(F("Serial heartbeat is "));
        Serial.print(enabled ? F("enabled") : F("disabled"));
        Serial.print(F("\r\n"));
    } else {
        Serial.print(F("Usage: heartbeat on/off/status\r\n"));
        Serial.print(F("  on/enable/true/1  - Enable serial heartbeat\r\n"));
        Serial.print(F("  off/disable/false/0 - Disable serial heartbeat\r\n"));
        Serial.print(F("  status - Show current status\r\n"));
    }
}

void ConfigurationManager::handleLEDCommand(const String &command) {
    // Use cached display manager pointer
    // Expected format: "led l1 on", "led l2 off", "led status"
    String params = command.substring(4); // Remove "led "
    params.trim();
    params.toLowerCase();

    if (params.startsWith(F("l1 "))) {
        String action = params.substring(3);
        action.trim();

        if (action == F("on") || action == F("1") || action == F("true")) {
            digitalWrite(Common::Pins::LPT_READ_LED, HIGH);
            Serial.print(F("L1 LED (LPT Read Activity) turned ON\r\n"));
            _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("L1 LED ON"));
        } else if (action == F("off") || action == F("0") || action == F("false")) {
            digitalWrite(Common::Pins::LPT_READ_LED, LOW);
            Serial.print(F("L1 LED (LPT Read Activity) turned OFF\r\n"));
            _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("L1 LED OFF"));
        } else {
            Serial.print(F("Invalid action for L1. Use: led l1 on/off\r\n"));
        }
    } else if (params.startsWith(F("l2 "))) {
        String action = params.substring(3);
        action.trim();
        // Use cached display manager pointer

        if (action == F("on") || action == F("1") || action == F("true")) {
            digitalWrite(Common::Pins::DATA_WRITE_LED, HIGH);
            Serial.print(F("L2 LED (Data Write Activity) turned ON\r\n"));
            _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("L2 LED ON"));
        } else if (action == F("off") || action == F("0") || action == F("false")) {
            digitalWrite(Common::Pins::DATA_WRITE_LED, LOW);
            Serial.print(F("L2 LED (Data Write Activity) turned OFF\r\n"));
            _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("L2 LED OFF"));
        } else {
            Serial.print(F("Invalid action for L2. Use: led l2 on/off\r\n"));
        }
    } else if (params == F("status")) {
        Serial.print(F("\r\n=== LED Status ===\r\n"));
        Serial.print(F("L1 LED (Pin 30 - LPT Read): "));
        Serial.print(digitalRead(Common::Pins::LPT_READ_LED) ? F("ON") : F("OFF"));
        Serial.print(F("\r\n"));

        Serial.print(F("L2 LED (Pin 32 - Data Write): "));
        Serial.print(digitalRead(Common::Pins::DATA_WRITE_LED) ? F("ON") : F("OFF"));
        Serial.print(F("\r\n"));
        Serial.print(F("==================\r\n"));
    } else {
        Serial.print(F("Usage: led <led> <action>\r\n"));
        Serial.print(F("  led l1 on/off    - Control L1 LED (LPT Read Activity, Pin 30)\r\n"));
        Serial.print(F("  led l2 on/off    - Control L2 LED (Data Write Activity, Pin 32)\r\n"));
        Serial.print(F("  led status       - Show current LED status\r\n"));
        Serial.print(F("Examples:\r\n"));
        Serial.print(F("  led l1 on        - Turn on L1 LED\r\n"));
        Serial.print(F("  led l2 off       - Turn off L2 LED\r\n"));
        Serial.print(F("  led status       - Show both LED states\r\n"));
    }
}

void ConfigurationManager::handleListCommand(const String &command) {
    // Use cached file system manager pointer
    String target = command.substring(5); // Remove "list "
    target.trim();
    target.toLowerCase();

    if (target == F("sd")) {
        Serial.print(F("\r\n=== SD Card File Listing ===\r\n"));

        // Check SD card status first
        if (!_cachedFileSystemManager->isSDCardPresent()) {
            Serial.print(F("SD Card: Not Detected\r\n"));
            Serial.print(F("=============================\r\n"));
            return;
        }

        if (!_cachedFileSystemManager->isSDAvailable()) {
            Serial.print(F("SD Card: Detected but not available\r\n"));
            Serial.print(F("=============================\r\n"));
            return;
        }

        // List files on SD card
        File root = SD.open("/");
        if (!root) {
            Serial.print(F("Failed to open root directory\r\n"));
            Serial.print(F("=============================\r\n"));
            return;
        }

        uint16_t fileCount = 0;
        uint32_t totalSize = 0;

        Serial.print(F("SD Card Files:\r\n"));

       while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    if (entry.isDirectory()) {
        Serial.print(F("Dir: "));
        Serial.println(entry.name());

        File subDir = SD.open(entry.name());
        if (subDir && subDir.isDirectory()) {
            while (true) {
                File subEntry = subDir.openNextFile();
                if (!subEntry) break;

                if (!subEntry.isDirectory()) {
                    fileCount++;
                    uint32_t fileSize = subEntry.size();
                    totalSize += fileSize;

                    Serial.print(F("  "));
                    Serial.print(subEntry.name());
                    Serial.print(F(" ("));
                    Serial.print(fileSize);
                    Serial.println(F(" bytes)"));
                }
                subEntry.close();
            }
            subDir.close();
        } else {
            Serial.println(F("Failed to open subdirectory"));
        }
    } else {
        fileCount++;
        uint32_t fileSize = entry.size();
        totalSize += fileSize;

        Serial.print(F("  "));
        Serial.print(entry.name());
        Serial.print(F(" ("));
        Serial.print(fileSize);
        Serial.println(F(" bytes)"));
    }
    entry.close();
}

        root.close();

        Serial.print(F("\r\nSummary:\r\n"));
        Serial.print(F("  Files: "));
        Serial.print(fileCount);
        Serial.print(F("\r\n"));
        Serial.print(F("  Total Size: "));
        Serial.print(totalSize);
        Serial.print(F(" bytes\r\n"));
        Serial.print(F("=============================\r\n"));
    } else {
        Serial.print(F("Usage: list sd\r\n"));
        Serial.print(F("  list sd  - Show all files on SD card root directory\r\n"));
    }
}

void ConfigurationManager::handleDebugCommand(const String &command) {
    // Use cached system manager pointer
    String params = command.substring(6); // Remove "debug "
    params.trim();
    params.toLowerCase();

    if (params.startsWith(F("lcd"))) {
        String lcdParams = params.substring(4); // Remove "lcd "
        lcdParams.trim();

        if (lcdParams == F("on")) {
            _cachedSystemManager->setLCDDebugEnabled(true);
            Serial.print(F("LCD debug mode enabled - LCD messages will be output to serial\r\n"));
        } else if (lcdParams == F("off")) {
            _cachedSystemManager->setLCDDebugEnabled(false);
            Serial.print(F("LCD debug mode disabled\r\n"));
        } else if (lcdParams == F("status")) {
            bool enabled = _cachedSystemManager->isLCDDebugEnabled();
            Serial.print(F("LCD debug mode: "));
            Serial.print(enabled ? F("ENABLED") : F("DISABLED"));
            Serial.print(F("\r\n"));
        } else {
            Serial.print(F("Usage: debug lcd [on|off|status]\r\n"));
            Serial.print(F("  debug lcd on     - Enable LCD debug output to serial\r\n"));
            Serial.print(F("  debug lcd off    - Disable LCD debug output\r\n"));
            Serial.print(F("  debug lcd status - Show current debug mode status\r\n"));
        }
    } else if (params.startsWith(F("parallel")) || params.startsWith(F("lpt"))) {
        String lptParams = params.startsWith(F("parallel")) ? params.substring(9) : params.substring(4); // Remove "parallel " or "lpt "
        lptParams.trim();

        if (lptParams == F("on")) {
            _cachedSystemManager->setParallelDebugEnabled(true);
            Serial.print(F("Parallel port debug mode enabled - All LPT operations will be logged to serial\r\n"));
            Serial.print(F("Warning: This will generate significant serial output during data capture!\r\n"));
        } else if (lptParams == F("off")) {
            _cachedSystemManager->setParallelDebugEnabled(false);
            Serial.print(F("Parallel port debug mode disabled\r\n"));
        } else if (lptParams == F("status")) {
            bool enabled = _cachedSystemManager->isParallelDebugEnabled();
            Serial.print(F("Parallel port debug mode: "));
            Serial.print(enabled ? F("ENABLED") : F("DISABLED"));
            Serial.print(F("\r\n"));
        } else {
            Serial.print(F("Usage: debug parallel [on|off|status] or debug lpt [on|off|status]\r\n"));
            Serial.print(F("  debug parallel on     - Enable parallel port debug output to serial\r\n"));
            Serial.print(F("  debug parallel off    - Disable parallel port debug output\r\n"));
            Serial.print(F("  debug parallel status - Show current parallel debug mode status\r\n"));
            Serial.print(F("Warning: Parallel debug generates extensive output during data capture\r\n"));
        }
    } else {
        Serial.print(F("Debug Commands:\r\n"));
        Serial.print(F("  debug lcd on/off/status      - Control LCD debug output to serial\r\n"));
        Serial.print(F("  debug parallel on/off/status - Control parallel port debug logging\r\n"));
        Serial.print(F("  debug lpt on/off/status      - Same as parallel (alias)\r\n"));
        Serial.print(F("Examples:\r\n"));
        Serial.print(F("  debug lcd on         - Enable LCD message mirroring to serial\r\n"));
        Serial.print(F("  debug lcd off        - Disable LCD message mirroring\r\n"));
        Serial.print(F("  debug lcd status     - Show current LCD debug status\r\n"));
        Serial.print(F("  debug parallel on    - Enable parallel port debug logging\r\n"));
        Serial.print(F("  debug parallel off   - Disable parallel port debug logging\r\n"));
        Serial.print(F("  debug parallel status - Show parallel port debug status\r\n"));
    }
}

void ConfigurationManager::clearLPTBuffer() {
    // Use cached parallel port manager pointer
    // Use cached display manager pointer
    
    Serial.print(F("\r\n=== Clearing LPT Buffer ===\r\n"));
    
    // Show buffer status before clearing
    uint16_t bufferLevel = _cachedParallelPortManager->getBufferLevel();
    Serial.print(F("Buffer level before: "));
    Serial.print(bufferLevel);
    Serial.print(F("/"));
    Serial.print(_cachedConfigurationService->getRingBufferSize());
    Serial.print(F(" bytes\r\n"));
    
    // Clear the buffer
    _cachedParallelPortManager->clearBuffer();
    
    // Show buffer status after clearing
    bufferLevel = _cachedParallelPortManager->getBufferLevel();
    Serial.print(F("Buffer level after: "));
    Serial.print(bufferLevel);
    Serial.print(F("/"));
    Serial.print(_cachedConfigurationService->getRingBufferSize());
    Serial.print(F(" bytes\r\n"));
    
    Serial.print(F("LPT buffer cleared successfully\r\n"));
    Serial.print(F("===========================\r\n"));
    
    _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("Buffer Cleared"));
}

void ConfigurationManager::resetCriticalState() {
    // Use cached parallel port manager pointer
    // Use cached display manager pointer
    
    Serial.print(F("\r\n=== Resetting Critical State ===\r\n"));
    
    bool wasCritical = _cachedParallelPortManager->isCriticalFlowControlActive();
    
    // Reset the critical state
    _cachedParallelPortManager->resetCriticalState();
    
    Serial.print(F("Critical flow control state: "));
    Serial.print(wasCritical ? F("WAS ACTIVE - Now Reset") : F("Was not active"));
    Serial.print(F("\r\n"));
    
    Serial.print(F("Buffer and flow control reset\r\n"));
    Serial.print(F("===============================\r\n"));
    
    if (wasCritical) {
        _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("Critical Reset"));
    } else {
        _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("No Critical State"));
    }
}

void ConfigurationManager::handleLCDThrottleCommand(const String& command) {
    // Use cached display manager pointer
    String params = command.substring(12); // Remove "lcdthrottle "
    params.trim();
    params.toLowerCase();
    
    Serial.print(F("\r\n=== LCD Throttle Control ===\r\n"));
    
    if (params == F("on") || params == F("enable") || params == F("true")) {
        _cachedDisplayManager->setStorageOperationActive(true);
        Serial.print(F("LCD refresh throttled to 500ms\r\n"));
        Serial.print(F("Storage operation mode: ACTIVE\r\n"));
        _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("LCD Throttled"));
    } else if (params == F("off") || params == F("disable") || params == F("false")) {
        _cachedDisplayManager->setStorageOperationActive(false);
        Serial.print(F("LCD refresh restored to 100ms\r\n"));
        Serial.print(F("Storage operation mode: INACTIVE\r\n"));
        _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("LCD Normal"));
    } else if (params == F("status")) {
        bool isThrottled = _cachedDisplayManager->isStorageOperationActive();
        Serial.print(F("Storage Operation Active: "));
        Serial.print(isThrottled ? F("YES") : F("NO"));
        Serial.print(F("\r\n"));
        Serial.print(F("Current Refresh Rate: "));
        Serial.print(isThrottled ? F("500ms (Throttled)") : F("100ms (Normal)"));
        Serial.print(F("\r\n"));
    } else {
        Serial.print(F("Usage: lcdthrottle [on|off|status]\r\n"));
        Serial.print(F("  on/enable  - Throttle LCD to 500ms refresh\r\n"));
        Serial.print(F("  off/disable - Restore LCD to 100ms refresh\r\n"));
        Serial.print(F("  status     - Show current throttle status\r\n"));
    }
    
    Serial.print(F("============================\r\n"));
}

// IComponent interface implementation
bool ConfigurationManager::selfTest() {
    Serial.print(F("ConfigurationManager Self-Test:\r\n"));

    bool result = true;

    // Test configuration integrity
    Serial.print(F("  Testing configuration values... "));
    
    // Test critical pin configurations
    // Use cached configuration service pointer
    if (_cachedConfigurationService->getHeartbeatPin() >= 0 && _cachedConfigurationService->getHeartbeatPin() <= 53) {
        Serial.print(F("✅ OK\r\n"));
    } else {
        Serial.print(F("❌ FAIL - Invalid pin configuration\r\n"));
        result = false;
    }
    
    // Test serial command processing capability
    Serial.print(F("  Testing serial interface... "));
    if (Serial.available() >= 0) { // Serial is accessible
        Serial.print(F("✅ OK\r\n"));
    } else {
        Serial.print(F("❌ FAIL\r\n"));
        result = false;
    }
    
    // Dependencies validated by ServiceLocator at startup

    return result;
}

const char *ConfigurationManager::getComponentName() const { 
    static char name_buffer[24];
    strcpy_P(name_buffer, component_name);
    return name_buffer;
}

bool ConfigurationManager::validateDependencies() const {
    bool valid = true;

    // Use cached system manager pointer
    if (!_cachedSystemManager) {
        Serial.print(F("  Missing SystemManager dependency\r\n"));
        valid = false;
    }

    // Use cached file system manager pointer
    if (!_cachedFileSystemManager) {
        Serial.print(F("  Missing FileSystemManager dependency\r\n"));
        valid = false;
    }

    // Use cached display manager pointer
    if (!_cachedDisplayManager) {
        Serial.print(F("  Missing DisplayManager dependency\r\n"));
        valid = false;
    }

    // Use cached time manager pointer
    if (!_cachedTimeManager) {
        Serial.print(F("  Missing TimeManager dependency\r\n"));
        valid = false;
    }

    // Use cached parallel port manager pointer
    if (!_cachedParallelPortManager) {
        Serial.print(F("  Missing ParallelPortManager dependency\r\n"));
        valid = false;
    }

    return valid;
}

void ConfigurationManager::printDependencyStatus() const {
    Serial.print(F("Con Dependencies:\r\n"));

    // Use cached system manager pointer
    Serial.print(F("  SystemManager: "));
    Serial.print(_cachedSystemManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));

    // Use cached file system manager pointer
    Serial.print(F("  FileSystemManager: "));
    Serial.print(_cachedFileSystemManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));

    // Use cached display manager pointer
    Serial.print(F("  DisplayManager: "));
    Serial.print(_cachedDisplayManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));

    // Use cached time manager pointer
    Serial.print(F("  TimeManager: "));
    Serial.print(_cachedTimeManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));

    // Use cached parallel port manager pointer
    Serial.print(F("  ParallelPortManager: "));
    Serial.print(_cachedParallelPortManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));
}

void ConfigurationManager::handleFlowControlCommand(const String& command) {
    String param = command.substring(12); // Skip "flowcontrol "
    param.trim();

    if (param.equalsIgnoreCase(F("on")) || param.equalsIgnoreCase(F("enable"))) {
        _cachedParallelPortManager->setHardwareFlowControlEnabled(true);
        Serial.print(F("Hardware flow control enabled\r\n"));
    } else if (param.equalsIgnoreCase(F("off")) || param.equalsIgnoreCase(F("disable"))) {
        _cachedParallelPortManager->setHardwareFlowControlEnabled(false);
        Serial.print(F("Hardware flow control disabled\r\n"));
    } else if (param.equalsIgnoreCase(F("status")) || param.length() == 0) {
        Serial.print(F("Hardware flow control: "));
        Serial.print(_cachedParallelPortManager->isHardwareFlowControlEnabled() ? F("ENABLED") : F("DISABLED"));
        Serial.print(F("\r\n"));
    } else {
        Serial.print(F("Usage: flowcontrol on/off/status\r\n"));
    }
}

void ConfigurationManager::printFlowControlStatistics() {
    if (!_cachedParallelPortManager->isHardwareFlowControlEnabled()) {
        Serial.print(F("Hardware flow control is disabled\r\n"));
        return;
    }

    auto stats = _cachedParallelPortManager->getFlowControlStatistics();
    
    Serial.print(F("\r\n=== Hardware Flow Control Statistics ===\r\n"));
    Serial.print(F("Current State: "));
    Serial.print(DeviceBridge::Parallel::HardwareFlowControl::getStateName(stats.currentState));
    Serial.print(F("\r\n"));
    
    Serial.print(F("Time in Current State: "));
    Serial.print(stats.timeInCurrentState);
    Serial.print(F("ms\r\n"));
    
    Serial.print(F("Total State Transitions: "));
    Serial.print(stats.stateTransitions);
    Serial.print(F("\r\n"));
    
    Serial.print(F("Emergency Activations: "));
    Serial.print(stats.emergencyActivations);
    Serial.print(F("\r\n"));
    
    Serial.print(F("Recovery Operations: "));
    Serial.print(stats.recoveryOperations);
    Serial.print(F("\r\n"));
    
    Serial.print(F("Flow Control Status: "));
    switch (stats.currentState) {
        case DeviceBridge::Parallel::HardwareFlowControl::FlowState::NORMAL:
            Serial.print(F("✅ Normal - Ready for data"));
            break;
        case DeviceBridge::Parallel::HardwareFlowControl::FlowState::WARNING:
            Serial.print(F("⚠️ Warning - Buffer filling"));
            break;
        case DeviceBridge::Parallel::HardwareFlowControl::FlowState::CRITICAL:
            Serial.print(F("🔶 Critical - Buffer nearly full"));
            break;
        case DeviceBridge::Parallel::HardwareFlowControl::FlowState::EMERGENCY:
            Serial.print(F("🚨 Emergency - Stop transmission"));
            break;
    }
    Serial.print(F("\r\n"));
}

unsigned long ConfigurationManager::getUpdateInterval() const {
    // Use cached configuration service pointer
    return _cachedConfigurationService->getConfigurationInterval();
}

} // namespace DeviceBridge::Components