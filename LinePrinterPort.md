# Line Printer Terminal (LPT)

## Summary

The `Line Printer Terminal` (LPT), `Line Printer Port` or just `Parallel Port` is a simple 8bit parallel control interface.  
The primary intention of this port was to use the `Centronics` printer terminals with `IBM` computers.  This design is based
on the original design now known as `Standard Parallel Port` (SPP) with 8 data lines, 5 status lines and 4 control lines.

## Pinout

| Name         | DB25  |  Direction | Register |
|--------------|-------|------------|----------|
| /Strobe      | 1     |  Input     | Control  |
| D0           | 2     |  Input     | Data     |
| D1           | 3     |  Input     | Data     |
| D2           | 4     |  Input     | Data     |
| D3           | 5     |  Input     | Data     |
| D4           | 6     |  Input     | Data     |
| D5           | 7     |  Input     | Data     |
| D6           | 8     |  Input     | Data     |
| D7           | 9     |  Input     | Data     |
| /Acknowledge | 10    |  Output    | Status   |
| Busy         | 11    |  Output    | Status   |
| Paper Out    | 12    |  Output    | Status   |
| Select       | 13    |  Output    | Status   |
| /Auto Feed   | 14    |  Input     | Control  |
| /Error       | 15    |  Output    | Status   |
| /Initialize  | 16    |  Input     | Control  |
| /Select In   | 17    |  Input     | Control  |
| Ground       | 18-25 |  Power     |          |

## Timing Diagram

```plantuml

<style>
timingDiagram {
  .output {
    LineColor lightGreen
  }
  .input {
    LineColor lightBlue
    BackgroundColor lightBlue
  }
  .memory {
    LineColor lightCoral
    BackgroundColor lightCoral
  }
}
</style>

binary  "/Strobe"        as Strobe       <<input>>
concise "Data Bits 0..7" as Data         <<input>>
binary  "/Acknowledge"   as Ack          <<output>>
binary  "Busy"           as Busy         <<output>>
binary  "Paper Out"      as PaperOut     <<output>>
binary  "Select"         as Select       <<output>>
binary  "/Auto Feed"     as AutoFeed     <<input>>
binary  "/Error"         as Error        <<output>>
binary  "/Initialize"    as Init         <<input>>
binary  "Select In"      as SelectIn     <<input>>
 
concise "Buffer"         as RingBuffer   <<memory>>
concise "Read Index"     as BufferIndex  <<memory>>

Strobe is high
Data is ""
AutoFeed is low
Init is low

Error is high
Select is high
PaperOut is low
Busy is low
Ack is high

RingBuffer is ""
BufferIndex is 0

@0 as :set_data
Data is 0xAC

@1 as :strobe_pulse
Strobe is low

@2 as :strobe_reset
Strobe is high

@3 as :start_read
Busy is high

@4 as :read_value
RingBuffer is 0xAC
BufferIndex is "+1"

@5 as :read_ack
Ack is low
Busy is low
Data is ""

```
