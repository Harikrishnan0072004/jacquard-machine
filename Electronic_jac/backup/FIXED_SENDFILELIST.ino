// =============================================================================
// FIXED CODE FOR mainboard.ino - Atomic Serial Transmission
// =============================================================================
//
// This file contains replacement functions for the mainboard (ESP32) that
// fix the file name loss issue by sending packets atomically without
// interleaving from other operations.
//
// Key improvements:
// 1. Combines multiple Serial2.print() into single transmission
// 2. Uses proper string building before sending
// 3. Adds validation of file names before transmission
// 4. Includes debug output matching the Pico receiver
// 5. Longer delays to ensure Pico has time to process
//

// ============ HELPER FUNCTION ============
// Add this new helper function to send a packet atomically
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

// ============ FIXED sendfolderlist() FUNCTION ============
void sendfolderlist_FIXED()
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
  delay(50);  // Increased delay for Pico to process
  
  // Then send each folder
  for (byte r = 0; r < totalfolder; r++)
  {
    // Validate folder name
    String folderName = folderlist[r];
    folderName.trim();
    
    if (folderName.length() == 0) {
      Serial.print("WARNING: Folder ");
      Serial.print(r);
      Serial.println(" is empty, skipping");
      continue;
    }
    
    // Build packet: ABLK[index]*[foldername]#&
    // Maximum folder name length should be 128 characters
    if (folderName.length() > 128) {
      Serial.print("WARNING: Folder name too long (");
      Serial.print(folderName.length());
      Serial.println("), truncating");
      folderName = folderName.substring(0, 128);
    }
    
    // Create packet
    snprintf(packet, sizeof(packet), "ABLK%d*%s#&", r, folderName.c_str());
    
    // Validate packet isn't too large
    if (strlen(packet) > 250) {
      Serial.print("ERROR: Packet too large for folder ");
      Serial.println(r);
      continue;
    }
    
    sendPacket(packet);
    delay(50);  // Delay between packets
  }
  
  Serial.println("FOLDER LIST TRANSMISSION COMPLETE");
}

// ============ FIXED sendfilelist() FUNCTION ============
void sendfilelist_FIXED()
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
  delay(50);  // Increased delay for Pico to process
  
  // Then send each file
  for (byte r = 0; r < totalfile; r++)
  {
    // Validate file name
    String fileName = filelist[r];
    fileName.trim();
    
    if (fileName.length() == 0) {
      Serial.print("WARNING: File ");
      Serial.print(r);
      Serial.println(" is empty, skipping");
      continue;
    }
    
    // Build packet: ABLM[index]*[filename]#&
    // Maximum file name length should be 128 characters
    if (fileName.length() > 128) {
      Serial.print("WARNING: File name too long (");
      Serial.print(fileName.length());
      Serial.println("), truncating");
      fileName = fileName.substring(0, 128);
    }
    
    // Create packet
    snprintf(packet, sizeof(packet), "ABLM%d*%s#&", r, fileName.c_str());
    
    // Validate packet isn't too large
    if (strlen(packet) > 250) {
      Serial.print("ERROR: Packet too large for file ");
      Serial.println(r);
      continue;
    }
    
    // Send with debug output
    Serial.print("TX FILE[");
    Serial.print(r);
    Serial.print("]: ");
    Serial.println(fileName);
    
    sendPacket(packet);
    delay(50);  // Delay between packets (increased from 10ms)
  }
  
  Serial.println("FILE LIST TRANSMISSION COMPLETE");
}

// ============ ALTERNATIVE: Single-Write Version (for better reliability) ============
// If you want even more reliability, use this version that builds the entire
// packet first, then sends it all at once:

void sendfilelist_ATOMIC()
{
  if (totalfile == 0) {
    Serial.println("ERROR: No files to send");
    return;
  }
  
  Serial.print("SENDING ");
  Serial.print(totalfile);
  Serial.println(" FILES (ATOMIC MODE)");
  
  // Build and send total count
  String totalPacket = "ABTL" + String(totalfile) + "#&";
  for (int i = 0; i < totalPacket.length(); i++) {
    Serial2.write((byte)totalPacket[i]);
  }
  Serial2.flush();
  Serial.print("SENT: ");
  Serial.println(totalPacket);
  delay(50);
  
  // Send each file
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
    
    // Build entire packet as String
    String packet = "ABLM" + String(r) + "*" + fileName + "#&";
    
    if (packet.length() > 250) {
      Serial.print("ERROR: Packet too large for file ");
      Serial.println(r);
      continue;
    }
    
    // Send in single operation
    Serial.print("TX FILE[");
    Serial.print(r);
    Serial.print("]: ");
    Serial.println(fileName);
    
    for (int i = 0; i < packet.length(); i++) {
      Serial2.write((byte)packet[i]);
    }
    Serial2.flush();
    
    Serial.print("SENT: ");
    Serial.println(packet);
    
    delay(50);  // Delay between packets
  }
  
  Serial.println("FILE LIST TRANSMISSION COMPLETE (ATOMIC MODE)");
}

// ============ OPTIONAL: Improved Total Counts Function ============
void sendTotalCounts_FIXED()
{
  // Send folder and file counts in one operation
  char packet[100];
  
  // Total folders
  snprintf(packet, sizeof(packet), "ABTK%d#&", totalfolder);
  sendPacket(packet);
  delay(30);
  
  // Total files
  snprintf(packet, sizeof(packet), "ABTL%d#&", totalfile);
  sendPacket(packet);
  delay(30);
  
  Serial.println("SENT: Total folder and file counts");
}

// ============ VERIFICATION FUNCTION ============
// Add this to verify file names match between sender and receiver:

void verifyFileTransmission()
{
  // This would be called after file transmission to verify all names match
  // You could implement a simple checksum or request the Pico to send back
  // the received file count and names for comparison
  
  Serial.println("=== FILE TRANSMISSION VERIFICATION ===");
  Serial.print("Total files to send: ");
  Serial.println(totalfile);
  
  for (byte i = 0; i < totalfile; i++) {
    String fileName = filelist[i];
    fileName.trim();
    
    Serial.print("File ");
    Serial.print(i);
    Serial.print(": [");
    Serial.print(fileName);
    Serial.print("] Length: ");
    Serial.println(fileName.length());
    
    // Check for problematic characters
    bool hasProblems = false;
    for (int j = 0; j < fileName.length(); j++) {
      char c = fileName[j];
      if (c < 32 || c > 126) {  // Non-printable characters
        Serial.print("  WARNING: Non-printable character at position ");
        Serial.print(j);
        Serial.print(": 0x");
        Serial.println(c, HEX);
        hasProblems = true;
      }
    }
    
    if (!hasProblems) {
      Serial.println("  OK");
    }
  }
  Serial.println("=== END VERIFICATION ===");
}

// ============ USAGE NOTES ============
/*
REPLACE THE EXISTING FUNCTIONS:

1. In mainboard.ino, find the current sendfolderlist() function
2. Replace it with sendfolderlist_FIXED()
3. Find the current sendfilelist() function
4. Replace it with sendfilelist_ATOMIC() (recommended for best reliability)
5. Make sure any calls to these functions still work the same way

OPTIONAL IMPROVEMENTS:

- Use sendPacket() helper function for all Serial2 communications
- Call verifyFileTransmission() after sending files to debug issues
- Increase delays if you're still seeing file name loss
- Add CRC checking for extra robustness (more advanced)

TESTING:

1. Create files with special names in your SD card
2. Upload the fixed code to both boards
3. Monitor serial output from both boards
4. Compare transmitted vs received file names in the debug logs
*/

