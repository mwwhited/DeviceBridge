# Device Bridge

## Summary

The original intention of this device is to bridge the old world line printer port world of the Tektronics TDS2024 with the more modern world of USB. This project is loosly based on the work from [Boriz](https://github.com/boriz/CentronicsArduino) but is a total rewrite of my own design.

## Design Goals

* When the print button on the TDS2024 the screen capture/print out should be stored on the bridge
* Captures should stored locally on the bridge device
* Capture to SD card or local EEPROM
* Allow user to select between SD, EEPROM, and direct transfer
* If SD is not found then only record to EEPROM
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
* Add support for storing files on a W25Q128 (CS pin 3)
  * When SD Card is removed or used selected the files may be stored on the eeprom
  * look at using the NASA EEFS.
  * https://forum.arduino.cc/t/nasa-eefs/329543/3
  * https://github.com/slviajero/EepromFS
  * https://github.com/feilipu/avr_eefs
  * https://www.instructables.com/Introduction-to-LittleFs-Write-LittleFs-Read-Littl/
  * https://github.com/arduino-libraries/Arduino_MKRMEM
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
  * LCD and Keypad
    * [OSEPP 16x2 LCD Dispaly and Keyboard Shield](https://osepp.com/electronic-modules/shields/45-16-2-lcd-display-keypad-shield)
    * Based on suggestion, cut trace for shield pin 10 to prevent possible damage.  If you want control there are other options noted in the linked reference.
      * https://forum.arduino.cc/t/warning-to-users-of-some-vendors-lcd-keypad-shields/94673
  * Deek Robot Data Logger
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
  * Custom IO for LPT Port
    * Used 2x13 2.54mm header for 2x13 IDC to DB-25
    * header pins should be mapped as shown in the pinout table
    * note the Strobe line should be mapped to a hardware interrupt pin

## Pin Outs

### LCD Shield

| LCD pin name | Arduino |
|--------------|---------|
| Reset        | pin 8   |
| Enable       | pin 9   |
| Data4        | pin 4   |
| Data5        | pin 5   |
| Data6        | pin 6   |
| Data7        | pin 7   |

### SD Card Data Logger Shield

| Use                 | Arduino   |
|---------------------|-----------|
| SPI SCLK            | ICSP SCLK |
| SPI MOSI            | ICSP MOSI |
| SPI MISO            | ICSP MISO |
| SD Chip Select      | pin 10    |
| EEPROM Chip Select  | pin 3     |
| RTC I2C SCL         | I2C SCL   |
| RTC I2C SDA         | I2C SDA   |

### Parallel interface

The [line printer terminal](LinePrinterPort.md) interface is mapped as below

| Name         | DB25  | Arduino | Direction |Notes                                                                      |
|--------------|-------|---------|-----------|---------------------------------------------------------------------------|
| /Strobe      | 1     | pin 18  | Input     | Pullup enabled. Attached to falling edge interrupt.                       |
| D0           | 2     | pin 25  | Input     | Parallel data.                                                            |
| D1           | 3     | pin 27  | Input     | Parallel data.                                                            |
| D2           | 4     | pin 29  | Input     | Parallel data.                                                            |
| D3           | 5     | pin 31  | Input     | Parallel data.                                                            |
| D4           | 6     | pin 33  | Input     | Parallel data.                                                            |
| D5           | 7     | pin 35  | Input     | Parallel data.                                                            |
| D6           | 8     | pin 37  | Input     | Parallel data.                                                            |
| D7           | 9     | pin 39  | Input     | Parallel data.                                                            |
| /Acknowledge | 10    | pin 41  | Output    | Generate falling edge to acknowledge data.                                |
| Busy         | 11    | pin 43  | Output    | Set high on the falling edge of Strobe. Set low after acknowledging data. |
| Paper Out    | 12    | pin 45  | Output    | Not used. Forced low.                                                     |
| Select       | 13    | pin 47  | Output    | Not used. Forced high.                                                    |
| /Auto Feed   | 14    | pin 22  | Input     | Pullup enabled                                                            |
| /Error       | 15    | pin 24  | Output    | Not used. Forced high.                                                    |
| /Initialize  | 16    | pin 26  | Input     | Pullup enabled                                                            |
| /Select In   | 17    | pin 28  | Input     | Pullup enabled                                                            |
| Ground       | 18-25 | pin 28  | Power     |                                                                           |

## Action Sequence Diagrams

### Initialize

```plantuml

-> printer: activate
activate printer
printer -> printer: +Error,+Select,-PaperOut,-Busy,+Ack
printer -> terminal: ^Strobe,^Data
note right: Add ^AutoFeed,^Init,^SelectIn
deactivate printer

```

### On Print

```plantuml

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

```

### On New File

```plantuml
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
```

### Buffer to File

```plantuml
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
```
## Notes

* [Arduino SD Library](https://www.arduino.cc/reference/en/libraries/sd/)
* [SD Card with Logic Level hookup](https://learn.sparkfun.com/tutorials/microsd-shield-and-sd-breakout-hookup-guide#sd-card-breakout-boards)
* [Deek Robot - Data Logger v1](https://forum.arduino.cc/t/deek-robot-data-logging-shield-v1-0/621963)
  * https://www.hackster.io/cowboydaniel/deek-robot-datalogging-shield-tutorial-for-mega-1cd2b1
  * https://drive.google.com/file/d/1T5GEPMKtLjfiJvdL_DHEbsTk9sAIzThP/view
* [OSEPP - 16×2 LCD Display & Keypad Shield](https://www.osepp.com/electronic-modules/shields/45-16-2-lcd-display-keypad-shield)

