# Quick Test Guide - After Applying Fixes

## ⚡ 5-Minute Testing Procedure

### Step 1: Upload Code (5 minutes)
```
1. Close any serial monitors
2. Upload RP2040F.ino to Raspberry Pi Pico
   - Select Board: Raspberry Pi Pico
   - Click Upload
3. Upload mainboard.ino to ESP32
   - Select Board: ESP32 (your model)
   - Click Upload
```

### Step 2: Open Serial Monitors (2 windows)
```
Window 1 (RP2040):
- Port: COM? (Pico)
- Baud: 230400
- Keep open while testing

Window 2 (ESP32):
- Port: COM? (Mainboard)
- Baud: 230400
- Keep open while testing
```

### Step 3: Initial Boot Test (Wait ~10 seconds)
**What to see on ESP32:**
```
Electronics Jacquard CPU
nc: 12
Start to run
```

**What to see on RP2040:**
```
TILT ELECTRONICS
BANGALORE
(Should then wait for commands)
```

✅ **If you see this:** Code uploaded correctly
❌ **If not:** Try uploading again

---

## 🧪 Test 1: File List Reception (1 minute)

**On RP2040 LCD:** Should see list of files from SD card

**On ESP32 Serial:** Should see:
```
SENDING 5 FILES
TX FILE[0]: design1.bmp
SENT: ABLM0*design1.bmp#&
TX FILE[1]: design2.bmp
SENT: ABLM1*design2.bmp#&
TX FILE[2]: design3.bmp
SENT: ABLM2*design3.bmp#&
FILE LIST TRANSMISSION COMPLETE
```

**On RP2040 Serial:** Should see:
```
RECEIVED PACKET: [LM0*design1.bmp]
File[0]: [design1.bmp] Length: 12
RECEIVED PACKET: [LM1*design2.bmp]
File[1]: [design2.bmp] Length: 12
RECEIVED PACKET: [LM2*design3.bmp]
File[2]: [design3.bmp] Length: 12
```

✅ **Pass:** All file names match exactly
❌ **Fail:** File names truncated or mismatched

---

## 🧪 Test 2: File Name Length Test (1 minute)

**Test with long file names:**
- Copy a file with name: `VeryLongDesignName2024Jan15.bmp`
- Restart boards
- Check if full name appears on both serial monitors

**Expected on RP2040:**
```
File[0]: [VeryLongDesignName2024Jan15.bmp] Length: 29
```

✅ **Pass:** Full name displays without truncation
❌ **Fail:** Name is cut off

---

## 🧪 Test 3: Folder Change Test (2 minutes)

**On RP2040 LCD:**
1. Navigate to folder list (using arrow keys)
2. Select different folder (press OK)
3. Should see new file list

**Expected on ESP32 Serial:**
```
Folder Change: 1
(Then it should read new folder and send files)

SENDING 3 FILES
TX FILE[0]: pattern1.bmp
SENT: ABLM0*pattern1.bmp#&
...
FILE LIST TRANSMISSION COMPLETE
```

**Expected on RP2040 Serial:**
```
SENT FOLDER CHANGE: ABF1#&
```

✅ **Pass:** New folder files appear on LCD
❌ **Fail:** Files don't update after folder change

---

## 🧪 Test 4: Error Checking (1 minute)

**Look for ERROR messages:**
- ESP32 Serial: Should NOT see "ERROR"
- RP2040 Serial: Should NOT see "ERROR"

**If you see errors like:**
```
ERROR: Timeout waiting for header 'A'
ERROR: Incomplete packet received
ERROR: Packet buffer overflow
```

**Actions:**
1. Press Reset on RP2040
2. Check serial cable connection
3. Verify files on SD card exist and are valid

---

## ✅ Final Success Checklist

- [ ] File names appear on LCD
- [ ] No file names are truncated
- [ ] File count matches actual files
- [ ] Folder changes work smoothly
- [ ] No ERROR messages in serial output
- [ ] Both serial monitors show clean packets
- [ ] System doesn't hang or crash
- [ ] File transmission completes within 5 seconds

**If all checked:** ✅ **FIX IS WORKING!**

---

## 🆘 Quick Troubleshooting

### Problem: Files still missing/truncated

**Quick fix:**
1. Close all serial monitors
2. Press Reset button on RP2040
3. Press Reset button on ESP32
4. Reopen serial monitors
5. Wait 10 seconds for boot
6. Try again

### Problem: Serial output shows errors

**Quick fix:**
1. Check that `data3[300]` is in RP2040 code
2. Check that `sendPacket()` exists in mainboard code
3. Recompile and reupload

### Problem: Folder changes not working

**Quick fix:**
1. Make sure you're in folder menu on LCD
2. Check ESP32 serial for "Folder Change:" message
3. Wait 2 seconds after selecting folder
4. Check if file list updates

### Problem: System hangs

**Quick fix:**
1. Press Reset button immediately
2. Wait 5 seconds
3. Try again
4. If persists, check for infinite loops in parsing

---

## 📊 Performance Expectations

After fix, you should see:
- **File list transmission time:** < 5 seconds for 50 files
- **Folder change response time:** < 2 seconds
- **Serial baud rate:** 230400 (no errors)
- **Error rate:** 0 (no lost packets)
- **System stability:** No crashes or hangs

---

## 📝 What to Report if Issues Remain

If problems persist, provide:
1. **Serial output logs** (copy both ESP32 and RP2040 output)
2. **File names** on SD card (the ones being lost)
3. **Number of files** in each folder
4. **LCD display** screenshot (if possible)
5. **Error messages** from serial monitors

---

## 🎯 Next Steps

1. **Upload and run tests above**
2. **Compare your results with expected output**
3. **If all tests pass:** System is fixed! ✅
4. **If any test fails:** Check troubleshooting section
5. **Document any remaining issues:** Save logs for reference

---

**Status: Ready to Test!**

The code fixes are complete and ready for testing. Upload and follow this guide!

