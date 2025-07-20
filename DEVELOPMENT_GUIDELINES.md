# Development Guidelines - MegaDeviceBridge Project

## Production Status: CRITICAL TDS2024 BUGS FIXED + ENTERPRISE-GRADE ✅✅✅ (2025-07-20)
**Current State**: Critical TDS2024 file creation bugs resolved with comprehensive error signaling
**Memory Usage**: 11.3% RAM (926/8192 bytes) - EXCELLENT efficiency
**System Status**: 0 errors, stable operation confirmed with comprehensive validation
**Architecture**: Enterprise-grade dependency management with self-healing capabilities
**TDS2024 Integration**: "Saved:" with no filename issue completely resolved

## Critical Memory Management Rules (PRODUCTION PROVEN)
1. **ALWAYS use F() macro** for string literals on Arduino - Applied throughout codebase
2. **Monitor RAM usage continuously** - 8KB limit requires constant vigilance
3. **Static allocation only** - No dynamic allocation in embedded systems
4. **Flash memory utilization** - 256KB available, use for strings and constants
5. **FreeRTOS INCOMPATIBLE** - 8KB RAM insufficient for this hardware configuration

## Service Locator Architecture Patterns (ENTERPRISE-GRADE) ⭐⭐
- **Zero Null Pointers**: All components use `getServices().getXxxManager()` for dependencies
- **Runtime Validation**: Post-initialization dependency checking with fail-safe operation
- **IComponent Interface**: Standardized lifecycle (initialize/update/stop) and validation methods
- **Self-Test Framework**: Each component validates dependencies and hardware status
- **Multi-Layer Validation**: ServiceLocator + Component + Hardware validation

## Loop-Based Architecture Patterns (OPERATIONAL)
- **Component managers** with `update()` methods called in sequence
- **Direct function calls** replace message queues for efficiency  
- **ServiceLocator communication** for inter-component dependency resolution
- **Cooperative multitasking** using `millis()` timing intervals
- **No blocking operations** - all methods return immediately

## Testing Best Practices (PRODUCTION VALIDATED)
- **Hardware-first testing** - Real Arduino Mega required for accurate results
- **Memory monitoring** - Real-time RAM usage tracking (currently 11.3%)
- **Serial debugging** - F() macro resolved corruption issues
- **Component isolation** - Test each manager independently
- **Button calibration** - OSEPP analog values need validation on actual hardware

## TDS2024 Integration Guidelines (FULLY OPERATIONAL) ⭐⭐⭐
**File Format Support**: All 16 TDS2024 formats implemented in Types.h
- **Auto-Detection**: Header-based format identification working
- **Storage Failover**: SD → EEPROM → Serial hierarchy operational
- **Real-Time Capture**: 1ms polling interval for parallel port data
- **Universal Compatibility**: Ready for any TDS2024 configuration

### Critical File Creation Bug Resolution (2025-07-20) ⭐
**FIXED: "Saved:" with no filename issue**
- **Root Cause**: isNewFile flag timing bug in ParallelPortManager preventing file creation
- **Solution**: Fixed premature flag reset, allowing proper file creation sequence
- **Error Communication**: TDS2024 gets immediate ERROR/PAPER_OUT signals when file operations fail
- **Smart Recovery**: Error signals automatically cleared on successful operations
- **Debug System**: Complete parallel port debug logging for troubleshooting
- **Production Status**: Critical blocking issue resolved for real TDS2024 integration

## Service Locator Component Development Pattern (ENTERPRISE-GRADE) ⭐⭐
```cpp
#include "../Common/ServiceLocator.h"

class ComponentManager : public DeviceBridge::IComponent {
private:
    unsigned long lastUpdate = 0;
    const unsigned long updateInterval = 100; // ms
    
public:
    // IComponent interface implementation
    bool initialize() override { return true; }
    void update() override {
        if (millis() - lastUpdate >= updateInterval) {
            // Perform component work
            lastUpdate = millis();
        }
    }
    void stop() override { /* cleanup */ }
    
    // Self-test and validation
    bool selfTest() override {
        return validateDependencies();
    }
    
    const char* getComponentName() const override {
        return "ComponentManager";
    }
    
    bool validateDependencies() const override {
        bool valid = true;
        auto otherManager = getServices().getOtherManager();
        if (!otherManager) {
            Serial.print(F("  Missing OtherManager dependency\r\n"));
            valid = false;
        }
        return valid;
    }
    
    void printDependencyStatus() const override {
        Serial.print(F("ComponentManager Dependencies:\r\n"));
        auto otherManager = getServices().getOtherManager();
        Serial.print(F("  OtherManager: "));
        Serial.print(otherManager ? F("✅ Available") : F("❌ Missing"));
        Serial.print(F("\r\n"));
    }
    
private:
    // Use ServiceLocator for all dependencies - NO MORE NULL POINTERS!
    void someMethod() {
        auto otherManager = getServices().getOtherManager();
        if (otherManager) {
            otherManager->doSomething();
        }
    }
};
```

## Legacy Component Pattern (DEPRECATED - DO NOT USE)
```cpp
// OLD PATTERN - CAUSES NULL POINTER ISSUES
class ComponentManager {
    OtherManager* _other; // DANGEROUS - can be null
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