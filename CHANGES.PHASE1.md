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

## 2025-07-19 - Component-Based Architecture

### Added
- **Component-Based Design**: Separated monolithic main.cpp into discrete component classes
- **Component Managers**:
  - ParallelPortManager - LPT data capture with file boundary detection
  - FileSystemManager - Unified storage interface (SD/EEPROM/Serial)
  - DisplayManager - LCD interface with menu system and button handling  
  - TimeManager - RTC integration and time services
  - SystemManager - System coordination, monitoring, and configuration

- **Common Infrastructure**:
  - Common/Types.h - Shared data structures for inter-component communication
  - Common/Config.h - System configuration constants and pin definitions
  - ARCHITECTURE.md - Design decisions and system documentation

- **Enhanced main.cpp**:
  - Component lifecycle management (initialize → start → monitor)
  - Robust error handling with descriptive serial output
  - Clean separation of hardware initialization and component coordination
  - Proper FreeRTOS object verification

### Changed
- **main.cpp**: Complete refactor from inline task functions to component orchestration
- **Pin Definitions**: Centralized in Common/Config.h with proper namespace organization
- **Error Handling**: Enhanced with component-level error reporting and recovery
- **Memory Management**: Static allocation strategy for all components

### Architecture Benefits
- **Maintainability**: Clear separation of concerns between subsystems
- **Testability**: Components can be unit tested independently
- **Scalability**: Easy to add new components or modify existing ones
- **Debugging**: Component-level isolation simplifies troubleshooting
- **Code Reuse**: Components are self-contained and reusable

### Hardware-Specific Updates
- **OSEPP LCD Keypad Shield v1**: Button mapping and analog value constants
- **Winbond 25Q128FVSG**: Structure prepared for 16MB SPI Flash filesystem
- **Arduino Mega 2560**: Pin assignments verified and organized by function

### File Structure
```
src/
├── main.cpp (184 lines) - System initialization and component coordination
├── Common/ - Shared infrastructure
│   ├── Types.h - Inter-component communication structures  
│   └── Config.h - System configuration and pin definitions
├── Components/ - Component manager classes (~1200 lines total)
│   ├── ParallelPortManager.h/cpp - LPT data capture
│   ├── FileSystemManager.h/cpp - Storage operations
│   ├── DisplayManager.h/cpp - LCD and user interface
│   ├── TimeManager.h/cpp - RTC and time services
│   └── SystemManager.h/cpp - System monitoring and coordination
├── Parallel/ - Hardware abstraction (existing, enhanced)
└── User/ - User interface (existing, enhanced)
```

### Memory Usage Optimization
- **Static Allocation**: All components created at startup (no dynamic allocation)
- **Stack Sizing**: Component-specific stack sizes based on actual usage
- **Queue Management**: Centralized queue creation with proper error checking
- **Resource Protection**: Mutex usage optimized for minimal contention