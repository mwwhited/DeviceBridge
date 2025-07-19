#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

// Hardware abstraction layers
#include "./Parallel/Port.h"
#include "./User/Display.h"

// Component managers
#include "./Components/ParallelPortManager.h"
#include "./Components/FileSystemManager.h"
#include "./Components/DisplayManager.h"
#include "./Components/TimeManager.h"
#include "./Components/SystemManager.h"

// Common definitions
#include "./Common/Types.h"
#include "./Common/Config.h"

// FreeRTOS Communication Objects
QueueHandle_t dataQueue = NULL;
QueueHandle_t displayQueue = NULL;
QueueHandle_t commandQueue = NULL;

SemaphoreHandle_t spiMutex = NULL;
SemaphoreHandle_t i2cMutex = NULL;
SemaphoreHandle_t serialMutex = NULL;

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

void setup()
{
  Serial.begin(DeviceBridge::Common::Serial::BAUD_RATE);
  while (!Serial) { delay(10); }
  
  Serial.println("Device Bridge Initializing...");
  
  // Initialize hardware
  printerPort.initialize();
  display.initialize();
  
  // Create FreeRTOS communication objects
  dataQueue = xQueueCreate(
    DeviceBridge::Common::RTOS::DATA_QUEUE_SIZE, 
    sizeof(DeviceBridge::Common::DataChunk));
  displayQueue = xQueueCreate(
    DeviceBridge::Common::RTOS::DISPLAY_QUEUE_SIZE, 
    sizeof(DeviceBridge::Common::DisplayMessage));
  commandQueue = xQueueCreate(
    DeviceBridge::Common::RTOS::COMMAND_QUEUE_SIZE, 
    sizeof(DeviceBridge::Common::SystemCommand));
  
  spiMutex = xSemaphoreCreateMutex();
  i2cMutex = xSemaphoreCreateMutex();
  serialMutex = xSemaphoreCreateMutex();
  
  // Verify FreeRTOS objects created successfully
  if (!dataQueue || !displayQueue || !commandQueue || 
      !spiMutex || !i2cMutex || !serialMutex) {
    Serial.println("FATAL: Failed to create FreeRTOS objects");
    while(1) { delay(1000); }
  }
  
  // Create component managers
  parallelPortManager = new DeviceBridge::Components::ParallelPortManager(printerPort, dataQueue);
  fileSystemManager = new DeviceBridge::Components::FileSystemManager(dataQueue, displayQueue, spiMutex);
  displayManager = new DeviceBridge::Components::DisplayManager(display, displayQueue, commandQueue);
  timeManager = new DeviceBridge::Components::TimeManager(displayQueue, i2cMutex);
  systemManager = new DeviceBridge::Components::SystemManager(commandQueue, displayQueue, serialMutex);
  
  // Verify component creation
  if (!parallelPortManager || !fileSystemManager || !displayManager || 
      !timeManager || !systemManager) {
    Serial.println("FATAL: Failed to create component managers");
    while(1) { delay(1000); }
  }
  
  // Set up component cross-references for system manager
  systemManager->setParallelPortManager(parallelPortManager);
  systemManager->setFileSystemManager(fileSystemManager);
  systemManager->setDisplayManager(displayManager);
  systemManager->setTimeManager(timeManager);
  
  // Initialize all components
  Serial.println("Initializing components...");
  
  if (!parallelPortManager->initialize()) {
    Serial.println("WARNING: Parallel port initialization failed");
  }
  
  if (!fileSystemManager->initialize()) {
    Serial.println("WARNING: File system initialization failed");
  }
  
  if (!displayManager->initialize()) {
    Serial.println("WARNING: Display initialization failed");
  }
  
  if (!timeManager->initialize()) {
    Serial.println("WARNING: Time manager initialization failed");
  }
  
  if (!systemManager->initialize()) {
    Serial.println("WARNING: System manager initialization failed");
  }
  
  // Start all component tasks
  Serial.println("Starting component tasks...");
  
  if (!parallelPortManager->start()) {
    Serial.println("FATAL: Failed to start parallel port task");
    while(1) { delay(1000); }
  }
  
  if (!fileSystemManager->start()) {
    Serial.println("FATAL: Failed to start file manager task");
    while(1) { delay(1000); }
  }
  
  if (!displayManager->start()) {
    Serial.println("FATAL: Failed to start display task");
    while(1) { delay(1000); }
  }
  
  if (!timeManager->start()) {
    Serial.println("FATAL: Failed to start time manager task");
    while(1) { delay(1000); }
  }
  
  if (!systemManager->start()) {
    Serial.println("FATAL: Failed to start system manager task");
    while(1) { delay(1000); }
  }
  
  Serial.println("All systems initialized successfully!");
  Serial.println("Starting FreeRTOS scheduler...");
  
  // Start FreeRTOS scheduler - this never returns
  vTaskStartScheduler();
  
  // Should never reach here
  Serial.println("FATAL: FreeRTOS scheduler failed to start");
  while(1) { delay(1000); }
}

void loop()
{
  // FreeRTOS scheduler handles execution, this should never be reached
  Serial.println("ERROR: Reached Arduino loop() - FreeRTOS scheduler problem!");
  delay(1000);
}