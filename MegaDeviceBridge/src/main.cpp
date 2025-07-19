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
  
  Serial.print(F("Device Bridge Initializing...\r\n"));
  
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
    Serial.print(F("FATAL: Failed to create FreeRTOS objects\r\n"));
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
    Serial.print(F("FATAL: Failed to create component managers\r\n"));
    while(1) { delay(1000); }
  }
  
  // Set up component cross-references for system manager
  systemManager->setParallelPortManager(parallelPortManager);
  systemManager->setFileSystemManager(fileSystemManager);
  systemManager->setDisplayManager(displayManager);
  systemManager->setTimeManager(timeManager);
  
  // Initialize all components
  Serial.print(F("Initializing components...\r\n"));
  
  if (!parallelPortManager->initialize()) {
    Serial.print("WARNING: Parallel port initialization failed\r\n");
  }
  
  if (!fileSystemManager->initialize()) {
    Serial.print("WARNING: File system initialization failed\r\n");
  }
  
  if (!displayManager->initialize()) {
    Serial.print("WARNING: Display initialization failed\r\n");
  }
  
  if (!timeManager->initialize()) {
    Serial.print("WARNING: Time manager initialization failed\r\n");
  }
  
  if (!systemManager->initialize()) {
    Serial.print("WARNING: System manager initialization failed\r\n");
  }
  
  // Start all component tasks
  Serial.print(F("Starting component tasks...\r\n"));
  
  Serial.print(F("Starting parallel port manager...\r\n"));
  if (!parallelPortManager->start()) {
    Serial.print(F("FATAL: Failed to start parallel port task\r\n"));
    while(1) { delay(1000); }
  }
  Serial.print(F("Parallel port manager started OK\r\n"));
  
  Serial.print(F("Starting file system manager...\r\n"));
  if (!fileSystemManager->start()) {
    Serial.print(F("FATAL: Failed to start file manager task\r\n"));
    while(1) { delay(1000); }
  }
  Serial.print(F("File system manager started OK\r\n"));
  
  Serial.print(F("Starting display manager...\r\n"));
  if (!displayManager->start()) {
    Serial.print(F("FATAL: Failed to start display task\r\n"));
    while(1) { delay(1000); }
  }
  Serial.print(F("Display manager started OK\r\n"));
  
  Serial.print(F("Starting time manager...\r\n"));
  if (!timeManager->start()) {
    Serial.print(F("FATAL: Failed to start time manager task\r\n"));
    while(1) { delay(1000); }
  }
  Serial.print(F("Time manager started OK\r\n"));
  
  Serial.print(F("Starting system manager...\r\n"));
  if (!systemManager->start()) {
    Serial.print(F("FATAL: Failed to start system manager task\r\n"));
    while(1) { delay(1000); }
  }
  Serial.print(F("System manager started OK\r\n"));
  
  Serial.print(F("About to print success message...\r\n"));
  delay(100);
  Serial.print(F("All systems initialized successfully!\r\n"));
  delay(100);
  Serial.print(F("About to start scheduler...\r\n"));
  delay(100);
  Serial.print(F("Starting FreeRTOS scheduler...\r\n"));
  delay(100);
  
  // Start FreeRTOS scheduler - this never returns
  vTaskStartScheduler();
  
  // Should never reach here
  Serial.print(F("FATAL: FreeRTOS scheduler failed to start\r\n"));
  while(1) { delay(1000); }
}

void loop()
{
  // FreeRTOS scheduler handles execution, this should never be reached
  Serial.print(F("ERROR: Reached Arduino loop() - FreeRTOS scheduler problem!\r\n"));
  delay(1000);
}