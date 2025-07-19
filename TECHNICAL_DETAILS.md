# Technical Implementation Details

## Component Manager Conversion Details

### DisplayManager.cpp - Complete Rewrite
- **Before**: FreeRTOS task with `xTaskCreate()`, `xQueueReceive()`, mutex operations
- **After**: Loop-based `update()` method with `millis()` timing
- **Communication**: Direct component references via `setTimeManager()`, `setSystemManager()`
- **Features**: OSEPP button handling, menu system, direct callbacks

### FileSystemManager.cpp - Direct Callback Architecture
- **Before**: Task-based queue processing with `xSemaphoreTake()`/`xSemaphoreGive()`
- **After**: `processDataChunk()` method for immediate data processing
- **Communication**: Direct display messages via `setDisplayManager()`
- **Features**: SD/EEPROM/Serial storage failover, file type management

### TimeManager.cpp - Simplified RTC Interface  
- **Before**: FreeRTOS task with I2C mutex protection, `vTaskDelayUntil()`
- **After**: Periodic `update()` using `millis()` timing
- **Communication**: Direct time display updates to DisplayManager
- **Features**: DS1307 RTC integration, Unix timestamp support

### SystemManager.cpp - Arduino-Native Monitoring
- **Before**: Complex task health monitoring with `uxTaskGetStackHighWaterMark()`
- **After**: Simple system monitoring with Arduino `freeRam()` function
- **Communication**: Direct component references via `setComponentManagers()`
- **Features**: System status, error handling, memory monitoring

### ParallelPortManager.cpp - Callback-Based Data Capture
- **Before**: Task-based with queue communication for data chunks
- **After**: Direct callbacks to FileSystemManager via `processDataChunk()`
- **Features**: Real-time LPT data capture, file boundary detection

## Communication Architecture Transformation
- **Before**: `xQueueSend()` → `xQueueReceive()` with blocking operations
- **After**: Direct method calls and callback functions
- **Examples**:
  - `xQueueSend(displayQueue, &msg)` → `_displayManager->displayMessage(type, message)`
  - `xQueueReceive(dataQueue, &chunk)` → `_fileSystemManager->processDataChunk(chunk)`

## Pin Assignments (Arduino Mega)
```cpp
// LCD Shield (OSEPP)
LCD_RESET: 8, LCD_ENABLE: 9, LCD_D4-D7: 4-7, Buttons: A0

// Storage  
SD_CS: 10, EEPROM_CS: 3, SPI: 50-53, I2C: 20-21

// Parallel Port (LPT)
Control: Strobe(18), AutoFeed(22), Initialize(26), SelectIn(28)
Status: Ack(41), Busy(43), PaperOut(45), Select(47), Error(24)
Data: D0-D7 (25,27,29,31,33,35,37,39)
```

## OSEPP Button Specifications
Expected analog values for button detection:
- **RIGHT**: ~0
- **UP**: ~144  
- **DOWN**: ~329
- **LEFT**: ~504
- **SELECT**: ~741
- **NONE**: ~1023

## Memory Optimization Techniques Applied
1. **FreeRTOS Elimination**: Removed ~4KB overhead
2. **String Flash Storage**: F() macro for all literals
3. **Direct Communication**: Eliminated queue memory
4. **Static Allocation**: No heap fragmentation
5. **Component Lifecycle**: Proper initialization/cleanup