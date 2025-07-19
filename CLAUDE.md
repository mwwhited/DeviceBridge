# Claude Memory - MegaDeviceBridge Project

## Project Status: PRODUCTION DEPLOYED ✅ (2025-07-19)
Arduino Mega 2560 Device Bridge for Tektronix TDS2024 oscilloscope parallel port data capture. Loop-based architecture successfully operational.

## Essential Project Facts
- **Base Directory**: `/current/src` (working directory)
- **Arduino Project**: `/current/src/MegaDeviceBridge/` (PlatformIO project)
- **Source Code**: `/current/src/MegaDeviceBridge/src/` (Arduino .cpp files)
- **Current Branch**: `dev/remove-rtos` (production ready)
- **Architecture**: Loop-based cooperative multitasking (FreeRTOS eliminated)

## Production Metrics (VERIFIED)
- **RAM Usage**: 11.3% (926/8192 bytes) - EXCELLENT efficiency
- **System Status**: 0 errors, stable operation confirmed
- **All Components**: Operational and tested on real hardware

## Critical Technical Rules
- **F() Macro MANDATORY**: ALL Arduino string literals must use F("text") - prevents RAM corruption
- **No FreeRTOS**: 8KB RAM insufficient - loop-based architecture only
- **Memory Limit**: 8KB RAM total - current usage 11.3% is excellent
- **Direct Communication**: Components use direct function calls, no queues/mutexes

## TDS2024 Integration: COMPLETE ✅
- **All 16 File Formats**: Implemented in Common/Types.h
- **Auto-Detection**: Header-based format identification working
- **Storage Failover**: SD → EEPROM → Serial hierarchy operational
- **Ready for Testing**: Real oscilloscope integration pending

## Component Status: ALL OPERATIONAL ✅
- **main.cpp**: Loop-based system coordination deployed with heartbeat LED
- **5 Component Managers**: All rewritten for direct communication
- **Hardware**: LCD, buttons, SD, RTC, EEPROM all working (validated)
- **Visual Status**: Heartbeat LED (pin 13) blinks every 500ms indicating system operational
- **Serial Interface**: Full configuration menu implemented
- **Serial Output**: Clean debugging with F() macro applied

## Serial Interface: ENHANCED ✅
**Complete Configuration Interface via ConfigurationManager Component**:
- `validate/test` - Hardware validation
- `info` - System information
- `status` - Detailed component status with heartbeat state
- `time` - Show current time
- `time set YYYY-MM-DD HH:MM` - Set RTC time
- `storage sd/eeprom/serial/auto` - Change storage preference
- `heartbeat on/off/status` - Control serial status messages (default: OFF)
- `restart/reset` - Software reset
- `help` - Show command menu

**Key Features**:
- **Clean Serial Output**: No status spam by default (heartbeat OFF)
- **ConfigurationManager Component**: Proper architecture, not in main.cpp
- **Multiple Command Syntax**: Flexible command parsing

## Documentation Status: CURRENT ✅
- **ARCHITECTURE.md**: Loop-based architecture documented
- **TECHNICAL_DETAILS.md**: Production implementation details
- **DEVELOPMENT_GUIDELINES.md**: Proven patterns and rules
- **TODO.md**: Current tasks and final phase work
- All documentation reflects production-deployed state