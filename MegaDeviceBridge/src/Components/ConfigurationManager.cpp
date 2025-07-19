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
        
    } else if (command.equalsIgnoreCase(F("time"))) {
        printCurrentTime();
        
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
    Serial.print(F("\r\nStorage Commands:\r\n"));
    Serial.print(F("  storage sd        - Use SD card storage\r\n"));
    Serial.print(F("  storage eeprom    - Use EEPROM storage\r\n"));
    Serial.print(F("  storage serial    - Use serial transfer\r\n"));
    Serial.print(F("  storage auto      - Auto-select storage\r\n"));
    Serial.print(F("\r\nSystem Commands:\r\n"));
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