#include "DisplayManager.h"
#include "TimeManager.h"
#include "SystemManager.h"
#include "../Common/Types.h"
#include <string.h>
#include <Arduino.h>

namespace DeviceBridge::Components {

// Button constants for OSEPP LCD Keypad Shield v1 (verified specifications)
constexpr uint16_t BUTTON_RIGHT = 0;
constexpr uint16_t BUTTON_UP = 144;
constexpr uint16_t BUTTON_DOWN = 329;
constexpr uint16_t BUTTON_LEFT = 504;
constexpr uint16_t BUTTON_SELECT = 741;
constexpr uint16_t BUTTON_NONE = 1023;

DisplayManager::DisplayManager(User::Display& display)
    : _display(display)
    , _timeManager(nullptr)
    , _systemManager(nullptr)
    , _lastMessageTime(0)
    , _showingTime(false)
    , _inMenu(false)
    , _menuState(MAIN_MENU)
    , _menuSelection(0)
    , _lastButtonTime(0)
    , _lastButtonState(BUTTON_NONE)
{
    memset(_currentMessage, 0, sizeof(_currentMessage));
    memset(_currentLine2, 0, sizeof(_currentLine2));
    strcpy(_currentMessage, "Ready");
}

DisplayManager::~DisplayManager() {
    stop();
}

bool DisplayManager::initialize() {
    // Display should already be initialized by main
    return true;
}

void DisplayManager::update() {
    // Check for button presses
    uint16_t buttonValue = readButtons();
    if (buttonValue != BUTTON_NONE && buttonValue != _lastButtonState) {
        handleButtonPress(buttonValue);
        _lastButtonState = buttonValue;
        _lastButtonTime = millis();
    } else if (buttonValue == BUTTON_NONE) {
        _lastButtonState = BUTTON_NONE;
    }
    
    // Update display content
    updateDisplay();
}

void DisplayManager::stop() {
    _inMenu = false;
    _showingTime = false;
}

void DisplayManager::updateDisplay() {
    // Check if we should show time when idle
    if (!_inMenu && (millis() - _lastMessageTime) > Common::Display::IDLE_TIME_MS) {
        if (!_showingTime) {
            _showingTime = true;
            if (_timeManager) {
                char timeStr[32];
                _timeManager->getFormattedTime(timeStr, sizeof(timeStr));
                showTimeDisplay(timeStr);
            }
        }
    }
}

void DisplayManager::processMessage(const Common::DisplayMessage& msg) {
    _lastMessageTime = millis();
    _showingTime = false;
    
    switch (msg.type) {
        case Common::DisplayMessage::STATUS:
            showMessage(msg.message, msg.line2);
            break;
        case Common::DisplayMessage::ERROR:
            showError(msg.message);
            break;
        case Common::DisplayMessage::INFO:
            showMessage(msg.message, msg.line2);
            break;
        case Common::DisplayMessage::TIME:
            showTimeDisplay(msg.message);
            break;
        case Common::DisplayMessage::MENU:
            showMenuScreen();
            break;
    }
}

void DisplayManager::showMainScreen() {
    _display.clear();
    _display.setCursor(0, 0);
    _display.print(_currentMessage);
    
    if (strlen(_currentLine2) > 0) {
        _display.setCursor(0, 1);
        _display.print(_currentLine2);
    }
}

void DisplayManager::showTimeDisplay(const char* timeStr) {
    _display.clear();
    _display.setCursor(0, 0);
    _display.print(F("Device Bridge"));
    _display.setCursor(0, 1);
    _display.print(timeStr);
}

void DisplayManager::showMenuScreen() {
    _display.clear();
    _display.setCursor(0, 0);
    _display.print(getMenuTitle(_menuState));
    _display.setCursor(0, 1);
    _display.print(F(">"));
    _display.print(getMenuOption(_menuState, _menuSelection));
}

uint16_t DisplayManager::readButtons() {
    uint32_t currentTime = millis();
    
    // Debounce buttons
    if (currentTime - _lastButtonTime < BUTTON_DEBOUNCE_MS) {
        return _lastButtonState;
    }
    
    uint16_t buttonValue = analogRead(A0);
    
    // Tolerance for analog readings
    if (buttonValue < 50) return BUTTON_RIGHT;
    if (buttonValue < 200) return BUTTON_UP;
    if (buttonValue < 400) return BUTTON_DOWN;
    if (buttonValue < 600) return BUTTON_LEFT;
    if (buttonValue < 800) return BUTTON_SELECT;
    
    return BUTTON_NONE;
}

void DisplayManager::handleButtonPress(uint16_t button) {
    if (_inMenu) {
        navigateMenu(button);
    } else {
        // Enter menu on any button press when not in menu
        enterMenu();
    }
}

void DisplayManager::navigateMenu(uint16_t button) {
    switch (button) {
        case BUTTON_UP:
            if (_menuSelection > 0) {
                _menuSelection--;
            }
            showMenuScreen();
            break;
            
        case BUTTON_DOWN:
            if (_menuSelection < getMenuOptionCount(_menuState) - 1) {
                _menuSelection++;
            }
            showMenuScreen();
            break;
            
        case BUTTON_SELECT:
            executeMenuSelection();
            break;
            
        case BUTTON_LEFT:
            exitMenu();
            break;
    }
}

void DisplayManager::executeMenuSelection() {
    switch (_menuState) {
        case MAIN_MENU:
            switch (_menuSelection) {
                case 0: // Storage
                    _menuState = STORAGE_MENU;
                    _menuSelection = 0;
                    break;
                case 1: // File Type
                    _menuState = FILETYPE_MENU;
                    _menuSelection = 0;
                    break;
                case 2: // Config
                    _menuState = CONFIG_MENU;
                    _menuSelection = 0;
                    break;
            }
            showMenuScreen();
            break;
            
        case STORAGE_MENU:
            sendCommand(Common::SystemCommand::STORAGE_SELECT, _menuSelection);
            exitMenu();
            break;
            
        case FILETYPE_MENU:
            sendCommand(Common::SystemCommand::FILE_TYPE, _menuSelection);
            exitMenu();
            break;
            
        case CONFIG_MENU:
            sendCommand(Common::SystemCommand::CONFIG_SAVE, _menuSelection);
            exitMenu();
            break;
    }
}

void DisplayManager::sendCommand(Common::SystemCommand::Type type, uint8_t value, const char* data) {
    if (_systemManager) {
        Common::SystemCommand cmd;
        cmd.type = type;
        cmd.value = value;
        if (data) {
            strncpy(cmd.data, data, sizeof(cmd.data) - 1);
            cmd.data[sizeof(cmd.data) - 1] = '\0';
        } else {
            cmd.data[0] = '\0';
        }
        
        _systemManager->processSystemCommand(cmd);
    }
}

void DisplayManager::showMessage(const char* message, const char* line2) {
    strncpy(_currentMessage, message, sizeof(_currentMessage) - 1);
    _currentMessage[sizeof(_currentMessage) - 1] = '\0';
    
    if (line2) {
        strncpy(_currentLine2, line2, sizeof(_currentLine2) - 1);
        _currentLine2[sizeof(_currentLine2) - 1] = '\0';
    } else {
        _currentLine2[0] = '\0';
    }
    
    _lastMessageTime = millis();
    _showingTime = false;
    
    if (!_inMenu) {
        showMainScreen();
    }
}

void DisplayManager::showError(const char* error) {
    _display.clear();
    _display.setCursor(0, 0);
    _display.print(F("ERROR:"));
    _display.setCursor(0, 1);
    _display.print(error);
    
    _lastMessageTime = millis();
    _showingTime = false;
}

void DisplayManager::showStatus(const char* status) {
    showMessage(status);
}

void DisplayManager::enterMenu() {
    _inMenu = true;
    _menuState = MAIN_MENU;
    _menuSelection = 0;
    showMenuScreen();
}

void DisplayManager::exitMenu() {
    _inMenu = false;
    showMainScreen();
}

void DisplayManager::displayMessage(Common::DisplayMessage::Type type, const char* message, const char* line2) {
    Common::DisplayMessage msg;
    msg.type = type;
    strncpy(msg.message, message, sizeof(msg.message) - 1);
    msg.message[sizeof(msg.message) - 1] = '\0';
    
    if (line2) {
        strncpy(msg.line2, line2, sizeof(msg.line2) - 1);
        msg.line2[sizeof(msg.line2) - 1] = '\0';
    } else {
        msg.line2[0] = '\0';
    }
    
    processMessage(msg);
}
void DisplayManager::displayMessage(Common::DisplayMessage::Type type, const __FlashStringHelper* message, const __FlashStringHelper* line2) {
    Common::DisplayMessage msg;
    msg.type = type;

    // Copy from flash (PROGMEM) to RAM buffer
    strncpy_P(msg.message, (PGM_P)message, sizeof(msg.message) - 1);
    msg.message[sizeof(msg.message) - 1] = '\0';

    if (line2) {
        strncpy_P(msg.line2, (PGM_P)line2, sizeof(msg.line2) - 1);
        msg.line2[sizeof(msg.line2) - 1] = '\0';
    } else {
        msg.line2[0] = '\0';
    }

    processMessage(msg);
}

const char* DisplayManager::getMenuTitle(MenuState state) {
    switch (state) {
        case MAIN_MENU: return "Main Menu";
        case STORAGE_MENU: return "Storage";
        case FILETYPE_MENU: return "File Type";
        case CONFIG_MENU: return "Config";
        default: return "Menu";
    }
}

const char* DisplayManager::getMenuOption(MenuState state, uint8_t option) {
    switch (state) {
        case MAIN_MENU:
            switch (option) {
                case 0: return "Storage";
                case 1: return "File Type";
                case 2: return "Config";
                default: return "Option";
            }
            break;
            
        case STORAGE_MENU:
            if (option >= Common::StorageType::Count) return "Option";
            return (Common::StorageType(static_cast<Common::StorageType::Value>(option))).toSimple();
            
        case FILETYPE_MENU:
            if (option >= Common::FileType::Count) return "Option";
            return (Common::FileType(static_cast<Common::FileType::Value>(option))).toSimple();
            
        case CONFIG_MENU:
            switch (option) {
                case 0: return "Save";
                case 1: return "Reset";
                default: return "Option";
            }
            break;
            
        default:
            return "Option";
    }
}

uint8_t DisplayManager::getMenuOptionCount(MenuState state) {
    switch (state) {
        case MAIN_MENU: return 3;
        case STORAGE_MENU: return Common::StorageType::Count;
        case FILETYPE_MENU: return Common::FileType::Count;
        case CONFIG_MENU: return 2;
        default: return 1;
    }
}

} // namespace DeviceBridge::Components