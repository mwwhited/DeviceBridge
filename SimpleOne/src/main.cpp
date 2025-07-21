#include <Arduino.h>

  // Parallel port control pins
  constexpr uint8_t LPT_STROBE = 18;
  constexpr uint8_t LPT_AUTO_FEED = 22;
  constexpr uint8_t LPT_INITIALIZE = 26;
  constexpr uint8_t LPT_SELECT_IN = 28;

  // Parallel port status pins  
  constexpr uint8_t LPT_ACK = 41;
  constexpr uint8_t LPT_BUSY = 43;
  constexpr uint8_t LPT_PAPER_OUT = 45;
  constexpr uint8_t LPT_SELECT = 47;
  constexpr uint8_t LPT_ERROR = 24;

  // Parallel port data pins
  constexpr uint8_t LPT_D0 = 25;
  constexpr uint8_t LPT_D1 = 27;
  constexpr uint8_t LPT_D2 = 29;
  constexpr uint8_t LPT_D3 = 31;
  constexpr uint8_t LPT_D4 = 33;
  constexpr uint8_t LPT_D5 = 35;
  constexpr uint8_t LPT_D6 = 37;
  constexpr uint8_t LPT_D7 = 39;


// Data Pins (D0–D7 from Host)
const int dataPins[8] = {
    LPT_D0,
    LPT_D1,
    LPT_D2,
    LPT_D3,
    LPT_D4,
    LPT_D5,
    LPT_D6,
    LPT_D7,    
};

// Control Lines from Host
const int STROBE = LPT_STROBE; // Active low

// Status Lines to Host
const int ACK = LPT_ACK;    // Active low
const int BUSY = LPT_BUSY;   // Active high

void setup() {
  // Data lines as inputs (host writes to them)
  for (int i = 0; i < 8; i++) {
    pinMode(dataPins[i], INPUT);
  }

  pinMode(STROBE, INPUT_PULLUP); // active-low

  // Status lines as outputs (Arduino controls)
  pinMode(ACK, OUTPUT);
  pinMode(BUSY, OUTPUT);

  // Default status
  digitalWrite(ACK, HIGH);  // Idle = high
  digitalWrite(BUSY, LOW);  // Not busy = low

  Serial.begin(9600);
}

void loop() {
  // Wait for STROBE (low pulse from host)
  if (digitalRead(STROBE) == LOW) {
    digitalWrite(BUSY, HIGH);  // Set busy

    delayMicroseconds(1); // setup time

    // Read data
    byte data = 0;
    for (int i = 0; i < 8; i++) {
      if (digitalRead(dataPins[i])) {
        data |= (1 << i);
      }
    }

    // Simulate processing
    Serial.print("Received Byte: ");
    Serial.println(data, HEX);

    // Send ACK pulse
    digitalWrite(ACK, LOW);
    delayMicroseconds(5); // ACK pulse width
    digitalWrite(ACK, HIGH);

    digitalWrite(BUSY, LOW); // Done processing
    delay(1); // simulate processing delay
  }
}
