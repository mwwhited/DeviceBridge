#include "DisplayManager.h"
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

DisplayManager::DisplayManager(User::Display& display, QueueHandle_t displayQueue, QueueHandle_t commandQueue)
    : _display(display)
    , _displayQueue(displayQueue)
    , _commandQueue(commandQueue)
    , _taskHandle(nullptr)
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

bool DisplayManager::start() {
    if (_taskHandle != nullptr) {
        return false; // Already running
    }
    
    BaseType_t result = xTaskCreate(
        taskFunction,
        "Display",
        Common::RTOS::DISPLAY_STACK,
        this,
        Common::RTOS::DISPLAY_PRIORITY,
        &_taskHandle
    );
    
    return result == pdPASS;
}

void DisplayManager::stop() {
    if (_taskHandle != nullptr) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
}

void DisplayManager::taskFunction(void* pvParameters) {
    DisplayManager* manager = static_cast<DisplayManager*>(pvParameters);
    manager->runTask();
}

void DisplayManager::runTask() {
    Common::DisplayMessage msg;
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for (;;) {
        // Check for new messages
        while (xQueueReceive(_displayQueue, &msg, 0) == pdTRUE) {
            processMessage(msg);
        }
        
        // Handle button input
        uint16_t button = readButtons();
        if (button != BUTTON_NONE && button != _lastButtonState) {
            uint32_t currentTime = xTaskGetTickCount();
            if (currentTime - _lastButtonTime > pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)) {
                handleButtonPress(button);
                _lastButtonTime = currentTime;
            }
        }
        _lastButtonState = button;
        
        // Update display
        updateDisplay();
        
        // Maintain precise timing
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(Common::RTOS::DISPLAY_UPDATE_MS));
    }
}

void DisplayManager::processMessage(const Common::DisplayMessage& msg) {
    _lastMessageTime = xTaskGetTickCount();
    _showingTime = false;
    
    switch (msg.type) {
        case Common::DisplayMessage::TIME:
            if (!_inMenu && (xTaskGetTickCount() - _lastMessageTime) > pdMS_TO_TICKS(Common::Display::IDLE_TIME_MS)) {
                strncpy(_currentMessage, msg.message, sizeof(_currentMessage) - 1);
                _showingTime = true;
            }
            break;
            
        case Common::DisplayMessage::MENU:
            strncpy(_currentMessage, msg.message, sizeof(_currentMessage) - 1);
            strncpy(_currentLine2, msg.line2, sizeof(_currentLine2) - 1);
            break;
            
        default:
            strncpy(_currentMessage, msg.message, sizeof(_currentMessage) - 1);
            _currentLine2[0] = '\0';
            break;
    }
    
    _currentMessage[sizeof(_currentMessage) - 1] = '\0';
    _currentLine2[sizeof(_currentLine2) - 1] = '\0';
}

void DisplayManager::updateDisplay() {
    if (_inMenu) {
        showMenuScreen();
    } else if (_showingTime) {
        showTimeDisplay(_currentMessage);
    } else {
        showMainScreen();
    }
}

void DisplayManager::showMainScreen() {
    _display.updateStatus(_currentMessage);
}

void DisplayManager::showTimeDisplay(const char* timeStr) {
    _display.showTime(timeStr);
}

void DisplayManager::showMenuScreen() {
    const char* title = getMenuTitle(_menuState);
    const char* option = getMenuOption(_menuState, _menuSelection);
    
    char line2[32];
    snprintf(line2, sizeof(line2), ">%s", option);
    
    _display.showMenu(title, line2);
}

uint16_t DisplayManager::readButtons() {
    int buttonValue = analogRead(A0); // OSEPP LCD shield button pin
    
    // Map analog values to button constants with tolerance bands
    if (buttonValue < 50) return BUTTON_RIGHT;        // 0 ± 50
    if (buttonValue < 194) return BUTTON_UP;          // 144 ± 50  
    if (buttonValue < 416) return BUTTON_DOWN;        // 329 ± 87
    if (buttonValue < 622) return BUTTON_LEFT;        // 504 ± 118
    if (buttonValue < 891) return BUTTON_SELECT;      // 741 ± 150
    
    return BUTTON_NONE; // > 891, should be ~1023
}

void DisplayManager::handleButtonPress(uint16_t button) {
    if (_inMenu) {
        navigateMenu(button);
    } else {
        // Any button press enters menu from main screen
        if (button != BUTTON_NONE) {
            enterMenu();
        }
    }
}

void DisplayManager::navigateMenu(uint16_t button) {
    uint8_t maxOptions = getMenuOptionCount(_menuState);
    
    switch (button) {
        case BUTTON_UP:
            if (_menuSelection > 0) {
                _menuSelection--;
            }
            break;
            
        case BUTTON_DOWN:
            if (_menuSelection < maxOptions - 1) {
                _menuSelection++;
            }
            break;
            
        case BUTTON_SELECT:
            executeMenuSelection();
            break;
            
        case BUTTON_LEFT:
            exitMenu();
            break;
            
        default:
            break;
    }
}

void DisplayManager::executeMenuSelection() {
    switch (_menuState) {
        case MAIN_MENU:
            switch (_menuSelection) {
                case 0: _menuState = STORAGE_MENU; break;
                case 1: _menuState = FILETYPE_MENU; break;
                case 2: _menuState = CONFIG_MENU; break;
                case 3: exitMenu(); break;
            }
            break;
            
        case STORAGE_MENU:
            sendCommand(Common::SystemCommand::STORAGE_SELECT, _menuSelection);
            _menuState = MAIN_MENU;
            break;
            
        case FILETYPE_MENU:
            sendCommand(Common::SystemCommand::FILE_TYPE, _menuSelection);
            _menuState = MAIN_MENU;
            break;
            
        case CONFIG_MENU:
            // TODO: Implement config options
            _menuState = MAIN_MENU;
            break;
    }
    
    _menuSelection = 0;
}

void DisplayManager::sendCommand(Common::SystemCommand::Type type, uint8_t value, const char* data) {
    Common::SystemCommand cmd;
    cmd.type = type;
    cmd.value = value;
    
    if (data != nullptr) {
        strncpy(cmd.data, data, sizeof(cmd.data) - 1);
        cmd.data[sizeof(cmd.data) - 1] = '\0';
    } else {
        cmd.data[0] = '\0';
    }
    
    xQueueSend(_commandQueue, &cmd, 0);
}

void DisplayManager::enterMenu() {
    _inMenu = true;
    _menuState = MAIN_MENU;
    _menuSelection = 0;
}

void DisplayManager::exitMenu() {
    _inMenu = false;
    _showingTime = false;
    showMessage("Ready");
}

void DisplayManager::showMessage(const char* message, const char* line2) {
    strncpy(_currentMessage, message, sizeof(_currentMessage) - 1);
    _currentMessage[sizeof(_currentMessage) - 1] = '\0';
    
    if (line2 != nullptr) {
        strncpy(_currentLine2, line2, sizeof(_currentLine2) - 1);
        _currentLine2[sizeof(_currentLine2) - 1] = '\0';
    } else {
        _currentLine2[0] = '\0';
    }
    
    _lastMessageTime = xTaskGetTickCount();
    _showingTime = false;
}

void DisplayManager::showError(const char* error) {
    showMessage(error);
}

void DisplayManager::showStatus(const char* status) {
    showMessage(status);
}

// Static menu helper functions
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
                case 3: return "Exit";
                default: return "";
            }
            
        case STORAGE_MENU:
            switch (option) {
                case 0: return "SD Card";
                case 1: return "EEPROM";
                case 2: return "Serial";
                case 3: return "Auto";
                default: return "";
            }
            
        case FILETYPE_MENU:
            switch (option) {
                case 0: return "Auto";
                case 1: return "Binary";
                case 2: return "Bitmap";
                case 3: return "PNG";
                default: return "";
            }
            
        case CONFIG_MENU:
            switch (option) {
                case 0: return "Set Time";
                case 1: return "Save Config";
                case 2: return "Reset";
                default: return "";
            }
            
        default:
            return "";
    }
}

uint8_t DisplayManager::getMenuOptionCount(MenuState state) {
    switch (state) {
        case MAIN_MENU: return 4;
        case STORAGE_MENU: return 4;
        case FILETYPE_MENU: return 4;
        case CONFIG_MENU: return 3;
        default: return 0;
    }
}

} // namespace DeviceBridge::Components