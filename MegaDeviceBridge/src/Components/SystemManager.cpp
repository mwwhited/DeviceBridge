#include "SystemManager.h"
#include "ParallelPortManager.h"
#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "TimeManager.h"
#include <string.h>
#include <Arduino.h>

namespace DeviceBridge::Components {

SystemManager::SystemManager()
    : _parallelPortManager(nullptr)
    , _fileSystemManager(nullptr)
    , _displayManager(nullptr)
    , _timeManager(nullptr)
    , _systemStatus(Common::SystemStatus::INITIALIZING)
    , _lastError(Common::ErrorCode::NONE)
    , _lastSystemCheck(0)
    , _uptimeSeconds(0)
    , _errorCount(0)
    , _commandsProcessed(0)
{
}

SystemManager::~SystemManager() {
    stop();
}

bool SystemManager::initialize() {
    _systemStatus = Common::SystemStatus::INITIALIZING;
    return true;
}

void SystemManager::update() {
    // Periodic system monitoring (called from main loop)
    uint32_t currentTime = millis();
    if (currentTime - _lastSystemCheck >= Common::RTOS::SYSTEM_MONITOR_MS) {
        monitorSystemHealth();
        _lastSystemCheck = currentTime;
        _uptimeSeconds = currentTime / 1000;
    }
}

void SystemManager::stop() {
    // Nothing specific to stop for system manager
}

void SystemManager::processSystemCommand(const Common::SystemCommand& cmd) {
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
    if (_fileSystemManager == nullptr) return;
    if (value >= Common::StorageType::Count) return;  // avoid out-of-bounds
    
    Common::StorageType storageType = Common::StorageType(static_cast<Common::StorageType::Value>(value));    
    _fileSystemManager->setStorageType(storageType);
    sendDisplayMessage(Common::DisplayMessage::INFO, storageType.toString());
}

void SystemManager::processFileTypeCommand(uint8_t value) {
    if (_fileSystemManager == nullptr) return;

    if (value > Common::FileType::BINARY) return;  // avoid out-of-bounds

    Common::FileType fileType(static_cast<Common::FileType::Value>(value));
    
    // Display type name from PROGMEM
    sendDisplayMessage(Common::DisplayMessage::INFO, fileType.toString());

    // Apply the selected file type
    _fileSystemManager->setFileType(fileType);
}

void SystemManager::processTimeSetCommand(const char* data) {
    // TODO: Parse time string and set RTC
    // Format expected: "HH:MM:SS" or "DD/MM/YYYY HH:MM"
    sendDisplayMessage(Common::DisplayMessage::INFO, F("Time Set (TODO)"));
}

void SystemManager::processConfigSaveCommand() {
    // TODO: Save configuration to EEPROM
    sendDisplayMessage(Common::DisplayMessage::INFO, F("Config Saved"));
}

void SystemManager::monitorSystemHealth() {
    logSystemStatus();
}

void SystemManager::logSystemStatus() {
    Serial.print(F("Uptime: "));
    Serial.print(_uptimeSeconds);
    Serial.print(F("s, Errors: "));
    Serial.print(_errorCount);
    Serial.print(F(", Commands: "));
    Serial.print(_commandsProcessed);
    Serial.print(F("\r\n"));
}

void SystemManager::setSystemStatus(Common::SystemStatus status) {
    _systemStatus = status;
    
    const char* statusMessage;
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
    const char* errorMessage;
    
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

void SystemManager::sendDisplayMessage(Common::DisplayMessage::Type type, const char* message) {
    if (_displayManager) {
        _displayManager->displayMessage(type, message);
    }
}
void SystemManager::sendDisplayMessage(Common::DisplayMessage::Type type, const __FlashStringHelper* message) {
    if (_displayManager) {
        _displayManager->displayMessage(type, message);
    }
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
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}


void SystemManager::setComponentManagers(
    ParallelPortManager* parallelPort,
    FileSystemManager* fileSystem,
    DisplayManager* display,
    TimeManager* time
) {
    _parallelPortManager = parallelPort;
    _fileSystemManager = fileSystem;
    _displayManager = display;
    _timeManager = time;
}

} // namespace DeviceBridge::Components