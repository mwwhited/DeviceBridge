# Device Bridge Architecture

## Design Philosophy

### Component-Based Architecture
**Decision**: Break functionality into discrete component classes rather than monolithic code
**Rationale**: 
- Improved maintainability and testability
- Clear separation of concerns
- Easier debugging and development
- Follows embedded systems best practices

### FreeRTOS Task Design
**Decision**: Use task-based architecture with priority scheduling
**Rationale**:
- Real-time requirements for parallel port communication
- Concurrent I/O operations (capture while storing)
- Deterministic response times for interrupts
- Better resource management than cooperative multitasking

## System Components

### 1. ParallelPortManager
**Responsibility**: Manage LPT communication and data buffering
**Files**: `Components/ParallelPortManager.h/cpp`
**Features**:
- Wraps existing Parallel::Port classes
- Implements file boundary detection
- Manages data chunking for queues
- Handles interrupt coordination

### 2. FileSystemManager  
**Responsibility**: Storage operations across SD card and EEPROM
**Files**: `Components/FileSystemManager.h/cpp`
**Features**:
- Unified interface for SD/EEPROM/Serial
- Automatic failover between storage types
- File naming and metadata management
- Storage health monitoring

### 3. DisplayManager
**Responsibility**: LCD interface and user interaction
**Files**: `Components/DisplayManager.h/cpp`
**Features**:
- Status display coordination
- Button input handling
- Menu system navigation
- Time display when idle

### 4. TimeManager
**Responsibility**: RTC integration and time services
**Files**: `Components/TimeManager.h/cpp` 
**Features**:
- RTC hardware abstraction
- Time formatting for display/files
- Timestamp generation for data
- Time synchronization via serial

### 5. SystemManager
**Responsibility**: System coordination and monitoring
**Files**: `Components/SystemManager.h/cpp`
**Features**:
- Task health monitoring
- Resource usage tracking
- Error handling and recovery
- Configuration management

## Communication Architecture

### Queue-Based Messaging
```
ParallelPortManager → [DataQueue] → FileSystemManager
SystemManager → [DisplayQueue] → DisplayManager  
DisplayManager → [CommandQueue] → SystemManager
```

### Shared Data Structures
```cpp
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

struct SystemCommand {
  enum Type { STORAGE_SELECT, FILE_TYPE, TRANSFER_MODE } type;
  uint8_t value;
};
```

### Resource Protection
- **SPI Mutex**: Protects SD card and EEPROM access
- **I2C Mutex**: Protects RTC communication
- **Serial Mutex**: Protects debug/config communication

## Directory Structure

```
src/
├── main.cpp                     # FreeRTOS setup and task creation
├── Components/                  # Component classes
│   ├── ParallelPortManager.h/cpp
│   ├── FileSystemManager.h/cpp
│   ├── DisplayManager.h/cpp
│   ├── TimeManager.h/cpp
│   └── SystemManager.h/cpp
├── Parallel/                    # Hardware abstraction (existing)
│   ├── Port.h/cpp
│   ├── Control.h/cpp
│   ├── Status.h/cpp
│   └── Data.h/cpp
├── User/                        # User interface (existing)
│   └── Display.h/cpp
└── Common/                      # Shared definitions
    ├── Types.h                  # Common data structures
    └── Config.h                 # System configuration
```

## Design Decisions Log

### 2025-07-19 - Component Separation
**Problem**: Monolithic main.cpp with all task logic inline
**Solution**: Extract each major subsystem into component classes
**Benefits**: 
- Better code organization and maintainability
- Easier unit testing of individual components
- Clear interfaces between subsystems
- Simplified main.cpp focusing on system initialization

### 2025-07-19 - Manager Pattern
**Decision**: Use Manager classes rather than Service/Controller patterns
**Rationale**:
- Embedded systems typically have one instance per subsystem
- Manager pattern clearly indicates ownership of hardware resources
- Simpler than dependency injection for resource-constrained environment
- Matches existing Arduino/embedded conventions

### 2025-07-19 - Header Organization
**Decision**: Separate common types and configuration into Common/ directory
**Rationale**:
- Avoids circular dependencies between components
- Central location for system-wide constants
- Easier to maintain configuration across components
- Follows embedded systems project structure conventions

## Memory Management Strategy

### Stack Allocation
- Task stacks: 256-512 bytes per task (conservative)
- Component objects: Static allocation in main.cpp
- Avoid dynamic allocation due to fragmentation risks

### Queue Sizing
- DataQueue: 8 × 528 bytes = ~4.2KB (largest consumer)
- DisplayQueue: 4 × 36 bytes = ~144 bytes
- CommandQueue: 4 × 4 bytes = ~16 bytes
- Total queue memory: ~4.4KB

### Available Memory (Arduino Mega 2560)
- Total SRAM: 8192 bytes
- Task stacks: ~2KB
- Queues: ~4.5KB  
- Static variables: ~1KB
- Available heap: ~700 bytes (adequate for small allocations)

## Error Handling Strategy

### Graceful Degradation
1. SD card failure → Fall back to EEPROM
2. EEPROM failure → Fall back to serial transfer
3. Display failure → Continue operation, log to serial
4. RTC failure → Use system tick counter

### Recovery Mechanisms
- Component health checks in SystemManager
- Automatic resource reinitialization
- Queue overflow detection and mitigation
- Task watchdog monitoring

## Testing Strategy

### Unit Testing
- Mock hardware interfaces for component testing
- Test queue communication patterns
- Validate error handling paths

### Integration Testing  
- Component interaction verification
- Resource contention testing
- End-to-end data flow validation

### Hardware Testing
- Actual TDS2024 oscilloscope integration
- Timing validation with logic analyzer
- Stress testing with continuous operation