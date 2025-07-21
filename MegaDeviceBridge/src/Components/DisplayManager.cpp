#include "DisplayManager.h"
#include "TimeManager.h"
#include "SystemManager.h"
#include "../Common/Types.h"
#include "../Common/ServiceLocator.h"
#include "../Common/ConfigurationService.h"
#include <string.h>
#include <Arduino.h>

// PROGMEM component name for memory optimization
static const char component_name[] PROGMEM = "DisplayManager";

// PROGMEM menu strings for memory optimization
static const char menu_main[] PROGMEM = "Main Menu";
static const char menu_storage[] PROGMEM = "Storage";
static const char menu_filetype[] PROGMEM = "File Type";
static const char menu_config[] PROGMEM = "Config";
static const char menu_default[] PROGMEM = "Menu";

static const char option_storage[] PROGMEM = "Storage";
static const char option_filetype[] PROGMEM = "File Type";
static const char option_config[] PROGMEM = "Config";
static const char option_save[] PROGMEM = "Save";
static const char option_reset[] PROGMEM = "Reset";
static const char option_default[] PROGMEM = "Option";

// PROGMEM string pointer tables
static const char* const menu_titles[] PROGMEM = {
    menu_main,      // MAIN_MENU
    menu_storage,   // STORAGE_MENU  
    menu_filetype,  // FILETYPE_MENU
    menu_config     // CONFIG_MENU
};

static const char* const main_menu_options[] PROGMEM = {
    option_storage,   // 0
    option_filetype,  // 1
    option_config     // 2
};

static const char* const config_menu_options[] PROGMEM = {
    option_save,     // 0
    option_reset     // 1
};

// Shared buffer for menu string operations
static char menu_string_buffer[16];

// Helper function to safely read menu strings from PROGMEM
inline const char* getMenuProgmemString(const char* const* table, uint8_t index, uint8_t max_index) {
    if (index > max_index) {
        strcpy_P(menu_string_buffer, option_default);
        return menu_string_buffer;
    }
    strcpy_P(menu_string_buffer, (char*)pgm_read_word(&table[index]));
    return menu_string_buffer;
}

namespace DeviceBridge::Components
{

    // Button constants now accessed via ConfigurationService

    DisplayManager::DisplayManager(User::Display &display)
        : _display(display), _lastMessageTime(0), _lastDisplayUpdate(0), _normalUpdateInterval(ServiceLocator::getInstance().getConfigurationService()->getNormalDisplayInterval()), _storageUpdateInterval(ServiceLocator::getInstance().getConfigurationService()->getStorageDisplayInterval()),
          _menuState(MAIN_MENU), _menuSelection(0), _lastButtonTime(0), _lastButtonState(ServiceLocator::getInstance().getConfigurationService()->getButtonNoneValue())
    {
        // Initialize display flags (bit field)
        _displayFlags.showingTime = 0;
        _displayFlags.inMenu = 0;
        _displayFlags.storageOperationActive = 0;
        _displayFlags.reserved = 0;
        memset(_currentMessage, 0, sizeof(_currentMessage));
        memset(_currentLine2, 0, sizeof(_currentLine2));
        strcpy(_currentMessage, "Ready");
    }

    DisplayManager::~DisplayManager()
    {
        stop();
    }

    bool DisplayManager::initialize()
    {
        // Display should already be initialized by main
        return true;
    }

    void DisplayManager::update(unsigned long currentTime)
    {
        // Check for button presses (always responsive)
        uint16_t buttonValue = readButtons();
        auto* config = ServiceLocator::getInstance().getConfigurationService();
        if (buttonValue != config->getButtonNoneValue() && buttonValue != _lastButtonState)
        {
            handleButtonPress(buttonValue);
            _lastButtonState = buttonValue;
            _lastButtonTime = currentTime;
        }
        else if (buttonValue == config->getButtonNoneValue())
        {
            _lastButtonState = config->getButtonNoneValue();
        }

        // Adaptive display update based on storage operation state
        uint32_t updateInterval = _displayFlags.storageOperationActive ? _storageUpdateInterval : _normalUpdateInterval;
        
        if (currentTime - _lastDisplayUpdate >= updateInterval) {
            updateDisplay(currentTime);
            _lastDisplayUpdate = currentTime;
        }
    }

    void DisplayManager::stop()
    {
        _displayFlags.inMenu = false;
        _displayFlags.showingTime = false;
    }

    void DisplayManager::setStorageOperationActive(bool active)
    {
        _displayFlags.storageOperationActive = active;
        
        // If storage operation is ending, allow immediate display update and clock refresh
        if (!active) {
            _lastDisplayUpdate = 0;
            _displayFlags.showingTime = false; // Allow clock to refresh when idle
        }
    }

    void DisplayManager::updateDisplay(unsigned long currentTime)
    {
        auto timeManager = getServices().getTimeManager();
        // Check if we should show time when truly idle (not in menu, not during storage operations)
        if (!_displayFlags.inMenu && !_displayFlags.storageOperationActive && (currentTime - _lastMessageTime) > Common::Display::IDLE_TIME_MS)
        {
            if (!_displayFlags.showingTime)
            {
                _displayFlags.showingTime = true;
                char timeStr[32];
                timeManager->getFormattedTime(timeStr, sizeof(timeStr));
                showTimeDisplay(timeStr);
            }
        }
    }

    void DisplayManager::processMessage(const Common::DisplayMessage &msg)
    {
        _lastMessageTime = millis();

        switch (msg.type)
        {
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
            _displayFlags.showingTime = false;
            break;
        case Common::DisplayMessage::MENU:
            showMenuScreen();
            break;
        }
    }

    void DisplayManager::showMainScreen()
    {
        _display.clear();
        _display.setCursor(0, 0);
        _display.print(_currentMessage);

        if (strlen(_currentLine2) > 0)
        {
            _display.setCursor(0, 1);
            _display.print(_currentLine2);
        }
    }

    void DisplayManager::showTimeDisplay(const char *timeStr)
    {
        _display.clear();
        _display.setCursor(0, 0);
        _display.print(F("Device Bridge"));
        _display.setCursor(0, 1);
        _display.print(timeStr);
    }

    void DisplayManager::showMenuScreen()
    {
        _display.clear();
        _display.setCursor(0, 0);
        _display.print(getMenuTitle(_menuState));
        _display.setCursor(0, 1);
        _display.print(F(">"));
        _display.print(getMenuOption(_menuState, _menuSelection));
    }

    uint16_t DisplayManager::readButtons()
    {
        uint32_t currentTime = millis();

        // Debounce buttons
        if (currentTime - _lastButtonTime < BUTTON_DEBOUNCE_MS)
        {
            return _lastButtonState;
        }

        uint16_t buttonValue = analogRead(A0);

        // Tolerance for analog readings using ConfigurationService
        auto* config = ServiceLocator::getInstance().getConfigurationService();
        if (buttonValue < config->getRightThreshold())
            return config->getButtonRightValue();
        if (buttonValue < config->getUpThreshold())
            return config->getButtonUpValue();
        if (buttonValue < config->getDownThreshold())
            return config->getButtonDownValue();
        if (buttonValue < config->getLeftThreshold())
            return config->getButtonLeftValue();
        if (buttonValue < config->getSelectThreshold())
            return config->getButtonSelectValue();

        return config->getButtonNoneValue();
    }

    void DisplayManager::handleButtonPress(uint16_t button)
    {
        if (_displayFlags.inMenu)
        {
            navigateMenu(button);
        }
        else
        {
            // Enter menu on any button press when not in menu
            enterMenu();
        }
    }

    void DisplayManager::navigateMenu(uint16_t button)
    {
        // Reset idle timer on any menu interaction
        _lastMessageTime = millis();
        
        auto* config = ServiceLocator::getInstance().getConfigurationService();
        
        if (button == config->getButtonUpValue()) {
            if (_menuSelection > 0)
            {
                _menuSelection--;
            }
            showMenuScreen();
        }
        else if (button == config->getButtonDownValue()) {
            if (_menuSelection < getMenuOptionCount(_menuState) - 1)
            {
                _menuSelection++;
            }
            showMenuScreen();
        }
        else if (button == config->getButtonSelectValue()) {
            executeMenuSelection();
        }
        else if (button == config->getButtonLeftValue()) {
            exitMenu();
        }
    }

    void DisplayManager::executeMenuSelection()
    {
        switch (_menuState)
        {
        case MAIN_MENU:
            switch (_menuSelection)
            {
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

    void DisplayManager::sendCommand(Common::SystemCommand::Type type, uint8_t value, const char *data)
    {
        auto systemManager = getServices().getSystemManager();
        Common::SystemCommand cmd;
        cmd.type = type;
        cmd.value = value;
        if (data)
        {
            strncpy(cmd.data, data, sizeof(cmd.data) - 1);
            cmd.data[sizeof(cmd.data) - 1] = '\0';
        }
        else
        {
            cmd.data[0] = '\0';
        }

        systemManager->processSystemCommand(cmd);
    }

    void DisplayManager::showMessage(const char *message, const char *line2)
    {
        strncpy(_currentMessage, message, sizeof(_currentMessage) - 1);
        _currentMessage[sizeof(_currentMessage) - 1] = '\0';

        if (line2)
        {
            strncpy(_currentLine2, line2, sizeof(_currentLine2) - 1);
            _currentLine2[sizeof(_currentLine2) - 1] = '\0';
        }
        else
        {
            _currentLine2[0] = '\0';
        }

        _lastMessageTime = millis();
        _displayFlags.showingTime = false;

        if (!_displayFlags.inMenu)
        {
            showMainScreen();
        }
    }

    void DisplayManager::showError(const char *error)
    {
        _display.clear();
        _display.setCursor(0, 0);
        _display.print(F("ERROR:"));
        _display.setCursor(0, 1);
        _display.print(error);

        _lastMessageTime = millis();
        _displayFlags.showingTime = false;
    }

    void DisplayManager::showStatus(const char *status)
    {
        showMessage(status);
    }

    void DisplayManager::enterMenu()
    {
        _displayFlags.inMenu = true;
        _menuState = MAIN_MENU;
        _menuSelection = 0;
        _lastMessageTime = millis(); // Reset idle timer when entering menu
        _displayFlags.showingTime = false; // Ensure we're not showing time
        showMenuScreen();
    }

    void DisplayManager::exitMenu()
    {
        _displayFlags.inMenu = false;
        _displayFlags.showingTime = false; // Allow clock to refresh when idle
        showMainScreen();
    }

    void DisplayManager::displayMessage(Common::DisplayMessage::Type type, const char *message, const char *line2)
    {
        auto systemManager = getServices().getSystemManager();
        // Debug mode: output LCD messages to serial
        if (systemManager->isLCDDebugEnabled())
        {
            Serial.print(F("[LCD] "));
            switch (type)
            {
            case Common::DisplayMessage::INFO:
                Serial.print(F("INFO: "));
                break;
            case Common::DisplayMessage::ERROR:
                Serial.print(F("ERROR: "));
                break;
            case Common::DisplayMessage::STATUS:
                Serial.print(F("STATUS: "));
                break;
            default:
                Serial.print(F("MSG: "));
                break;
            }
            Serial.print(message);
            if (line2 && strlen(line2) > 0)
            {
                Serial.print(F(" | "));
                Serial.print(line2);
            }
            Serial.print(F("\r\n"));
        }

        Common::DisplayMessage msg;
        msg.type = type;
        strncpy(msg.message, message, sizeof(msg.message) - 1);
        msg.message[sizeof(msg.message) - 1] = '\0';

        if (line2)
        {
            strncpy(msg.line2, line2, sizeof(msg.line2) - 1);
            msg.line2[sizeof(msg.line2) - 1] = '\0';
        }
        else
        {
            msg.line2[0] = '\0';
        }

        processMessage(msg);
    }
    void DisplayManager::displayMessage(Common::DisplayMessage::Type type, const __FlashStringHelper *message, const __FlashStringHelper *line2)
    {
        auto systemManager = getServices().getSystemManager();
        // Debug mode: output LCD messages to serial
        if (systemManager->isLCDDebugEnabled())
        {
            Serial.print(F("[LCD] "));
            switch (type)
            {
            case Common::DisplayMessage::INFO:
                Serial.print(F("INFO: "));
                break;
            case Common::DisplayMessage::ERROR:
                Serial.print(F("ERROR: "));
                break;
            case Common::DisplayMessage::STATUS:
                Serial.print(F("STATUS: "));
                break;
            default:
                Serial.print(F("MSG: "));
                break;
            }
            Serial.print(message);
            if (line2)
            {
                Serial.print(F(" | "));
                Serial.print(line2);
            }
            Serial.print(F("\r\n"));
        }

        Common::DisplayMessage msg;
        msg.type = type;

        // Copy from flash (PROGMEM) to RAM buffer
        strncpy_P(msg.message, (PGM_P)message, sizeof(msg.message) - 1);
        msg.message[sizeof(msg.message) - 1] = '\0';

        if (line2)
        {
            strncpy_P(msg.line2, (PGM_P)line2, sizeof(msg.line2) - 1);
            msg.line2[sizeof(msg.line2) - 1] = '\0';
        }
        else
        {
            msg.line2[0] = '\0';
        }

        processMessage(msg);
    }

    const char *DisplayManager::getMenuTitle(MenuState state)
    {
        if (state < 4) {
            return getMenuProgmemString(menu_titles, (uint8_t)state, 3);
        } else {
            strcpy_P(menu_string_buffer, menu_default);
            return menu_string_buffer;
        }
    }

    const char *DisplayManager::getMenuOption(MenuState state, uint8_t option)
    {
        switch (state)
        {
        case MAIN_MENU:
            return getMenuProgmemString(main_menu_options, option, 2);

        case STORAGE_MENU:
            if (option >= Common::StorageType::Count) {
                strcpy_P(menu_string_buffer, option_default);
                return menu_string_buffer;
            }
            return (Common::StorageType(static_cast<Common::StorageType::Value>(option))).toSimple();

        case FILETYPE_MENU:
            if (option >= Common::FileType::Count) {
                strcpy_P(menu_string_buffer, option_default);
                return menu_string_buffer;
            }
            return (Common::FileType(static_cast<Common::FileType::Value>(option))).toSimple();

        case CONFIG_MENU:
            return getMenuProgmemString(config_menu_options, option, 1);

        default:
            strcpy_P(menu_string_buffer, option_default);
            return menu_string_buffer;
        }
    }

    uint8_t DisplayManager::getMenuOptionCount(MenuState state)
    {
        switch (state)
        {
        case MAIN_MENU:
            return 3;
        case STORAGE_MENU:
            return Common::StorageType::Count;
        case FILETYPE_MENU:
            return Common::FileType::Count;
        case CONFIG_MENU:
            return 2;
        default:
            return 1;
        }
    }

    // IComponent interface implementation
    bool DisplayManager::selfTest()
    {
        Serial.print(F("DisplayManager Self-Test:\r\n"));

        bool result = true;
 
        // Test LCD display functionality
        Serial.print(F("  Testing LCD display... "));
        
        // Test basic LCD operations
        _display.clear();
        _display.setCursor(0, 0);
        _display.print(F("Self-Test"));
        _display.setCursor(0, 1);
        _display.print(F("LCD OK"));
        
        Serial.print(F("✅ OK\r\n"));
        
        // Test button reading
        Serial.print(F("  Testing button interface... "));
        uint16_t buttonValue = analogRead(A0);
        auto* config = ServiceLocator::getInstance().getConfigurationService();
        
        if (buttonValue >= 0 && buttonValue <= 1023) {
            Serial.print(F("✅ OK (value: "));
            Serial.print(buttonValue);
            Serial.print(F(")\r\n"));
        } else {
            Serial.print(F("❌ FAIL\r\n"));
            result = false;
        }
        
        // Dependencies validated by ServiceLocator at startup

        return result;
    }

    const char *DisplayManager::getComponentName() const
    {
        static char name_buffer[24];
        strcpy_P(name_buffer, component_name);
        return name_buffer;
    }

    bool DisplayManager::validateDependencies() const
    {
        bool valid = true;

        auto timeManager = getServices().getTimeManager();
        if (!timeManager)
        {
            Serial.print(F("  Missing TimeManager dependency\r\n"));
            valid = false;
        }

        auto systemManager = getServices().getSystemManager();
        if (!systemManager)
        {
            Serial.print(F("  Missing SystemManager dependency\r\n"));
            valid = false;
        }

        return valid;
    }

    void DisplayManager::printDependencyStatus() const
    {
        Serial.print(F("DisplayManager Dependencies:\r\n"));

        auto timeManager = getServices().getTimeManager();
        Serial.print(F("  TimeManager: "));
        Serial.print(timeManager ? F("✅ Available") : F("❌ Missing"));
        Serial.print(F("\r\n"));

        auto systemManager = getServices().getSystemManager();
        Serial.print(F("  SystemManager: "));
        Serial.print(systemManager ? F("✅ Available") : F("❌ Missing"));
        Serial.print(F("\r\n"));
    }

    unsigned long DisplayManager::getUpdateInterval() const {
        auto configService = getServices().getConfigurationService();
        return configService ? configService->getDisplayInterval() : 100; // Default 100ms
    }

} // namespace DeviceBridge::Components