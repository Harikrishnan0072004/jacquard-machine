# Electronic Jacquard Communication Troubleshooting

This document records communication problems found during testing between the Main Board and TFT Front Panel.

## Communication Protocol

- UART speed: `230400`
- Main Board UART pins: RX `16`, TX `17`
- TFT UART pins: RX `9`, TX `8`
- Main Board to TFT frame: `AB...#&`
- TFT to Main Board frame: `AB...{}`
- Communication reply: `EJ`
- UART wiring: Main TX to TFT RX, Main RX to TFT TX, and common GND
- USB host: RP2040 native USB connector, D+ on pin 47 and D- on pin 46
- USB host VBUS: external 5 V supply confirmed
- USB stack: Arduino `Adafruit TinyUSB Host`

## Problem and Solution Record

| Problem / log evidence | Root cause | Solution | Status |
|---|---|---|---|
| TFT stopped during startup after `serialinput` and `after buffer clear`. | Main Board parser checked the `AB` header at the wrong array position and the receive index was not incremented correctly. | Corrected the parser to increment `nc1` and check `data3[0]`, `data3[1]`, and `data3[2]` as `A`, `B`, and command. | Solved |
| TFT communication log showed decimal values such as `6566685242483538`. | Received bytes were printed as decimal numbers. | Changed debug output to print received bytes as characters. Example: `ABD4*0#&`. | Solved |
| TFT parser sometimes produced `ABDABD6`. | A new `AB` frame could arrive while the previous frame was still being parsed. The second frame was appended to the first. | Added `AB` frame resynchronization in the TFT parser and separated Main Board response frames with delays. | Improved |
| TFT showed `Timeout in inner loop - resyncing` during `ABD3*1536#&`. | Per-byte USB debug printing delayed UART processing, and the inter-byte timeout was too short. | Removed per-byte TFT debug printing, print the complete frame after `#&`, and increased the inter-byte timeout from `200 ms` to `1000 ms`. | Solved |
| Compilation failed with `rout label used but not defined`, followed by many missing-function errors. | One extra closing brace moved the `rout:` label outside `serialinput()`. | Removed the extra closing brace so `rout:` remains inside `serialinput()`. | Solved |
| Changing F1 sometimes left the TFT screen stuck waiting for a response. | The TFT waited forever for the final status frame. Also, duplicate automatic status bursts could overlap the TFT's later `ABR` request. | Removed duplicate automatic status transmission after file change. The TFT sends `ABR` and receives one status response. Added a three-second timeout to the F1 status wait. | Solved / protected |
| Main Board serial monitor displayed unreadable characters around lock status. | `Serial.write()` printed raw binary bytes. | For readable diagnostics, use `Serial.print()` and `Serial.println()` instead of `Serial.write()` for status values. | Recommended |
| Hang could recur after a dropped or malformed UART frame. | Several parser branches and TFT status waits were unbounded; malformed data could also exceed fixed buffers. | Added parser deadlines, buffer bounds checks, bounded TFT status waits, empty-folder protection, and folder/file index validation. | Implemented; hardware retest required |
| Selecting an empty folder left stale file data on screen. | Main Board continued into `runninginit()` after `totalfile` became zero. | Reject the folder, restore the previous valid folder, and avoid opening stale file entries. | Solved |
| F2 repeat or one-pulse commands used incorrect state. | F2 repeat compared against `repeatcnt`; one-pulse commands used shared temporary `temp`. | Compare against `repeatcnt1` and use the dedicated active file indexes. | Solved |
| USB-to-SD copy was missing from the TFT project. | The current TFT and Main Board did not contain the old USB-host sender and SD-copy receiver. | Added native USB host setup, USB folder scanning, BMP/EJC transfer, 62-byte header transfer, card metadata, CRC-16 validation, and D1/D0 acknowledgements. | Implemented; hardware retest required |
| USB host pin conflict was suspected with keypad GPIO16/GPIO17. | The old project used GPIO16/GPIO17 for PIO USB, but the schematic uses the RP2040 native USB pins 46/47. | Use native host `USBHost.begin(0)` and keep keypad pins unchanged. | Solved |
| TFT compile failed with `File32` to `FsFile` conversion and `customKeypad` undeclared. | The TFT included regular RP2040 `SD.h` together with the USB SdFat filesystem, and USB functions appeared before the keypad object definition. | Removed the unused TFT `SD.h` dependency and added an `extern Keypad customKeypad` declaration. | Solved |

## Verified Working Sequence

A successful startup should contain these operations:

```text
ABC...{}
EJ
ABI...{}
ABM...#&
ABTK...#&
ABTL...#&
ABLK...#&
ABLM...#&
ABN...#&
ABn...#&
ABb...#&
ABi...#&
ABDK...#&
ABD2...#&
ABD3...#&
ABD4...#&
ABDM...#&
ABD6...#&
ABD7...#&
ABD8...#&
```

For an F1 file change, the expected sequence is:

```text
TFT sends: AB...EK...{}
TFT sends: ABR{}
Main Board sends: ABM...#&
Main Board sends: ABDK...#&
Main Board sends: ABD2...#&
Main Board sends: ABD3...#&
Main Board sends: ABD4...#&
Main Board sends: ABDM...#&
Main Board sends: ABD6...#&
Main Board sends: ABD7...#&
Main Board sends: ABD8...#&
```

## Non-Blocking Build Warnings

These messages are warnings or local editor configuration issues, not communication failures:

- Keypad library uses AVR `INPUT_PULLUP` emulation.
- `TOUCH_CS` is not defined, so TFT touch functions are unavailable.
- Multiple `SD.h` libraries are detected; Arduino reports which one it selected.
- VS Code may report missing Arduino headers if its include path is not configured, even though Arduino IDE compilation works.

## How to Add the Next Problem

Add a row using this format:

| Problem / exact log | Root cause | Solution | Status |
|---|---|---|---|
| Paste the smallest useful log section. | Explain the controlling code path. | Record the code or wiring change. | Open / Testing / Solved |

Always record:

1. Which board produced the log.
2. The exact command or frame being sent.
3. The last successful frame.
4. The first failed or missing frame.
5. The reason the fix worked.
6. The upload and hardware test result.
