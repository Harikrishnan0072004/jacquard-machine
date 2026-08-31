# TFT_Front_Panel.ino - Folder & File Copy Functions

## Overview
The code implements USB-to-SD card file transfer functionality using serial communication (Serial2) between the ESP32 S3 (Front Panel) and a Main Board. The main copy functions handle folder and file operations.

---

## 1. **usbCopyFolder()** - Line 1339
**Purpose:** Copy an entire folder from USB drive to the mainboard SD card

```cpp
bool usbCopyFolder(String folderName)
```

**Flow:**
1. Displays "Copy Folder:" on TFT screen
2. Calls `usbSendFolderCreate(folderName)` to create folder on mainboard
   - Returns: 0=no reply, 1=exists, 2=created, 3=mkdir failed
3. Scans files in the folder using `usbScanFilesInFolder()`
4. Loops through each file and calls `usbCopyOneFile()` for each
5. Tracks success/failure counts: `copiedOk`, `copiedFail`
6. Displays progress on TFT: "Folder Copy Done, OK: X, Failed: Y"
7. Calls `usbRefreshMainboardFolderList()` to update mainboard folder list
8. Returns true if all files copied successfully (copiedFail == 0)

**Key features:**
- Checks if USB drive is removed during copy
- Provides real-time UI feedback
- Gracefully handles existing folders (skips if already exists)
- Updates running status after completion

---

## 2. **usbCopyOneFile()** - Line 830
**Purpose:** Copy a single BMP/EJC file from USB folder to mainboard

```cpp
bool usbCopyOneFile(String folderName, String fileName)
```

**Complex Protocol Flow:**

### A. Format Validation
- Calls `usbCheckFileFormatMatch()` to verify file format
- Checks bitmap height at offset 0x16
- Validates payload matches expected card count

### B. File Opening (Serial2 Protocol)
1. Sends packet: `ABf6*{fileSize}#{}`
2. Waits for file-open reply from mainboard

### C. Header Transfer
1. Sends: `ABDo` + 62 bytes of bitmap metadata
2. Waits for `D1` acknowledgment

### D. Card Data Transfer (per-card loop)
For each card in the file (ttotal = 1 to height):

**Step 1 - Card Parameter Packet:**
- Sends: `ABct*{cardNumber}*{fileTotalData}#{}`
- Waits for `D2` acknowledgment

**Step 2 - Card Data Packet:**
- Reads card data from file buffer
- Calculates CRC16-CCITT for error checking
- Sends: `ABDe` (or `ABDs` for last card) + payload + 2-byte CRC
- Waits for `D1` (success) or `D0` (NACK)

**Step 3 - Retry Logic:**
- If NACK or timeout: retries up to 5 times with same buffer
- Updates TFT progress bar (0-100%)

### E. Completion
- Closes file
- Calls `usbCheckFileSize()` to verify received size matches
- Returns true if size matches (no data loss)
- Calls `usbAbortFileCopy()` if error occurs

**Key features:**
- Detailed error logging to Serial1
- Real-time progress display with percentage
- Automatic retry on CRC errors
- Validates data integrity by checking file size
- Gracefully skips files that already exist (never overwrites)

---

## 3. **usbSendFolderCreate()** - Line 1608
**Purpose:** Request mainboard to create a folder via serial protocol

```cpp
byte usbSendFolderCreate(String folderName)
```

**Protocol:**
- Sends: `ABf1*{folderName}#{}`
- Waits for reply byte pattern: `D<n>`
  - `D1` = Folder already exists
  - `D2` = Folder successfully created
  - `D3` = Mkdir failed
  - `0` = Timeout/no reply

**Returns:** 0, 1, 2, or 3 (see above)

---

## 4. **usbScanFilesInFolder()** - Line 600
**Purpose:** List all .BMP and .EJC files in a USB folder

```cpp
void usbScanFilesInFolder(String folderName)
```

**Features:**
- Stores up to 50 filenames in `usbFileList[]`
- Filters by extension (.BMP, .bmp, .EJC, .ejc)
- Sets `usbFileCount` and `usbCurrentFolderName`

---

## 5. **Helper Functions**

### usbCheckFileFormatMatch() - Line 530
- Validates BMP file format (height, payload alignment)
- Checks if file matches expected card count
- Displays error messages if invalid

### usbCheckFileExists() - Line 1127
- Sends: `ABf4*{fileName}#{}`
- Returns: 1=exists, 2=doesn't exist, 0=timeout

### usbCheckFileSize() - Line 1170
- Sends: `ABf5*{fileName}#{}`
- Parses response: `Dz*{size}#`
- Returns received file size on mainboard

### usbAbortFileCopy() - Line 1195
- Sends: `ABf7#{}`
- Aborts incomplete file transfer

### usbRefreshMainboardFolderList() - Line 1650
- Sends: `ABf3#{}`
- Waits for mainboard to send updated folder/file list
- Timeout: 5 seconds

---

## Serial2 Communication Protocol Summary

| Command | Format | Response | Meaning |
|---------|--------|----------|---------|
| **ABf1** | `ABf1*folderName#{}` | D1/D2/D3 | Create folder |
| **ABf2** | `ABf2*fileName#{}` | D1/D2 | Open file (exists/new) |
| **ABf3** | `ABf3#{}` | (folder list) | Refresh folder list |
| **ABf4** | `ABf4*fileName#{}` | D1/D2 | Check file exists |
| **ABf5** | `ABf5*fileName#{}` | Dz*size# | Get file size |
| **ABf6** | `ABf6*size#{}` | (ack) | File size notification |
| **ABf7** | `ABf7#{}` | (ack) | Abort file copy |
| **ABDo** | Header (62 bytes) | D1 | Bitmap header |
| **ABct** | `ABct*card*totaldata#{}` | D2 | Card parameters |
| **ABDe/s** | Card data + CRC | D1/D0 | Card data (e=end, s=last) |

---

## Error Handling & Retries

- **File Copy:** Retries up to 5 times per card on CRC failure
- **Folder Creation:** Returns error code immediately
- **Timeouts:** 3-10 seconds depending on operation
- **USB Removal:** Checks and aborts if drive removed mid-transfer

---

## Key Variables

```cpp
String usbFolderList[50];      // List of folders on USB
byte   usbFolderCount;         // Number of folders found
String usbFileList[50];        // List of files in current folder
byte   usbFileCount;           // Number of files found
String usbCurrentFolderName;   // Current folder being browsed
File32 usbSrcFile;             // Current source file handle
```

---

## Display Feedback (TFT Screen)

- **"Copy Folder:"** - Folder name being copied
- **"Reading drive..."** - Scanning USB files
- **"File Copy:"** - Current file name
- **Progress bar** - 0-100% with card count (e.g., "50% 25/50")
- **"Folder Copy Done"** - Final result with OK/Failed counts
- **"Updating Folder List"** - Refreshing mainboard status
