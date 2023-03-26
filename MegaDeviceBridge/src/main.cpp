#include <Arduino.h>
#include "PrinterInterface.h"

DeviceBridge::PrinterInterface printer = DeviceBridge::PrinterInterface(
    DeviceBridge::ControlInterface(18, 22, 26, 26),
    DeviceBridge::StatusInterface(41, 43, 45, 47, 24),
    DeviceBridge::DataInterface(25, 27, 29, 31, 33, 35, 37, 39));

void setup()
{
  // put your setup code here, to run once:
  printer.initialize();
}

void loop()
{
  // put your main code here, to run repeatedly:
  
  if (printer.isAlmostFull()){
    uint8_t buffer[512];
    uint16_t readBytes = printer.readData(buffer);
    if (readBytes){
      //TODO: write data somewhere
    }
  }
}