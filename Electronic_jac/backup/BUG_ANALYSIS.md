# File Name Loss Issue - Root Cause Analysis

## Summary
The front panel (RP2040 Pico) is losing file names during communication with the mainboard (ESP32). The issue stems from **multiple problems in the serial communication protocol and the file name parsing logic**.

---

## Problem 1: Incomplete String Buffer in Serial Reception

### Location
**RP2040F.ino, serialinput() function (Line 3221+)**

### Issue
The `inputString` variable accumulates data character by character, but there's no mechanism to ensure all characters of a file name have been received before parsing.

### Code Evidence
```cpp
while (true)
{
  if (Serial2.available() > 0)
  {
    byte inbyte = (byte)Serial2.read();
    char inChar = (char)inbyte;
    inputString += inChar;  // APPENDING characters one at a time
    data3[nc1] = inbyte;
    nc1 = nc1 + 1;
    
    // Check for packet ending "#&"
    if (data3[nc1 - 2] == 35)       // '#'
    {
      if (data3[nc1 - 1] == 38)     // '&'
      {
        // IMMEDIATELY PARSE without waiting for buffer to fill
        ...
      }
    }
  }
}
```

### Why This Causes File Names to Be Lost

1. **Race Condition**: Serial data arrives at 230400 baud, but processing happens character-by-character
2. **Incomplete Strings**: If there's any delay between characters, the parsing might happen before the complete file name has been received
3. **String Concatenation Overhead**: Using `inputString += inChar` in a loop is inefficient and can miss characters if the serial buffer fills up

### Example
- Message sent: `ABLM0*MyFileName.txt#&`
- If there's a delay after `MyFileNam`, the code might parse with incomplete data
- Result: File name might be stored as "MyFileNam" instead of "MyFileName.txt"

---

## Problem 2: Incorrect File Name Extraction Logic

### Location
**RP2040F.ino, serialinput() function, L M block (Line ~3595)**

### Issue
The file name extraction uses `indexOf()` twice on the same string, which can return the same position.

### Current Problematic Code
```cpp
if (data3[3] == 77)            // 'M'
{
  int e = inputString.indexOf('M');      // Find 'M'
  int f = inputString.indexOf('*');      // Find first '*'
  
  if (e != -1 && f != -1)
  {
    String pick = inputString.substring(e + 1, f);  // Get index
    int tempy = pick.toInt();
    
    e = inputString.indexOf('*');        // Find first '*' AGAIN (same result!)
    f = inputString.indexOf('#');        // Find '#'
    
    if (e != -1 && f != -1)
    {
      pick = inputString.substring(e + 1, f);  // Get filename
      
      if (tempy >= 0 && tempy < 50)
      {
        filelist_m[tempy] = pick;  // STORE FILENAME
```

### Why This Is A Problem

1. **Redundant indexOf() calls**: The second `indexOf('*')` returns the SAME position as the first
2. **No String Trimming**: File names may have whitespace or unwanted characters
3. **No Validation**: There's no check if the extracted file name is valid or empty

### Example
- Received: `ABLM0*FileName#&`
- First parse: `e=0, f=5` → `pick = "0"` → `tempy = 0` ✓
- Second parse: `e=5` → `substring(6, #position)` ✓
- **But if inputString has extra characters before 'M'**: Everything breaks!

---

## Problem 3: Serial Buffer Overflow Risk

### Location
**RP2040F.ino, serialinput() function (Line 3226+)**

### Issue
The data array `data3` is sized at 50 bytes, but file names can be much longer (up to 255 characters in SD cards).

### Code Evidence
```cpp
byte data3[50];  // Fixed 50-byte buffer!

void serialinput()
{ 
  inbufferclear();  // Clears data3
  inputString = "";

  while (true)
  {
    if (Serial2.available() > 0)
    {
      byte inbyte = (byte)Serial2.read();
      data3[nc1] = inbyte;  // CAN OVERFLOW if nc1 > 50!
      nc1 = nc1 + 1;
```

### Why This Loses File Names

- When a long file name arrives, `nc1` can exceed 50
- This causes a buffer overflow into adjacent memory
- File name data gets corrupted or lost
- Checking `if (nc1 > 50)` would prevent processing of longer names

---

## Problem 4: Missing Timeout Handling

### Location
**RP2040F.ino, serialinput() function**

### Issue
The function uses an infinite `while(true)` loop with no timeout mechanism.

### Problem
- If serial data is corrupted or incomplete, the function waits forever
- The front panel appears to freeze, losing subsequent file names
- No error recovery mechanism exists

---

## Problem 5: Transmission Side Issues

### Location
**mainboard.ino, sendfilelist() function (Line ~1205)**

### Issue
The transmission doesn't include any error checking or acknowledgment.

### Code Evidence
```cpp
void sendfilelist()
{
  for (byte r = 0; r <= totalfile; r++)
  {  
    Serial.print("TX FILE PACKET: ");
    Serial.print("ABLM");
    Serial.print(r);
    Serial.print("*");
    Serial.print(filelist[r]);  // DIRECTLY SENDS - NO VALIDATION
    Serial.print("#&");
    Serial.println();
    
    Serial2.print('A');
    Serial2.print('B');
    Serial2.print('L');
    Serial2.print('M');
    Serial2.print(r);
    Serial2.print('*');
    Serial2.print(filelist[r]);  // MULTIPLE SERIAL2.print() calls!
    Serial2.print('#');
    Serial2.print('&');
    delay(10);
  }
}
```

### Why This Loses File Names

1. **Multiple print() calls**: Each `Serial2.print()` is a separate write operation
   - Characters could be interleaved with other Serial2 operations
   - No guarantee file name stays together in transmission

2. **No validation**: If `filelist[r]` contains special characters or null bytes, transmission fails

3. **Insufficient delay**: 10ms delay might not be enough for the Pico to process at 230400 baud

---

## Root Causes Summary

| Issue | Severity | Impact |
|-------|----------|--------|
| Incomplete buffer fill before parsing | **CRITICAL** | File names cut off mid-transmission |
| Buffer overflow (50-byte limit) | **CRITICAL** | Memory corruption, file name loss |
| Race condition in serial reception | **HIGH** | Intermittent file name loss |
| Multiple print() calls not atomic | **HIGH** | Characters from different files mixed |
| No timeout or error recovery | **MEDIUM** | System can hang on bad data |
| Incorrect substring extraction | **MEDIUM** | File names with certain patterns lost |

---

## Recommended Fixes

### Fix 1: Implement Proper Packet Reception
- Read the entire packet into a buffer before parsing
- Use a state machine approach
- Add CRC or checksum validation

### Fix 2: Increase Buffer Sizes
- Change `byte data3[50]` to at least `byte data3[300]`
- Add bounds checking to prevent overflow

### Fix 3: Atomic Serial Transmission
- Combine all `Serial2.print()` calls into a single string
- Use `Serial2.write()` for direct byte writing when possible

### Fix 4: Add Timeout and Error Handling
- Implement a timeout mechanism (e.g., 1-second max wait)
- Add packet validation before processing

### Fix 5: Improve Parser Robustness
- Use lastIndexOf() or proper offset tracking instead of indexOf()
- Trim whitespace from extracted file names
- Validate file names before storing

### Fix 6: Add Acknowledgment Protocol
- Pico should acknowledge receipt of each file name
- ESP32 should retry if no acknowledgment received

---

## Files Affected

1. **RP2040F.ino** (Pico - Receiver)
   - `serialinput()` function (Line 3221)
   - `inbufferclear()` function
   - Global `data3` buffer declaration

2. **mainboard.ino** (ESP32 - Transmitter)
   - `sendfilelist()` function (Line ~1205)
   - `sendfolderlist()` function (Line ~1180)

---

## Testing Recommendations

1. Send file names with:
   - Special characters: `test@file#1.txt`
   - Long names: `VeryLongFileNameToTestBufferOverflow.txt`
   - Unicode characters: `тест.txt`
   - Names with spaces: `File Name With Spaces.txt`

2. Monitor serial output for:
   - Truncated file names in LCD display
   - Repeated or missing file entries
   - System hangs or resets

3. Add debug logging to both serialinput() and sendfilelist() to compare sent vs. received strings

