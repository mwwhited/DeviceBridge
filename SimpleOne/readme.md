To **stub out (simulate) an IEEE-1284-B printer port (Centronics interface)** using an **Arduino**, you’ll need to emulate the **SPP mode** (Standard Parallel Port) from the **printer's perspective** — that is, **receive data and respond with handshake signals**.

Below is a **basic stub** implementation of the **printer side (1284-B)** using an Arduino. It assumes a PC or microcontroller is sending data over a parallel port in SPP mode.

---

### 🧠 Understanding the Basics

The **host (PC)** drives:

* `D0–D7` (Data lines)
* `STROBE`, `AUTOFEED`, `INIT`, `SELECTIN` (Control lines)

The **printer (Arduino stub)** responds with:

* `ACK`, `BUSY`, `PE`, `SELECT`, `ERROR` (Status lines)

---

### 📌 Wiring Guide (Simplified)

| Parallel Pin | Signal    | Direction | Connect to Arduino        |
| ------------ | --------- | --------- | ------------------------- |
| 1            | STROBE#   | In        | D2                        |
| 2–9          | D0–D7     | In        | D3–D10                    |
| 10           | ACK#      | Out       | D11                       |
| 11           | BUSY      | Out       | D12                       |
| 12           | PE        | Out       | GND (simulate "no paper") |
| 13           | SELECT    | Out       | VCC (simulate "selected") |
| 14           | AUTOFEED# | In        | D13 (optional)            |
| 15           | ERROR#    | Out       | VCC (simulate "no error") |
| 16           | INIT#     | In        | —                         |
| 17           | SELECTIN# | In        | —                         |
| 18–25        | GND       | —         | GND                       |

---

### 🧰 Arduino Sketch – Stub Printer in SPP Mode

```cpp
// Data Pins (D0–D7 from Host)
const int dataPins[8] = {3, 4, 5, 6, 7, 8, 9, 10};

// Control Lines from Host
const int STROBE = 2; // Active low

// Status Lines to Host
const int ACK = 11;    // Active low
const int BUSY = 12;   // Active high

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
```

---

### ✅ What This Does:

* Emulates a simple Centronics printer in SPP mode.
* Waits for STROBE signal from host, reads 8-bit data, sends ACK.
* Prints received bytes to Serial Monitor.

---

### ⚠️ Notes:

* Actual IEEE 1284 timing is in nanoseconds — this is a **hobby-level approximation**.
* Use **5V logic levels** (level shift if needed for modern 3.3V devices).
* You may need **pull-up resistors** or **opto-isolation** depending on your PC parallel port.
