# Device Bridge TODO List

## High Priority (In Progress)
- [x] Update platformio.ini to include FreeRTOS dependency
- [x] Restructure main.cpp with FreeRTOS task architecture  
- [x] Create parallel port task for interrupt-driven data capture
- [x] Create file management task for storage operations
- [x] Implement inter-task communication with queues/semaphores
- [x] Create display management task
- [x] Add RTC task for time management
- [ ] Update Port class to work with FreeRTOS synchronization
- [ ] Update existing libraries for FreeRTOS compatibility

## Medium Priority  
- [ ] Test and verify RTOS implementation
- [ ] Implement file type detection based on header bytes
- [ ] Add button input handling for LCD shield
- [ ] Implement menu navigation system
- [ ] Add EEPROM filesystem support (NASA EEFS)
- [ ] Add configuration management via serial commands
- [ ] Implement file browsing/transfer via serial interface

## Low Priority
- [ ] Add comprehensive error handling and recovery
- [ ] Implement file compression for storage efficiency  
- [ ] Add support for multiple file formats (BMP, PNG, etc.)
- [ ] Optimize parallel port read performance with DMA
- [ ] Add web interface for remote management
- [ ] Implement automatic file naming with timestamps

## Hardware Improvements
- [ ] Test with actual Tektronix TDS2024 oscilloscope
- [ ] Verify timing requirements for parallel port communication
- [ ] Add hardware flow control support
- [ ] Test SD card reliability under continuous writes
- [ ] Validate EEPROM wear leveling implementation

## Documentation
- [x] Create TODO.md for action tracking
- [x] Create CHANGES.md for change history  
- [x] Create CLAUDE.md for implementation notes
- [ ] Update README.md with FreeRTOS architecture details
- [ ] Add API documentation for task interfaces
- [ ] Create troubleshooting guide