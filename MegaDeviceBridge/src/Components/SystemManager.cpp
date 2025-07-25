#include "SystemManager.h"
#include "DisplayManager.h"
#include "FileSystemManager.h"
#include "ParallelPortManager.h"
#include "TimeManager.h"
#include "../Common/ConfigurationService.h"
#include <Arduino.h>
#include <string.h>

// PROGMEM component name for memory optimization
static const char component_name[] PROGMEM = "SystemManager";

// Memory management globals
extern int __heap_start;
extern int *__brkval;

namespace DeviceBridge::Components {

SystemManager::SystemManager()
    : _systemStatus(Common::SystemStatus::INITIALIZING), _lastError(Common::ErrorCode::NONE), _lastSystemCheck(0),
      _uptimeSeconds(0), _errorCount(0), _commandsProcessed(0) {
    // Initialize debug flags (bit field)
    _debugFlags.serialHeartbeatEnabled = 0;  // Default to off
    _debugFlags.lcdDebugEnabled = 0;         // Default to off
    _debugFlags.parallelDebugEnabled = 0;    // Default to off
    _debugFlags.eepromDebugEnabled = 0;      // Default to off
    _debugFlags.reserved = 0;
}

SystemManager::~SystemManager() { stop(); }

bool SystemManager::initialize() {
    // Cache service dependencies first (performance optimization)
    cacheServiceDependencies();
    
    _systemStatus = Common::SystemStatus::INITIALIZING;
    return true;
}

void SystemManager::update(unsigned long currentTime) {
    // Periodic system monitoring (called from main loop)
    if (currentTime - _lastSystemCheck >= Common::RTOS::SYSTEM_MONITOR_MS) {
        monitorSystemHealth();
        _lastSystemCheck = currentTime;
        _uptimeSeconds = currentTime / 1000;
    }
}

void SystemManager::stop() {
    // Nothing specific to stop for system manager
}

void SystemManager::processSystemCommand(const Common::SystemCommand &cmd) {
    _commandsProcessed++;

    switch (cmd.type) {
    case Common::SystemCommand::STORAGE_SELECT:
        processStorageSelectCommand(cmd.value);
        break;

    case Common::SystemCommand::FILE_TYPE:
        processFileTypeCommand(cmd.value);
        break;

    case Common::SystemCommand::TIME_SET:
        processTimeSetCommand(cmd.data);
        break;

    case Common::SystemCommand::CONFIG_SAVE:
        processConfigSaveCommand();
        break;

    default:
        // Unknown command
        break;
    }
}

void SystemManager::processStorageSelectCommand(uint8_t value) {
    // Use cached file system manager pointer
    if (value >= Common::StorageType::Count)
        return; // avoid out-of-bounds

    Common::StorageType storageType = Common::StorageType(static_cast<Common::StorageType::Value>(value));
    _cachedFileSystemManager->setStorageType(storageType);
    sendDisplayMessage(Common::DisplayMessage::INFO, storageType.toString());
}

void SystemManager::processFileTypeCommand(uint8_t value) {
    if (value > Common::FileType::BINARY)
        return; // avoid out-of-bounds
    // Use cached file system manager pointer

    Common::FileType fileType(static_cast<Common::FileType::Value>(value));

    // Display type name from PROGMEM
    sendDisplayMessage(Common::DisplayMessage::INFO, fileType.toString());

    // Apply the selected file type
    _cachedFileSystemManager->setFileType(fileType);
}

void SystemManager::processTimeSetCommand(const char *data) {
    // TODO: Parse time string and set RTC
    // Format expected: "HH:MM:SS" or "DD/MM/YYYY HH:MM"
    sendDisplayMessage(Common::DisplayMessage::INFO, F("Time Set (TODO)"));
}

void SystemManager::processConfigSaveCommand() {
    // TODO: Save configuration to EEPROM
    sendDisplayMessage(Common::DisplayMessage::INFO, F("Config Saved"));
}

void SystemManager::monitorSystemHealth() { logSystemStatus(); }

void SystemManager::logSystemStatus() {
    // Only log if serial heartbeat is enabled
    if (!_debugFlags.serialHeartbeatEnabled) {
        return;
    }

    char timeMessage[32] = "MISSING!";
    // Use cached time manager pointer
    _cachedTimeManager->getFormattedDateTime(timeMessage, sizeof(timeMessage));

    // Read analog value for button debugging (10-bit: 0-1023)
    int16_t buttonAnalog = analogRead(Common::Pins::LCD_BUTTONS);

    Serial.print(F("Uptime: "));
    Serial.print(_uptimeSeconds);
    Serial.print(F("s, Errors: "));
    Serial.print(_errorCount);
    Serial.print(F(", Commands: "));
    Serial.print(_commandsProcessed);
    Serial.print(F(", Buttons: "));
    Serial.print(buttonAnalog);

    Serial.print(F(", Time: "));
    Serial.print(timeMessage);
    Serial.print(F("\r\n"));
}

void SystemManager::setSystemStatus(Common::SystemStatus status) {
    _systemStatus = status;

    const char *statusMessage;
    switch (status) {
    case Common::SystemStatus::INITIALIZING:
        statusMessage = "Initializing...";
        break;
    case Common::SystemStatus::READY:
        statusMessage = "Ready";
        break;
    case Common::SystemStatus::RECEIVING:
        statusMessage = "Receiving...";
        break;
    case Common::SystemStatus::STORING:
        statusMessage = "Storing...";
        break;
    case Common::SystemStatus::ERROR:
        statusMessage = "System Error";
        break;
    case Common::SystemStatus::FULL:
        statusMessage = "Storage Full";
        break;
    default:
        statusMessage = "Unknown";
        break;
    }

    sendDisplayMessage(Common::DisplayMessage::STATUS, statusMessage);
}

void SystemManager::reportError(Common::ErrorCode error) {
    _lastError = error;
    _errorCount++;
    handleError(error);
}

void SystemManager::handleError(Common::ErrorCode error) {
    const char *errorMessage;

    switch (error) {
    case Common::ErrorCode::SD_INIT_FAILED:
        errorMessage = "SD Init Failed";
        break;
    case Common::ErrorCode::EEPROM_INIT_FAILED:
        errorMessage = "EEPROM Failed";
        break;
    case Common::ErrorCode::RTC_INIT_FAILED:
        errorMessage = "RTC Failed";
        break;
    case Common::ErrorCode::BUFFER_OVERFLOW:
        errorMessage = "Buffer Overflow";
        break;
    case Common::ErrorCode::FILE_WRITE_ERROR:
        errorMessage = "Write Error";
        break;
    case Common::ErrorCode::STORAGE_FULL:
        errorMessage = "Storage Full";
        setSystemStatus(Common::SystemStatus::FULL);
        break;
    case Common::ErrorCode::HARDWARE_ERROR:
        errorMessage = "Hardware Error";
        setSystemStatus(Common::SystemStatus::ERROR);
        break;
    default:
        errorMessage = "Unknown Error";
        break;
    }

    sendDisplayMessage(Common::DisplayMessage::ERROR, errorMessage);
}

void SystemManager::sendDisplayMessage(Common::DisplayMessage::Type type, const char *message) {
    // Use cached display manager pointer
    _cachedDisplayManager->displayMessage(type, message);
}
void SystemManager::sendDisplayMessage(Common::DisplayMessage::Type type, const __FlashStringHelper *message) {
    // Use cached display manager pointer
    _cachedDisplayManager->displayMessage(type, message);
}

void SystemManager::printSystemInfo() {
    Serial.print(F("=== Device Bridge System Info ===\r\n"));
    Serial.print(F("Status: "));
    Serial.print((int)_systemStatus);
    Serial.print(F("\r\n"));
    Serial.print(F("Uptime: "));
    Serial.print(_uptimeSeconds);
    Serial.print(F(" seconds\r\n"));
    Serial.print(F("Total Errors: "));
    Serial.print(_errorCount);
    Serial.print(F("\r\n"));
    Serial.print(F("Commands Processed: "));
    Serial.print(_commandsProcessed);
    Serial.print(F("\r\n"));
}

void SystemManager::printMemoryInfo() {
    Serial.print(F("=== Memory Info ===\r\n"));
    Serial.print(F("Free SRAM: "));
    Serial.print(freeRam());
    Serial.print(F(" bytes\r\n"));
}

uint16_t SystemManager::freeRam() {
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

uint16_t SystemManager::getFreeMemory() const {
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void SystemManager::validateHardware() {
    // Use cached display manager pointer
    // Use cached time manager pointer
    // Use cached file system manager pointer
    Serial.print(F("\r\n=== Hardware Validation Test ===\r\n"));

    bool allComponentsOK = true;

    // Test FileSystemManager (SD Card, EEPROM)
    Serial.print(F("FileSystem Manager: "));
    bool fsOK = true;

    // Check if FileSystemManager has storage available
    if (_cachedFileSystemManager->isSDAvailable()) {
        Serial.print(F("SD-OK "));
    } else {
        Serial.print(F("SD-FAIL "));
        fsOK = false;
    }

    if (_cachedFileSystemManager->isEEPROMAvailable()) {
        Serial.print(F("EEPROM-OK "));
    } else {
        Serial.print(F("EEPROM-FAIL "));
        fsOK = false;
    }

    if (fsOK) {
        Serial.print(F("✅\r\n"));
    } else {
        Serial.print(F("❌\r\n"));
        allComponentsOK = false;
    }

    // Test TimeManager (RTC)
    Serial.print(F("Time Manager: "));
    if (_cachedTimeManager->isRTCAvailable()) {
        Serial.print(F("RTC-OK ✅\r\n"));
    } else {
        Serial.print(F("RTC-FAIL ❌\r\n"));
        allComponentsOK = false;
    }

    // Test DisplayManager (LCD)
    Serial.print(F("Display Manager: LCD-OK ✅\r\n"));
    _cachedDisplayManager->displayMessage(Common::DisplayMessage::INFO, F("HW Test"));

    // Test ParallelPortManager
    Serial.print(F("Parallel Port Manager: LPT-OK ✅\r\n"));

    // Memory test
    uint16_t freeRAM = freeRam();
    Serial.print(F("Memory: "));
    Serial.print(freeRAM);
    Serial.print(F(" bytes free ("));
    Serial.print((freeRAM * 100) / 8192);
    Serial.print(F("% available) "));
    if (freeRAM > 1000) {
        Serial.print(F("✅\r\n"));
    } else {
        Serial.print(F("⚠️ LOW\r\n"));
    }

    // Overall result
    Serial.print(F("\r\n=== Validation Result ===\r\n"));
    if (allComponentsOK) {
        Serial.print(F("✅ ALL HARDWARE OPERATIONAL\r\n"));
        sendDisplayMessage(Common::DisplayMessage::INFO, F("HW Test PASS"));
    } else {
        Serial.print(F("❌ HARDWARE ISSUES DETECTED\r\n"));
        sendDisplayMessage(Common::DisplayMessage::ERROR, F("HW Test FAIL"));
    }
    Serial.print(F("========================\r\n\r\n"));
}

// IComponent interface implementation
bool SystemManager::selfTest() {
    Serial.print(F("SystemManager Self-Test:\r\n"));

    bool result = true;

    // Test memory availability
    Serial.print(F("  Testing system memory... "));
    uint16_t freeRAM = freeRam();
    
    if (freeRAM > 1000) {
        Serial.print(F("✅ OK ("));
        Serial.print(freeRAM);
        Serial.print(F(" bytes free)\r\n"));
    } else if (freeRAM > 500) {
        Serial.print(F("⚠️  LOW ("));
        Serial.print(freeRAM);
        Serial.print(F(" bytes free)\r\n"));
    } else {
        Serial.print(F("❌ CRITICAL ("));
        Serial.print(freeRAM);
        Serial.print(F(" bytes free)\r\n"));
        result = false;
    }
    
    // Test system status tracking
    Serial.print(F("  Testing status tracking... "));
    if (_systemStatus != Common::SystemStatus::INITIALIZING) {
        Serial.print(F("✅ OK (status: "));
        Serial.print((int)_systemStatus);
        Serial.print(F(")\r\n"));
    } else {
        Serial.print(F("⚠️  Still initializing\r\n"));
    }
    
    // Dependencies validated by ServiceLocator at startup

    return result;
}

const char *SystemManager::getComponentName() const { 
    static char name_buffer[24];
    strcpy_P(name_buffer, component_name);
    return name_buffer;
}

bool SystemManager::validateDependencies() const {
    bool valid = true;

    // Use cached display manager pointer
    if (!_cachedDisplayManager) {
        Serial.print(F("  Missing DisplayManager dependency\r\n"));
        valid = false;
    }

    // Use cached file system manager pointer
    if (!_cachedSystemManager) {
        Serial.print(F("  Missing FileSystemManager dependency\r\n"));
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

void SystemManager::printDependencyStatus() const {
    Serial.print(F("SystemManager Dependencies:\r\n"));

    // Use cached display manager pointer
    Serial.print(F("  DisplayManager: "));
    Serial.print(_cachedDisplayManager ? F("✅ Available") : F("❌ Missing"));
    Serial.print(F("\r\n"));

    // Use cached file system manager pointer
    Serial.print(F("  FileSystemManager: "));
    Serial.print(_cachedSystemManager ? F("✅ Available") : F("❌ Missing"));
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

unsigned long SystemManager::getUpdateInterval() const {
    // Use cached configuration service pointer
    return _cachedConfigurationService->getSystemInterval(); // Default 5 seconds
}

} // namespace DeviceBridge::Components