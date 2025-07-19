#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>
#include "./Parallel/Port.h"
#include "./User/Display.h"
#include <SdFat.h>
#include <RTC.h>

// FreeRTOS Task Handles
TaskHandle_t parallelPortTaskHandle = NULL;
TaskHandle_t fileManagerTaskHandle = NULL;
TaskHandle_t displayTaskHandle = NULL;
TaskHandle_t rtcTaskHandle = NULL;
TaskHandle_t systemMonitorTaskHandle = NULL;

// FreeRTOS Queues
QueueHandle_t dataQueue = NULL;
QueueHandle_t displayQueue = NULL;
QueueHandle_t commandQueue = NULL;

// FreeRTOS Semaphores
SemaphoreHandle_t spiMutex = NULL;
SemaphoreHandle_t i2cMutex = NULL;
SemaphoreHandle_t serialMutex = NULL;

// Hardware instances
DeviceBridge::Parallel::Port _printer(
    DeviceBridge::Parallel::Control(18, 22, 26, 26),
    DeviceBridge::Parallel::Status(41, 43, 45, 47, 24),
    DeviceBridge::Parallel::Data(25, 27, 29, 31, 33, 35, 37, 39));
    
DeviceBridge::User::Display _display(8, 9, 4, 5, 6, 7);
SdFat sd;
RTC rtc;

// Data structures for inter-task communication
struct DataChunk {
  uint8_t data[512];
  uint16_t length;
  uint32_t timestamp;
  bool isNewFile;
  bool isEndOfFile;
};

struct DisplayMessage {
  enum Type { STATUS, ERROR, INFO, TIME } type;
  char message[32];
};

struct Command {
  enum Type { STORAGE_SELECT, FILE_TYPE, TRANSFER_MODE } type;
  uint8_t value;
};

// Task function declarations
void parallelPortTask(void *pvParameters);
void fileManagerTask(void *pvParameters);
void displayTask(void *pvParameters);
void rtcTask(void *pvParameters);
void systemMonitorTask(void *pvParameters);

void setup()
{
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  
  // Initialize hardware
  _printer.initialize();
  _display.initialize();
  
  // Create FreeRTOS objects
  dataQueue = xQueueCreate(8, sizeof(DataChunk));
  displayQueue = xQueueCreate(4, sizeof(DisplayMessage));
  commandQueue = xQueueCreate(4, sizeof(Command));
  
  spiMutex = xSemaphoreCreateMutex();
  i2cMutex = xSemaphoreCreateMutex();
  serialMutex = xSemaphoreCreateMutex();
  
  // Create tasks
  xTaskCreate(parallelPortTask, "ParallelPort", 256, NULL, 3, &parallelPortTaskHandle);
  xTaskCreate(fileManagerTask, "FileManager", 512, NULL, 2, &fileManagerTaskHandle);
  xTaskCreate(displayTask, "Display", 256, NULL, 1, &displayTaskHandle);
  xTaskCreate(rtcTask, "RTC", 128, NULL, 1, &rtcTaskHandle);
  xTaskCreate(systemMonitorTask, "SysMonitor", 128, NULL, 1, &systemMonitorTaskHandle);
  
  // Start FreeRTOS scheduler
  vTaskStartScheduler();
}

void loop()
{
  // FreeRTOS scheduler handles execution, this should never be reached
}

// Parallel Port Task - Highest priority for real-time data capture
void parallelPortTask(void *pvParameters)
{
  DataChunk chunk;
  uint32_t idleCounter = 0;
  bool fileInProgress = false;
  
  for (;;) {
    if (_printer.hasData()) {
      idleCounter = 0;
      
      // Check for new file start (first data after idle period)
      if (!fileInProgress) {
        chunk.isNewFile = true;
        fileInProgress = true;
        
        DisplayMessage msg;
        msg.type = DisplayMessage::STATUS;
        strcpy(msg.message, "Receiving...");
        xQueueSend(displayQueue, &msg, 0);
      } else {
        chunk.isNewFile = false;
      }
      
      // Read available data
      chunk.length = _printer.readData(chunk.data, 0, sizeof(chunk.data));
      chunk.timestamp = xTaskGetTickCount();
      chunk.isEndOfFile = false;
      
      // Send data to file manager
      if (xQueueSend(dataQueue, &chunk, portMAX_DELAY) != pdTRUE) {
        DisplayMessage msg;
        msg.type = DisplayMessage::ERROR;
        strcpy(msg.message, "Buffer Full!");
        xQueueSend(displayQueue, &msg, 0);
      }
    } else {
      idleCounter++;
      
      // Detect end of file after 2 seconds of no data
      if (fileInProgress && idleCounter > 2000) {
        chunk.isEndOfFile = true;
        chunk.length = 0;
        chunk.timestamp = xTaskGetTickCount();
        xQueueSend(dataQueue, &chunk, portMAX_DELAY);
        
        fileInProgress = false;
        idleCounter = 0;
        
        DisplayMessage msg;
        msg.type = DisplayMessage::STATUS;
        strcpy(msg.message, "Complete");
        xQueueSend(displayQueue, &msg, 0);
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(1)); // 1ms polling
  }
}

// File Manager Task - Handle storage operations
void fileManagerTask(void *pvParameters)
{
  DataChunk chunk;
  SdFile currentFile;
  char filename[32];
  uint32_t fileCounter = 0;
  bool sdAvailable = false;
  
  // Initialize storage
  if (xSemaphoreTake(spiMutex, portMAX_DELAY)) {
    sdAvailable = sd.begin(10); // SD CS pin 10
    xSemaphoreGive(spiMutex);
  }
  
  for (;;) {
    if (xQueueReceive(dataQueue, &chunk, portMAX_DELAY)) {
      
      if (chunk.isNewFile) {
        // Close previous file if open
        if (currentFile.isOpen()) {
          if (xSemaphoreTake(spiMutex, portMAX_DELAY)) {
            currentFile.close();
            xSemaphoreGive(spiMutex);
          }
        }
        
        // Create new filename
        snprintf(filename, sizeof(filename), "capture%04lu.bin", fileCounter++);
        
        // Try to open new file
        if (sdAvailable && xSemaphoreTake(spiMutex, portMAX_DELAY)) {
          if (!currentFile.open(filename, O_CREAT | O_WRITE)) {
            DisplayMessage msg;
            msg.type = DisplayMessage::ERROR;
            strcpy(msg.message, "File Error");
            xQueueSend(displayQueue, &msg, 0);
          }
          xSemaphoreGive(spiMutex);
        }
      }
      
      // Write data to file
      if (chunk.length > 0 && currentFile.isOpen()) {
        if (xSemaphoreTake(spiMutex, portMAX_DELAY)) {
          currentFile.write(chunk.data, chunk.length);
          currentFile.sync(); // Ensure data is written
          xSemaphoreGive(spiMutex);
        }
      }
      
      // Close file on end
      if (chunk.isEndOfFile && currentFile.isOpen()) {
        if (xSemaphoreTake(spiMutex, portMAX_DELAY)) {
          currentFile.close();
          xSemaphoreGive(spiMutex);
        }
        
        DisplayMessage msg;
        msg.type = DisplayMessage::INFO;
        snprintf(msg.message, sizeof(msg.message), "Saved: %s", filename);
        xQueueSend(displayQueue, &msg, 0);
      }
    }
  }
}

// Display Task - Manage LCD and user interface
void displayTask(void *pvParameters)
{
  DisplayMessage msg;
  char currentMessage[32] = "Ready";
  uint32_t lastUpdate = 0;
  
  for (;;) {
    // Check for new messages
    if (xQueueReceive(displayQueue, &msg, 0)) {
      strcpy(currentMessage, msg.message);
      lastUpdate = xTaskGetTickCount();
    }
    
    // Update display
    _display.updateStatus(currentMessage);
    
    // TODO: Handle button input and menu navigation
    // TODO: Show time when idle
    
    vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz update rate
  }
}

// RTC Task - Time management
void rtcTask(void *pvParameters)
{
  if (xSemaphoreTake(i2cMutex, portMAX_DELAY)) {
    rtc.begin();
    xSemaphoreGive(i2cMutex);
  }
  
  for (;;) {
    // Update time display when idle
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY)) {
      // TODO: Read RTC and format time
      xSemaphoreGive(i2cMutex);
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000)); // Update every second
  }
}

// System Monitor Task - Resource monitoring and error handling
void systemMonitorTask(void *pvParameters)
{
  for (;;) {
    // Monitor task stack usage
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
    
    // Monitor queue levels
    UBaseType_t dataQueueLevel = uxQueueMessagesWaiting(dataQueue);
    
    // Log to serial if enabled
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(10))) {
      if (dataQueueLevel > 6) { // Warning threshold
        Serial.println("Warning: Data queue nearly full");
      }
      xSemaphoreGive(serialMutex);
    }
    
    vTaskDelay(pdMS_TO_TICKS(5000)); // Monitor every 5 seconds
  }
}