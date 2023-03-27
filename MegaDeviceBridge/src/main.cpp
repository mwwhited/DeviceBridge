#include <Arduino.h>
#include "./Parallel/Port.h"
#include "./User/Display.h"

// See [Pin Outs](../PinOuts.md)

// https://arduinoinfo.mywikis.net/wiki/Timers-Arduino
DeviceBridge::Parallel::Port _printer(
    DeviceBridge::Parallel::Control(18, 22, 26, 26),
    DeviceBridge::Parallel::Status(41, 43, 45, 47, 24),
    DeviceBridge::Parallel::Data(25, 27, 29, 31, 33, 35, 37, 39));
    
DeviceBridge::User::Display _display(
  8, 9, 4, 5, 6, 7
);

void setup()
{
  // put your setup code here, to run once:
  _printer.initialize();
  _display.initialize();
}

void loop()
{
  // put your main code here, to run repeatedly:
  
  //TODO: some logic to detect new file
  //TODO: some logic to detect complete file
  //TODO: logic to detect file type
  //TODO: check input
  //TODO: is not printing show
  //TODO: if printing show status

  if (_printer.isAlmostFull()){
    uint8_t buffer[512];
    uint16_t readBytes = _printer.readData(buffer);
    if (readBytes){
      //TODO: write data somewhere
    }
  }
}