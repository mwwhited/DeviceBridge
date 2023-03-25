# Pin Outs

## LCD Shield

| LCD pin name | Arduino |
|--------------|---------|
| Reset        | pin 8   |
| Enable       | pin 9   |
| Data4        | pin 4   |
| Data5        | pin 5   |
| Data6        | pin 6   |
| Data7        | pin 7   |

## SD Card Data Logger Shield

| Use                 | Arduino   |
|---------------------|-----------|
| SPI SCLK            | ICSP SCLK |
| SPI MOSI            | ICSP MOSI |
| SPI MISO            | ICSP MISO |
| SD Chip Select      | pin 10    |
| EEPROM Chip Select  | pin 3     |
| RTC I2C SCL         | I2C SCL   |
| RTC I2C SDA         | I2C SDA   |

## Parallel interface

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
