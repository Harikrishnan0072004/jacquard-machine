# Implementation Guide: Fix File Name Loss Issue

## Overview
This guide provides step-by-step instructions to fix the file name loss issue between the ESP32 mainboard and RP2040 front panel.

## Phase 1: Preparation

### Step 1.1: Backup Your Files
```bash
# Create backups before making changes
Copy d:\Electronic_jac\mainboard\mainboard.ino to mainboard.ino.backup
Copy d:\Electronic_jac\RP2040F\RP2040F.ino to RP2040F.ino.backup
```

### Step 1.2: Understand the Problem
- The front panel loses file names during serial communication
- Root causes: buffer overflow, incomplete packet reception, race conditions
- Solution: Use proper packet handling and atomic transmission

---

## Phase 2: RP2040F (Front Panel) Changes

### Step 2.1: Increase Buffer Size

**File:** `d:\Electronic_jac\RP2040F\RP2040F.ino`

**Find (around line 70):**
```cpp
byte data3[50];
```

**Replace with:**
```cpp
byte data3[300];  // Increased from 50 to handle long file names
```

### Step 2.2: Add Global Variables

**File:** `d:\Electronic_jac\RP2040F\RP2040F.ino`

**Add after the buffer declaration (around line 80, with other globals):**
```cpp
unsigned long serialTimeout = 0;
const unsigned long SERIAL_TIMEOUT_MS = 2000;  // 2 second timeout
```

### Step 2.3: Replace serialinput() Function

**File:** `d:\Electronic_jac\RP2040F\RP2040F.ino`

**Find:** The `void serialinput()` function (starts at line 3221)

**Replace the entire function** with the fixed version from `FIXED_SERIALINPUT.ino`

**Important:** 
- Keep all the original function calls and logic intact
- Only replace the serialinput() function body
- The function name should remain `serialinput()` or rename calls if you use `serialinput_FIXED()`

---

## Phase 3: Mainboard (ESP32) Changes

### Step 3.1: Add Helper Function

**File:** `d:\Electronic_jac\mainboard\mainboard.ino`

**Add this helper function** (near the sendfolderlist() function, around line 1180):

```cpp
// Helper function to send packets atomically
void sendPacket(const char* packet) {
  if (packet == NULL || strlen(packet) == 0) {
    Serial.println("ERROR: Empty packet");
    return;
  }
  
  // Send entire packet as one operation to minimize fragmentation
  for (int i = 0; packet[i] != '\0'; i++) {
    Serial2.write((byte)packet[i]);
  }
  
  // Flush to ensure data is sent
  Serial2.flush();
  
  Serial.print("SENT: ");
  Serial.println(packet);
}
```

### Step 3.2: Replace sendfolderlist() Function

**File:** `d:\Electronic_jac\mainboard\mainboard.ino`

**Find:** The `void sendfolderlist()` function (around line 1180)

**Replace with:**
```cpp
void sendfolderlist()
{
  if (totalfolder == 0) {
    Serial.println("ERROR: No folders to send");
    return;
  }
  
  Serial.print("SENDING ");
  Serial.print(totalfolder);
  Serial.println(" FOLDERS");
  
  // First send total folder count
  char packet[300];
  snprintf(packet, sizeof(packet), "ABTK%d#&", totalfolder);
  sendPacket(packet);
  delay(50);
  
  // Then send each folder
  for (byte r = 0; r < totalfolder; r++)
  {
    String folderName = folderlist[r];
    folderName.trim();
    
    if (folderName.length() == 0) {
      Serial.print("WARNING: Folder ");
      Serial.print(r);
      Serial.println(" is empty, skipping");
      continue;
    }
    
    if (folderName.length() > 128) {
      Serial.print("WARNING: Folder name too long (");
      Serial.print(folderName.length());
      Serial.println("), truncating");
      folderName = folderName.substring(0, 128);
    }
    
    snprintf(packet, sizeof(packet), "ABLK%d*%s#&", r, folderName.c_str());
    
    if (strlen(packet) > 250) {
      Serial.print("ERROR: Packet too large for folder ");
      Serial.println(r);
      continue;
    }
    
    sendPacket(packet);
    delay(50);
  }
  
  Serial.println("FOLDER LIST TRANSMISSION COMPLETE");
}
```

### Step 3.3: Replace sendfilelist() Function

**File:** `d:\Electronic_jac\mainboard\mainboard.ino`

**Find:** The `void sendfilelist()` function (around line 1205)

**Replace with:**
```cpp
void sendfilelist()
{
  if (totalfile == 0) {
    Serial.println("ERROR: No files to send");
    return;
  }
  
  Serial.print("SENDING ");
  Serial.print(totalfile);
  Serial.println(" FILES");
  
  // First send total file count
  char packet[300];
  snprintf(packet, sizeof(packet), "ABTL%d#&", totalfile);
  sendPacket(packet);
  delay(50);
  
  // Then send each file
  for (byte r = 0; r < totalfile; r++)
  {
    String fileName = filelist[r];
    fileName.trim();
    
    if (fileName.length() == 0) {
      Serial.print("WARNING: File ");
      Serial.print(r);
      Serial.println(" is empty, skipping");
      continue;
    }
    
    if (fileName.length() > 128) {
      Serial.print("WARNING: File name too long (");
      Serial.print(fileName.length());
      Serial.println("), truncating");
      fileName = fileName.substring(0, 128);
    }
    
    snprintf(packet, sizeof(packet), "ABLM%d*%s#&", r, fileName.c_str());
    
    if (strlen(packet) > 250) {
      Serial.print("ERROR: Packet too large for file ");
      Serial.println(r);
      continue;
    }
    
    Serial.print("TX FILE[");
    Serial.print(r);
    Serial.print("]: ");
    Serial.println(fileName);
    
    sendPacket(packet);
    delay(50);
  }
  
  Serial.println("FILE LIST TRANSMISSION COMPLETE");
}
```

---

## Phase 4: Testing and Verification

### Step 4.1: Compile and Upload

1. **Upload to RP2040F (Front Panel):**
   - Open `d:\Electronic_jac\RP2040F\RP2040F.ino` in Arduino IDE
   - Select board: Raspberry Pi Pico
   - Click Upload
   - Monitor serial output (baud rate: 230400)

2. **Upload to Mainboard (ESP32):**
   - Open `d:\Electronic_jac\mainboard\mainboard.ino` in Arduino IDE
   - Select board: ESP32 (your specific model)
   - Click Upload
   - Monitor serial output (baud rate: 230400)

### Step 4.2: Testing Procedure

1. **Create test files** on the SD card with various names:
   - Normal: `test.txt`
   - With spaces: `My File Name.txt`
   - Long name: `VeryLongFileNameToTestBufferOverflow.txt`
   - With special chars: `test_2024-01-15.txt`

2. **Monitor serial output:**
   - Check RP2040F (Pico) serial output for received file names
   - Check mainboard (ESP32) serial output for transmitted file names
   - Compare transmitted vs. received names

3. **Example expected output (Mainboard):**
   ```
   SENDING 5 FILES
   SENT: ABTL5#&
   TX FILE[0]: test.txt
   SENT: ABLM0*test.txt#&
   TX FILE[1]: My File Name.txt
   SENT: ABLM1*My File Name.txt#&
   ...
   FILE LIST TRANSMISSION COMPLETE
   ```

4. **Example expected output (Front Panel - Pico):**
   ```
   RECEIVED PACKET: [LM0*test.txt]
   File[0]: [test.txt] Length: 8
   RECEIVED PACKET: [LM1*My File Name.txt]
   File[1]: [My File Name.txt] Length: 18
   ...
   ```

### Step 4.3: Verification Checklist

- [ ] All file names are received without truncation
- [ ] File names with spaces are preserved
- [ ] Long file names are handled (up to 128 chars)
- [ ] No "ERROR" messages in serial output
- [ ] Total file count matches actual files sent
- [ ] LCD display shows all file names correctly

---

## Phase 5: Troubleshooting

### Issue: Still losing file names

**Causes and solutions:**

1. **Changes not uploaded correctly**
   - Clean build: Delete build folders
   - Verify changes are in the actual uploaded code
   - Check Arduino IDE board selection

2. **SD card with extremely long file names (>128 chars)**
   - Rename files to be shorter
   - Or increase the 128-char limit in the code

3. **Serial port interference**
   - Close any other serial monitors
   - Use different USB cable
   - Try different USB port

4. **Baud rate mismatch**
   - Verify both use 230400 baud
   - Check Serial2.begin() settings

### Issue: System hangs or resets

**Causes:**

1. **Packet too large**
   - Solution: Reduce file name length
   - Or reduce number of files sent at once

2. **Timeout triggering**
   - Solution: Increase SERIAL_TIMEOUT_MS from 2000 to 5000

### Issue: File names appear corrupted

**Causes:**

1. **Special characters in file names**
   - Solution: Avoid special characters
   - Or add filtering in sendfilelist()

2. **Buffer overflow during old code**
   - Make sure you replaced data3[50] with data3[300]

---

## Phase 6: Optional Enhancements

### Enhancement 1: Add Logging Function

Add this to capture all file names for documentation:

```cpp
// In RP2040F.ino
void logReceivedFileNames() {
  Serial.println("\n=== RECEIVED FILE NAMES ===");
  Serial.print("Total files: ");
  Serial.println(totalfile);
  
  for (byte i = 0; i < totalfile; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(filelist[i]);
  }
  Serial.println("=== END FILE LIST ===\n");
}
```

### Enhancement 2: Add CRC Validation

For even better reliability, you can add packet CRC checking:

```cpp
// Calculate simple checksum
byte calculateCRC(const char* data) {
  byte crc = 0;
  while (*data) {
    crc += *data++;
  }
  return crc;
}
```

### Enhancement 3: Implement Acknowledgment

Send acknowledgment from Pico to ESP32:

```cpp
// In RP2040F.ino, after receiving file
void sendAcknowledge(byte fileIndex) {
  Serial2.print("ABACK");
  Serial2.print(fileIndex);
  Serial2.print("#&");
}
```

---

## Summary of Changes

### RP2040F.ino
| Line | Change | Reason |
|------|--------|--------|
| ~70 | Increase data3 buffer from 50 to 300 bytes | Prevent buffer overflow |
| ~80 | Add serialTimeout variables | Implement timeout protection |
| 3221 | Replace serialinput() function | Fix packet reception logic |

### mainboard.ino
| Line | Change | Reason |
|------|--------|--------|
| ~1150 | Add sendPacket() helper function | Enable atomic transmission |
| ~1180 | Replace sendfolderlist() function | Use atomic transmission |
| ~1205 | Replace sendfilelist() function | Use atomic transmission |

---

## Files Referenced

- `BUG_ANALYSIS.md` - Detailed technical analysis of the problem
- `FIXED_SERIALINPUT.ino` - Complete fixed serialinput() function
- `FIXED_SENDFILELIST.ino` - Complete fixed transmission functions

---

## Support

If you encounter issues:

1. Review the `BUG_ANALYSIS.md` for technical details
2. Check your serial output against the expected output examples
3. Verify all changes were applied correctly
4. Test with simpler file names first, then gradually add complexity

