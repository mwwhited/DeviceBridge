# Device Bridge Change History

## 2025-07-18 - FreeRTOS Implementation

### Added
- **FreeRTOS Integration**: Converted from Arduino loop-based to RTOS task-based architecture
- **Task Architecture**: 
  - ParallelPortTask (Priority 3) - Real-time data capture from LPT port
  - FileManagerTask (Priority 2) - Storage operations (SD card/EEPROM) 
  - DisplayTask (Priority 1) - LCD updates and user interface
  - RTCTask (Priority 1) - Time management and display
  - SystemMonitorTask (Priority 1) - Resource monitoring and error handling

- **Inter-task Communication**:
  - dataQueue - 512-byte chunks from parallel port to file manager
  - displayQueue - Status messages to LCD display
  - commandQueue - User commands and configuration
  - Mutex protection for SPI, I2C, and Serial resources

- **Enhanced Display Class**:
  - Added updateStatus() method for task communication
  - Added showTime() for clock display when idle
  - Added showMenu() for user interface navigation

### Changed
- **platformio.ini**: Added feilipu/FreeRTOS@^10.4.6-4 dependency
- **main.cpp**: Complete restructure from Arduino loop to FreeRTOS scheduler
- **Display.h/cpp**: Extended interface for multi-task communication

### Technical Details
- **Memory Management**: Configured task stack sizes (256-512 bytes)
- **Timing**: 1ms polling for parallel port, 100ms display updates
- **File Detection**: 2-second idle timeout to detect end of transmission
- **Queue Sizes**: 8 data chunks, 4 display messages, 4 commands
- **Error Handling**: Queue overflow detection and status reporting

### Dependencies Added
- Arduino_FreeRTOS.h - Core RTOS functionality
- queue.h - Inter-task message queues  
- semphr.h - Semaphores and mutexes
- task.h - Task management

### Architecture Benefits
- **Concurrent Operations**: Parallel data capture while writing to storage
- **Deterministic Timing**: Real-time response to parallel port interrupts
- **Resource Protection**: Mutex-protected hardware access (SPI/I2C)
- **Scalability**: Easy to add new tasks and features
- **Error Recovery**: System monitoring and graceful degradation

### Next Steps
- Test with actual hardware and oscilloscope
- Implement file type detection algorithm
- Add button input handling for user interface
- Optimize memory usage and stack sizes