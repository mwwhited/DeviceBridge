#pragma once

#include <Arduino.h>
#include "../User/Display.h"
#include "../Common/Types.h"
#include "../Common/Config.h"
#include "../Common/ServiceLocator.h"

namespace DeviceBridge::Components {

// Forward declarations
class TimeManager;
class SystemManager;

class DisplayManager : public DeviceBridge::IComponent {
private:
    User::Display& _display;
    // Note: No longer storing direct references - using ServiceLocator
    
    // Display state
    char _currentMessage[Common::Limits::MAX_MESSAGE_LENGTH];
    char _currentLine2[Common::Limits::MAX_MESSAGE_LENGTH];
    uint32_t _lastMessageTime;
    
    // Display state flags (bit field optimization)
    struct {
        uint8_t showingTime : 1;
        uint8_t inMenu : 1;
        uint8_t storageOperationActive : 1;
        uint8_t reserved : 5;  // For future flags
    } _displayFlags;
    uint32_t _lastDisplayUpdate;
    uint32_t _normalUpdateInterval;
    uint32_t _storageUpdateInterval;
    
    // Menu system state
    enum MenuState {
        MAIN_MENU,
        STORAGE_MENU,
        FILETYPE_MENU,
        CONFIG_MENU
    } _menuState;
    uint8_t _menuSelection;
    
    // Display management
    void updateDisplay(unsigned long currentTime);
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
        
    
    // Lifecycle management (IComponent interface)
    bool initialize() override final;
    void update(unsigned long currentTime) override final;  // Called from main loop
    void stop() override final;
    
    // IComponent interface implementation
    bool selfTest() override final;
    const char* getComponentName() const override final;
    bool validateDependencies() const override final;
    void printDependencyStatus() const override final;
    unsigned long getUpdateInterval() const override final;
    
    // Display control
    void showMessage(const char* message, const char* line2 = nullptr);
    void showError(const char* error);
    void showStatus(const char* status);
    void enterMenu();
    void exitMenu();
    
    // Message processing (called by other components)
    void displayMessage(Common::DisplayMessage::Type type, const char* message, const char* line2 = nullptr);
    void displayMessage(Common::DisplayMessage::Type type, const __FlashStringHelper* message, const __FlashStringHelper* line2 = nullptr);
    
    // Status inquiry
    bool isShowingMenu() const { return _displayFlags.inMenu; }
    
    // Storage operation control for LCD refresh throttling
    void setStorageOperationActive(bool active);
    bool isStorageOperationActive() const { return _displayFlags.storageOperationActive; }
    
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