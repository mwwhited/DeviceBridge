#include <stdint.h>
#include <Arduino.h>
#include "Status.h"

namespace DeviceBridge::Parallel
{
  Status::Status(
      uint8_t acknowledge,
      uint8_t busy,
      uint8_t paperOut,
      uint8_t selected,
      uint8_t error)
  {
    _acknowledge = acknowledge;
    _busy = busy;
    _paperOut = paperOut;
    _selected = selected;
    _error = error;
  }

  void Status::initialize()
  {
    pinMode(_error, OUTPUT); // Error - normally high
    digitalWrite(_error, true);

    pinMode(_selected, OUTPUT); // Select - normally high
    digitalWrite(_selected, true);

    pinMode(_paperOut, OUTPUT); // Paper out - normally low
    digitalWrite(_paperOut, false);

    pinMode(_busy, OUTPUT); // Busy - normally low
    digitalWrite(_busy, false);

    pinMode(_acknowledge, OUTPUT); // Ack - normally high
    digitalWrite(_acknowledge, true);
  }

  void Status::setBusy(){
    // set busy
    digitalWrite(_busy, true);
  }
  void Status::setBusy(bool busy) {
    digitalWrite(_busy, busy);
  }

  void Status::setAck(){
    // set acknowledgement
    digitalWrite(_acknowledge, false);
    
    //TODO: might need spin lock

    // reset acknowledgement
    digitalWrite(_acknowledge, true);
    // reset busy
    digitalWrite(_busy, false);
    
  }

  void Status::sendAcknowledgePulse() {
    // Send a brief acknowledge pulse
    digitalWrite(_acknowledge, false);
    delayMicroseconds(5);  // Brief pulse
    digitalWrite(_acknowledge, true);
  }

  void Status::setError(bool error) {
    digitalWrite(_error, !error);  // Error is active LOW
  }

  void Status::setPaperOut(bool paperOut) {
    digitalWrite(_paperOut, paperOut);
  }

  void Status::setSelect(bool select) {
    digitalWrite(_selected, select);
  }
}