# Development Guidelines

## Critical Memory Management Rules
1. **ALWAYS use F() macro** for string literals on Arduino
2. **Monitor RAM usage continuously** - 8KB limit is strict
3. **Prefer static allocation** over dynamic for embedded systems
4. **Flash memory is abundant** (256KB) - use it for strings

## Architecture Patterns
- **Component managers** with update() methods
- **Direct function calls** over message queues
- **Callback mechanisms** for inter-component communication
- **Timed execution** using millis() for scheduling

## Testing Best Practices
- **Start with minimal tests** and expand gradually
- **Include hardware validation** in test suite
- **Monitor memory usage** in real-time
- **Test on actual hardware** not just simulation

## Serial Communication Crisis Resolution
**Critical Issue**: Serial output corruption with garbled characters
**Root Cause**: String literals stored in RAM instead of Flash memory
**Solution**: Applied F() macro to ALL string literals
**Result**: Clean serial output + additional RAM freed

## Build Configuration
- **Test Environment**: Simplified dependencies (Unity only for basic tests)
- **Production Environment**: Full hardware libraries
- **platformio.ini**: Use `test_build_src = false` not deprecated options

## Hardware Testing Strategy
1. **Start Minimal**: Basic compilation and simple tests first
2. **Iterative Expansion**: Add complexity gradually
3. **Hardware Validation**: Real device testing essential
4. **Memory Monitoring**: Continuous RAM tracking critical