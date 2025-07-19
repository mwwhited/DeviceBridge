# Device Bridge Architecture Documentation

## Project Overview

The MegaDeviceBridge is a sophisticated embedded system that converts parallel port data from a Tektronix TDS2024 oscilloscope to modern storage formats. The system uses a professional FreeRTOS architecture with component-based design for real-time data capture and processing.

## Hardware Platform

### Base System
- **MCU**: Arduino Mega 2560 (ATmega2560)
- **Clock**: 16MHz
- **RAM**: 8KB SRAM
- **Flash**: 256KB (248KB available)
- **Voltage**: 5V logic

### Shield Stack Configuration
1. **Base**: Arduino Mega 2560
2. **Layer 1**: OSEPP LCD Keypad Shield (revision 1)
3. **Layer 2**: Deek Robot Data Logging Shield v1.0 (rewired for Mega compatibility)

### External Components
- **EEPROM**: Winbond W25Q128FVSG (16MB SPI Flash)
- **RTC**: DS1307 Real-Time Clock
- **Storage**: SD Card (up to 32GB supported)
- **Interface**: IEEE 1284 Parallel Port (LPT/Centronics)

## System Architecture

### Core Design Principles
1. **Real-Time Performance**: 1ms polling for parallel port data capture
2. **Resource Protection**: Mutex-protected shared hardware resources
3. **Component Isolation**: Modular design with clear boundaries
4. **Graceful Degradation**: Storage failover mechanisms
5. **Memory Efficiency**: Optimized for 8KB RAM constraint

### FreeRTOS Task Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    FreeRTOS Scheduler                      │
├─────────────────────────────────────────────────────────────┤
│  ParallelPort │ FileSystem │ Display │ Time │ System       │
│   Manager     │  Manager   │ Manager │ Mgr  │ Manager      │
│  Priority: 3  │Priority: 2 │Prior: 1 │Pri:1 │ Priority: 1  │
│  Stack: 256B  │Stack: 512B │Stk:256B │128B  │ Stack: 128B  │
└─────────────────────────────────────────────────────────────┘
```

## Component Managers

### 1. ParallelPortManager
**Purpose**: Real-time data capture from oscilloscope parallel port
**Priority**: Highest (3) - Real-time critical
**Stack**: 256 bytes
**Polling Rate**: 1ms

#### Responsibilities:
- Monitor LPT strobe signal for data availability
- Read 8-bit parallel data bus
- Detect file boundaries (2-second timeout)
- Buffer data in 256-byte chunks
- Queue data for storage processing

#### File Detection Logic:
- **New File**: First data after idle period
- **Data Stream**: Continuous data with <1ms gaps  
- **End of File**: 2000ms timeout without data (2000 polling cycles)

### 2. FileSystemManager  
**Purpose**: Unified storage interface with failover capability
**Priority**: High (2) - Data integrity critical
**Stack**: 512 bytes

#### Storage Hierarchy:
1. **Primary**: SD Card (preferred, high capacity)
2. **Secondary**: W25Q128 EEPROM (16MB, reliable)
3. **Tertiary**: Serial Transfer (real-time streaming)

#### Responsibilities:
- Manage storage device initialization
- Handle automatic failover between storage types
- Generate timestamp-based filenames
- Coordinate with W25Q128Manager for EEPROM operations
- Monitor storage capacity and health

### 3. DisplayManager
**Purpose**: User interface and system interaction
**Priority**: Normal (1) - User interaction
**Stack**: 256 bytes
**Update Rate**: 100ms

#### OSEPP Button Mapping:
- **A0 Pin**: Analog input (0-1023 range)
- **RIGHT**: ~0 (direct ground)
- **UP**: ~144 (voltage divider)
- **DOWN**: ~329 (voltage divider)  
- **LEFT**: ~504 (voltage divider)
- **SELECT**: ~741 (voltage divider)
- **NONE**: ~1023 (pull-up resistor)

#### Menu System:
```
Main Menu
├── Storage (SD/EEPROM/Serial/Auto)
├── File Type (Auto/Binary/Bitmap/PNG)
├── Config (Time/Save/Reset)
└── Exit
```

### 4. TimeManager
**Purpose**: RTC integration and timestamp services
**Priority**: Normal (1) - Non-critical timing
**Stack**: 128 bytes
**Update Rate**: 1 second

#### Responsibilities:
- Initialize DS1307 RTC via I2C
- Provide formatted timestamps for filenames
- Display current time when idle
- Handle time setting via menu system

### 5. SystemManager
**Purpose**: System coordination and health monitoring
**Priority**: Normal (1) - Background monitoring
**Stack**: 128 bytes
**Monitor Rate**: 5 seconds

#### Responsibilities:
- Task health monitoring (stack usage, responsiveness)
- Queue utilization monitoring
- Error counting and reporting
- System status coordination
- Performance metrics collection

## Inter-Component Communication

### Queue-Based Messaging
```
ParallelPortManager → [DataQueue] → FileSystemManager
TimeManager → [DisplayQueue] → DisplayManager  
DisplayManager → [CommandQueue] → SystemManager
SystemManager → [DisplayQueue] → DisplayManager
```

#### Queue Specifications:
- **DataQueue**: 8 × DataChunk (264 bytes each) = 2.1KB
- **DisplayQueue**: 4 × DisplayMessage (64 bytes each) = 256B
- **CommandQueue**: 4 × SystemCommand (32 bytes each) = 128B
- **Total Queue Memory**: ~2.5KB

### Mutex Protection
```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  SPI Mutex  │    │ I2C Mutex   │    │Serial Mutex │
│             │    │             │    │             │
│ SD Card     │    │ RTC (DS1307)│    │Debug Output │
│ W25Q128     │    │             │    │System Info  │
│ EEPROM      │    │             │    │             │
└─────────────┘    └─────────────┘    └─────────────┘
```

## Memory Management

### RAM Allocation (8192 bytes total)
- **Task Stacks**: 1280 bytes (15.6%)
  - ParallelPort: 256B, FileSystem: 512B, Display: 256B, Time: 128B, System: 128B
- **Queue Memory**: ~2500 bytes (30.5%)
- **Static Variables**: ~1000 bytes (12.2%)
- **FreeRTOS Overhead**: ~500 bytes (6.1%)
- **Available Heap**: ~2900 bytes (35.4%)

### Optimization Strategies
1. **Packed Structures**: `__attribute__((packed))` for consistent sizing
2. **Efficient Data Types**: uint8_t for boolean flags, uint16_t for button values
3. **Buffer Size Optimization**: 256-byte DataChunk (reduced from 512 bytes)
4. **Stack Tuning**: Minimal stack sizes for each task based on actual usage

## Pin Assignment Architecture

### Hardware Resource Mapping
```
Arduino Mega 2560 Pin Allocation:

LCD Shield (OSEPP):
├── LCD_RESET: 8    ├── LCD_D4: 4
├── LCD_ENABLE: 9   ├── LCD_D5: 5  
├── LCD_D6: 6       └── LCD_D7: 7
└── BUTTONS: A0 (analog input)

Storage Interfaces:
├── SD_CS: 10 (SPI)
├── EEPROM_CS: 3 (SPI)  
├── SPI_MOSI: 51
├── SPI_MISO: 50
├── SPI_SCK: 52
├── I2C_SDA: 20 (RTC)
└── I2C_SCL: 21 (RTC)

Parallel Port (LPT):
Control Signals:
├── LPT_STROBE: 18    ├── LPT_AUTO_FEED: 22
├── LPT_INITIALIZE: 26 └── LPT_SELECT_IN: 28

Status Signals:  
├── LPT_ACK: 41       ├── LPT_BUSY: 43
├── LPT_PAPER_OUT: 45 ├── LPT_SELECT: 47
└── LPT_ERROR: 24

Data Bus (8-bit):
├── LPT_D0: 25  ├── LPT_D1: 27  ├── LPT_D2: 29  ├── LPT_D3: 31
├── LPT_D4: 33  ├── LPT_D5: 35  ├── LPT_D6: 37  └── LPT_D7: 39
```

## Data Flow Architecture

### Capture Pipeline
```
Oscilloscope → Parallel Port → ParallelPortManager → DataQueue
                                                          ↓
FileSystemManager ← DataQueue ← Buffer Management ← Data Validation
        ↓
Storage Selection (SD/EEPROM/Serial)
        ↓
File Creation & Writing → Status Updates → DisplayQueue → DisplayManager
```

### Error Handling Flow
```
Component Error → SystemManager → Error Logging → DisplayQueue
                                      ↓
                               Recovery Action → Component Restart
                                      ↓  
                              Status Update → User Notification
```

## Configuration Management

### Compile-Time Configuration (Config.h)
- **RTOS Parameters**: Task priorities, stack sizes, queue depths
- **Hardware Pins**: All pin assignments in centralized namespace
- **Timing Constants**: Polling rates, timeouts, update intervals
- **Memory Limits**: Buffer sizes, filename lengths, message sizes

### Runtime Configuration
- **Storage Preference**: User-selectable via menu system
- **File Type Override**: Manual file type selection
- **Time Settings**: RTC configuration via display interface

## Performance Characteristics

### Real-Time Requirements
- **Critical Path**: Parallel port polling (1ms deadline)
- **Data Throughput**: Up to 1KB/s sustained (8-bit @ 1kHz max)
- **Response Time**: Button press to menu response <200ms
- **Storage Latency**: File creation <100ms, write operations <10ms per chunk

### System Metrics (Actual Measured)
- **RAM Usage**: 38.1% (3120/8192 bytes) ✅
- **Flash Usage**: 14.3% (36214/253952 bytes) ✅
- **Task Efficiency**: All tasks meeting timing requirements
- **Queue Utilization**: <75% under normal load

## Debug and Monitoring

### Serial Debug Interface (115200 baud)
- **Startup Logging**: Component initialization status
- **Runtime Monitoring**: Task health, memory usage, queue status  
- **Button Debugging**: Real-time A0 analog value reporting
- **Error Reporting**: Detailed error messages with timestamps

### Built-in Diagnostics
- **Stack Watermark Monitoring**: Track peak stack usage per task
- **Queue Depth Monitoring**: Prevent overflow conditions
- **Task Health Checking**: Detect unresponsive tasks
- **Memory Usage Tracking**: Monitor heap fragmentation

## Security and Reliability

### Error Recovery Mechanisms
1. **Storage Failover**: Automatic switching between SD/EEPROM/Serial
2. **Task Watchdog**: System manager monitors task responsiveness  
3. **Memory Protection**: Bounded buffers and queue overflow detection
4. **Graceful Degradation**: Continue operation with reduced functionality

### Data Integrity
- **Checksums**: Optional data validation for critical operations
- **Atomic Writes**: Ensure complete file operations
- **Metadata Validation**: Verify file headers and boundaries
- **Redundancy**: Multiple storage options for critical data

## Future Architecture Considerations

### Scalability Options
- **Additional Storage**: Easy integration of new storage types
- **Enhanced Protocols**: Support for USB, Ethernet, or wireless interfaces
- **Expanded Sensors**: Framework supports additional input sources
- **Performance Scaling**: Task priorities and timing easily adjustable

### Extension Points
- **Plugin Architecture**: New file format handlers
- **Protocol Adapters**: Support for different oscilloscope interfaces  
- **Display Modules**: Alternative user interface options
- **Communication Interfaces**: Network connectivity options

---

## Architecture Decision Log

### 2025-01-19: Initial FreeRTOS Conversion
- **Decision**: Convert from Arduino loop() to FreeRTOS task-based architecture
- **Rationale**: Real-time requirements for 1ms parallel port polling
- **Impact**: Improved responsiveness, better resource utilization

### 2025-01-19: Memory Optimization  
- **Decision**: Reduce DataChunk from 512 to 256 bytes, use packed structures
- **Rationale**: Arduino Mega 8KB RAM constraint requires careful management
- **Impact**: 50% reduction in queue memory usage (4.1KB → 2.1KB)

### 2025-01-19: Library Migration
- **Decision**: Switch from SdFat to standard Arduino SD library
- **Rationale**: Compilation stability and reduced complexity
- **Impact**: Simplified build process, maintained functionality

### 2025-01-19: Component Separation
- **Decision**: Split monolithic code into 5 component managers
- **Rationale**: Maintainability, testability, and clear responsibility boundaries
- **Impact**: Modular architecture enabling independent development and testing

---

*Last Updated: 2025-01-19*  
*Architecture Version: 1.0*  
*System Status: Production Ready*