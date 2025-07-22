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
#include "./Components/HeartbeatLEDManager.h"

// Common definitions
#include "./Common/Types.h"
#include "./Common/Config.h"
#include "./Common/ServiceLocator.h"
#include "./Common/ConfigurationService.h"

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

// Component managers - Array-based management
DeviceBridge::IComponent* components[7];
DeviceBridge::Common::ConfigurationService* configurationService = nullptr;

// Component indices for easy access
constexpr uint8_t PARALLEL_PORT_INDEX = 0;
constexpr uint8_t FILE_SYSTEM_INDEX = 1;
constexpr uint8_t DISPLAY_INDEX = 2;
constexpr uint8_t TIME_INDEX = 3;
constexpr uint8_t SYSTEM_INDEX = 4;
constexpr uint8_t CONFIGURATION_INDEX = 5;
constexpr uint8_t HEARTBEAT_LED_INDEX = 6;
constexpr uint8_t COMPONENT_COUNT = 7;

// Update intervals (milliseconds) - now accessed via ConfigurationService

void setup()
{
  Serial.begin(DeviceBridge::Common::Serial::BAUD_RATE);
  while (!Serial) { delay(10); }
  
  Serial.print(F("Device Bridge Initializing (Loop-based)...\r\n"));
  Serial.flush();
  
  // Initialize hardware
  Serial.print(F("Initializing printer port...\r\n"));
  Serial.flush();
  printerPort.initialize();
  
  Serial.print(F("Initializing display...\r\n"));
  Serial.flush();
  display.initialize();
  
  // Initialize ServiceLocator
  Serial.print(F("Initializing ServiceLocator...\r\n"));
  Serial.flush();
  DeviceBridge::ServiceLocator::initialize();
  DeviceBridge::ServiceLocator& services = DeviceBridge::ServiceLocator::getInstance();
  
  // Create component managers (no queues/mutexes needed)
  Serial.print(F("Creating component managers...\r\n"));
  Serial.flush();
  
  Serial.print(F("Creating ParallelPortManager...\r\n"));
  Serial.flush();
  components[PARALLEL_PORT_INDEX] = new DeviceBridge::Components::ParallelPortManager(printerPort);
  
  Serial.print(F("Creating FileSystemManager...\r\n"));
  Serial.flush();
  components[FILE_SYSTEM_INDEX] = new DeviceBridge::Components::FileSystemManager();
  
  Serial.print(F("Creating DisplayManager...\r\n"));
  Serial.flush();
  components[DISPLAY_INDEX] = new DeviceBridge::Components::DisplayManager(display);
  
  Serial.print(F("Creating TimeManager...\r\n"));
  Serial.flush();
  components[TIME_INDEX] = new DeviceBridge::Components::TimeManager();
  
  Serial.print(F("Creating SystemManager...\r\n"));
  Serial.flush();
  components[SYSTEM_INDEX] = new DeviceBridge::Components::SystemManager();
  
  Serial.print(F("Creating ConfigurationManager...\r\n"));
  Serial.flush();
  components[CONFIGURATION_INDEX] = new DeviceBridge::Components::ConfigurationManager();
  
  Serial.print(F("Creating HeartbeatLEDManager...\r\n"));
  Serial.flush();
  components[HEARTBEAT_LED_INDEX] = new DeviceBridge::Components::HeartbeatLEDManager();
  
  Serial.print(F("Creating ConfigurationService...\r\n"));
  Serial.flush();
  configurationService = new DeviceBridge::Common::ConfigurationService();
  
  Serial.print(F("Component managers created successfully\r\n"));
  Serial.flush();
  
  // Verify component creation
  bool allComponentsCreated = true;
  for (uint8_t i = 0; i < COMPONENT_COUNT; i++) {
    if (!components[i]) {
      Serial.print(F("FATAL: Failed to create component "));
      Serial.print(i);
      Serial.print(F("\r\n"));
      allComponentsCreated = false;
    }
  }
  if (!configurationService || !allComponentsCreated) {
    Serial.print(F("FATAL: Failed to create component managers\r\n"));
    while(1) { delay(1000); }
  }
  
  // Register all components with ServiceLocator
  Serial.print(F("Registering components with ServiceLocator...\r\n"));
  services.registerDisplay(&display);
  services.registerParallelPortManager(static_cast<DeviceBridge::Components::ParallelPortManager*>(components[PARALLEL_PORT_INDEX]));
  services.registerFileSystemManager(static_cast<DeviceBridge::Components::FileSystemManager*>(components[FILE_SYSTEM_INDEX]));
  services.registerDisplayManager(static_cast<DeviceBridge::Components::DisplayManager*>(components[DISPLAY_INDEX]));
  services.registerTimeManager(static_cast<DeviceBridge::Components::TimeManager*>(components[TIME_INDEX]));
  services.registerSystemManager(static_cast<DeviceBridge::Components::SystemManager*>(components[SYSTEM_INDEX]));
  services.registerConfigurationManager(static_cast<DeviceBridge::Components::ConfigurationManager*>(components[CONFIGURATION_INDEX]));
  services.registerHeartbeatLEDManager(static_cast<DeviceBridge::Components::HeartbeatLEDManager*>(components[HEARTBEAT_LED_INDEX]));
  services.registerConfigurationService(configurationService);
  
  // Validate all dependencies are registered
  if (!services.validateAllDependencies()) {
    Serial.print(F("FATAL: Service dependency validation failed\r\n"));
    while(1) { delay(1000); }
  }
  
  // Initialize all components
  Serial.print(F("Initializing components...\r\n"));
  
  for (uint8_t i = 0; i < COMPONENT_COUNT; i++) {
    Serial.print(F("Initializing component "));
    Serial.print(i);
    Serial.print(F(": "));
    Serial.print(components[i]->getComponentName());
    Serial.print(F("...\r\n"));
    Serial.flush();
    
    if (!components[i]->initialize()) {
      Serial.print(F("WARNING: Component "));
      Serial.print(components[i]->getComponentName());
      Serial.print(F(" initialization failed\r\n"));
    } else {
      Serial.print(F("Component "));
      Serial.print(components[i]->getComponentName());
      Serial.print(F(" initialized OK\r\n"));
    }
    Serial.flush();
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
}

void loop()
{
  unsigned long currentTime = millis();
  
  // Update all components using encapsulated timing
  for (uint8_t i = 0; i < COMPONENT_COUNT; i++) {
    if (components[i]->shouldUpdate(currentTime)) {
      components[i]->update(currentTime);
      components[i]->markUpdated(currentTime);
    }
  }
  
  // Small delay to prevent overwhelming the CPU
  delayMicroseconds(10);
}