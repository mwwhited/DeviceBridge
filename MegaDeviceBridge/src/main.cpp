#include <Arduino.h>
#include "./Printer/PrinterInterface.h"

// See [Pin Outs](../PinOuts.md)

// https://arduinoinfo.mywikis.net/wiki/Timers-Arduino
DeviceBridge::Printer::PrinterInterface printer = DeviceBridge::Printer::PrinterInterface(
    DeviceBridge::Printer::ControlInterface(18, 22, 26, 26),
    DeviceBridge::Printer::StatusInterface(41, 43, 45, 47, 24),
    DeviceBridge::Printer::DataInterface(25, 27, 29, 31, 33, 35, 37, 39));

void setup()
{
  // put your setup code here, to run once:
  printer.initialize();
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

  if (printer.isAlmostFull()){
    uint8_t buffer[512];
    uint16_t readBytes = printer.readData(buffer);
    if (readBytes){
      //TODO: write data somewhere
    }
  }
}