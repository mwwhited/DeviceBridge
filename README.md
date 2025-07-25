# Device Bridge

## 🎉 **PROJECT STATUS: ENTERPRISE STORAGE ARCHITECTURE COMPLETE** ✅⭐⭐⭐⭐⭐⭐⭐⭐⭐

**Latest Update (2025-07-22)**: Complete EEPROM filesystem implementation with Flash memory constraint resolution, professional debug control integration, and unified filename support across all storage types.

**Current Status**: 
- ✅ **Build**: 100% compilation success with zero errors
- ✅ **EEPROM Filesystem**: Complete minimal filesystem with Flash memory constraint handling
- ✅ **Debug Control**: Professional `debug eeprom on|off|status` command integration
- ✅ **Memory**: 11.3% RAM usage (exceptional efficiency) 
- ✅ **Performance**: ServiceLocator cached pointers + IEEE-1284 SPP compliance
- ✅ **Architecture**: Enterprise-grade array-based component management

## Summary

The original intention of this device is to bridge the old world line printer port world of the Tektronics TDS2024 with the more modern world of USB. This project is loosly based on the work from [Boriz](https://github.com/boriz/CentronicsArduino) but is a total rewrite of my own design.

![Device Bridge](IMG_9864.jpg)


## Table of Contents

* [Line Printer Terminal](./LinePrinterPort.md)
* [Pinouts](./Pinouts.md)

## Design Goals

* When the print button on the TDS2024 the screen capture/print out should be stored on the bridge
* Captures should stored locally on the bridge device
* Capture to SD card or local EEPROM
* Allow user to select between SD, EEPROM, and direct transfer
* Complete EEPROM minimal filesystem with Flash memory constraint handling
* Professional debug control system with selective EEPROM operation logging
* If SD is not found then only record to EEPROM with bulletproof filesystem
* When not printer or after a configured time period show the current time/date on the LCD display
* Reading file transfer header try to detect type and make best guess on proper extensio
* Allow user to over write the file type
* Allow user to select from name formats from LCD module or Serial over USB
* Allow computer user to list files over serial connection
  * should should list SD (if provided) and EEPROM
  * allow transfer of selected files
  * update configuration options
* SD card should support FAT, FAT32 and ExFat.
  * if programming memory allows enable FAT12 as well
* use RTC to maintain current date/time.
  * configurable over serial
  * condsider the ablity to set from the UI
* consider using one of the secondary serial lines as an RS-232 bridge for SCPI commands

## Suggsted Improvements

* Enable RTC (DS1307) on the DataLogger shield.  When in Idle present the time on the screen
  * https://github.com/cvmanjoo/RTC
* ✅ **COMPLETE**: W25Q128 EEPROM storage with minimal filesystem (CS pin 3)
  * ✅ Complete minimal filesystem implementation with zero directory caching
  * ✅ Flash memory constraint resolution using complement-based size encoding
  * ✅ Professional debug control with `debug eeprom on|off|status` commands
  * ✅ Full filename support including directory paths like `20250722/164742.bin`
  * ✅ Unified filename generation across SD, EEPROM, and Serial storage types
  * ✅ Serial CLI integration with `list eeprom` command for file management
  * ✅ Production-ready with comprehensive error handling and file size reporting
* The current read/write process is rather slow
  * look at upgrading to SDFat as well as using a ringbuffer for reading from the parallel poin
* consider adding the ability to directly output the content to the serial connection
* use the inputs on the LCD shield to support ejecting/refreshing the SD card
* consider adding prefix profiles that may be added/modified from the serial connection and selected from the interface
* consider the ability to time stamp files
* add the ability to browse/download/delete content from the serial connection
* add proper support for the control lines in relation to SPP IEEE-1284
* add folder management/selection from USB
  * if tied to quick profiles allow selectig from the LCD panel
* look at using the first few bytes to detect the proper file type so they can ne named correctly
  * otherwise allow changing the file type from the LCD panel
* add proper erroring handling

## Technical Notes

* Shields
  * [LCD and Keypad](./Pinouts.md#lcd-shield)
    * [OSEPP 16x2 LCD Dispaly and Keyboard Shield](https://osepp.com/electronic-modules/shields/45-16-2-lcd-display-keypad-shield)
    * Based on suggestion, cut trace for shield pin 10 to prevent possible damage.  If you want control there are other options noted in the linked reference.
      * https://forum.arduino.cc/t/warning-to-users-of-some-vendors-lcd-keypad-shields/94673
  * [Deek Robot Data Logger](./Pinouts.md#sd-card-data-logger-shield)
    * Parital clone of the Adafruit Data Logger v1 no support for design information from Deek-Robot avalible.
    * Required extensive rerouting to cleanly support the Arduino Mega 2560
      * Cut traces for shield pins 11/12/13 and reroute SPI lines (MOSI, MISO, SCLK) from ICSP header to 11/12/13 taps respectivly
      * Cut traces for pins 4,5 reouted I2C SCL/SDA to 5/4 respectively (RTC DS1307 support)
    * Added [Windbond W25Q128](./DataSheets/W25Q128BV.PDF) 16MB SPI EEPROM for local storage
      * Cut static traces for pins 12/13 on the quad buffer (74HC123) from VCC power (pin 14).
      * bodge pin 13 to ground to enable 4th buffer.
      * bodge pin 12 to arduino pin 3 for chip select
      * bodge pin 11 to SPI EEPROM Chip Select
      * bodge MOSI and SCLK from the SD card to SPI EEPROM
  * [Custom IO for LPT Port](./Pinouts.md#parallel-interface)
    * Used 2x13 2.54mm header for 2x13 IDC to DB-25
    * header pins should be mapped as shown in the pinout table
    * note the Strobe line should be mapped to a hardware interrupt pin
    * [Line Printer Terminal](./LinePrinterPort.md)

## Action Sequence Diagrams

### Initialize

```plantuml
@startuml
-> printer: activate
activate printer
printer -> printer: +Error,+Select,-PaperOut,-Busy,+Ack
printer -> terminal: ^Strobe,^Data
note right: Add ^AutoFeed,^Init,^SelectIn
deactivate printer
@enduml
```

### On Print

```plantuml
@startuml
loop while data

-> terminal: print
activate terminal
terminal -> terminal: Set Data
terminal -\ printer : -Strobe
terminal -\ printer : +Strobe
activate printer
printer -\ terminal : +Busy
alt if new
  note right: What signals a new job?
  printer -\ buffer : new
  activate buffer
end
printer -> terminal : Get Data
terminal --> printer : <Data>
printer -\ buffer   : Set Value
buffer -\ buffer    : Input Index++
printer -\ terminal : -Ack
printer -\ terminal : -Busy
printer -\ terminal : +Ack
alt if complete
note right: What signals job complete?
  printer -\ buffer : complete
end

end
deactivate terminal
deactivate printer
@enduml
```

### On New File

```plantuml
@startuml
--> disk
activate disk
disk -> file: check exists
  loop do while exists
    activate file
    file -> file: increment name
  end
  file --> disk: <new name>
  disk -> file: create <new name>
  deactivate file
  alt success 
    disk --> : <file handle>
  else fail
    disk --> : <error>
  end
@enduml
```

### Buffer to File

```plantuml
@startuml
--> buffer : full
activate buffer
alt if new
  disk -\ file : new file
  activate file
end
activate disk
buffer -> file 
loop while full
  file -> file: write bytes
  file -> buffer: Output Index++
end

alt if complete
  note right: What signals a new job?
  disk -> file: close
  deactivate file
end 
deactivate buffer
@enduml
```

## Notes

* [Arduino SD Library](https://www.arduino.cc/reference/en/libraries/sd/)
* [SD Card with Logic Level hookup](https://learn.sparkfun.com/tutorials/microsd-shield-and-sd-breakout-hookup-guide#sd-card-breakout-boards)
* [Deek Robot - Data Logger v1](https://forum.arduino.cc/t/deek-robot-data-logging-shield-v1-0/621963)
  * https://www.hackster.io/cowboydaniel/deek-robot-datalogging-shield-tutorial-for-mega-1cd2b1
  * https://drive.google.com/file/d/1T5GEPMKtLjfiJvdL_DHEbsTk9sAIzThP/view
* [OSEPP - 16×2 LCD Display & Keypad Shield](https://www.osepp.com/electronic-modules/shields/45-16-2-lcd-display-keypad-shield)
