#include <Arduino.h>

// Hardware abstraction layers
#include "./Parallel/Port.h"
#include "./User/Display.h"

// Component managers
#include "./Components/ParallelPortManager.h"
#include "./Components/FileSystemManager.h"
#include "./Components/DisplayManager.h"
#include "./Components/TimeManager.h"
#include "./Components/SystemManager.h"
#include "./Components/ConfigurationManager.h"

// Common definitions
#include "./Common/Types.h"
#include "./Common/Config.h"
#include "./Common/ServiceLocator.h"

// Hardware instances
DeviceBridge::Parallel::Port printerPort(
    DeviceBridge::Parallel::Control(
        DeviceBridge::Common::Pins::LPT_STROBE,
        DeviceBridge::Common::Pins::LPT_AUTO_FEED,
        DeviceBridge::Common::Pins::LPT_INITIALIZE,
        DeviceBridge::Common::Pins::LPT_SELECT_IN),
    DeviceBridge::Parallel::Status(
        DeviceBridge::Common::Pins::LPT_ACK,
        DeviceBridge::Common::Pins::LPT_BUSY,
        DeviceBridge::Common::Pins::LPT_PAPER_OUT,
        DeviceBridge::Common::Pins::LPT_SELECT,
        DeviceBridge::Common::Pins::LPT_ERROR),
    DeviceBridge::Parallel::Data(
        DeviceBridge::Common::Pins::LPT_D0,
        DeviceBridge::Common::Pins::LPT_D1,
        DeviceBridge::Common::Pins::LPT_D2,
        DeviceBridge::Common::Pins::LPT_D3,
        DeviceBridge::Common::Pins::LPT_D4,
        DeviceBridge::Common::Pins::LPT_D5,
        DeviceBridge::Common::Pins::LPT_D6,
        DeviceBridge::Common::Pins::LPT_D7));

DeviceBridge::User::Display display(
    DeviceBridge::Common::Pins::LCD_RESET,
    DeviceBridge::Common::Pins::LCD_ENABLE,
    DeviceBridge::Common::Pins::LCD_D4,
    DeviceBridge::Common::Pins::LCD_D5,
    DeviceBridge::Common::Pins::LCD_D6,
    DeviceBridge::Common::Pins::LCD_D7);

// Component managers
DeviceBridge::Components::ParallelPortManager* parallelPortManager = nullptr;
DeviceBridge::Components::FileSystemManager* fileSystemManager = nullptr;
DeviceBridge::Components::DisplayManager* displayManager = nullptr;
DeviceBridge::Components::TimeManager* timeManager = nullptr;
DeviceBridge::Components::SystemManager* systemManager = nullptr;
DeviceBridge::Components::ConfigurationManager* configurationManager = nullptr;

// Timing for cooperative multitasking
unsigned long lastParallelUpdate = 0;
unsigned long lastFileSystemUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastTimeUpdate = 0;
unsigned long lastSystemUpdate = 0;
unsigned long lastHeartbeatUpdate = 0;
unsigned long lastConfigurationUpdate = 0;

// Update intervals (milliseconds)
const unsigned long PARALLEL_INTERVAL = 1;      // 1ms for real-time data capture
const unsigned long FILESYSTEM_INTERVAL = 10;   // 10ms for file operations
const unsigned long DISPLAY_INTERVAL = 100;     // 100ms for display updates
const unsigned long TIME_INTERVAL = 1000;       // 1s for time operations
const unsigned long SYSTEM_INTERVAL = 5000;     // 5s for system monitoring
const unsigned long HEARTBEAT_INTERVAL = 500;   // 500ms for blink heartbeat LED
const unsigned long CONFIGURATION_INTERVAL = 50; // 50ms for configuration/serial commands

void setup()
{
  pinMode(DeviceBridge::Common::Pins::HEARTBEAT, OUTPUT);
  digitalWrite(DeviceBridge::Common::Pins::HEARTBEAT, LOW); // Start with LED off

  Serial.begin(DeviceBridge::Common::Serial::BAUD_RATE);
  while (!Serial) { delay(10); }
  
  Serial.print(F("Device Bridge Initializing (Loop-based)...\r\n"));
  
  // Initialize hardware
  printerPort.initialize();
  display.initialize();
  
  // Initialize ServiceLocator
  DeviceBridge::ServiceLocator::initialize();
  DeviceBridge::ServiceLocator& services = DeviceBridge::ServiceLocator::getInstance();
  
  // Create component managers (no queues/mutexes needed)
  parallelPortManager = new DeviceBridge::Components::ParallelPortManager(printerPort);
  fileSystemManager = new DeviceBridge::Components::FileSystemManager();
  displayManager = new DeviceBridge::Components::DisplayManager(display);
  timeManager = new DeviceBridge::Components::TimeManager();
  systemManager = new DeviceBridge::Components::SystemManager();
  configurationManager = new DeviceBridge::Components::ConfigurationManager();
  
  // Verify component creation
  if (!parallelPortManager || !fileSystemManager || !displayManager || 
      !timeManager || !systemManager || !configurationManager) {
    Serial.print(F("FATAL: Failed to create component managers\r\n"));
    while(1) { delay(1000); }
  }
  
  // Register all components with ServiceLocator
  Serial.print(F("Registering components with ServiceLocator...\r\n"));
  services.registerDisplay(&display);
  services.registerParallelPortManager(parallelPortManager);
  services.registerFileSystemManager(fileSystemManager);
  services.registerDisplayManager(displayManager);
  services.registerTimeManager(timeManager);
  services.registerSystemManager(systemManager);
  services.registerConfigurationManager(configurationManager);
  
  // Validate all dependencies are registered
  if (!services.validateAllDependencies()) {
    Serial.print(F("FATAL: Service dependency validation failed\r\n"));
    while(1) { delay(1000); }
  }
  
  // Initialize all components
  Serial.print(F("Initializing components...\r\n"));
  
  if (!parallelPortManager->initialize()) {
    Serial.print(F("WARNING: Parallel port initialization failed\r\n"));
  }
  
  if (!fileSystemManager->initialize()) {
    Serial.print(F("WARNING: File system initialization failed\r\n"));
  }
  
  if (!displayManager->initialize()) {
    Serial.print(F("WARNING: Display initialization failed\r\n"));
  }
  
  if (!timeManager->initialize()) {
    Serial.print(F("WARNING: Time manager initialization failed\r\n"));
  }
  
  if (!systemManager->initialize()) {
    Serial.print(F("WARNING: System manager initialization failed\r\n"));
  }
  
  if (!configurationManager->initialize()) {
    Serial.print(F("WARNING: Configuration manager initialization failed\r\n"));
  }
  
  Serial.print(F("All systems initialized successfully!\r\n"));
  
  // Run post-initialization system self-test
  Serial.print(F("Running post-initialization system self-test...\r\n"));
  bool selfTestPassed = services.runSystemSelfTest();
  
  if (selfTestPassed) {
    Serial.print(F("✅ System self-test PASSED - Device Bridge ready for operation.\r\n"));
  } else {
    Serial.print(F("⚠️  System self-test completed with warnings - Check component status.\r\n"));
  }
  
  Serial.print(F("Connect TDS2024 to parallel port and use LCD buttons for control.\r\n"));
  
  // Initialize timing
  lastParallelUpdate = millis();
  lastFileSystemUpdate = millis();
  lastDisplayUpdate = millis();
  lastTimeUpdate = millis();
  lastSystemUpdate = millis();
  lastConfigurationUpdate = millis();
}

void loop()
{
  unsigned long currentTime = millis();
  
  // Parallel port manager - highest priority, most frequent
  if (currentTime - lastParallelUpdate >= PARALLEL_INTERVAL) {
    parallelPortManager->update();
    lastParallelUpdate = currentTime;
  }
  
  // File system manager - handle data storage
  if (currentTime - lastFileSystemUpdate >= FILESYSTEM_INTERVAL) {
    fileSystemManager->update();
    lastFileSystemUpdate = currentTime;
  }
  
  // Display manager - user interface updates
  if (currentTime - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    displayManager->update();
    lastDisplayUpdate = currentTime;
  }
  
  // Time manager - RTC and time-based operations
  if (currentTime - lastTimeUpdate >= TIME_INTERVAL) {
    timeManager->update();
    lastTimeUpdate = currentTime;
  }
  
  // System manager - monitoring and health checks
  if (currentTime - lastSystemUpdate >= SYSTEM_INTERVAL) {
    systemManager->update();
    lastSystemUpdate = currentTime;
  }

  // Heartbeat manager
  if (currentTime - lastHeartbeatUpdate >= HEARTBEAT_INTERVAL) {
    digitalWrite(DeviceBridge::Common::Pins::HEARTBEAT, !digitalRead(DeviceBridge::Common::Pins::HEARTBEAT));
    lastHeartbeatUpdate = currentTime;
  }
  
  // Configuration manager - serial commands and settings
  if (currentTime - lastConfigurationUpdate >= CONFIGURATION_INTERVAL) {
    configurationManager->update();
    lastConfigurationUpdate = currentTime;
  }
  
  // Small delay to prevent overwhelming the CPU
  delayMicroseconds(100);
}