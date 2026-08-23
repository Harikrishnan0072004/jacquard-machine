// =============================================================================
// FIXED CODE FOR RP2040F.ino - Serial Input with Proper Packet Handling
// =============================================================================
// 
// This is a replacement for the serialinput() function that fixes the 
// file name loss issues. Copy this entire function to replace the existing
// serialinput() function starting at line 3221 in RP2040F.ino
//
// Key improvements:
// 1. Uses a state machine for packet reception
// 2. Validates complete packets before parsing
// 3. Properly handles file names up to 128 characters
// 4. Includes timeout protection
// 5. Better error handling
//

// ============ BUFFER SIZE ADJUSTMENTS (do these first!) ============
// In RP2040F.ino, find and replace:
// OLD: byte data3[50];
// NEW: byte data3[300];  // Increased from 50 to handle long file names

// ============ GLOBAL VARIABLES TO ADD ============
// Add these near the top of RP2040F.ino with other globals:
/*
unsigned long serialTimeout = 0;
const unsigned long SERIAL_TIMEOUT_MS = 2000;  // 2 second timeout
*/

// ============ FIXED serialinput() FUNCTION ============

void serialinput_FIXED()
{
  Serial.print("Serial2.available BEFORE = ");
  Serial.println(Serial2.available());
  
  nc1 = 0;
  inbufferclear();
  inputString = "";
  serialTimeout = millis();
  
  // STATE 1: Wait for header 'A'
  while (millis() - serialTimeout < SERIAL_TIMEOUT_MS) {
    if (Serial2.available() > 0) {
      byte inbyte = (byte)Serial2.read();
      data3[nc1] = inbyte;
      
      if (data3[0] == 65) { // 'A'
        nc1 = 1;
        break;
      } else {
        nc1 = 0;  // Reset if not 'A'
      }
    }
  }
  
  if (nc1 == 0) {
    Serial.println("ERROR: Timeout waiting for header 'A'");
    return;
  }
  
  // STATE 2: Wait for header 'B'
  serialTimeout = millis();
  while (millis() - serialTimeout < SERIAL_TIMEOUT_MS) {
    if (Serial2.available() > 0) {
      byte inbyte = (byte)Serial2.read();
      data3[nc1] = inbyte;
      
      if (data3[1] == 66) { // 'B'
        nc1 = 2;
        break;
      } else {
        nc1 = 0;  // Reset invalid header
        return;
      }
    }
  }
  
  if (nc1 == 0) {
    Serial.println("ERROR: Timeout waiting for header 'B'");
    return;
  }
  
  // STATE 3: Read entire packet until we see "#&"
  serialTimeout = millis();
  while (millis() - serialTimeout < SERIAL_TIMEOUT_MS) {
    if (Serial2.available() > 0) {
      byte inbyte = (byte)Serial2.read();
      
      // Safety check: prevent buffer overflow
      if (nc1 >= 299) {
        Serial.println("ERROR: Packet buffer overflow!");
        nc1 = 0;
        return;
      }
      
      data3[nc1] = inbyte;
      nc1++;
      
      // Reset timeout on each received character
      serialTimeout = millis();
      
      // Check for packet end: "#&"
      if (nc1 >= 3 && data3[nc1 - 2] == 35 && data3[nc1 - 1] == 38) {
        // Valid packet received
        break;
      }
    }
  }
  
  // Verify packet completion
  if (nc1 < 4 || data3[nc1 - 2] != 35 || data3[nc1 - 1] != 38) {
    Serial.println("ERROR: Incomplete packet received");
    Serial.print("nc1 = "); Serial.println(nc1);
    return;
  }
  
  // Build inputString from data3 (skip 'A' and 'B', exclude '#' and '&')
  inputString = "";
  for (int i = 2; i < nc1 - 2; i++) {
    inputString += (char)data3[i];
  }
  
  Serial.print("RECEIVED PACKET: [");
  Serial.print(inputString);
  Serial.println("]");
  
  // =========== PACKET PARSING ===========
  
  // Command is in data3[2]
  byte command = data3[2];
  
  // -------------------------------------------------
  // D - Display parameters
  // -------------------------------------------------
  if (command == 68) { // 'D'
    byte subcommand = data3[3];
    
    // D K - File number
    if (subcommand == 75) { // 'K'
      int idx_K = inputString.indexOf('K');
      int idx_hash = inputString.indexOf('#');
      if (idx_K != -1 && idx_hash != -1 && idx_hash > idx_K) {
        String val = inputString.substring(idx_K + 1, idx_hash);
        filenum_m = val.toInt();
        Serial.println("filenum RECEIVED");
      }
      return;
    }
    
    // D 2 - Pick number
    if (subcommand == 50) { // '2'
      int idx_star = inputString.indexOf('*');
      int idx_hash = inputString.indexOf('#');
      if (idx_star != -1 && idx_hash != -1 && idx_hash > idx_star) {
        String val = inputString.substring(idx_star + 1, idx_hash);
        pickno = val.toInt();
        Serial.println("pickno RECEIVED");
      }
      return;
    }
    
    // D 3 - Total pick
    if (subcommand == 51) { // '3'
      int idx_star = inputString.indexOf('*');
      int idx_hash = inputString.indexOf('#');
      if (idx_star != -1 && idx_hash != -1 && idx_hash > idx_star) {
        String val = inputString.substring(idx_star + 1, idx_hash);
        height = val.toInt();
        Serial.println("height RECEIVED");
      }
      return;
    }
    
    // D 4 - Repeat count
    if (subcommand == 52) { // '4'
      int idx_star = inputString.indexOf('*');
      int idx_hash = inputString.indexOf('#');
      if (idx_star != -1 && idx_hash != -1 && idx_hash > idx_star) {
        String val = inputString.substring(idx_star + 1, idx_hash);
        repeatcnt = val.toInt();
        Serial.println("repeatcnt RECEIVED");
        if (filerunningmode == 1) d = 1;
      }
      return;
    }
    
    // D M - File number 1
    if (subcommand == 77) { // 'M'
      int idx_M = inputString.indexOf('M');
      int idx_hash = inputString.indexOf('#');
      if (idx_M != -1 && idx_hash != -1 && idx_hash > idx_M) {
        String val = inputString.substring(idx_M + 1, idx_hash);
        filenum1_m = val.toInt();
        Serial.println("filenum1 RECEIVED");
      }
      return;
    }
    
    // D 6 - Pick number 1
    if (subcommand == 54) { // '6'
      int idx_star = inputString.indexOf('*');
      int idx_hash = inputString.indexOf('#');
      if (idx_star != -1 && idx_hash != -1 && idx_hash > idx_star) {
        String val = inputString.substring(idx_star + 1, idx_hash);
        pickno1 = val.toInt();
        Serial.println("pickno1 RECEIVED");
      }
      return;
    }
    
    // D 7 - Height 1
    if (subcommand == 55) { // '7'
      int idx_star = inputString.indexOf('*');
      int idx_hash = inputString.indexOf('#');
      if (idx_star != -1 && idx_hash != -1 && idx_hash > idx_star) {
        String val = inputString.substring(idx_star + 1, idx_hash);
        height1 = val.toInt();
        Serial.println("height1 RECEIVED");
      }
      return;
    }
    
    // D 8 - Repeat count 1
    if (subcommand == 56) { // '8'
      int idx_star = inputString.indexOf('*');
      int idx_hash = inputString.indexOf('#');
      if (idx_star != -1 && idx_hash != -1 && idx_hash > idx_star) {
        String val = inputString.substring(idx_star + 1, idx_hash);
        repeatcnt1 = val.toInt();
        Serial.println("repeatcnt1 RECEIVED");
        if (filerunningmode == 2) d = 1;
      }
      return;
    }
  }
  
  // -------------------------------------------------
  // b - BtoT mode
  // -------------------------------------------------
  if (command == 98) { // 'b'
    BtoTmode = data3[3] - 48;
    Serial.print("BtoTmode = ");
    Serial.println(BtoTmode);
    return;
  }
  
  // -------------------------------------------------
  // i - LtoR mode
  // -------------------------------------------------
  if (command == 105) { // 'i'
    LtoRmode = data3[3] - 48;
    Serial.print("LtoRmode = ");
    Serial.println(LtoRmode);
    return;
  }
  
  // -------------------------------------------------
  // l - Lock value
  // -------------------------------------------------
  if (command == 108) { // 'l'
    int idx_l = inputString.indexOf('l');
    int idx_hash = inputString.indexOf('#');
    if (idx_l != -1 && idx_hash != -1 && idx_hash > idx_l) {
      String val = inputString.substring(idx_l + 1, idx_hash);
      lockdays1 = val.toInt();
    }
    return;
  }
  
  // -------------------------------------------------
  // T - Total folder / file
  // -------------------------------------------------
  if (command == 84) { // 'T'
    byte subcommand = data3[3];
    
    // T K - Total folders
    if (subcommand == 75) { // 'K'
      int idx_K = inputString.indexOf('K');
      int idx_hash = inputString.indexOf('#');
      if (idx_K != -1 && idx_hash != -1 && idx_hash > idx_K) {
        String val = inputString.substring(idx_K + 1, idx_hash);
        totalfolder_m = val.toInt();
        totalfolder = totalfolder_m;
        Serial.print("total folder: ");
        Serial.println(totalfolder_m);
      }
    }
    
    // T L - Total files
    if (subcommand == 76) { // 'L'
      int idx_L = inputString.indexOf('L');
      int idx_hash = inputString.indexOf('#');
      if (idx_L != -1 && idx_hash != -1 && idx_hash > idx_L) {
        String val = inputString.substring(idx_L + 1, idx_hash);
        totalfile_m = val.toInt();
        totalfile = totalfile_m;
        Serial.print("total file: ");
        Serial.println(totalfile_m);
      }
    }
    return;
  }
  
  // -------------------------------------------------
  // n - Number of cards
  // -------------------------------------------------
  if (command == 110) { // 'n'
    if (nc1 >= 5) {
      nc = ((data3[3] - 48) * 10) + (data3[4] - 48);
      Serial.print("No of Cards = ");
      Serial.println(nc);
    }
    return;
  }
  
  // -------------------------------------------------
  // L - List of folders / files (FIXED!)
  // -------------------------------------------------
  if (command == 76) { // 'L'
    byte subcommand = data3[3];
    
    // L K - Folder
    if (subcommand == 75) { // 'K'
      int idx_K = inputString.indexOf('K');
      int idx_star = inputString.indexOf('*');
      int idx_hash = inputString.indexOf('#');
      
      // Extract index
      if (idx_K != -1 && idx_star != -1 && idx_hash != -1 && 
          idx_star > idx_K && idx_hash > idx_star) {
        String idxStr = inputString.substring(idx_K + 1, idx_star);
        int folderIdx = idxStr.toInt();
        
        // Extract folder name
        String folderName = inputString.substring(idx_star + 1, idx_hash);
        folderName.trim();  // Remove whitespace
        
        if (folderIdx >= 0 && folderIdx < 50 && folderName.length() > 0) {
          folderlist_m[folderIdx] = folderName;
          folderlist[folderIdx] = folderName;
          
          Serial.print("Folder[");
          Serial.print(folderIdx);
          Serial.print("]: [");
          Serial.print(folderName);
          Serial.println("]");
          
          // Signal when last folder received
          if (folderIdx == totalfolder_m - 1) {
            d = 1;
          }
        }
      }
      return;
    }
    
    // L M - File (MAIN FIX FOR FILE NAME LOSS!)
    if (subcommand == 77) { // 'M'
      int idx_M = inputString.indexOf('M');
      int idx_star = inputString.indexOf('*');
      int idx_hash = inputString.indexOf('#');
      
      // Extract index
      if (idx_M != -1 && idx_star != -1 && idx_hash != -1 && 
          idx_star > idx_M && idx_hash > idx_star) {
        String idxStr = inputString.substring(idx_M + 1, idx_star);
        int fileIdx = idxStr.toInt();
        
        // Extract file name
        String fileName = inputString.substring(idx_star + 1, idx_hash);
        fileName.trim();  // Remove whitespace
        
        if (fileIdx >= 0 && fileIdx < 50 && fileName.length() > 0) {
          filelist_m[fileIdx] = fileName;
          filelist[fileIdx] = fileName;
          
          Serial.print("File[");
          Serial.print(fileIdx);
          Serial.print("]: [");
          Serial.print(fileName);
          Serial.print("] Length: ");
          Serial.println(fileName.length());
          
          // Signal when all files received
          if (fileIdx == totalfile_m - 1) {
            d = 1;
          }
        } else {
          Serial.print("ERROR: Invalid file index or empty name. idx=");
          Serial.print(fileIdx);
          Serial.print(" len=");
          Serial.println(fileName.length());
        }
      } else {
        Serial.println("ERROR: Failed to parse file packet");
        Serial.print("idx_M="); Serial.print(idx_M);
        Serial.print(" idx_star="); Serial.print(idx_star);
        Serial.print(" idx_hash="); Serial.println(idx_hash);
      }
      return;
    }
  }
  
  // -------------------------------------------------
  // M - File running mode
  // -------------------------------------------------
  if (command == 77) { // 'M'
    filerunningmode = data3[3] - 48;
    Serial.print("File running mode: ");
    Serial.println(filerunningmode);
    return;
  }
  
  // -------------------------------------------------
  // F - Folder number
  // -------------------------------------------------
  if (command == 70) { // 'F'
    int idx_F = inputString.indexOf('F');
    int idx_hash = inputString.indexOf('#');
    if (idx_F != -1 && idx_hash != -1 && idx_hash > idx_F) {
      String val = inputString.substring(idx_F + 1, idx_hash);
      int folderNum = val.toInt();
      foldernum = (byte)folderNum;
      kk1 = folderNum;
      Serial.print("Current Folder Number: ");
      Serial.println(folderNum);
    }
    return;
  }
  
  Serial.print("Unknown command: ");
  Serial.println((char)command);
}

// ============ END OF FIXED FUNCTION ============

