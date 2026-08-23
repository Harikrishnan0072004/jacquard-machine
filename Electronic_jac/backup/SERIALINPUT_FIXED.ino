// ======================================================================
// PROPERLY FORMATTED serialinput() FUNCTION
// Copy and paste this to replace the current serialinput() function
// in RP2040F.ino starting at line 3223
// ======================================================================

void serialinput()
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
        nc1 = 0;
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
        nc1 = 0;
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
      
      if (nc1 >= 299) {
        Serial.println("ERROR: Packet buffer overflow!");
        nc1 = 0;
        return;
      }
      
      data3[nc1] = inbyte;
      nc1++;
      
      serialTimeout = millis();
      
      if (nc1 >= 3 && data3[nc1 - 2] == 35 && data3[nc1 - 1] == 38) {
        break;
      }
    }
  }
  
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
  
  // Now parse the packet
  if (data3[0] == 65 && data3[1] == 66) { // 'A' and 'B'
    // -------------------------------------------------
    // D - Display
    // -------------------------------------------------
    if (data3[2] == 68) { // 'D'
      // D K - File number
      if (data3[3] == 75) { // 'K'
        int e = inputString.indexOf('K');
        int f = inputString.indexOf('#');
        if (e != -1 && f != -1) {
          String pick = inputString.substring(e + 1, f);
          filenum_m = pick.toInt();
          Serial.println("filenum RECEIVED");
        }
        goto rout;
      }

      // D 2 - Pick number
      if (data3[3] == 50) { // '2'
        int e = inputString.indexOf('*');
        int f = inputString.indexOf('#');
        if (e != -1 && f != -1) {
          String pick = inputString.substring(e + 1, f);
          pickno = pick.toInt();
          Serial.println("pickno RECEIVED");
        }
        goto rout;
      }

      // D 3 - Total pick
      if (data3[3] == 51) { // '3'
        int e = inputString.indexOf('*');
        int f = inputString.indexOf('#');
        if (e != -1 && f != -1) {
          String pick = inputString.substring(e + 1, f);
          height = pick.toInt();
          Serial.println("height RECEIVED");
        }
        goto rout;
      }

      // D 4 - Repeat count
      if (data3[3] == 52) { // '4'
        int e = inputString.indexOf('*');
        int f = inputString.indexOf('#');
        if (e != -1 && f != -1) {
          String pick = inputString.substring(e + 1, f);
          repeatcnt = pick.toInt();
          Serial.println("repeatcnt RECEIVED");
          if (filerunningmode == 1)
            d = 1;
        }
        goto rout;
      }

      // D M - File number 1
      if (data3[3] == 77) { // 'M'
        int e = inputString.indexOf('M');
        int f = inputString.indexOf('#');
        if (e != -1 && f != -1) {
          String pick = inputString.substring(e + 1, f);
          filenum1_m = pick.toInt();
          Serial.println("filenum1 RECEIVED");
        }
        goto rout;
      }

      // D 6 - Pick number 1
      if (data3[3] == 54) { // '6'
        int e = inputString.indexOf('*');
        int f = inputString.indexOf('#');
        if (e != -1 && f != -1) {
          String pick = inputString.substring(e + 1, f);
          pickno1 = pick.toInt();
          Serial.println("pickno1 RECEIVED");
        }
        goto rout;
      }

      // D 7 - Height 1
      if (data3[3] == 55) { // '7'
        int e = inputString.indexOf('*');
        int f = inputString.indexOf('#');
        if (e != -1 && f != -1) {
          String pick = inputString.substring(e + 1, f);
          height1 = pick.toInt();
          Serial.println("height1 RECEIVED");
        }
        goto rout;
      }

      // D 8 - Repeat count 1
      if (data3[3] == 56) { // '8'
        int e = inputString.indexOf('*');
        int f = inputString.indexOf('#');
        if (e != -1 && f != -1) {
          String pick = inputString.substring(e + 1, f);
          repeatcnt1 = pick.toInt();
          Serial.println("repeatcnt1 RECEIVED");
          if (filerunningmode == 2)
            d = 1;
        }
        goto rout;
      }
    }

    // -------------------------------------------------
    // b - BtoT mode
    // -------------------------------------------------
    if (data3[2] == 98) { // 'b'
      BtoTmode = data3[3] % 48;
      delay(100);
      Serial.print("BtoTmode = ");
      Serial.println(BtoTmode);
      return;
    }

    // -------------------------------------------------
    // i - LtoR mode
    // -------------------------------------------------
    if (data3[2] == 105) { // 'i'
      LtoRmode = data3[3] % 48;
      delay(100);
      Serial.print("LtoRmode = ");
      Serial.println(LtoRmode);
      return;
    }

    // -------------------------------------------------
    // l - Lock value
    // -------------------------------------------------
    if (data3[2] == 108) { // 'l'
      int e = inputString.indexOf('l');
      int f = inputString.indexOf('#');
      if (e != -1 && f != -1) {
        String pick = inputString.substring(e + 1, f);
        lockdays1 = pick.toInt();
      }
      goto rout;
    }

    // -------------------------------------------------
    // T - Total folder / file
    // -------------------------------------------------
    if (data3[2] == 84) { // 'T'
      // T K - Total folders
      if (data3[3] == 75) { // 'K'
        int e = inputString.indexOf('K');
        int f = inputString.indexOf('#');
        if (e != -1 && f != -1) {
          String pick = inputString.substring(e + 1, f);
          totalfolder_m = pick.toInt();
          totalfolder = totalfolder_m;
          Serial.print("total folder:\t");
          Serial.println(totalfolder_m);
        }
      }

      // T L - Total files
      if (data3[3] == 76) { // 'L'
        int e = inputString.indexOf('L');
        int f = inputString.indexOf('#');
        if (e != -1 && f != -1) {
          String pick = inputString.substring(e + 1, f);
          totalfile_m = pick.toInt();
          totalfile = totalfile_m;
          Serial.print("total file:\t");
          Serial.println(totalfile_m);
        }
      }
      goto rout;
    }

    // -------------------------------------------------
    // n - Number of cards
    // -------------------------------------------------
    if (data3[2] == 110) { // 'n'
      nc = (data3[3] % 48) * 10;
      nc = (data3[4] % 48) + nc;
      delay(100);
      Serial.print("No of Cards = ");
      Serial.println(nc);
      return;
    }

    // -------------------------------------------------
    // L - List of folders / files
    // -------------------------------------------------
    if (data3[2] == 76) { // 'L'
      // L K - Folder
      if (data3[3] == 75) { // 'K'
        int idx_K = inputString.indexOf('K');
        int idx_star = inputString.indexOf('*');
        int idx_hash = inputString.indexOf('#');
        
        if (idx_K != -1 && idx_star != -1 && idx_hash != -1 && 
            idx_star > idx_K && idx_hash > idx_star) {
          String idxStr = inputString.substring(idx_K + 1, idx_star);
          int folderIdx = idxStr.toInt();
          String folderName = inputString.substring(idx_star + 1, idx_hash);
          folderName.trim();
          
          if (folderIdx >= 0 && folderIdx < 50 && folderName.length() > 0) {
            folderlist_m[folderIdx] = folderName;
            folderlist[folderIdx] = folderName;
            Serial.print("Folder[");
            Serial.print(folderIdx);
            Serial.print("]: [");
            Serial.print(folderName);
            Serial.print("] Length: ");
            Serial.println(folderName.length());
            
            if (folderIdx == totalfolder_m - 1) {
              d = 1;
            }
          }
        }
        goto rout;
      }

      // L M - File
      if (data3[3] == 77) { // 'M'
        int idx_M = inputString.indexOf('M');
        int idx_star = inputString.indexOf('*');
        int idx_hash = inputString.indexOf('#');
        
        if (idx_M != -1 && idx_star != -1 && idx_hash != -1 && 
            idx_star > idx_M && idx_hash > idx_star) {
          String idxStr = inputString.substring(idx_M + 1, idx_star);
          int fileIdx = idxStr.toInt();
          String fileName = inputString.substring(idx_star + 1, idx_hash);
          fileName.trim();
          
          if (fileIdx >= 0 && fileIdx < 50 && fileName.length() > 0) {
            filelist_m[fileIdx] = fileName;
            filelist[fileIdx] = fileName;
            Serial.print("File[");
            Serial.print(fileIdx);
            Serial.print("]: [");
            Serial.print(fileName);
            Serial.print("] Length: ");
            Serial.println(fileName.length());
            
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
        goto rout;
      }
    }

    // -------------------------------------------------
    // M - File running mode
    // -------------------------------------------------
    if (data3[2] == 77) { // 'M'
      filerunningmode = data3[3] - 48;
      Serial.print("File running mode\t");
      Serial.println(filerunningmode);
      goto rout;
    }

    // -------------------------------------------------
    // F - Folder number
    // -------------------------------------------------
    if (data3[2] == 70) { // 'F'
      int e = inputString.indexOf('F');
      int f = inputString.indexOf('#');
      if (e != -1 && f != -1) {
        String pick = inputString.substring(e + 1, f);
        int tempy = pick.toInt();
        foldernum = (byte)tempy;
        kk1 = tempy;
        Serial.print("Current Folder Number\t");
        Serial.println(tempy);
      }
      goto rout;
    }

    // -------------------------------------------------
    // I - Master initialization display
    // -------------------------------------------------
    if (data3[2] == 73) { // 'I'
      if (data3[3] == 49) { // '1'
        int e = inputString.indexOf('*');
        int f = inputString.indexOf('#');
        if (e != -1 && f != -1) {
          String pick = inputString.substring(e + 1, f);
          if (lcdcnt >= 4) {
            lcd.clear();
            lcdcnt = 0;
          }
          lcd.setCursor(0, lcdcnt);
          lcd.print(pick);
          lcdcnt = lcdcnt + 1;
        }
      }
      goto rout;
    }

    // Packet ended correctly but command was not recognized
    Serial.println("Unknown command");
  }

rout:
  Serial.println("out");
}

// ======================================================================
// END OF FIXED serialinput() FUNCTION
// ======================================================================
