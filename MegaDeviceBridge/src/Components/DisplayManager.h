#pragma once

#include <Arduino.h>
#include "../User/Display.h"
#include "../Common/Types.h"
#include "../Common/Config.h"

namespace DeviceBridge::Components {

// Forward declarations
class TimeManager;
class SystemManager;

class DisplayManager {
private:
    User::Display& _display;
    TimeManager* _timeManager;
    SystemManager* _systemManager;
    
    // Display state
    char _currentMessage[Common::Limits::MAX_MESSAGE_LENGTH];
    char _currentLine2[Common::Limits::MAX_MESSAGE_LENGTH];
    uint32_t _lastMessageTime;
    bool _showingTime;
    bool _inMenu;
    
    // Menu system state
    enum MenuState {
        MAIN_MENU,
        STORAGE_MENU,
        FILETYPE_MENU,
        CONFIG_MENU
    } _menuState;
    uint8_t _menuSelection;
    
    // Display management
    void updateDisplay();
    void processMessage(const Common::DisplayMessage& msg);
    void showMainScreen();
    void showTimeDisplay(const char* timeStr);
    void showMenuScreen();
    
    // Button handling
    void handleButtonPress(uint16_t button);
    uint16_t readButtons();
    
    // Menu navigation
    void navigateMenu(uint16_t button);
    void executeMenuSelection();
    void sendCommand(Common::SystemCommand::Type type, uint8_t value, const char* data = nullptr);
    
public:
    DisplayManager(User::Display& display);
    ~DisplayManager();
    
    // Set component references
    void setTimeManager(TimeManager* manager) { _timeManager = manager; }
    void setSystemManager(SystemManager* manager) { _systemManager = manager; }
    
    // Lifecycle management
    bool initialize();
    void update();  // Called from main loop
    void stop();
    
    // Display control
    void showMessage(const char* message, const char* line2 = nullptr);
    void showError(const char* error);
    void showStatus(const char* status);
    void enterMenu();
    void exitMenu();
    
    // Message processing (called by other components)
    void displayMessage(Common::DisplayMessage::Type type, const char* message, const char* line2 = nullptr);
    
    // Status inquiry
    bool isShowingMenu() const { return _inMenu; }
    
private:
    // Button debouncing
    uint32_t _lastButtonTime;
    uint16_t _lastButtonState;
    static constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;
    
    // Menu strings
    static const char* getMenuTitle(MenuState state);
    static const char* getMenuOption(MenuState state, uint8_t option);
    static uint8_t getMenuOptionCount(MenuState state);
};

} // namespace DeviceBridge::Components