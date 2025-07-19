#pragma once

#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include "../User/Display.h"
#include "../Common/Types.h"
#include "../Common/Config.h"

namespace DeviceBridge::Components {

class DisplayManager {
private:
    User::Display& _display;
    QueueHandle_t _displayQueue;
    QueueHandle_t _commandQueue;
    TaskHandle_t _taskHandle;
    
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
    
    // Task function (static for FreeRTOS)
    static void taskFunction(void* pvParameters);
    
    // Instance handling for task function
    void runTask();
    
    // Display management
    void updateDisplay();
    void processMessage(const Common::DisplayMessage& msg);
    void showMainScreen();
    void showTimeDisplay(const char* timeStr);
    void showMenuScreen();
    
    // Button handling
    void handleButtonPress(uint8_t button);
    uint8_t readButtons();
    
    // Menu navigation
    void navigateMenu(uint8_t button);
    void executeMenuSelection();
    void sendCommand(Common::SystemCommand::Type type, uint8_t value, const char* data = nullptr);
    
public:
    DisplayManager(User::Display& display, QueueHandle_t displayQueue, QueueHandle_t commandQueue);
    ~DisplayManager();
    
    // Lifecycle management
    bool initialize();
    bool start();
    void stop();
    
    // Display control
    void showMessage(const char* message, const char* line2 = nullptr);
    void showError(const char* error);
    void showStatus(const char* status);
    void enterMenu();
    void exitMenu();
    
    // Status inquiry
    bool isShowingMenu() const { return _inMenu; }
    TaskHandle_t getTaskHandle() const { return _taskHandle; }
    
private:
    // Button debouncing
    uint32_t _lastButtonTime;
    uint8_t _lastButtonState;
    static constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;
    
    // Menu strings
    static const char* getMenuTitle(MenuState state);
    static const char* getMenuOption(MenuState state, uint8_t option);
    static uint8_t getMenuOptionCount(MenuState state);
};

} // namespace DeviceBridge::Components