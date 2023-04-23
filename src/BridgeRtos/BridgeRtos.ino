#define ENABLE_DEDICATED_SPI 0

#include <Arduino_FreeRTOS.h>
#include <SdFatConfig.h>
#include <Arduino.h>
#include "./Parallel_Port.h"
#include "./User_Display.h"
#include "./Storage_SdCard.h"
#include "./Storage_SpiEeprom.h"

#define SERIAL_BAUD 9600

// See [Pin Outs](../PinOuts.md)

// https://arduinoinfo.mywikis.net/wiki/Timers-Arduino

DeviceBridge::Parallel::Port _printer(
    DeviceBridge::Parallel::Control(18, 22, 26, 26),
    DeviceBridge::Parallel::Status(41, 43, 45, 47, 24),
    DeviceBridge::Parallel::Data(25, 27, 29, 31, 33, 35, 37, 39));
    
DeviceBridge::User::Display _display(
  8, 9, 4, 5, 6, 7
);

DeviceBridge::Storage::StorageInterface _sdCard = DeviceBridge::Storage::SdCard(
  10
);

DeviceBridge::Storage::StorageInterface _eeprom = DeviceBridge::Storage::SpiEeprom(
  3
);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_BAUD);  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }
  
  _printer.initialize();
  _display.initialize();
  _sdCard.initialize();

}

void loop()
{
  // Empty. Things are done in Tasks.
    
  // //TODO: some logic to detect new file
  // //TODO: some logic to detect complete file
  // //TODO: logic to detect file type
  // //TODO: check input
  // //TODO: is not printing show
  // //TODO: if printing show status

  // if (_printer.isAlmostFull()){
  //   uint8_t buffer[512];
  //   uint16_t readBytes = _printer.readData(buffer);
  //   if (readBytes){
  //     //TODO: write data somewhere
  //   }
  // }
}
