# Development Guidelines - MegaDeviceBridge Project

## Production Status: DEPLOYED ✅ (2025-07-19)
**Current State**: Loop-based architecture operational on Arduino Mega 2560
**Memory Usage**: 11.3% RAM (926/8192 bytes) - EXCELLENT efficiency
**System Status**: 0 errors, stable operation confirmed

## Critical Memory Management Rules (PRODUCTION PROVEN)
1. **ALWAYS use F() macro** for string literals on Arduino - Applied throughout codebase
2. **Monitor RAM usage continuously** - 8KB limit requires constant vigilance
3. **Static allocation only** - No dynamic allocation in embedded systems
4. **Flash memory utilization** - 256KB available, use for strings and constants
5. **FreeRTOS INCOMPATIBLE** - 8KB RAM insufficient for this hardware configuration

## Loop-Based Architecture Patterns (OPERATIONAL)
- **Component managers** with `update()` methods called in sequence
- **Direct function calls** replace message queues for efficiency  
- **Callback mechanisms** for inter-component communication
- **Cooperative multitasking** using `millis()` timing intervals
- **No blocking operations** - all methods return immediately

## Testing Best Practices (PRODUCTION VALIDATED)
- **Hardware-first testing** - Real Arduino Mega required for accurate results
- **Memory monitoring** - Real-time RAM usage tracking (currently 11.3%)
- **Serial debugging** - F() macro resolved corruption issues
- **Component isolation** - Test each manager independently
- **Button calibration** - OSEPP analog values need validation on actual hardware

## TDS2024 Integration Guidelines (IMPLEMENTED)
**File Format Support**: All 16 TDS2024 formats implemented in Types.h
- **Auto-Detection**: Header-based format identification working
- **Storage Failover**: SD → EEPROM → Serial hierarchy operational
- **Real-Time Capture**: 1ms polling interval for parallel port data
- **Universal Compatibility**: Ready for any TDS2024 configuration

## Component Development Pattern (PRODUCTION PROVEN)
```cpp
class ComponentManager {
private:
    unsigned long lastUpdate = 0;
    const unsigned long updateInterval = 100; // ms
    
public:
    void update() {
        if (millis() - lastUpdate >= updateInterval) {
            // Perform component work
            lastUpdate = millis();
        }
    }
    
    // Direct communication methods (no queues)
    void setOtherManager(OtherManager* manager) { _other = manager; }
};
```

## Build Configuration (PRODUCTION)
- **No FreeRTOS**: Removed from platformio.ini completely
- **Standard Arduino Libraries**: SD, Wire, LiquidCrystal sufficient
- **Test Environment**: Unity framework for basic validation
- **Memory Tracking**: Built-in Arduino freeRam() function
- **F() Macro**: Applied to ALL string literals throughout codebase

## Hardware Testing Results (CONFIRMED)
✅ **Arduino Mega 2560**: 11.3% RAM usage measured  
✅ **OSEPP LCD Shield**: Display and buttons operational  
✅ **Data Logger Shield**: SD card and RTC working  
✅ **W25Q128 EEPROM**: SPI communication established  
✅ **Serial Output**: Clean debugging with F() macro  
✅ **System Stability**: 0 errors during extended operation