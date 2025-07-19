# Claude Implementation Notes

## Project Context
**Device Bridge** - Arduino Mega 2560 project for bridging Tektronix TDS2024 oscilloscope parallel port to modern USB/storage

### Hardware Specifications (CONFIRMED)
- **MCU**: Arduino Mega 2560 (ATmega2560, 8KB SRAM, 256KB Flash)
- **LCD Shield**: OSEPP LCD Keypad Shield (revision 1) - 16x2 LCD with 6 buttons
- **Data Logger Shield**: Deek Robot Data Logging Shield v1.0 (rewired for Mega compatibility)
  - **SD Card**: SPI interface via modified shield
  - **RTC**: DS1307 I2C real-time clock
  - **EEPROM**: Winbond W25Q128FVSG (128Mbit = 16MB SPI Flash) - user added
- **Parallel Port**: Custom breakout for DB-25 connector to TDS2024

## Architecture Decisions

### FreeRTOS Task Design
Chose task-based approach over cooperative multitasking for several reasons:
- **Real-time Requirements**: Parallel port interrupts need deterministic response
- **Concurrent I/O**: Must capture data while writing to storage simultaneously  
- **Resource Contention**: SPI bus shared between SD card and EEPROM needs protection
- **User Interface**: LCD updates shouldn't block critical data capture

### Task Priorities
1. **ParallelPortTask (Priority 3)**: Highest - must respond to interrupts immediately
2. **FileManagerTask (Priority 2)**: Medium - storage operations are important but not real-time
3. **DisplayTask, RTCTask, SystemMonitorTask (Priority 1)**: Low - user interface and monitoring

### Memory Management
- **Task Stacks**: Conservative sizing (256-512 bytes) due to Arduino Mega's 8KB RAM
- **Queue Sizing**: Balanced between responsiveness and memory usage
- **Buffer Strategy**: 512-byte chunks match SD card sector size for efficiency

## Hardware Integration Notes

### Parallel Port (LPT) Interface
- Uses existing interrupt-driven approach from original code
- Ring buffer maintained for compatibility with existing Port class
- FreeRTOS tasks poll buffer rather than blocking interrupts

### Pin Assignments (from Pinouts.md)
```
LCD Shield: pins 4-9 (4-bit mode)
SD Card: SPI bus + pin 10 (CS)
EEPROM: SPI bus + pin 3 (CS)  
RTC: I2C bus (SCL/SDA)
Parallel Port: pins 18-47 (control, status, data)
```

### SPI Bus Sharing
SD card and EEPROM share SPI bus - requires mutex protection to prevent corruption during concurrent access.

## File Detection Strategy
**Problem**: No explicit "start/end of file" signals from TDS2024
**Solution**: Use timeout-based detection (2 seconds of idle = end of file)

**Challenges**:
- False end-of-file if transmission pauses
- Need to distinguish between files and continuous data stream
- Future: Analyze data headers to detect file boundaries

## Storage Architecture
**Primary**: SD card (FAT filesystem via SdFat library)
**Secondary**: EEPROM W25Q128 (16MB) with NASA EEFS filesystem
**Fallback**: Direct serial transfer to PC

## Error Handling Philosophy
- **Graceful Degradation**: If SD fails, try EEPROM; if EEPROM fails, try serial
- **Queue Monitoring**: System monitor task watches for buffer overflows
- **User Feedback**: Display task shows error messages immediately
- **Recovery**: Tasks designed to continue operation after errors

## Development Notes

### Code Organization
```
src/
├── main.cpp - FreeRTOS task definitions and setup
├── Parallel/ - LPT port hardware abstraction
├── User/ - LCD display and interface  
└── Storage/ - File system abstraction (future)
```

### Testing Strategy
1. **Unit Testing**: Individual task functions with mock hardware
2. **Integration Testing**: Task communication via queues
3. **Hardware Testing**: With actual TDS2024 oscilloscope
4. **Stress Testing**: Continuous operation and error injection

### Future Enhancements
- **File Type Detection**: Parse headers to determine .bmp, .png, etc.
- **Configuration Management**: Store settings in EEPROM
- **Web Interface**: ESP32 module for wireless configuration
- **Data Compression**: Reduce storage requirements
- **Faster Transfer**: DMA for parallel port reading

## Memory Usage Estimates
```
Task Stacks: 5 × 256-512 bytes = ~2KB
Queues: (8×528) + (4×36) + (4×4) = ~4.5KB  
Static Variables: ~1KB
Available for heap: ~500 bytes
```

**Note**: Very tight on RAM - monitor stack usage carefully

## Lessons Learned
1. **Arduino FreeRTOS**: Limited by 8KB RAM - requires careful memory management
2. **Interrupt Handling**: Keep ISRs minimal, use task notifications for processing
3. **Hardware Sharing**: Always protect shared resources with mutexes
4. **Error Recovery**: Design for graceful degradation from the start
5. **Documentation**: Critical for embedded systems - hardware details change frequently

## Current Implementation Status (2025-07-19)

### Component Architecture - COMPLETED ✅
Created clean component-based architecture:
- **Components/ParallelPortManager** - LPT data capture with file boundary detection
- **Components/FileSystemManager** - Unified storage interface (SD/EEPROM/Serial)
- **Components/DisplayManager** - LCD interface with menu system and button handling
- **Components/TimeManager** - RTC integration and time services
- **Components/SystemManager** - System coordination, monitoring, and configuration

### FreeRTOS Integration - COMPLETED ✅
- Task-based architecture with priority scheduling
- Inter-task communication via queues (DataChunk, DisplayMessage, SystemCommand)
- Resource protection with mutexes (SPI, I2C, Serial)
- Component lifecycle management
- System health monitoring

### File Structure - COMPLETED ✅
```
src/
├── main.cpp - System initialization and component coordination
├── Common/ - Shared types and configuration constants
│   ├── Types.h - Data structures for inter-component communication
│   └── Config.h - System configuration and pin definitions
├── Components/ - Component manager classes
│   ├── ParallelPortManager.h/cpp - LPT data capture
│   ├── FileSystemManager.h/cpp - Storage operations  
│   ├── DisplayManager.h/cpp - LCD and user interface
│   ├── TimeManager.h/cpp - RTC and time services
│   └── SystemManager.h/cpp - System monitoring
├── Parallel/ - Hardware abstraction (existing)
│   ├── Port.h/cpp - LPT port management
│   ├── Control.h/cpp - Control signal management
│   ├── Status.h/cpp - Status signal management
│   └── Data.h/cpp - Data line management
└── User/ - User interface (existing, enhanced)
    └── Display.h/cpp - LCD display control
```

### Hardware Integration Status
- **Parallel Port**: ✅ Existing implementation working, enhanced for FreeRTOS
- **LCD Display**: ✅ Enhanced with button handling for OSEPP LCD Keypad Shield v1
- **SD Card**: ✅ Implemented via SdFat library with mutex protection
- **EEPROM (W25Q128FVSG)**: 🚧 Structure ready, needs filesystem implementation
- **RTC (DS1307)**: ✅ Basic implementation, needs time setting features

### Next Critical Tasks
1. **Complete main.cpp refactoring** - Convert old task functions to component initialization
2. **EEPROM filesystem integration** - Implement W25Q128FVSG support with wear leveling
3. **Button mapping verification** - Confirm OSEPP shield v1 button analog values
4. **File type detection** - Add header-based file format detection
5. **Hardware testing** - Test with actual TDS2024 oscilloscope

### Memory Management Strategy
- **Static allocation**: All components allocated in main.cpp (no dynamic allocation)
- **Task stacks**: Conservative sizing (256-512 bytes) for 8KB SRAM limit
- **Queue memory**: ~4.5KB total for inter-task communication
- **Available heap**: ~700 bytes remaining for temporary allocations

### Key Design Decisions Made
- **Component managers over service classes**: Better resource ownership model
- **Queue-based communication**: Decoupled inter-task messaging  
- **Mutex-protected hardware**: Prevents SPI/I2C bus conflicts
- **Timeout-based file detection**: 2-second idle = end of file
- **Graceful storage degradation**: SD → EEPROM → Serial fallback

## Key Files to Monitor
- `platformio.ini` - Dependencies and build configuration
- `main.cpp` - System initialization and component coordination
- `Components/*.cpp` - Component implementations
- `Common/Config.h` - System configuration and pin mappings
- `Common/Types.h` - Inter-component communication structures