#include "SdFat_Adafruit_Fork.h"
#include "usbh_helper.h"
#include <SPI.h>
// #include "SdFat.h"
// #include <Adafruit_TinyUSB.h>
#include <FS.h>
 //#include <SD.h>
//#include <FSImpl.h>
//#include <vfs_api.h>
//#include <sd_defines.h>
//#include <sd_diskio.h>
#include <Keypad.h>
#include <Wire.h>
#include <stdio.h>
#include <EEPROM.h>
#include <LiquidCrystal_PCF8574.h>
#include <RPi_Pico_TimerInterrupt.h>

// // 1. MUST BE FIRST: Configure TinyUSB Host Flags for Mass Storage Mode
 #define CFG_TUH_MSC      1  

/*I2C PINS */
#define LCD_SDA   6 //16 
#define LCD_SCL   7 //17
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display


#define RXD2 9
#define TXD2 8

#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // four columns

// Optional CRC test: 0 = disabled; set to 5 to flip payload byte 5 once.
#define CDC_FLIP_TEST_BYTE 0

File root;
File root3;
File bMap;
File bMap1;
File bMap3;

//////////////////////////////////////////////////////////////////////////////////
const int nr = 16; //nr=number of ribbons   
//////////////////////////////////////////////////////////////////////////////////

/*const int Menu = 13;   //Key2 
const int Enter = 27;  //Key4 
const int F1 = 4;      //Key1
const int F2 = 14;     //Key3
const int Up = 26;     //Key5
const int Down = 32;   //Key8
const int Left = 25;   //Key6
const int Right = 33;  //Key7*/

int nc = 12; //nc=number of cards per ribbon


byte kk1,lkk1,d=0;
int nc1 = 0 ; 
byte pulcnt = 0; 
byte esc = 0;
byte data2[1030]; // Increased to match ESP32 buffer size

const byte ROWS = 4; //four rows
const byte COLS = 3;
byte lcdcnt = 0, temp1 = 0;
byte data1[1030]; // Increased to match ESP32 buffer size
char pass[8];
char SysNo[8];
char out[11];
char file1[15];
char customKey = ' ';
String filelist[50],    s_name,foldername1,   filename2,folderlist[50],   ls_name;
String filelist_m[50],  foldername_m,filename1_m,       folderlist_m[50],  filename2_m; 
char ch;
unsigned long height = 0,temp3 = 0;
unsigned long height1 = 0;
int totaldata = 0,nextlockvalue = 0,lockvalue = 0,lockdays = 0;
int nextlockvalue1 = 0,lockvalue1 = 0,lockdays1 = 0;
String inputString = "";
byte check_bit = 0,tempq = 0;     //   Check bits
byte filereadcnt = 0,foldernum = 0;
byte data3[1030];  // Increased to match ESP32 buffer size
byte pluseupcheck,lastpickno1;
unsigned long serialTimeout = 0;
const unsigned long SERIAL_TIMEOUT_MS = 2000;  // 2 second timeout
unsigned long lastpickno;
byte filerunningmode = 1,BtoTmode = 0,LtoRmode = 0;
byte break2;
byte setup11=0;
String str;
int mcnt = 0,r,c,m;
unsigned int bitmapOffset = 62;
byte Menu2 = 0,Enter2 = 0,F12 = 0,F22 = 0,Up2 = 0,Down2 = 0,Left2 = 0,Right2 = 0;
byte motorstatus = 0,temp = 0;
byte totalfile = 0,totalfolder = 0;
int totalfile_m = 0,totalfolder_m = 0;
int filenum_m = 0,filenum1_m = 0;
byte filenum = 0,lockid = 0;
byte filenum1 = 0;
byte tempg1 = 0;
unsigned long cnt = 0,tcnt = 0;
char Direction = 0;
unsigned long pickno = 0,Sysnumber = 0;
unsigned long pickno1 = 0;
unsigned long repeatcnt = 0;
long repeatcnt1 = 0;
byte customChar[] = {
  0b00100,
  0b00110,
  0b00111,
  0b11111,
  0b11111,
  0b00111,
  0b00110,
  0b00100
};

byte Check[] = {
0b00000,
0b00001,
0b00011,
0b10110,
0b11100,
0b01000,
0b00000,
0b00000
};

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', '4'},  // Row 1 (R1) -> Buttons: 1, 2, 3, 4
  {'5', '6', '7', '8'},  // Row 2 (R2) -> Buttons: 5, 6, 7, 8
  {'9', '0', '*', 'L'},  // Row 3 (R3) -> Buttons: 9, 0, [ESC key = '*'], [Left key]
  {'B', 'R', 'E', 'A'}   // Row 4 (R4) -> Buttons: [Down key = 'B'], [Right key], [OK key = 'E'], [Up key = 'A']
};

byte pin_rows[ROW_NUM]      = {15, 13, 11, 10}; 
byte pin_column[COLUMN_NUM] = {16, 17, 14, 12};
Keypad customKeypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );
 /* USB GLOBALVARIABLE*/


// USB Host MSC Block Device object which implemented API for use with SdFat
Adafruit_USBH_MSC_BlockDevice msc_block_dev;
FatVolume fatfs;    
File32 usbSrcFile;
bool is_mounted = false;
#ifndef USE_TINYUSB_HOST
  #error This example requires usb stack configured as host in "Tools -> USB Stack -> Adafruit TinyUSB Host"
#endif
#define LANGUAGE_ID 0x0409  // Language ID: English
const int ledPin = 25;
//Dev Info Structure
typedef struct {
  tusb_desc_device_t desc_device;
  uint16_t manufacturer[32];
  uint16_t product[48];
  uint16_t serial[16];
  bool mounted;
} dev_info_t;
dev_info_t dev_info[CFG_TUH_DEVICE_MAX] = { 0 };
RPI_PICO_Timer usbTimer(0);
bool usbMountFailed         = false;  // TC-1.4: device attached, fatfs.begin() failed
volatile bool usbBrowseActive       = false;  // true whenever any USB browse screen is showing
volatile bool usbRemovedDuringBrowse = false; // set by tuh_umount_cb() if pulled mid-browse
String usbFolderList[50];
byte   usbFolderCount = 0;
String usbFileList[50];
byte   usbFileCount = 0;
String usbCurrentFolderName = "";

 void usbhostsetup();
  void  usbhostloop();
//   bool usbTimerCallback(repeating_timer_t *rt) {
//   usbhostloop();
//   return true; // keep repeating
// }
void setup() {
  delay(3000);   
  //Serial2.begin(230400, SERIAL_8N1, RXD2, TXD2);
  Serial2.setRX(RXD2); 
  Serial2.setTX(TXD2);
  Serial2.setFIFOSize(2048); // Increased to match ESP32 RX buffer size (2048) 
  Serial2.begin(230400);
  delay(300);
  Serial.begin(230400);
  delay(300);
  Serial.printf("Configuring SDA Pin: %d, SCL Pin: %d\n", LCD_SDA, LCD_SCL);
  // 1. Assign pins to Wire1 (GPIO 14/15 belong strictly to I2C1/Wire1)
  bool sdaValid = Wire1.setSDA(LCD_SDA);
  bool sclValid = Wire1.setSCL(LCD_SCL);
  Wire1.begin(); 
  Wire1.setTimeout(250); 
  scanI2CBus();
  //EEPROM.begin(255); 
   // delay(300);
  //analogReadResolution(12);
  //delay(300);
  /*pinMode(Menu, INPUT);
  pinMode(Enter, INPUT);
  pinMode(F1, INPUT);
  pinMode(F2, INPUT);
  pinMode(Up, INPUT);
  pinMode(Down, INPUT);
  pinMode(Left, INPUT);
  pinMode(Right, INPUT);*/
 //  delay(100);
  //lcd test...
  // lcd.begin(20, 4);  
   lcd.begin(20, 4, Wire1); 
    lcd.setBacklight(255); 
  lcd.clear();           
  lcd.setCursor(0, 0);   
  lcd.print("hari");
  delay(100);
  lcd.createChar(0, customChar); 
  lcd.createChar(1, Check);
  delay(100);
  lcd.setBacklight(255);
  delay(100);
  lcd.clear();
  lcd.setCursor(3, 1);
  lcd.print("TilT ELECTRONICS");
  lcd.setCursor(5, 2);
  lcd.print("BANGALORE");  
  delay(2000);    
  tempg1 = 0;
  lastpickno1 = 0;
  foldername1 ="";
  SysNo[0]='0';
  SysNo[1]='0';
  SysNo[2]='0';
  SysNo[3]='0';
  SysNo[4]='0';
  SysNo[5]='0';
  SysNo[6]='0';
  SysNo[7]='0';    
  totaldata = ((nc * nr) + 4);
 // root3 = SD.open("/");  
  setup11 = 0;  
  totalfile = 0;
  totalfolder = 0;
  inbufferclear();
  //readfolderlist(root3);    
  //setupoffolder();        
  usbhostsetup();
  communicationchecking();      
  delay(500);  
  masterinit();
  delay(10);  
  masterverify();
  delay(100);  
}

void loop() 
 { 
     Serial.println("--- Loop Start ---");
     runningdesign();
     Serial.println("Exited: runningdesign() successfully.");
     delay(100);  
     Serial1.println("Entering: mainmenu()...");
     mainmenu();    
     Serial1.println("Exited: mainmenu() successfully.");
  
  }
void usbhostsetup() {
  Serial1.begin(230400);
  Serial1.println("TinyUSB Host: Device Info Example");
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  delay(5000);
  digitalWrite(ledPin, LOW);
  USBHost.begin(0);
}

void usbhostloop() {
  USBHost.task();
  Serial1.flush();
}
 //--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+
void print_device_descriptor(tuh_xfer_t *xfer);

void utf16_to_utf8(uint16_t *temp_buf, size_t buf_len);

void print_lsusb(void) {
  bool no_device = true;

  for (uint8_t daddr = 1; daddr < CFG_TUH_DEVICE_MAX + 1; daddr++) {
    // TODO can use tuh_mounted(daddr), but tinyusb has an bug
    // use local connected flag instead
    dev_info_t *dev = &dev_info[daddr - 1];

    if (dev->mounted) {
      Serial1.printf("Device %u: ID %04x:%04x %s %s\r\n", daddr,
                    dev->desc_device.idVendor, dev->desc_device.idProduct,
                    (char *) dev->manufacturer, (char *) dev->product);

      no_device = false;
    }
  }

  if (no_device) {
    Serial1.println("No device connected (except hub)");
  }
}

void duplicateFile(const char *sourceFile, const char *destFile) {
    Serial1.printf("Duplicating file: %s -> %s\n", sourceFile, destFile);

    File32 src = fatfs.open(sourceFile, O_RDONLY);
    if (!src) {
        Serial1.println("Error: Failed to open source file.");
        return;
    }

    File32 dest = fatfs.open(destFile, O_RDWR | O_CREAT | O_TRUNC);
    if (!dest) {
        Serial1.println("Error: Failed to create destination file.");
        src.close();
        return;
    }

    char buffer[512]={0};  // Use a larger buffer for efficiency
    int bytesRead;
    long long file_write_size = 0;

    while ((bytesRead = src.read(buffer, sizeof(buffer))) > 0) {
      file_write_size += bytesRead;
      if (dest.write(buffer, bytesRead) != bytesRead) {
        Serial1.println("Error: Write failed.");
        break;
      }
    }

    Serial1.printf("file_write_size:%ld\n",file_write_size);
    // Ensure data is fully written to storage
    dest.flush();

    Serial1.println("File duplication completed successfully.");
    
    src.close();
    dest.close();
}
void readTxtFile(const char *filename) {
  
  if(!filename)
    return;
  else
  {
    Serial1.printf("Reading file: %s\n", filename);

    File32 file = fatfs.open(filename, O_RDONLY); // Open file in read-only mode
    if (!file) {
        Serial1.println("Failed to open file.");
        return;
    }

    char buffer[128]={0}; // Buffer to store file content
    while (file.available()) {
        int bytesRead = file.read(buffer, sizeof(buffer) - 1);
        buffer[bytesRead] = '\0';  // Null-terminate the string
        Serial1.print(buffer);     // Print file content
    }
    
    Serial1.println();  // Print newline after file content
    file.close();       // Close the file after reading
    Serial1.println("File read completed.");
  }
}

void listTxtFiles(const char *dirname) {
    Serial1.printf("Listing .txt files in: %s\n", dirname);
    bool isTxt = false;

    File32 dir = fatfs.open(dirname);  // Use File32 instead of FsFile
    if (!dir) {
        Serial1.println("Failed to open directory.");
        return;
    }

    char filename[64]={0}; // Buffer to store file name
    while (true) {
        File32 entry = dir.openNextFile();
        if (!entry) break;
        entry.getName(filename, sizeof(filename)); // Use getName() to retrieve the file name
        // Check if the file extension is ".txt"
        if (strstr(filename, ".txt")) {
          isTxt = true;
          Serial1.printf("Filename: %s - Size: %llu\n", filename, entry.size());            
          //Print the file content in the Serial1
          readTxtFile(filename);
          //Create/Duplicate the file
          duplicateFile(filename,"sample.txt");
        }
        if(isTxt == false)
        {
          Serial1.println("There is no *.txt files");
        }
        entry.close();
    }

    dir.close();
} 
// Invoked when device is mounted (configured)
void tuh_mount_cb(uint8_t daddr) {
  Serial1.printf("Device attached, address = %d\r\n", daddr);

  dev_info_t *dev = &dev_info[daddr - 1];
  dev->mounted = true;
  // Set the LED pin as an OUTPUT
  digitalWrite(ledPin, HIGH);
  tuh_descriptor_get_device(daddr, &dev->desc_device, 18, print_device_descriptor, 0);   // Get Device Descriptor
  msc_block_dev.begin(daddr);
  
  msc_block_dev.setActiveLUN(0);  // For simplicity this example only support LUN 0

  is_mounted = fatfs.begin(&msc_block_dev);

   if (is_mounted) {
     usbMountFailed = false;
    //List the directory content once FAT FS mounted
     fatfs.ls(&Serial1, LS_SIZE); 
    //listTxtFiles("/");
   }else {
//     usbMountFailed = true;   // TC-1.4: device present, filesystem unreadable
  }

}
/// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr) {
  Serial1.printf("Device removed, address = %d\r\n", daddr);
  dev_info_t *dev = &dev_info[daddr - 1];
  dev->mounted = false;
  digitalWrite(ledPin, LOW);
  // unmount file system
  is_mounted = false;
  fatfs.end();
  // end block device
  msc_block_dev.end();
  // print device summary
  print_lsusb();
}
void print_device_descriptor(tuh_xfer_t *xfer) {
  if (XFER_RESULT_SUCCESS != xfer->result) {
    Serial1.printf("Failed to get device descriptor\r\n");
    return;
  }

  uint8_t const daddr = xfer->daddr;
  dev_info_t *dev = &dev_info[daddr - 1];
  tusb_desc_device_t *desc = &dev->desc_device;

  Serial1.printf("Device %u: ID %04x:%04x\r\n", daddr, desc->idVendor, desc->idProduct);
  Serial1.printf("Device Descriptor:\r\n");
  Serial1.printf("  bLength             %u\r\n"     , desc->bLength);
  Serial1.printf("  bDescriptorType     %u\r\n"     , desc->bDescriptorType);
  Serial1.printf("  bcdUSB              %04x\r\n"   , desc->bcdUSB);
  Serial1.printf("  bDeviceClass        %u\r\n"     , desc->bDeviceClass);
  Serial1.printf("  bDeviceSubClass     %u\r\n"     , desc->bDeviceSubClass);
  Serial1.printf("  bDeviceProtocol     %u\r\n"     , desc->bDeviceProtocol);
  Serial1.printf("  bMaxPacketSize0     %u\r\n"     , desc->bMaxPacketSize0);
  Serial1.printf("  idVendor            0x%04x\r\n" , desc->idVendor);
  Serial1.printf("  idProduct           0x%04x\r\n" , desc->idProduct);
  Serial1.printf("  bcdDevice           %04x\r\n"   , desc->bcdDevice);

  // Get String descriptor using Sync API
  Serial1.printf("  iManufacturer       %u     ", desc->iManufacturer);
  if (XFER_RESULT_SUCCESS ==
      tuh_descriptor_get_manufacturer_string_sync(daddr, LANGUAGE_ID, dev->manufacturer, sizeof(dev->manufacturer))) {
    utf16_to_utf8(dev->manufacturer, sizeof(dev->manufacturer));
    Serial1.printf((char *) dev->manufacturer);
  }
  Serial1.printf("\r\n");

  Serial1.printf("  iProduct            %u     ", desc->iProduct);
  if (XFER_RESULT_SUCCESS ==
      tuh_descriptor_get_product_string_sync(daddr, LANGUAGE_ID, dev->product, sizeof(dev->product))) {
    utf16_to_utf8(dev->product, sizeof(dev->product));
    Serial1.printf((char *) dev->product);
  }
  Serial1.printf("\r\n");

  Serial1.printf("  iSerialNumber       %u     ", desc->iSerialNumber);
  if (XFER_RESULT_SUCCESS ==
      tuh_descriptor_get_serial_string_sync(daddr, LANGUAGE_ID, dev->serial, sizeof(dev->serial))) {
    utf16_to_utf8(dev->serial, sizeof(dev->serial));
    Serial1.printf((char *) dev->serial);
  }
  Serial1.printf("\r\n");

  Serial1.printf("  bNumConfigurations  %u\r\n", desc->bNumConfigurations);

  // print device summary
  print_lsusb();
}
//--------------------------------------------------------------------+
// String Descriptor Helper
//--------------------------------------------------------------------+
static void _convert_utf16le_to_utf8(const uint16_t *utf16, size_t utf16_len, uint8_t *utf8, size_t utf8_len) {
  // TODO: Check for runover.
  (void) utf8_len;
  // Get the UTF-16 length out of the data itself.

  for (size_t i = 0; i < utf16_len; i++) {
    uint16_t chr = utf16[i];
    if (chr < 0x80) {
      *utf8++ = chr & 0xff;
    } else if (chr < 0x800) {
      *utf8++ = (uint8_t) (0xC0 | (chr >> 6 & 0x1F));
      *utf8++ = (uint8_t) (0x80 | (chr >> 0 & 0x3F));
    } else {
      // TODO: Verify surrogate.
      *utf8++ = (uint8_t) (0xE0 | (chr >> 12 & 0x0F));
      *utf8++ = (uint8_t) (0x80 | (chr >> 6 & 0x3F));
      *utf8++ = (uint8_t) (0x80 | (chr >> 0 & 0x3F));
    }
    // TODO: Handle UTF-16 code points that take two entries.
  }
}

// Count how many bytes a utf-16-le encoded string will take in utf-8.
static int _count_utf8_bytes(const uint16_t *buf, size_t len) {
  size_t total_bytes = 0;
  for (size_t i = 0; i < len; i++) {
    uint16_t chr = buf[i];
    if (chr < 0x80) {
      total_bytes += 1;
    } else if (chr < 0x800) {
      total_bytes += 2;
    } else {
      total_bytes += 3;
    }
    // TODO: Handle UTF-16 code points that take two entries.
  }
  return total_bytes;
}

void utf16_to_utf8(uint16_t *temp_buf, size_t buf_len) {
  size_t utf16_len = ((temp_buf[0] & 0xff) - 2) / sizeof(uint16_t);
  size_t utf8_len = _count_utf8_bytes(temp_buf + 1, utf16_len);

  _convert_utf16le_to_utf8(temp_buf + 1, utf16_len, (uint8_t *) temp_buf, buf_len);
  ((uint8_t *) temp_buf)[utf8_len] = '\0';
}

void usbScanFolders()
{
  usbFolderCount = 0;
  String name;

  File32 dir = fatfs.open("/");
  if (!dir) {
    Serial.println("ERROR: Could not open USB root directory");
    return;
  }

  while (true)
  {
    File32 entry = dir.openNextFile();
    if (!entry) break;

    char nameBuf[64] = {0};
    entry.getName(nameBuf, sizeof(nameBuf));
    name = String(nameBuf);
    name.trim();

    if (!entry.isDirectory()) {
      entry.close();
      continue;
    }

    // Skip junk/system folders (same filter as SD readfolderlist())
    if ((name.indexOf('.') >= 0) || name.startsWith("SYSTEM") || name.startsWith("System")) {
      Serial.print("SKIP (junk): ");
      Serial.println(name);
      entry.close();
      continue;
    }

    if (usbFolderCount < 50) {
      usbFolderList[usbFolderCount] = name;
      usbFolderCount++;
      Serial.print("FOUND FOLDER: ");
      Serial.println(name);
    } else {
      Serial.println("WARNING: usbFolderList full, truncating");
    }

    entry.close();
    delay(20);
  }
  dir.close();

  Serial.print("usbFolderCount = ");
  Serial.println(usbFolderCount);
}

void usbPrintFolderListLCD()
{
  lcd.clear();
  if (usbFolderCount == 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("No folders on USB");
    return;
  }
  lcd.setCursor(0, 0);
  lcd.print("USB Folders: ");
  lcd.print(usbFolderCount);
  for (byte i = 0; i < usbFolderCount && i < 3; i++)
  {
    lcd.setCursor(0, i + 1);
    String disp = usbFolderList[i];
    if (disp.length() > 20) disp = disp.substring(0, 20);
    lcd.print(disp);
  }
}
bool usbWaitForMount()
{
  usbBrowseActive = true;
  usbRemovedDuringBrowse = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("USB Drive");

  unsigned long lastBlink = 0;
  bool blinkOn = true;

  while (true)
  {
    usbhostloop();   // must run every pass or tuh_mount_cb() never fires

    // --- Success: device attached AND filesystem mounted ---
    if (dev_info[0].mounted && is_mounted)
    {
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("USB Drive Ready");
      delay(400);
      usbBrowseActive = false;
      return true;
    }

    // --- TC-1.4: device attached, mount failed (corrupt/unsupported fs) ---
    if (dev_info[0].mounted && usbMountFailed)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("USB Read Error");
      lcd.setCursor(0, 1);
      lcd.print("Unsupported/Corrupt");
      lcd.setCursor(0, 2);
      lcd.print("Remove drive to retry");
      lcd.setCursor(0, 3);
      lcd.print("* = Back");

      // Stay here until either the bad drive is removed (fall back to the
      // normal insert-prompt loop) or the user cancels out entirely.
      while (dev_info[0].mounted)
      {
        usbhostloop();
        customKey = customKeypad.getKey();
        if (customKey == '*')
        {
          usbBrowseActive = false;
          return false;
        }
      }
      // Device was pulled — umount callback already cleared usbMountFailed.
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("USB Drive");
      lastBlink = 0;
      continue;
    }

    // --- Device attached but not yet mounted (brief transitional state) ---
    if (dev_info[0].mounted && !is_mounted && !usbMountFailed)
    {
      lcd.setCursor(0, 1);
      lcd.print("Reading drive...    ");
    }
    // --- TC-1.1: nothing attached — idle blink prompt ---
    else if (millis() - lastBlink > 500)
    {
      lastBlink = millis();
      blinkOn = !blinkOn;
      lcd.setCursor(0, 1);
      lcd.print(blinkOn ? "Insert USB Drive... " : "                    ");
    }

    customKey = customKeypad.getKey();
    if (customKey == '*')
    {
      usbBrowseActive = false;
      return false;
    }
  }
}

bool usbCheckRemoved()
{
  usbhostloop();   // keep servicing the stack even while just polling

  if (usbRemovedDuringBrowse)
  {
    usbRemovedDuringBrowse = false;
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("USB Drive Removed");
    delay(1000);
    return true;
  }
  return false;
}
void usbBrowseFolders()
{
  if (usbFolderCount == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("No folders on USB");
    delay(2000);
    return;
  }

  int ck1 = 0;

ufloop:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("USB Folders ");
  lcd.print(ck1 + 1);
  lcd.print("/");
  lcd.print(usbFolderCount);

  // previous
  if (ck1 == 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("                    ");
  }
  else
  {
    lcd.setCursor(2, 1);
    String prev = usbFolderList[ck1 - 1];
    if (prev.length() > 18) prev = prev.substring(0, 18);
    lcd.print(prev);
  }

  // current (highlighted with '>')
  lcd.setCursor(0, 2);
  lcd.print(">");
  lcd.setCursor(2, 2);
  String cur = usbFolderList[ck1];
  if (cur.length() > 18) cur = cur.substring(0, 18);
  lcd.print(cur);

  // next
  if (ck1 >= usbFolderCount - 1)
  {
    lcd.setCursor(0, 3);
    lcd.print("                    ");
  }
  else
  {
    lcd.setCursor(2, 3);
    String nxt = usbFolderList[ck1 + 1];
    if (nxt.length() > 18) nxt = nxt.substring(0, 18);
    lcd.print(nxt);
  }

ufkey:
  usbhostloop();
  customKey = customKeypad.getKey();

  if (customKey == 'B')      // scroll down
  {
    ck1 += 1;
    if (ck1 >= usbFolderCount) ck1 = usbFolderCount - 1;
    goto ufloop;
  }

  if (customKey == 'A')      // scroll up
  {
    ck1 -= 1;
    if (ck1 < 0) ck1 = 0;
    goto ufloop;
  } 
  if (customKey == 'E')      // open folder -> browse its files
  {
 /*       // --- TEST: send raw USB scan index directly to mainboard ---
    char fbuf[50];
    snprintf(fbuf, sizeof(fbuf), "ABF%d#{}", ck1);
    Serial1.print("RAW OUTBOUND PACKET: ");
    Serial1.println(fbuf); // Prints the exact string "ABF[index]#{}" to your log
    for (int i = 0; fbuf[i] != '\0'; i++) {
      Serial2.write((byte)fbuf[i]);
    }

    Serial2.flush();
    Serial1.print("SENT USB FOLDER INDEX: ");
    Serial1.print(ck1);
    Serial1.print("  Name: ");
    Serial1.println(usbFolderList[ck1]);
    // --- end test send ---    */
    usbScanFilesInFolder(usbFolderList[ck1]);
    usbBrowseFiles();
    goto ufloop;              // back from files -> redraw folder list at same position
  } 
  if (customKey == 'R')      // copy currently highlighted folder to mainboard SD
{
  usbCopyFolder(usbFolderList[ck1]);
  goto ufloop;
}

  if (customKey == '*')      // exit -> back to Editmenu -> back to mainmenu
  {
    return;
  }

  goto ufkey;
}
void usbScanFilesInFolder(String folderName)
{ 
  usbCurrentFolderName = folderName; 
  usbFileCount = 0;
  String name;
  String path = "/" + folderName;

  File32 dir = fatfs.open(path.c_str());
  if (!dir) {
    Serial.print("ERROR: Could not open USB folder: ");
    Serial.println(path);
    return;
  }

  while (true)
  {
    File32 entry = dir.openNextFile();
    if (!entry) break;

    char nameBuf[64] = {0};
    entry.getName(nameBuf, sizeof(nameBuf));
    name = String(nameBuf);
    name.trim();

    if (entry.isDirectory()) {
      entry.close();
      continue;
    }

    if (!(name.endsWith(".BMP") || name.endsWith(".bmp") ||
          name.endsWith(".EJC") || name.endsWith(".ejc"))) {
      entry.close();
      continue;
    }

    if (usbFileCount < 50) {
      usbFileList[usbFileCount] = name;
      usbFileCount++;
      Serial.print("FOUND FILE: ");
      Serial.println(name);
    } else {
      Serial.println("WARNING: usbFileList full, truncating");
    }

    entry.close();
    delay(20);
  }
  dir.close();

  Serial.print("usbFileCount = ");
  Serial.println(usbFileCount);
}
void usbBrowseFiles()
{
  if (usbFileCount == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("No matching files");
    delay(2000);
    return;
  }

  int ck2 = 0;

ufiloop:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("USB Files ");
  lcd.print(ck2 + 1);
  lcd.print("/");
  lcd.print(usbFileCount);

  if (ck2 == 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("                    ");
  }
  else
  {
    lcd.setCursor(2, 1);
    String prev = usbFileList[ck2 - 1];
    if (prev.length() > 18) prev = prev.substring(0, 18);
    lcd.print(prev);
  }

  lcd.setCursor(0, 2);
  lcd.print(">");
  lcd.setCursor(2, 2);
  String cur = usbFileList[ck2];
  if (cur.length() > 18) cur = cur.substring(0, 18);
  lcd.print(cur);

  if (ck2 >= usbFileCount - 1)
  {
    lcd.setCursor(0, 3);
    lcd.print("                    ");
  }
  else
  {
    lcd.setCursor(2, 3);
    String nxt = usbFileList[ck2 + 1];
    if (nxt.length() > 18) nxt = nxt.substring(0, 18);
    lcd.print(nxt);
  }

ufikey:
  customKey = customKeypad.getKey();
    if (customKey == 'R')
  {
    usbCopySingleFile(usbFileList[ck2]);
    goto ufiloop;     // redraw file list at same position
  }

  if (customKey == 'B')
  {
    ck2 += 1;
    if (ck2 >= usbFileCount) ck2 = usbFileCount - 1;
    goto ufiloop;
  }

  if (customKey == 'A')
  {
    ck2 -= 1;
    if (ck2 < 0) ck2 = 0;
    goto ufiloop;
  }

  if (customKey == '*')
  {
    return;   // back to usbBrowseFolders()
  }

  goto ufikey;
}

// ---- Read BMP height field (offset 0x16) from a File32 source ----
unsigned long usbFindHeight(File32 &f)
{
  unsigned long ht = 0;
  f.seekSet(0x16);
  for (int i = 0; i < 4; i++)
  {
    ht += ((unsigned long)f.read() << (8 * i));
  }
  if (ht > 99999) ht = 99999;
  return ht;
}
// Header-ல height படிச்சு, file size-ஐ வெச்சு இந்த file-க்கான
// real totaldata-ஐ derive பண்ணும் (global nc சாராம)
bool usbDeriveTotalData(File32 &f, unsigned long srcSize,
                         unsigned long &heightOut, unsigned int &totalDataOut)
{
  heightOut = usbFindHeight(f);          // 0x16-ல இருந்து actual height படி
  if (heightOut == 0) return false;

  unsigned long payloadBytes = (srcSize > bitmapOffset) ? (srcSize - bitmapOffset) : 0;
  if (payloadBytes % heightOut != 0) {
    Serial1.println("WARNING: payload not exact multiple of height - file may be malformed");
    return false;
  }

  totalDataOut = (unsigned int)(payloadBytes / heightOut);   // exact totaldata இந்த file-க்கு
  return true;
}

// ---- Send 'f1' folder-create packet, wait for D1/D2/D3 ----
// returns: 1=existed, 2=created, 3=mkdir failed on mainboard, 0=no reply/timeout
/*
byte usbSendFolderCreate(String folderName)
{
  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('1');
  Serial2.print('*'); Serial2.print(folderName);
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();
  byte ackbuf[4] = {0};
  byte n = 0;
  unsigned long qo1 = 0;
  while (qo1 < 290000)
  {
    qo1++;
    if (Serial2.available() > 0)
    {
      byte inbyte = (byte)Serial2.read();
      if (n < 4) ackbuf[n] = inbyte;
      n++;
      if (n >= 2 && ackbuf[0] == 68)   // 'D'
      {
        if (ackbuf[1] == 49) return 1;   // '1'
        if (ackbuf[1] == 50) return 2;   // '2'
        if (ackbuf[1] == 51) return 3;   // '3'  <-- mkdir failed
      }
    }
  }
  return 0;   // timeout
} */ // ---- Send 'f5' file-size query, parse numeric reply "ABDz*{size}#&" ----
byte usbSendFolderCreate(String folderName)
{
  // clear any stale incoming bytes so reply parsing stays in sync
  while (Serial2.available() > 0) { Serial2.read(); }

  // Debug: show the exact packet being transmitted
  Serial.print("TX: ABf1*"); Serial.print(folderName); Serial.println("#{}");
  Serial1.print("TX COMMAND: ABf1*"); Serial1.print(folderName); Serial1.println("#{}");

  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('1');
  Serial2.print('*'); Serial2.print(folderName);
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();

  byte state = 0;   // 0=waiting for 'D', 1=waiting for digit
  unsigned long qo1 = 0;
  while (qo1 < 290000)
  {
    qo1++;
    if (Serial2.available() > 0)
    {
      byte inbyte = (byte)Serial2.read();
      // Debug: show incoming bytes while waiting for ack
      Serial.print("RX byte: "); Serial.println(inbyte);
      qo1 = 0;                      // reset timeout on any byte activity
      if (state == 0) {
        if (inbyte == 68) state = 1;   // 'D' — only now start trusting the stream
        // anything else: ignore and keep scanning for 'D'
      } else {
        if (inbyte == 49) { Serial.println("RX: D1 (exists)"); return 1; }    // '1'
        if (inbyte == 50) { Serial.println("RX: D2 (created)"); return 2; }    // '2'
        if (inbyte == 51) { Serial.println("RX: D3 (mkdir failed)"); return 3; }    // '3'
        state = 0;                     // unexpected byte after 'D', resync
      }
    }
  }
  Serial1.println("ERROR: usbSendFolderCreate timeout waiting for D<n>");
  return 0;
}

unsigned long usbCheckFileSize(String fileName)
{
  Serial1.print("TX COMMAND: ABf5*"); Serial1.print(fileName); Serial1.println("#{}");
  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('5');
  Serial2.print('*'); Serial2.print(fileName);
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();

  String resp = "";
  unsigned long qo1 = 0;
  while (qo1 < 290000)
  {
    qo1++;
    if (Serial2.available() > 0)
    {
      resp += (char)Serial2.read();
      if (resp.length() > 60) resp.remove(0, resp.length() - 60);
      int zIdx = resp.indexOf("Dz*");
      if (zIdx != -1)
      {
        int hashIdx = resp.indexOf('#', zIdx);
        if (hashIdx != -1)
        {
          String sizeStr = resp.substring(zIdx + 3, hashIdx);
          sizeStr.trim();
          return (unsigned long)sizeStr.toInt();
        }
      }
    }
  }
  return 0xFFFFFFFF;   // timeout sentinel
}

// Close the incomplete destination file after a card fails all retries.
void usbAbortFileCopy()
{
  Serial1.println("TX COMMAND: ABORT COPY ABf7#{}");
  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('7');
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();
}
// ---- Send 'f2' file-open packet, wait for D1/D2 ----
// returns: 1=existed/recreated, 2=newly created, 0=timeout
/*byte usbSendFileOpen(String fileName)
{
  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('2');
  Serial2.print('*'); Serial2.print(fileName);
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();

  byte ackbuf[4] = {0};
  byte n = 0;
  unsigned long qo1 = 0;
  while (qo1 < 290000)
  {
    qo1++;
    if (Serial2.available() > 0)
    {
      byte inbyte = (byte)Serial2.read();
      if (n < 4) ackbuf[n] = inbyte;
      n++;
      if (n >= 2 && ackbuf[0] == 68)
      {
        if (ackbuf[1] == 49) return 1;
        if (ackbuf[1] == 50) return 2;
      }
    }
  }
  return 0;
} */
byte usbSendFileOpen(String fileName)
{  /*
  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('2');
  Serial2.print('*'); Serial2.print(fileName);
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();

  byte b0 = 0, b1 = 0;
  unsigned long qo1 = 0;
  while (qo1 < 290000)
  {
    qo1++;
    if (Serial2.available() > 0)
    {
      byte inbyte = (byte)Serial2.read();
      b0 = b1;
      b1 = inbyte;
      if (b0 == 68)   // 'D'
      {
        if (b1 == 49) return 1;
        if (b1 == 50) return 2;
      }
    }
  } */
  while (Serial2.available() > 0) { Serial2.read(); }

// Build packet atomically
char packet[256];
snprintf(packet, sizeof(packet), "ABf2*%s#{}", fileName.c_str());
Serial1.print("TX COMMAND: "); Serial1.println(packet);

// Send in a single write loop then flush
for (int i = 0; packet[i] != '\0'; i++) Serial2.write((byte)packet[i]);
Serial2.flush();

// Wait for D<n> reply (robust)
byte b0 = 0, b1 = 0;
unsigned long start = millis();
while (millis() - start < 3000) { // 3s timeout
if (Serial2.available() > 0) {
byte inb = (byte)Serial2.read();
b0 = b1; b1 = inb;
if (b0 == 'D') {
if (b1 == '1') return 1;
if (b1 == '2') return 2;
}
}
}
Serial.println("ERROR: usbSendFileOpen timeout");
return 0;
}
// ---- Send 'f4' file-exists check packet (READ-ONLY), wait for D1/D2 ----
// returns: 1=exists, 2=doesn't exist, 0=timeout
byte usbCheckFileExists(String fileName)
{
  // clear any stale bytes to avoid misinterpreting prior replies
  while (Serial2.available() > 0) { Serial2.read(); }

  // Debug: show outgoing packet
  Serial.print("TX: ABf4*"); Serial.print(fileName); Serial.println("#{}");
  Serial1.print("TX COMMAND: ABf4*"); Serial1.print(fileName); Serial1.println("#{}");

  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('4');
  Serial2.print('*'); Serial2.print(fileName);
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();
  
  byte b0 = 0, b1 = 0;
  unsigned long qo1 = 0;
  while (qo1 < 290000)
  {
    qo1++;
    if (Serial2.available() > 0)
    {
      byte inbyte = (byte)Serial2.read();
      // Debug: log the reply bytes received
      Serial.print("RX byte: "); Serial.println(inbyte);
      b0 = b1;
      b1 = inbyte;
      if (b0 == 68)   // 'D'
      {
        if (b1 == 49) { Serial.println("RX: D1 (exists)"); return 1; }   // exists
        if (b1 == 50) { Serial.println("RX: D2 (doesn't exist)"); return 2; }   // doesn't exist
      }
    }
  }

  Serial.println("ERROR: usbCheckFileExists timeout waiting for D<n>");
  return 0;   // timeout
}
// ---- Copy one BMP/EJC file from USB into folderName on mainboard SD ----
bool usbCopyOneFile(String folderName, String fileName)
{
  String path = "/" + folderName + "/" + fileName;
  Serial1.print("COPY FILE START: "); Serial1.println(path);
  usbSrcFile = fatfs.open(path.c_str(), O_RDONLY);
  if (!usbSrcFile)
  {
    Serial1.print("ERROR: cannot open USB file: ");
    Serial1.println(path);
    return false;
  }
  unsigned long srcSize = usbSrcFile.size();
  Serial1.print("SENDING FILE: ");
  Serial1.print(fileName);
  Serial1.print("  SIZE: ");
  Serial1.print(srcSize);
  Serial1.println(" bytes");

  byte fr = usbSendFileOpen(fileName);
  if (fr == 0)
  {
    Serial1.println("ERROR: no reply on file-open");
    usbSrcFile.close();
    return false;
  }
  // If the mainboard reports the file already exists, do NOT overwrite or delete it.
  // Treat this as a skipped file (successfully ignored) to match "never replace" policy.
  if (fr == 1)
  {
    Serial1.println("File exists on mainboard - skipping (no overwrite)");
    lcd.clear(); lcd.setCursor(0,1); lcd.print("File exists - Skip");
    delay(1000);
    usbSrcFile.close();
    return true; // Skipped is considered OK (not a failure)
  }

  // Track bytes actually sent over Serial2 for diagnostics
  unsigned long bytesSent = 0;

  // If mainboard created a fresh file (D2), announce the exact source size so receiver can bound writes
  if (fr == 2) {
    char pkt[64];
    snprintf(pkt, sizeof(pkt), "ABf6*%lu#{}", srcSize);
    // send atomically
    for (int i = 0; pkt[i] != '\0'; i++) Serial2.write((byte)pkt[i]);
    Serial2.flush();
    Serial1.print("TX: ABf6*"); Serial1.print(srcSize); Serial1.println("#{}");
    delay(10);
  }

 /* unsigned long height;
  if (srcSize > bitmapOffset) {
    unsigned long payloadBytes = (srcSize > bitmapOffset) ? (srcSize - bitmapOffset) : 0UL;
    height = (payloadBytes + totaldata - 1) / totaldata; // number of cards needed to cover payload
  } else {
    height = 0;
  }  
  if (height == 0)
  {
    Serial1.println("ERROR: bad/zero height, aborting file");
    usbSrcFile.close();
    return false;
  } */
      unsigned long height;
    unsigned int fileTotalData;
    if (!usbDeriveTotalData(usbSrcFile, srcSize, height, fileTotalData))
    {
      Serial1.println("ERROR: could not derive height/totaldata from file header");
      usbSrcFile.close();
      return false;
    }
    Serial1.print("Derived totaldata="); Serial1.print(fileTotalData);
    Serial1.print("  height="); Serial1.println(height);
  lcd.setCursor(0, 1);
  lcd.print("                    ");
  lcd.setCursor(0, 1);
  String disp = fileName;
  if (disp.length() > 20) disp = disp.substring(0, 20);
  lcd.print(disp);

  // --- offset (62 bytes) ---
  Serial2.print('A'); Serial2.print('B'); Serial2.print('D'); Serial2.print('o');
  // Send the first bitmapOffset bytes starting at the very beginning of the file
  usbSrcFile.seekSet(0);
  for (int m = 0; m < bitmapOffset; m++)
  {
    Serial2.write((byte)usbSrcFile.read());
    bytesSent++;
    delayMicroseconds(50);
  }
  Serial2.print('{'); Serial2.print('}');
  Serial2.flush();
  Serial1.print("DEBUG: bytesSent after header="); Serial1.println(bytesSent);

  {
    byte ackbuf[4] = {0};
    byte n = 0;
    unsigned long qo1 = 0;
    bool ok = false;
    while (qo1 < 190000)
    {
      qo1++;
      if (Serial2.available() > 0)
      {
        byte inbyte = (byte)Serial2.read();
        if (n < 4) ackbuf[n] = inbyte;
        n++;
        if (n >= 2 && ackbuf[0] == 68 && ackbuf[1] == 49) { ok = true; break; }
      }
    }
    if (!ok)
    {
      Serial1.println("ERROR: offset ack timeout");
      usbSrcFile.close();
      return false;
    }
  }

  // --- per-card loop ---
  unsigned long ttotal = 0;
  bool ok = true;

  while (ttotal < height)
  {
    ttotal++;

    if (usbCheckRemoved()) { ok = false; break; }

    float tf = ((float)ttotal / (float)height) * 100.0;
    lcd.setCursor(0, 2);
    lcd.print("                    ");
    lcd.setCursor(0, 2);
    lcd.print((int)tf); lcd.print("% "); lcd.print(ttotal); lcd.print("/"); lcd.print(height);

    bool cardAcked = false;
    byte retries = 0;

    // Read the card once before the retry loop. Retries must resend the exact
    // same payload; counting bytes again during a retry would shorten it.
    usbSrcFile.seekSet(((ttotal - 1) * fileTotalData) + bitmapOffset);
    unsigned long bytesRemaining = srcSize - (bitmapOffset + ((ttotal - 1) * fileTotalData));
    size_t toSend = (bytesRemaining > (unsigned long)fileTotalData) ? fileTotalData : (size_t)bytesRemaining;
    uint8_t pickBuf[600];
    for (size_t m = 0; m < toSend; m++)
    {
      pickBuf[m] = (byte)usbSrcFile.read();
      bytesSent++;
      delayMicroseconds(10);
    }
    uint16_t crc = crc16_ccitt(pickBuf, toSend);

    while (!cardAcked && retries < 5)
    {
      if (usbCheckRemoved()) { ok = false; break; }

      // tag packet
      Serial2.print('A'); Serial2.print('B'); Serial2.print('c'); Serial2.print('t');
      Serial2.print('*'); Serial2.print(ttotal);
      Serial2.print('*'); Serial2.print(fileTotalData);
      Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
      delay(1);

      // card data packet
      Serial2.print('A'); Serial2.print('B'); Serial2.print('D');
      Serial2.print((ttotal == height) ? 's' : 'e');

      // Copy the clean card into a transmit buffer for this attempt.
      uint8_t txBuf[600];
      for (size_t m = 0; m < toSend; m++) txBuf[m] = pickBuf[m];

    #if CDC_FLIP_TEST_BYTE > 0
      // Flip only the selected payload byte on the first attempt. The CRC was
      // calculated from the clean buffer, so the receiver must reject it.
      if (retries == 0 && toSend >= CDC_FLIP_TEST_BYTE)
      {
        txBuf[CDC_FLIP_TEST_BYTE - 1] ^= 0xFF;
        Serial1.print("CDC TEST: flipped payload byte ");
        Serial1.println(CDC_FLIP_TEST_BYTE);
      }
    #endif

      // Send the clean payload followed by its two-byte CRC.
for (size_t m = 0; m < toSend; m++) Serial2.write(txBuf[m]);
Serial2.write((byte)(crc >> 8));
Serial2.write((byte)(crc & 0xFF));

Serial2.print('#'); Serial2.print('&'); Serial2.print('}');
Serial2.flush();
Serial1.print("DEBUG: bytesSent after sending card "); Serial1.print(ttotal); Serial1.print(" = "); Serial1.println(bytesSent);
Serial1.print("DEBUG: card "); Serial1.print(ttotal); Serial1.print(" CRC sent = 0x"); Serial1.println(crc, HEX);
Serial1.print("TX CARD: card="); Serial1.print(ttotal);
Serial1.print(" payloadBytes="); Serial1.print(toSend);
Serial1.print(" attempt="); Serial1.println(retries + 1);

      // wait ack
      byte ackbuf[4] = {0};
      byte n = 0;
      unsigned long qo1 = 0;
      //while (qo1 < 90000)
      //{
        //qo1++;
        unsigned long ackStart = millis();
          while (millis() - ackStart < 500)   // 500ms fixed wait, SD write delay-க்கு போதுமான buffer
        {
        if (Serial2.available() > 0)
        {
          byte inbyte = (byte)Serial2.read();
          if (n < 4) ackbuf[n] = inbyte;
          n++;
          if (n >= 2 && ackbuf[0] == 68)
          {
            if (ackbuf[1] == 49)
            {
              cardAcked = true;
              Serial1.print("RX ACK D1 for card "); Serial1.println(ttotal);
            }
            else if (ackbuf[1] == 48)
            {
              Serial1.print("RX NACK D0 for card "); Serial1.println(ttotal);
            }
            break;
          }
        }
        delayMicroseconds(500);
      }

      if (!cardAcked)
      {
        retries++;
        Serial1.print("Retrying card "); Serial1.print(ttotal);
        Serial1.print(" (attempt "); Serial1.print(retries + 1);
        Serial1.println(")");
      }

    }

    if (!cardAcked)
    {
      Serial1.print("CRC FAILED: file="); Serial1.print(fileName);
      Serial1.print(" card="); Serial1.print(ttotal);
      Serial1.print(" after "); Serial1.print(retries);
      Serial1.println(" retries; failed payload bit is not identified");
      Serial1.print("ERROR: card "); Serial1.print(ttotal); Serial1.println(" failed after retries");
      Serial1.println("COPY FILE ABORTED: closing incomplete destination and moving to next file");
      usbAbortFileCopy();
      ok = false;
      break;
    }
  }

  usbSrcFile.close();
  

  if (ok)
  {
    unsigned long recvSize = usbCheckFileSize(fileName);
    Serial1.print("RECEIVED FILE: ");
    Serial1.print(fileName);
    Serial1.print("  SIZE: ");
    Serial1.print(recvSize);
    Serial1.println(" bytes");

    Serial1.print("DEBUG: bytesSent reported by sender = "); Serial1.println(bytesSent);

    if (recvSize == srcSize)
    {
      Serial1.println("SIZE MATCH - no data loss");
    }
    else
    {
      Serial1.println("SIZE MISMATCH - possible data loss!");
      ok = false;   // so OK/FAILED counters in usbCopyFolder() reflect it too
    }
  }

  Serial1.print("COPY FILE RESULT: ");
  Serial1.println(ok ? "SUCCESS" : "FAILED");

  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print(ok ? "File Copied" : "File FAILED");
  delay(400);

  return ok;

}
// ---- Let user pick a destination folder from the mainboard's known folder list ----
bool usbSelectDestinationFolder(String &destName)
{
  if (totalfolder_m == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 1); lcd.print("No Folders Known");
    lcd.setCursor(0, 2); lcd.print("Refresh First");
    delay(2000);
    return false;
  }

  int ck3 = 0;

udloop:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Copy To ");
  lcd.print(ck3 + 1);
  lcd.print("/");
  lcd.print(totalfolder_m);

  if (ck3 == 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("                    ");
  }
  else
  {
    lcd.setCursor(2, 1);
    String prev = folderlist_m[ck3 - 1];
    if (prev.length() > 18) prev = prev.substring(0, 18);
    lcd.print(prev);
  }

  lcd.setCursor(0, 2);
  lcd.print(">");
  lcd.setCursor(2, 2);
  String cur = folderlist_m[ck3];
  if (cur.length() > 18) cur = cur.substring(0, 18);
  lcd.print(cur);

  if (ck3 >= totalfolder_m - 1)
  {
    lcd.setCursor(0, 3);
    lcd.print("                    ");
  }
  else
  {
    lcd.setCursor(2, 3);
    String nxt = folderlist_m[ck3 + 1];
    if (nxt.length() > 18) nxt = nxt.substring(0, 18);
    lcd.print(nxt);
  }

udkey:
  customKey = customKeypad.getKey();

  if (customKey == 'B')
  {
    ck3 += 1;
    if (ck3 >= totalfolder_m) ck3 = totalfolder_m - 1;
    goto udloop;
  }
  if (customKey == 'A')
  {
    ck3 -= 1;
    if (ck3 < 0) ck3 = 0;
    goto udloop;
  }
  if (customKey == 'E')
  {
    destName = folderlist_m[ck3];
    destName.trim();
    return true;
  }
  if (customKey == '*')
  {
    return false;
  }
  goto udkey;
}
// ---- Copy the currently-browsed USB file to a user-chosen destination folder on mainboard SD ----
void usbCopySingleFile(String sourceFileName)
{
  String destFolder;
  if (!usbSelectDestinationFolder(destFolder))
  {
    return;   // cancelled -> back to file browse
  }

  // Debug: show selected destination and source file
  Serial.print("SingleFile: source="); Serial.print(sourceFileName);
  Serial.print("  dest="); Serial.println(destFolder);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Copy To:");
  lcd.setCursor(0, 1);
  String disp = destFolder;
  if (disp.length() > 20) disp = disp.substring(0, 20);
  lcd.print(disp);
  delay(500);

  // Sets copyfoldername on mainboard to destFolder. It already exists there,
  // so this returns D1 (existed) and does NOT mkdir -- safe.
  Serial.print("Requesting folder on mainboard: ABf1*"); Serial.print(destFolder); Serial.println("#{}");
  byte fr = usbSendFolderCreate(destFolder);
  if (fr == 0)
  {
    lcd.clear(); lcd.setCursor(0, 1); lcd.print("No Reply From Main");
    Serial.println("ERROR: usbSendFolderCreate returned 0");
    delay(2000);
    return;
  }
  if (fr == 3)
  {
    lcd.clear();
    lcd.setCursor(0, 1); lcd.print("Folder Error on");
    lcd.setCursor(0, 2); lcd.print("Mainboard SD Card");
    Serial.println("ERROR: usbSendFolderCreate returned 3 (mkdir failed)");
    delay(3000);
    return;
  }
   // ---- NEW: check destination file existence before touching anything ----
  Serial.print("Checking existence on mainboard: ABf4*"); Serial.print(sourceFileName); Serial.println("#{}");
  byte existsCheck = usbCheckFileExists(sourceFileName);
  if (existsCheck == 0)
  {
    lcd.clear(); lcd.setCursor(0, 1); lcd.print("No Reply From Main");
    Serial.println("ERROR: usbCheckFileExists returned 0 (timeout)");
    delay(2000);
    return;
  }
  if (existsCheck == 1)
  {
    lcd.clear();
    lcd.setCursor(0, 1); lcd.print("File Exists");
    lcd.setCursor(0, 2); lcd.print("Copy Skipped");
    Serial.println("INFO: File exists on mainboard, skipping copy");
    delay(2000);
    return;
  }
  // existsCheck == 2 -> doesn't exist, safe to proceed

  // Read from usbCurrentFolderName (source, on USB) -- write to destFolder
  // (set via copyfoldername above, on mainboard)
  Serial.print("Starting usbCopyOneFile for "); Serial.println(sourceFileName);
  bool ok = usbCopyOneFile(usbCurrentFolderName, sourceFileName);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Single File Copy");
  lcd.setCursor(0, 1); lcd.print(ok ? "Completed OK" : "FAILED");
  Serial.print("usbCopyOneFile result: "); Serial.println(ok ? "OK" : "FAILED");
  delay(2000);
}
// ---- Ask mainboard to rescan its SD folders and resend the updated list ----
bool usbRefreshMainboardFolderList()
{
  Serial1.println("Requesting mainboard folder-list refresh...");

  while (Serial2.available() > 0) { Serial2.read(); }  // clear stale bytes

  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('3');
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();

  d = 0;
  unsigned long startWait = millis();
  while (millis() - startWait < 5000)
  {
    if (Serial2.available() > 0)
    {
      serialinput();
      if (d == 1)
      {
        d = 0;
        Serial1.println("Mainboard folder list refreshed.");
        return true;
      }
      startWait = millis();   // keep resetting while packets are still arriving
    }
  }
  Serial1.println("ERROR: timeout waiting for folder-list refresh");
  return false;
}
// ---- Copy an entire USB folder into mainboard SD ----
bool usbCopyFolder(String folderName)
{
  Serial1.print("COPY FOLDER START: /"); Serial1.println(folderName);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Copy Folder:");
  lcd.setCursor(0, 1);
  String disp = folderName;
  if (disp.length() > 20) disp = disp.substring(0, 20);
  lcd.print(disp);
  delay(500);

  byte fr = usbSendFolderCreate(folderName);
  if (fr == 0)
  {
    lcd.clear(); lcd.setCursor(0, 1); lcd.print("No Reply From Main");
    delay(2000);
    return false;
  }
  if (fr == 1)
 {
   lcd.clear();
   lcd.setCursor(0, 1); lcd.print("Folder Already");
   lcd.setCursor(0, 2); lcd.print("Exists - Skip");
   delay(2000);
   return false;
 }
  if (fr == 3)
  {
    lcd.clear();
    lcd.setCursor(0, 1); lcd.print("Mkdir Failed on");
    lcd.setCursor(0, 2); lcd.print("Mainboard SD Card");
    delay(3000);
    return false;
  }
  // fr == 1 (existed) or 2 (created) -> proceed

  usbScanFilesInFolder(folderName);   // fills usbFileList[]/usbFileCount for THIS folder
  Serial1.print("COPY FOLDER FILE COUNT: "); Serial1.println(usbFileCount);

  if (usbFileCount == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 1); lcd.print("No Files To Copy");
    delay(2000);
    return false;
  }

  byte copiedOk = 0, copiedFail = 0;

  for (byte i = 0; i < usbFileCount; i++)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("File "); lcd.print(i + 1); lcd.print("/"); lcd.print(usbFileCount);

    if (usbCheckRemoved())
    {
      lcd.setCursor(0, 3);
      lcd.print("Copy Aborted");
      delay(2000);
      break;
    }

    bool r = usbCopyOneFile(folderName, usbFileList[i]);
    if (r) copiedOk++; else copiedFail++;
    delay(300);
  }

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Folder Copy Done");
  lcd.setCursor(0, 1); lcd.print("OK: "); lcd.print(copiedOk);
  lcd.setCursor(0, 2); lcd.print("Failed: "); lcd.print(copiedFail);
  delay(3000);

    lcd.clear();
  lcd.setCursor(0, 1); lcd.print("Updating Folder List");
  usbRefreshMainboardFolderList();

  return (copiedFail == 0);
}
void scanI2CBus() {
  byte error, address;
  int deviceCount = 0;

  Serial.println("\nScanning Wire bus addresses (0x01 - 0x7F)...");
  Serial.println("--------------------------------------------");

  for (address = 1; address < 127; address++) {
    // Begin communication sequence with the target address
    Wire1.beginTransmission(address);
    
    // Check if the device acknowledges the address packet
    error = Wire1.endTransmission();

    if (error == 0) {
      Serial.print("SUCCESS: Device acknowledged at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      deviceCount++;
    } 
    else if (error == 4) {
      Serial.print("BUS ERROR: Unknown hardware fault at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  Serial.println("--------------------------------------------");
  if (deviceCount == 0) {
    Serial.println("Scan finished: No active I2C devices discovered.");
    Serial.println("Tip: Check external 4.7k ohm pull-up resistors on GPIO 14 & 15.");
  } else {
    Serial.printf("Scan finished: Discovered %d active device(s).\n\n", deviceCount);
  }
}
void inbufferclear()
{
  for (int r = 0; r < (int)sizeof(data3); r++) // clear data3 buffer
  {
    data3[r] = 0;
  }
}

void setupoffolder()
{   
ii12:
    
  kk1 = kk1 % 48;  
  if (kk1 > 12)
  {
   s_name = filelist[0];
   s_name.trim();
   kk1 = s_name.length();
   delay(10); 
   //rtc.writenvram(45, kk1);             
   delay(10);
   //writenvram1(s_name,0,kk1);                 
   EEPROM.write(11, (byte)0);
   EEPROM.commit();
  }
  else
  {
   //s_name =readnvram1(0,kk1);   
   for (int m = 0; m <= totalfile; m++)
   {
    if (filelist[m].equalsIgnoreCase(s_name))
    {
     temp = m;
     EEPROM.write(11, temp);
     EEPROM.commit();
     goto ii1; 
    }    
   }
  } 
  s_name = filelist[0];
  s_name.trim();
  kk1 = s_name.length();
  delay(10); 
  //rtc.writenvram(45, kk1);             
  delay(10);
  //writenvram1(s_name,0,kk1);              
  EEPROM.write(11, 0); 
  EEPROM.commit();  
ii1: 
  delay(10);  
  //kk1 = rtc.readnvram(46);  
  delay(10); 
  kk1 = kk1 % 48;
  if (kk1 > 12)
  {
   s_name = filelist[0];
   s_name.trim();
   kk1 = s_name.length();
   delay(10); 
   //rtc.writenvram(46, kk1);             
   delay(10);
   //writenvram1(s_name,33,kk1);              
   EEPROM.write(32, 0);
   EEPROM.commit();  
  }
  else
  {
   //s_name =readnvram1(33,kk1);
   s_name.trim();   
   for (int m = 0; m <= totalfile; m++)
   {
    if (filelist[m].equalsIgnoreCase(s_name))
    {
     temp = m;     
     EEPROM.write(32, temp);
     EEPROM.commit();
     goto ii2; 
    }    
   }  
  }
  s_name = filelist[0];
  s_name.trim();
  kk1 = s_name.length();
  delay(10); 
  //rtc.writenvram(46, kk1);             
  delay(10);
  //writenvram1(s_name,33,kk1);              
  EEPROM.write(32, 0);
  EEPROM.commit();  
ii2: 
  temp = temp;
}

void masterverify()
{
 lcdcnt = 0;
 lcd.clear();
 while(true)
 {
  if (Serial2.available() > 0)
  {     
   serialinput(); 
   //Serial.print("masterverifyed\n");
      delay(50);  
   if (d == 1)
   {
    d = 0;                  
    return;
   }                
  }               
 }  
}

void(* resetFunc) (void) = 0;

void readfolderlist(File root2)
{ 
  s_name = ""; 
  Serial.print("folderread"); 
  Serial.println('\t');
  Serial.println(totalfolder);
  while (true)
  {
    bMap3 = root2.openNextFile();    
    if (!bMap3)
    {
      if (totalfolder != 0)
      totalfolder = totalfolder ;  //-1
      root2.rewindDirectory();  
      bMap3.close();      
      Serial.println('\t');
      Serial.write(totalfolder);
      return;
    }
    else
    {    
      s_name = bMap3.name();
      s_name.trim();       
      if (((s_name.indexOf('.') >= 0) || (s_name.startsWith("SYSTEM"))) || (s_name.startsWith("System")))
      {
       
      }
      else
      {
       folderlist[totalfolder] = s_name;
       folderlist[totalfolder].trim();
       totalfolder += 1;
       Serial.println('\t');
       Serial.print(totalfolder); 
       Serial.print(s_name);             
      }
    }
    delay(20);
  }
  if (totalfolder < 1)
  {
   lcd.clear();
   lcd.setCursor(3, 1);
   lcd.print("Folder Error!");
   lcd.setCursor(0, 2);
   lcd.print("Keep 2 Folders");
   lcd.setCursor(10, 3);
   lcd.print("In SDCard");   
   delay(2000); 
  }          
}

void opennext(File root1)
{ 
  s_name = ""; 
  while (true)
  {
    bMap = root1.openNextFile();    
    if (!bMap)
    {
      if (totalfile != 0)
      //totalfile = totalfile - 1;
      //root1.rewindDirectory();  
      //bMap.close();      
      return;
    }
    else
    {    
      s_name = bMap.name();
      s_name.trim();
      if ((s_name.endsWith(".BMP")) || (s_name.endsWith(".EJC")))
      {
       filelist[totalfile] = s_name;
       filelist[totalfile].trim();
       totalfile += 1;  
      }       
    }
    delay(20);
  }     
}
long FindHeight(byte f)
{
  unsigned long ht = 0;
  if (f == 1)
  {    
    bMap.seek(0x16);
    delay(1);
    ht = 0;
    for (int i = 0; i < 4; i++)
    {
      ht += (bMap.read() << (8 * i));
      delay(1);
    }    
  }  
  else
  {    
    bMap1.seek(0x16);
    delay(1);
    ht = 0;
    for (int i = 0; i < 4; i++)
    {
      ht += (bMap1.read() << (8 * i));
      delay(1);
    }    
    delay(1);
  }
  return ht;
}

void folderselection1()
{  
 int ck1 = 0;   
 unsigned long qo1 = 0;
 pulcnt = 0;
 esc = 0;
fs: 
 lcd.clear();  
 lcd.setCursor(0, 0);
 lcd.print("Select Folder<  /  >");   
 lcd.setCursor(0, 1);
 lcd.print("       ");
 s_name = folderlist[ck1];
 lcd.setCursor(0, 1);
 lcd.print(s_name);  
 if (totalfolder > 9)
   {
    lcd.setCursor(17, 0);
    lcd.print("  ");
    lcd.setCursor(17, 0);
   } 
   else
   {
    lcd.setCursor(17, 0);
    lcd.print("  ");
    lcd.setCursor(18, 0);    
   } 
 lcd.print(totalfolder + 1); 
 lcd.setCursor(6, 2);
 lcd.print(">"); 
 
s1loop1s:
   if (ck1 > 9)
   {
    lcd.setCursor(15, 0);
    lcd.print("  ");
    lcd.setCursor(14, 0);
   } 
   else
   {
    lcd.setCursor(15, 0);    
   }
      
   lcd.print(ck1 + 1); 
      
   if (ck1 == 0)
   {
    lcd.setCursor(0, 1);
    lcd.print("                    "); 
    lcd.setCursor(0, 1);
    lcd.print(s_name);
   }
   else
   {
    lcd.setCursor(7, 1);
    lcd.print("             ");
    lcd.setCursor(7, 1);
    lcd.print(folderlist[ck1-1]);
   }
 
  lcd.setCursor(7, 2);
  lcd.print("             ");
  lcd.setCursor(7, 2);
  lcd.print(folderlist[ck1]);
  
  if (ck1 == totalfolder)
   {
    lcd.setCursor(0, 3);
    lcd.print("                    "); 
   }
   else
   {
    lcd.setCursor(7, 3);
    lcd.print("             ");
    lcd.setCursor(7, 3);
    lcd.print(folderlist[ck1+1]);
   }   
sloop1s: 
 char customKey = customKeypad.getKey(); 
  if (customKey == 'B')
   {
    ck1 += 1;
    if (ck1 >= totalfolder)
    {
      ck1 = totalfolder;  
    }
    goto s1loop1s;
   }

  if (customKey == 'A')
   {
      ck1 -= 1;
      if (ck1 <= 0)
      {
        ck1 = 0;  
      }
      goto s1loop1s;
   }
   
   if (customKey == 'E')
   {
    s_name = folderlist[ck1];
    s_name.trim();              
    foldername1 = s_name;   
    Serial2.print('A');
    Serial2.print('B');
    Serial2.print('f');
    Serial2.print('1');
    Serial2.print('*');
    Serial2.print(foldername1);
    Serial2.print('#');
    Serial2.print('{');
    Serial2.print('}');   
    delay(100);
    while (true)
    {      
     if (Serial2.available() > 0)
      {
       byte inbyte = (byte)Serial2.read();          
       data1[temp1] = inbyte;                      
       temp1 = temp1 + 1;   
       if (data1[temp1-2] == 68)   //D 
       {
        if (data1[temp1-1] == 49)    // 1 this comes from main board  - folder is already there by sending back D1  is ack
        {     
         lcd.setCursor(0, 2);
         lcd.print("      ");       
         lcd.setCursor(0, 2);
         lcd.print("Select");                     
         delay(100);   
         qo1 = 0;
         pulcnt = 1;
         return;
        }
        if (data1[temp1-1] == 50)
        {
         lcd.setCursor(0, 2);
         lcd.print("      ");       
         lcd.setCursor(0, 2);
         lcd.print("YES   ");   
         delay(100);
         qo1 = 0;
         pulcnt = 1;
         return;
        }                    
       }
      }                  
      if (qo1 >= 290000)
      {
        qo1 = 0;
        goto fs;
      }
    }            
   }

   if (customKey == '*')
   {
    if (pulcnt == 1)
    {     
     resetFunc(); 
    }
    esc = 1;
    return;
   }   
   goto sloop1s; 
}
void filecopying(unsigned int fnum)
{  
 unsigned long ttotal = 0, qo1 = 0;
 float tf = 0;
 int tk = 0;
 //bMap.close();
 delay(100); 
 //bMap = SD.open(String("/" + foldername1 + "/" + filelist[fnum]), FILE_READ); 
 //bMap = SD.open(String("/" +foldername1 + "/" + filelist[fnum]), FILE_READ); 
 delay(100); 
 height = FindHeight(1);
 delay(100); 
 Serial.print("File Hight:- ");
 Serial.println(height);
fcm: 
 lcd.clear();  
 lcd.setCursor(0, 0);
 lcd.print("File: ");
 lcd.setCursor(6, 0);
 lcd.print(filelist[fnum]);
 lcd.setCursor(4, 1);
 lcd.print("Is Copying");
 Serial.println("Sending Offset...");
 //Serial2.flush(); 
  delayMicroseconds(50);                   
  Serial2.print('A');  //ABD START TAGE  AND END TAGE {}
  Serial2.print('B');  
  Serial2.print('D');  
  Serial2.print('o');
  delayMicroseconds(500);
  bMap.seek(1);
  delayMicroseconds(500);
  for (int m = 0; m < bitmapOffset; m++)
  {
   delayMicroseconds(50);
   data2[m] = (byte)bMap.read();                         
   delayMicroseconds(50);
   Serial2.write(data2[m]);  
   delayMicroseconds(50);
   //Serial.write(data2[m]);
   }  
  delayMicroseconds(50);
  Serial2.print('{');  
  Serial2.print('}');  
  Serial.println("Offset...");
  temp1 = 0;
  qo1 = 0;
  while (true)
  {
   qo1 = qo1 + 1;
   if (Serial2.available() > 0)
    {
     byte inbyte = (byte)Serial2.read();          
     data1[temp1] = inbyte;                           
     if (data1[0] == 68)
     {
      temp1 = temp1 + 1;   
      if (data1[1] == 49)
      {     
       //lcd.setCursor(0, 1);
       //lcd.print("      ");       
       //lcd.setCursor(0, 1);
       //lcd.print("YES   ");
       Serial.println("Offset Send Successfully...");   
       delay(5);  
       qo1 = 0;
       ttotal = 0; 
       Serial.println("Sending Data...");  
       goto fc;          
      }                                        
     }
    }                  
    if (qo1 >= 190000)
    {
      qo1 = 0;
      goto fcm;
    }
  }
 qo1 = 0;  
  
fc:
 ttotal = ttotal + 1; 
 
 if ( ttotal > height)
 {
  lcd.setCursor(3, 3);
  lcd.print("File Copied");
  bMap.close();
  delay(2000);  
  return; 
 } 
 Serial.print("CARD NO: ");
 Serial.println(ttotal);
 
rs:
  
  tf = (float)ttotal / (float)height;// * 100);
  tf = tf * 100; 
  tk = (int)tf;
  
  lcd.setCursor(3, 2); 
  lcd.print(tk);    
  lcd.setCursor(6, 2); 
  lcd.print("% Completed..");
  delayMicroseconds(50);
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('c');
  Serial2.print('t');
  Serial2.print('*');
  Serial2.print(ttotal);
  Serial2.print('#');
  Serial2.print('{');
  Serial2.print('}');     
  delay(1);            // 1. Send the standard data packet header
Serial2.print('A'); 
Serial2.print('B'); 
Serial2.print('D'); 

if (ttotal == height) {
    Serial2.print('s'); // Triggers 'FILE Close' on receiver
} else {
    Serial2.print('e'); // Normal mid-file card packet
} 
delayMicroseconds(50);

// 2. Transmit exactly 'totaldata' bytes of raw binary card payload
bMap.seek(((ttotal - 1) * totaldata) + bitmapOffset); 
delayMicroseconds(50); 
for (int m = 0; m < totaldata; m++) {
    delayMicroseconds(10);
    data2[m] = (byte)bMap.read();
    delayMicroseconds(50); 
    Serial2.write(data2[m]);
    delayMicroseconds(10); 
}

Serial2.print('#'); 
Serial2.print('&'); 
Serial2.print('}'); // Ends the framing sequence
  
  // Serial2.print('A');  
  // Serial2.print('B'); 
  // Serial2.print('D');    
  // if (ttotal == height)
  // {
  //  Serial2.print('s'); 
  // }
  // else
  // {
  //  Serial2.print('e');
  // } 
  // delayMicroseconds(50);
  // bMap.seek(((ttotal - 1) * totaldata) + bitmapOffset); 
  // delayMicroseconds(50);  
  // for (int m = 0; m < totaldata; m++)
  // {
  //  delayMicroseconds(10);
  //  data2[m] = (byte)bMap.read();
  //  delayMicroseconds(50);  
  //  Serial2.write(data2[m]);
  //  delayMicroseconds(10);                                                                    
  // }
  // delayMicroseconds(10);
  // Serial2.print('{');  
  // Serial2.print('}');   
  delayMicroseconds(10);
  Serial.println("Data Send..");
  qo1 = 0;
  temp1 = 0;
   while (true)
  {
   qo1 = qo1 + 1;
   if (Serial2.available() > 0)
    {
     byte inbyte = (byte)Serial2.read();          
     data1[temp1] = inbyte;                           
     if (data1[0] == 68)
     {
      temp1 = temp1 + 1;   
      if (data1[1] == 49)
      {     
       //lcd.setCursor(0, 1);
       //lcd.print("      ");       
       //lcd.setCursor(0, 1);
       //lcd.print("YES   ");   
       Serial.println("YES");
       delay(5);
       qo1 = 0;   
       goto fc;          
      }
      else
      {
        Serial.println("NO");
        delay(5);
        qo1 = 0;
        goto rs;                                        
      }
     }
    }                  
    if (qo1 >= 90000)
    {
      Serial.println("TIMER");
      //ttotal = ttotal - 1;
      qo1 = 0;
      goto rs;
    }
    delayMicroseconds(500);
  } 
   
}

void fileselection1()
{  
 int ck1 = 0;   
 long qo1 = 0;                             
 String selfilename = "";
fs1: 
 lcd.clear();  
 lcd.setCursor(0, 0);
 lcd.print("Select File  <  /  >");   
 lcd.setCursor(0, 1);
 lcd.print("       ");
 s_name = filelist[ck1]; 
 s_name.trim();
 kk1 = s_name.length();
 kk1 = kk1 % 48;
 kk1 = kk1 - 4;
 s_name.remove(kk1); 
 selfilename = s_name;
 lcd.setCursor(0, 1);
 lcd.print(s_name);  
 
 if (totalfile > 9)
   {
    lcd.setCursor(17, 0);
    lcd.print("  ");
    lcd.setCursor(17, 0);
   } 
   else
   {
    lcd.setCursor(17, 0);
    lcd.print("  ");
    lcd.setCursor(18, 0);    
   } 
 lcd.print(totalfile + 1); 
 lcd.setCursor(6, 2);
 lcd.print(">"); 
 
s1loop1s1:
   if (ck1 > 9)
   {
    lcd.setCursor(15, 0);
    lcd.print("  ");
    lcd.setCursor(14, 0);
   } 
   else
   {
    lcd.setCursor(15, 0);    
   }
      
   lcd.print(ck1 + 1); 
      
   if (ck1 <= 0)
   {
    lcd.setCursor(0, 1);
    lcd.print("                    "); 
    lcd.setCursor(0, 1);
    lcd.print(selfilename);
    ck1 = 0;
   }
   else
   {
    lcd.setCursor(7, 1);
    lcd.print("             ");
    lcd.setCursor(7, 1);
    s_name = filelist[ck1-1];
    s_name.trim();
    kk1 = s_name.length();
    kk1 = kk1 % 48;
    kk1 = kk1 - 4;
    s_name.remove(kk1);
    lcd.print(s_name);
   }
 
  lcd.setCursor(7, 2);
  lcd.print("             ");
  lcd.setCursor(7, 2);
  s_name = filelist[ck1];
  s_name.trim();
  kk1 = s_name.length();
  kk1 = kk1 % 48;
  kk1 = kk1 - 4;
  s_name.remove(kk1);
  lcd.print(s_name); 
  
  if (ck1 == totalfile)
   {
    lcd.setCursor(0, 3);
    lcd.print("                    "); 
   }
   else
   {
    lcd.setCursor(7, 3);
    lcd.print("             ");
    lcd.setCursor(7, 3);
    s_name = filelist[ck1+1];
    s_name.trim();
    kk1 = s_name.length();
    kk1 = kk1 % 48;
    kk1 = kk1 - 4;
    s_name.remove(kk1);
    lcd.print(s_name);    
   }   
 
sloop1s1: 
 char customKey = customKeypad.getKey(); 
  if (customKey == 'B')
   {
    ck1 += 1;
    if (ck1 > totalfile)
    {
      ck1 = totalfile;  
    }
    goto s1loop1s1;
   }

  if (customKey == 'A')
   {
      ck1 -= 1;
      if (ck1 < 0)
      {
        ck1 = 0;  
      }
      goto s1loop1s1;
   }
   
   if (customKey == 'E')
   {
    s_name = filelist[ck1];
    s_name.trim();                  
    Serial.println(s_name);
    Serial2.print('A');
    Serial2.print('B');
    Serial2.print('f');
    Serial2.print('2');
    Serial2.print('*');
    Serial2.print(s_name);
    Serial2.print('#');
    Serial2.print('{');
    Serial2.print('}');   
    delay(10);
    Serial.println("File Name send");
    qo1 = 0;
    while (true)
    {      
     qo1 = qo1 + 1; 
     if (Serial2.available() > 0)
      {
       byte inbyte = (byte)Serial2.read();          
       data1[temp1] = inbyte;                      
       temp1 = temp1 + 1;   
       if (data1[temp1-2] == 68)
       {
        Serial.println("Getting Replay");                   
        if (data1[temp1-1] == 49)
        {     
         lcd.setCursor(0, 2);
         lcd.print("      ");       
         lcd.setCursor(0, 2);
         lcd.print("Select");                     
         delay(10); 
         filecopying(ck1);  
         pulcnt = 1;
         goto fs1;
        }
        if (data1[temp1-1] == 50)
        {
         lcd.setCursor(0, 2);
         lcd.print("      ");       
         lcd.setCursor(0, 2);
         lcd.print("YES   ");   
         delay(10);
         filecopying(ck1);                  
         pulcnt = 1;
         goto fs1;
        }         
       }
      }                  
      if (qo1 >= 920000)
      {
       goto fs1;
      }
    }

            
    goto fs1;
   }

   if (customKey == '*')
   {
    return;
   }   
   goto sloop1s1; 
}

//bug here 
byte folderselection() 
{   
  String tk;  
  lcd.clear();   
  lcd.setCursor(0, 0); 
  lcd.print("Folders:-           ");   
  s_name = folderlist_m[foldernum];  
  int ck1 = foldernum; // Starts at current folder index rather than resetting to 0
  
  s1loop1: 
  lcd.setCursor(6, 2); 
  lcd.write(0); 
  
  if (ck1 > 9) 
  { 
    lcd.setCursor(18, 0); 
    lcd.print("  "); 
    lcd.setCursor(18, 0); 
  }  
  else 
  { 
    lcd.setCursor(19, 0);     
  } 
  lcd.print(ck1 + 1);  
       
  // --- ROW 1 DISPLAY: PREVIOUS FOLDER ---
  if (ck1 == 0) 
  { 
    lcd.setCursor(0, 1); 
    lcd.print("                    ");  
  } 
  else 
  { 
    lcd.setCursor(7, 1); 
    lcd.print("             "); 
    lcd.setCursor(7, 1); 
    tk = folderlist_m[ck1 - 1]; // FIX: Corrected missing operator minus symbol
    tk.remove(12);  
    lcd.print(tk); 
    if(folderlist_m[foldernum].equalsIgnoreCase(folderlist_m[ck1 - 1])) 
    { 
      lcd.print(" "); 
      lcd.write(1);   
    } 
  } 
  // --- ROW 2 DISPLAY: SELECTED HOVER FOLDER ---
  lcd.setCursor(7, 2); 
  lcd.print("             "); 
  lcd.setCursor(7, 2); 
  tk = folderlist_m[ck1]; 
  tk.remove(12);  
  lcd.print(tk); 
  if(folderlist_m[foldernum].equalsIgnoreCase(folderlist_m[ck1])) 
  { 
    lcd.print(" ");  
    lcd.write(1);   
  } 
      
  // --- ROW 3 DISPLAY: NEXT FOLDER ---
  if (ck1 >= (totalfolder_m - 1)) // FIX: Bounds check ensures index+1 never goes out of limits
  { 
    lcd.setCursor(0, 3); 
    lcd.print("                    ");  
  } 
  else 
  { 
    lcd.setCursor(7, 3); 
    lcd.print("             "); 
    lcd.setCursor(7, 3); 
    tk = folderlist_m[ck1 + 1]; 
    tk.remove(12);  
    lcd.print(tk); 
    if(folderlist_m[foldernum].equalsIgnoreCase(folderlist_m[ck1 + 1])) 
    {  
      lcd.print(" "); 
      lcd.write(1);   
    } 
  }    
  
  sloop1:  
  char customKey = customKeypad.getKey();  
  if (customKey == 'B') // Key B scrolls down the list
  { 
    ck1 += 1; 
    if (ck1 >= totalfolder_m) 
    { 
      ck1 = totalfolder_m - 1; // FIX: Strict clamping preventing overflow entries
    } 
    goto s1loop1; 
  } 
 
  if (customKey == 'A') // Key A scrolls up the list
  { 
    ck1 -= 1; 
    if (ck1 < 0) 
    { 
      ck1 = 0;   
    } 
    goto s1loop1; 
  } 
      if (customKey == 'E') {
    temp = ck1;   
                                ////** this below line change folder current to defalut then  target
    // // 1. Get the actual folder name string from your array
    // String targetFolderName = folderlist_m[ck1]; 
    // targetFolderName.trim(); // Ensure no hidden trailing spaces
    
    // // 2. Clear out any old residual serial data bytes
    // while(Serial2.available() > 0) { Serial2.read(); }
    
    // // 3. Transmit the complete packet using the full name string
    //         // Validation framing character end
    //   Serial2.print('A'); 
    //   Serial2.print('B'); 
    //   Serial2.print('F');            
    //   // FIX: Use write() instead of print() to guarantee exactly one single binary byte is sent
    //   Serial2.write((byte)tempq);      // <-- sent as a RAW BINARY BYTE, not ASCII text!
    //   Serial2.print('#'); 
    //   Serial2.print('{'); 
    //   Serial2.print('}');
    //   Serial2.flush();
    // delay(10);
    return ck1; 
  }
  if (customKey == '*') // Key * cancels selection actions
  { 
    return foldernum; 
  }    
  goto sloop1;  
}

byte initaalfolderselection()
{  
 String tk; 
 lcd.clear();  
 lcd.setCursor(0, 0);
 lcd.print("Folders:-           ");  
 s_name = folderlist_m[foldernum];  

 //int ck1 = 0;   
   int ck1 = foldernum;   // start from current folder
 lcd.setCursor(6, 2);
 lcd.write(0);
 
s1loop1:
   if (ck1 > 9)
   {
    lcd.setCursor(18, 0);
    lcd.print("  ");
    lcd.setCursor(18, 0);
   } 
   else
   {
    lcd.setCursor(19, 0);    
   }
      
   lcd.print(ck1 + 1); 
      
   if (ck1 == 0)
   {
    lcd.setCursor(0, 1);
    lcd.print("                    "); 
    //lcd.setCursor(0, 1);
    //lcd.print(s_name);
   }
   else
   {
    lcd.setCursor(7, 1);
    lcd.print("             ");
    lcd.setCursor(7, 1);
    tk = folderlist_m[ck1-1];
    tk.remove(12); 
    lcd.print(tk);
    if(folderlist_m[foldernum].equalsIgnoreCase(folderlist_m[ck1-1]))
    {
      lcd.print(" ");
      lcd.write(1);  
    }
   }
 
  lcd.setCursor(7, 2);
  lcd.print("             ");
  lcd.setCursor(7, 2);
  tk = folderlist_m[ck1];
  tk.remove(12); 
  lcd.print(tk);
  if(folderlist_m[foldernum].equalsIgnoreCase(folderlist_m[ck1]))
  {
   lcd.print(" "); 
   lcd.write(1);  
  }
     
  if (ck1 == totalfolder_m)
   {
    lcd.setCursor(0, 3);
    lcd.print("                    "); 
   }
   else
   {
    lcd.setCursor(7, 3);
    lcd.print("             ");
    lcd.setCursor(7, 3);
    tk = folderlist_m[ck1+1];
    tk.remove(12); 
    lcd.print(tk);
    if(folderlist_m[foldernum].equalsIgnoreCase(folderlist_m[ck1 + 1]))
    { 
      lcd.print(" ");
      lcd.write(1);  
    }
   }   
 
sloop1: 
 char customKey = customKeypad.getKey(); 
  if (customKey == 'B')
   {
    ck1 += 1;
    if (ck1 >= totalfolder_m)
    {
      ck1 = totalfolder_m;  
    }
    goto s1loop1;
   }

  if (customKey == 'A')
   {
      ck1 -= 1;
      if (ck1 <= 0)
      {
        ck1 = 0;  
      }
      goto s1loop1;
   }
   
   if (customKey == 'E')
   {    
    temp = ck1;
    return ck1;
   }

   if (customKey == '*')
   {
    return foldernum;
   }   
   goto sloop1; 
}

// Folder selection
// IMPORTANT: This function only selects/returns the folder number.
// It does NOT modify F1/F2 file-selection logic.

byte folderselectioncustom()
{
    String tk;

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Folders:-           ");

    // Start from currently selected folder
    int ck1 = foldernum;

    // Safety check
    if (totalfolder_m == 0)
    {
        return foldernum;
    }

    if (ck1 < 0)
    {
        ck1 = 0;
    }

    if (ck1 >= totalfolder_m)
    {
        ck1 = totalfolder_m - 1;
    }

    lcd.setCursor(6, 2);
    lcd.write(0);


s1loop1:

    // Safety check before accessing folderlist_m[ck1]
  
    if (ck1 < 0)
    {
        ck1 = 0;
    }

    if (ck1 >= totalfolder_m)
    {
        ck1 = totalfolder_m - 1;
    }

    // Folder number
    if (ck1 > 9)
    {
        lcd.setCursor(18, 0);
        lcd.print("  ");
        lcd.setCursor(18, 0);
    }
    else
    {
        lcd.setCursor(19, 0);
    }

    lcd.print(ck1 + 1);
    // Previous folder
    if (ck1 == 0)
    {
        lcd.setCursor(0, 1);
        lcd.print("                    ");
    }
    else
    {
        lcd.setCursor(7, 1);
        lcd.print("             ");

        lcd.setCursor(7, 1);

        tk = folderlist_m[ck1 - 1];
        tk.remove(12);

        lcd.print(tk);

        if (folderlist_m[foldernum].equalsIgnoreCase(
                folderlist_m[ck1 - 1]))
        {
            lcd.print(" ");
            lcd.write(1);
        }
    }
    // Current folder
    lcd.setCursor(7, 2);
    lcd.print("             ");

    lcd.setCursor(7, 2);

    tk = folderlist_m[ck1];
    tk.remove(12);

    lcd.print(tk);

    if (folderlist_m[foldernum].equalsIgnoreCase(
            folderlist_m[ck1]))
    {
        lcd.print(" ");
        lcd.write(1);
    }
    // Next folder

    if (ck1 >= totalfolder_m - 1)
    {
        lcd.setCursor(0, 3);
        lcd.print("                    ");
    }
    else
    {
        lcd.setCursor(7, 3);
        lcd.print("             ");

        lcd.setCursor(7, 3);

        tk = folderlist_m[ck1 + 1];
        tk.remove(12);

        lcd.print(tk);

        if (folderlist_m[foldernum].equalsIgnoreCase(
                folderlist_m[ck1 + 1]))
        {
            lcd.print(" ");
            lcd.write(1);
        }
    }

sloop1:

    char customKey = customKeypad.getKey();
    // B = NEXT FOLDER
    if (customKey == 'B')
    {
        if (ck1 < totalfolder_m - 1)
        {
            ck1++;
        }

        goto s1loop1;
    }
    // A = PREVIOUS FOLDER
    if (customKey == 'A')
    {
        if (ck1 > 0)
        {
            ck1--;
        }

        goto s1loop1;
    }

    // E = SELECT FOLDER
    if (customKey == 'E')
    {
        temp = ck1;

        Serial.print("Folder selected: ");
        Serial.println(ck1);

        return ck1;
    }
    // * = CANCEL
    if (customKey == '*')
    {
        return foldernum;
    }


    goto sloop1;
}

byte fileselection(byte r, byte c, byte fn)
{ 
 int ck = fn; 
 lcd.setCursor(c - 1, r);
 lcd.print(">"); 
 
s1loop: 
 lcd.setCursor(c, r);
 lcd.print("             ");
 lcd.setCursor(c, r);
 
 s_name = filelist_m[ck];
 s_name.trim();
 
 // SAFE COMPACT DISPLAY NAME STRIPPING (Bypasses negative index math crashes)
 int dotIndex = s_name.lastIndexOf('.');
 if (dotIndex > 0) {
     s_name = s_name.substring(0, dotIndex); // Drops extension safetly
 }
 if (s_name.length() > 12) {
     s_name = s_name.substring(0, 12); // Bound check screen length limit
 }
 
 lcd.print(s_name); 
 lcd.setCursor(18, 0);
 lcd.print("St"); 
 
sloop: 
 customKey = customKeypad.getKey(); 
  if (customKey == 'B')
   {
    ck += 1;
    // ARRAY BOUNDS HARDENING: Ensures index never matches size boundary
    if (ck >= totalfile_m)
    {
     ck = 0;  
    }   
    goto s1loop;
   }

  if (customKey == 'A')
   {
      ck -= 1;
      if (ck < 0)
      {
        // ARRAY BOUNDS HARDENING: Wraps backwards safely
        ck = (totalfile_m > 0) ? (totalfile_m - 1) : 0;        
      }
      goto s1loop;
   }
   
   if (customKey == 'E')
   {    
    Serial.println("\n--- [FRONT PANEL FILE CONFIRMED] ---");
    Serial.print("Confirmed Active Selection Index ID: "); Serial.println(ck);
    Serial.print("System Transferring Filename Reference: "); Serial.println(filelist_m[ck]);
    Serial.println("------------------------------------\n");
    return ck;
   }

   if (customKey == '*')
   {
    Serial.println("\n--- [FRONT PANEL FILE CANCELLED] ---");
    Serial.print("Reverting Back to Fallback Index: "); Serial.println(fn);
    Serial.println("------------------------------------\n");
    return fn; // Fixed from original code return ck (which broke cancel rules)
   }   
   goto sloop;
}

byte finddig(byte w,unsigned long hig)
 {
  byte wck123 = 0;  

  if( w>=1 && w<=5) {
    wck123 = (hig/(unsigned long)(pow(10,(w-1))) % 10);
  } else {
    //Print to denote the failure case
  }
  return wck123;
/*
  if (w == 1) 
  {
    hig1 = (hig % 10);
    wck123 = hig1;
  }    
  else if (w == 2) 
  {
    hig1 = ((hig / 10) % 10);
    wck123 = hig1; 
  }
  else if (w == 3) 
  {
    hig1 = ((hig / 100) % 10);
    wck123 = hig1;
  }
  else if (w == 4) 
  {
    hig1 = ((hig / 1000) % 10);
    wck123 = hig1; 
  }
  else if (w == 5) 
  {
    hig1 = ((hig / 10000) % 10);
    wck123 = hig1; 
  }
*/
 }

unsigned long readvalue(byte r1,byte c1,byte sz,unsigned long pic)
{      
 int k2 = 0;
 lcd.cursor();
 lcd.blink();
 lcd.setCursor(c1, r1);
 lcd.print(pic);  
 lcd.setCursor(c1, r1);
 motorstatus = 0;
 mcnt = 0; 
 tcnt = 0;
 ch =' '; 
 pass[0]='0';  
 pass[1]='0';
 pass[2]='0';
 pass[3]='0';
 pass[4]='0';
 pass[5]='0';
 pass[6]='0';
 pass[7]='0';
 
 SysNo[0]='0';
 SysNo[1]='0';
 SysNo[2]='0';
 SysNo[3]='0';
 SysNo[4]='0';
 SysNo[5]='0';
 SysNo[6]='0';
 SysNo[7]='0'; 

 if (pic != 0)
 {
  byte r12;
  lcd.setCursor(c1, r1);
  for (int m = 0; m < sz; m++)
  {
   r12 =  finddig((sz - m),pic);
   pass[m] = r12 + 48;
   lcd.print(pass[m]);
  }  
 }
 else
 {
  lcd.setCursor(c1, r1);  
  if (sz == 1)
  lcd.print("0");  
  if (sz == 2)
  lcd.print("00");  
  if (sz == 3)
  lcd.print("000");  
  if (sz == 4)
  lcd.print("0000");  
  if (sz == 5)
  lcd.print("00000");  
 }  
 temp = 0;
 lcd.setCursor(c1, r1);
 motorstatus = pass[0];
 temp = motorstatus % 48;
 mcnt = temp;
 r = 0; 
 m = 0;  
 sz = sz - 1;
rploop: 
 customKey = customKeypad.getKey();  

  if (customKey >= 48 && customKey <= 57)
   {
     mcnt = customKey % 48;
     customKey = 'D';
     //goto menu;  
   }
   
  if (customKey == 'B')
   {
      mcnt = mcnt - 1;
      if (mcnt < 0)
      {
        mcnt = 9;
      }
      lcd.setCursor(c1+m, r1);
      lcd.print(mcnt);
      lcd.setCursor(c1+m, r1);
      motorstatus = mcnt + 48;
      pass[m] = motorstatus;      
   }
   
  if (customKey == 'A')
   {
      mcnt = mcnt + 1;
      if (mcnt > 9)
      {
        mcnt = 0;
      }
      lcd.setCursor(c1+m, r1);
      lcd.print(mcnt);
      lcd.setCursor(c1+m, r1);
      motorstatus = mcnt + 48;
      pass[m] = motorstatus;      
   }

  if (customKey == 'D')
   {
      lcd.setCursor(c1+m, r1);
      lcd.print(mcnt);
      lcd.setCursor(c1+m, r1);
      motorstatus = mcnt + 48;
      pass[m] = motorstatus;      
      m = m + 1;
      if (m > sz)
      {
        m = 0;
      }
      motorstatus = pass[m];
      mcnt = motorstatus % 48;      
      lcd.setCursor(c1+m, r1);
      lcd.print(mcnt);
      lcd.setCursor(c1+m, r1);            
   }   

   if (customKey == 'C')
   {
      lcd.setCursor(c1+m, r1);
      lcd.print(mcnt);
      lcd.setCursor(c1+m, r1);
      motorstatus = mcnt + 48;
      pass[m] = motorstatus;      
      m = m - 1;
      if (m < 0)
      {
        m = sz;
      }
      motorstatus = pass[m];
      mcnt = motorstatus % 48;      
      lcd.setCursor(c1+m, r1);
      lcd.print(mcnt);
      lcd.setCursor(c1+m, r1);            
   }
   
   if (customKey == 'E')
   {
     motorstatus = 0;
     byte ckk = 0;
     byte k = 0;
     byte tempp = 0;     
     tcnt = 0;     
     cnt = 0;     
   valloop:  
      ckk = sz - k;      
      if (k == 4)
      {
        motorstatus = pass[ckk];
        temp = motorstatus % 48; //) * 10);
        tcnt = tcnt + (temp * 10000);                
      }       
      if (k == 3)
      {
        motorstatus = pass[ckk];
        cnt = motorstatus % 48; //) * 10000);      
        cnt = cnt * 1000;      
        tcnt = tcnt + cnt;        
      }       
      if (k == 2)
      {
       motorstatus = pass[ckk];
       temp = motorstatus % 48; //) * 10);
       cnt = temp * 100;      
       tcnt = tcnt + cnt;       
      } 
      if (k == 1)
      {
       tempp = 0;
       motorstatus = pass[ckk];
       temp = motorstatus % 48;
       temp = temp * 10; 
       tcnt = tcnt + temp;             
      } 
      if (k == 0)
      {
       motorstatus = pass[ckk];
       temp = motorstatus % 48;              
       tcnt = temp;       
      }
       k = k + 1;
      if (k > sz)
      {       
       return tcnt; 
      }     
     goto valloop;      
   }

   if (customKey == '*')
   {
      ch = customKey;
      return tcnt;
   }   
   goto rploop;
}


byte findsize(unsigned long hig)
 {
  byte wck12 = 0;  
  if (hig <= 99999) 
  wck12 = 0;  
  if (hig <= 9999)
  wck12 = 1; 
  if (hig <= 999)
  wck12 = 2; 
  if (hig <= 99)
  wck12 = 3; 
  if (hig <= 9)
  wck12 = 4;
  return wck12;
 }

void runningdisplay()
{
 byte ck12 = 0;
 byte ck13 = 0; 
 lcd.clear();  
 delay(20); 
 //lcd.begin(20, 4); 
 lcd.clear();  
 lcd.noCursor(); 
 lcd.setCursor(0, 0);
 lcd.print("BD:-"); 
 if (temp3 <= 425000)
 {
  lcd.setCursor(19, 0);
  lcd.print("R"); 
 } 
 lcd.setCursor(5, 0); 
 s_name = filelist_m[filenum_m];
 s_name.trim();
 kk1 = s_name.length();
 kk1 = kk1 % 48;
 kk1 = kk1 - 4;
 s_name.remove(kk1);
 lcd.print(s_name); 
 lcd.setCursor(0, 1);
 lcd.print("     ");
 //delay(100); 
 if (pickno <= 99999) 
 ck12 = 0;  
 if (pickno <= 9999)
 ck12 = 1; 
 if (pickno <= 999)
 ck12 = 2; 
 if (pickno <= 99)
 ck12 = 3; 
 if (pickno <= 9)
 ck12 = 4;
 lcd.setCursor(ck12, 1);
 lcd.print(pickno);
 lcd.setCursor(5, 1);
 lcd.print("-"); 
 lcd.setCursor(6, 1);
 lcd.print(height);
 lcd.print("          ");
 lcd.setCursor(12, 1);
 lcd.print("        ");
 lcd.setCursor(12, 1);
 lcd.print("Rpt:-");
 //if (repeatcnt <= 999)
 //ck12 = 14; 
 //if (repeatcnt <= 99)
 //ck12 = 15; 
 //if (repeatcnt <= 9)
 //ck12 = 16;
 //lcd.setCursor(ck12, 1); 
 lcd.print(repeatcnt);
 if (filerunningmode == 2)
  {
   lcd.setCursor(0, 2);
   lcd.print("BR:-");
   lcd.setCursor(5, 2);
   s_name = filelist_m[filenum1_m];
   s_name.trim();
   kk1 = s_name.length();
   kk1 = kk1 % 48;
   kk1 = kk1 - 4;
   s_name.remove(kk1);
   lcd.print(s_name);   
   lcd.setCursor(0, 3);
   lcd.print("     ");
   if (pickno1 <= 99999) 
   ck13 = 0;   
   if (pickno1 <= 9999)
   ck13 = 1;   
   if (pickno1 <= 999)
   ck13 = 2;   
   if (pickno1 <= 99)
   ck13 = 3;   
   if (pickno1 <= 9)
   ck13 = 4;
   lcd.setCursor(ck13, 3);
   lcd.print(pickno1);
   lcd.setCursor(5, 3);
   lcd.print("-");
   lcd.setCursor(6, 3); 
   lcd.print(height1);
   lcd.print("          ");
   lcd.setCursor(12, 3);
   lcd.print("        ");
   lcd.setCursor(12, 3);
   lcd.print("Rpt:-");
   //if (repeatcnt1 <= 999)
   //ck12 = 14; 
   //if (repeatcnt1 <= 99)
   //ck12 = 15; 
   //if (repeatcnt1 <= 9)
   //ck12 = 16;
   //lcd.setCursor(ck12, 3);
   lcd.print(repeatcnt1);
   //lcd.setCursor(0, 0);
   //lcd.print("F1:"); 
  }  
}

void runningdesign()
{
  Serial.print("Serial2.available BEFORE = ");
  Serial.println(Serial2.available());
rnmain:
  delay(100);
  runningstatus();
  delay(10);
  byte ck12 = 0;
  byte ck13 = 0;
  byte key = 0;

  // Flicker Fix: Millisecond anchor prevents continuous display rewriting
  unsigned long lastDisplayRefresh = 0;

rnloop:
  if (millis() - lastDisplayRefresh > 250) {
    runningdisplay();
    lastDisplayRefresh = millis();
  }

  byte temp1 = 0;

loopv:
  if (Serial2.available() > 0)
  {
    Serial.println("ok");
    d = 0;
    serialinput();
    delay(10);
    if (d == 1 || filerunningmode == 2)
    {
      d = 0;
      if (pulcnt == 0) { temp3 = 0; }
      else { pulcnt = 0; temp3 = 425010; }
      lastDisplayRefresh = 0;
      goto rnloop;
    }
  }

  temp3 = temp3 + 1;
  if (temp3 <= 425000) { goto loopv; }
  else
  {
    temp3 = 425010;
    lcd.setCursor(18, 0);
    lcd.print("*R");
  }

  customKey = customKeypad.getKey();
  if (customKey == 'E')
  {
    pulcnt = 0; temp3 = 0;
    return;
  }

  if (customKey == '*')
  {
    customKey = ' ';
    lcd.setCursor(18, 0);
    lcd.print("St");
    byte tem1 = 1;

  ed: // this lable controll

    // ---------------------------------------------------------------------------
    // STEP 1: primary file selection
    // ---------------------------------------------------------------------------
    if (tem1 == 1)
    {
      pluseupcheck = filenum_m;
      temp = fileselection(0, 5, filenum_m);
      
      Serial.println("\n==============================================");
      Serial.println("[DEBUG] USER COMPLETED FILE SELECTION MENU");  
      Serial.print("[DEBUG] Selected File Index (temp): ");
      Serial.println(temp);
      Serial.print("[DEBUG] Current Master Folder (foldernum): ");
      Serial.println(foldernum);
        Serial.print("[DEBUG] filenum_m BEFORE overwrite: ");
      Serial.println(filenum_m);
      // If your code updates filenum_m right here, we print it:
     // filenum_m = temp; 
      Serial.print("[DEBUG] filenum_m AFTER overwrite: ");
      Serial.println(filenum_m);
      // Let's watch the upcoming Serial2 transmit data loop
      Serial.println("[DEBUG] Monitoring outbound Serial2 stream next...");
      Serial.println("==============================================\n");
      // --- END DEBUG TRACKER ---
      if (customKey == '*')
      {
        customKey = ' ';
        lcd.noCursor(); lcd.noBlink();
        pulcnt = 0; temp3 = 0;
        goto rnmain;
      }
      if (filenum_m != temp)
      {  
              char dbgPacket[20];
    snprintf(dbgPacket, sizeof(dbgPacket), "ABEK%d#{}", temp);
    Serial.print("[DEBUG] >>> SENDING PACKET: ");
    Serial.println(dbgPacket);

        Serial2.print('A'); Serial2.print('B'); Serial2.print('E'); Serial2.print('K');
        Serial2.print(temp); Serial2.print('#');
        Serial2.print('{'); Serial2.print('}');
      }else
  {
    Serial.println("[DEBUG] >>> NO PACKET SENT (filenum_m == temp, condition false)");
  }   filenum_m = temp;               // moved to AFTER the comparison
     Serial.print("[DEBUG] filenum_m AFTER overwrite: ");
     Serial.println(filenum_m);
      tem1 = 2;
      // falls through intentionally: tem1 is now 2, so STEP 2 below runs immediately
      // on this same pass through "ed:"
    }

    // ---------------------------------------------------------------------------
    // STEP 2: wait for slave ack after file selection
    // ---------------------------------------------------------------------------
    if (tem1 == 2)
    {
      lcd.setCursor(16, 0); lcd.print("Wait");
      delay(20); runningstatus(); d = 0;
      unsigned long lastDataTime = millis();

      while (true)
      {
        if (millis() - lastDataTime > 500) {
          Serial.println("Data stream finished or idle. Continuing to execution.");
          goto f1cout;
        }
        if (Serial2.available() > 0)
        {
          serialinput(); lastDataTime = millis(); delay(50);
          if (d == 1 || filerunningmode == 2) { d = 0; runningdisplay(); goto f1cout; }
        }
      }
      
    
    // <-- STEP 2 scope closes here. f1cout: below is OUTSIDE it on purpose (see note
    //     at top of file) and runs for every tem1 1..6, not only tem1==2.

    // ===============================================================================
    // SHARED: primary pick-number (pickno) display + live edit.
    // Runs on every pass through "ed:" regardless of tem1 -- intentional, do not gate.
    // ===============================================================================
    f1cout:
      lcd.setCursor(18, 0); lcd.print(" ");
      lcd.setCursor(18, 0); lcd.print("St");
      lcd.setCursor(3, 0);  lcd.print(" ");
      lcd.setCursor(0, 1);  lcd.print(" ");
      lcd.setCursor(6, 1);  lcd.print(" ");
      lcd.setCursor(6, 1);  lcd.print(height);
      lcd.setCursor(12, 1); lcd.print(" ");
      lcd.setCursor(12, 1); lcd.print("Rpt:-");
      lcd.print(repeatcnt);
      lastpickno = pickno;

    rdpic:
      ck12 = findsize(height);
      pickno = readvalue(1, ck12, (5 - ck12), pickno);
      if (customKey == '*')
      {
        customKey = ' ';
        lcd.noCursor(); lcd.noBlink();
        pulcnt = 0; temp3 = 0;
        goto rnmain;
      }
      if (height > 0 && pickno > height)
      {
        lcd.setCursor(18, 0); lcd.print("Er");
        lcd.setCursor(15, 0); delay(1000);
        lcd.setCursor(18, 0); lcd.print(" ");
        lcd.setCursor(18, 0); lcd.print("St");
        goto rdpic;
      }
      if (filerunningmode == 2)
      {
        if ((pluseupcheck == filenum_m)) {
          if ((pickno == 0) || (pickno == lastpickno)) { goto out; }
          else {
            
            Serial2.print('A'); Serial2.print('B'); Serial2.print('E'); Serial2.print('2');
            Serial2.print('*'); Serial2.print(pickno); Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
          }
        }
      }
      else {
        Serial2.print('A'); Serial2.print('B'); Serial2.print('E'); Serial2.print('2');
        Serial2.print('*'); Serial2.print(pickno); Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
      } 
    out:
      lcd.setCursor(16, 0); lcd.print("Wait");
      delay(100); runningstatus(); d = 0;
      unsigned long pickerWaitClock = millis();

      while (true)
      {
        if (millis() - pickerWaitClock > 300) {
          Serial.println("Picker data stream updated or idle. Returning to canvas flow.");
          goto f1cout1;
        }
        if (Serial2.available() > 0)
        {
          serialinput(); pickerWaitClock = millis(); delay(50);
          if (d == 1) { d = 0; runningdisplay(); goto f1cout1; }
        }
      }

    f1cout1:
      lcd.setCursor(16, 0); lcd.print("    ");
      lcd.setCursor(18, 0); lcd.print("St");
      lcd.setCursor(0, 1);  lcd.print(" ");

      if (pickno <= 99999) ck12 = 0;
      if (pickno <= 9999)  ck12 = 1;
      if (pickno <= 999)   ck12 = 2;
      if (pickno <= 99)    ck12 = 3;
      if (pickno <= 9)     ck12 = 4;
      lcd.setCursor(ck12, 1); lcd.print(pickno);
      lcd.setCursor(0, 3); lcd.print(" ");

      if (filerunningmode == 2)
      {
        if (pickno1 <= 99999) ck12 = 0;
        if (pickno1 <= 9999)  ck12 = 1;
        if (pickno1 <= 999)   ck12 = 2;
        if (pickno1 <= 99)    ck12 = 3;
        if (pickno1 <= 9)     ck12 = 4;
        lcd.setCursor(ck12, 3); lcd.print(pickno1);
      }
      lcd.noCursor(); lcd.noBlink();
  }
    // ===============================================================================
    // END SHARED pick-number block
    // ===============================================================================

    // ---------------------------------------------------------------------------
    // STEP 3: repeat count (primary)
    // ---------------------------------------------------------------------------
    if (tem1 == 3)
    {
      lcd.setCursor(12, 1); lcd.print(" ");
      lcd.setCursor(12, 1); lcd.print("Rpt:-");
      lcd.print(repeatcnt); lastpickno = repeatcnt;
      repeatcnt = readvalue(1, 17, 3, repeatcnt);
      if (customKey == '*') {
        customKey = ' '; lcd.noCursor(); lcd.noBlink();
        pulcnt = 0; temp3 = 0; goto rnmain;
      }
      if (repeatcnt != lastpickno) {
        Serial2.print('A'); Serial2.print('B'); Serial2.print('E'); Serial2.print('3');
        Serial2.print('*'); Serial2.print(repeatcnt); Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
      }
      lcd.setCursor(12, 1); lcd.print(" ");
      lcd.setCursor(12, 1); lcd.print("Rpt:-");
      lcd.print(repeatcnt); lcd.noCursor(); lcd.noBlink();
    }

    // ---------------------------------------------------------------------------
    // STEP 4: secondary file selection
    // ---------------------------------------------------------------------------
    if (tem1 == 4)
    {
      temp = 0; temp = fileselection(2, 5, filenum1_m);
      if (customKey == '*') {
        customKey = ' '; lcd.noCursor(); lcd.noBlink();
        pulcnt = 0; temp3 = 0; goto rnmain;
      }
      if (filenum1_m != temp) {
        Serial2.print('A'); Serial2.print('B'); Serial2.print('E');
        Serial2.print('L'); Serial2.print(temp); Serial2.print('{'); Serial2.print('}');
      }
       filenum1_m = temp;   // <-- add this
    }

    // ---------------------------------------------------------------------------
    // STEP 5: wait for ack, then secondary pick-number (pickno1) display + edit
    // (self-contained -- f1cout2:/rdpic1: are correctly scoped inside this block,
    // unlike the primary f1cout/rdpic block above; this one only runs for tem1==5)
    // ---------------------------------------------------------------------------
    if (tem1 == 5)
    {
      lcd.setCursor(16, 0); lcd.print("Wait");
      delay(100); runningstatus(); d = 0;
      while (true) {
        if (Serial2.available() > 0) {
          serialinput(); delay(50);
          if (d == 1) { d = 0; runningdisplay(); delay(100); goto f1cout2; }
        }
      }
      f1cout2:
      lcd.setCursor(18, 0); lcd.print(" "); lcd.setCursor(18, 0); lcd.print("St");
      lcd.setCursor(3, 2);  lcd.print(" "); lcd.setCursor(0, 3);  lcd.print(" ");
      lcd.setCursor(6, 3);  lcd.print(" "); lcd.setCursor(6, 3);  lcd.print(height1);
      lcd.setCursor(12, 3); lcd.print(" "); lcd.setCursor(12, 3); lcd.print("Rpt:-");
      lcd.print(repeatcnt1);

      rdpic1:
      ck12 = findsize(height1); lastpickno = pickno1;
      pickno1 = readvalue(3, ck12, (5 - ck12), pickno1);
      if (customKey == '*') { customKey = ' '; lcd.noCursor(); lcd.noBlink(); pulcnt = 0; temp3 = 0; goto rnmain; }
      if (pickno1 > height1) { lcd.setCursor(18, 0); lcd.print("Er"); delay(1000); lcd.setCursor(18, 0); lcd.print("St"); goto rdpic1; }
      if (pickno1 != lastpickno) {
        Serial2.print('A'); Serial2.print('B'); Serial2.print('E'); Serial2.print('6');
        Serial2.print('*'); Serial2.print(pickno1); Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
      }
      lcd.setCursor(0, 3); lcd.print(" ");
      if (pickno1 <= 99999) ck12 = 0;
      if (pickno1 <= 9999)  ck12 = 1;
      if (pickno1 <= 999)   ck12 = 2;
      if (pickno1 <= 99)    ck12 = 3;
      if (pickno1 <= 9)     ck12 = 4;
      lcd.setCursor(ck12, 3); lcd.print(pickno1);
      lcd.setCursor(6, 3);   lcd.print(" "); lcd.setCursor(6, 3);   lcd.print(height1);
      lcd.noCursor(); lcd.noBlink();
    }

    // ---------------------------------------------------------------------------
    // STEP 6: repeat count (secondary)
    // ---------------------------------------------------------------------------
    if (tem1 == 6)
    {
      lcd.setCursor(12, 3); lcd.print(" "); lcd.setCursor(12, 3); lcd.print("Rpt:-");
      lcd.print(repeatcnt1); lastpickno = repeatcnt1;
      repeatcnt1 = readvalue(3, 17, 3, repeatcnt1);
      if ((customKey == '*') || (repeatcnt1 == lastpickno)) {
        customKey = ' '; lcd.noCursor(); lcd.noBlink(); pulcnt = 0; temp3 = 0; goto rnmain;
      }
      else {
        Serial2.print('A'); Serial2.print('B'); Serial2.print('E'); Serial2.print('7');
        Serial2.print('*'); Serial2.print(repeatcnt1); Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
      }
      lcd.setCursor(12, 3); lcd.print(" "); lcd.setCursor(12, 3); lcd.print("Rpt:-");
      lcd.print(repeatcnt1); lcd.noCursor(); lcd.noBlink();
    }

    tem1 = tem1 + 1;
    if (filerunningmode == 2) {
      if (tem1 > 6) { lcd.clear(); pulcnt = 0; temp3 = 0; goto rnmain; }
    }
    else {
      if (tem1 > 3) { lcd.clear(); pulcnt = 0; temp3 = 0; goto rnmain; }
    }
    goto ed;
  } // <-- closes "if (customKey == '*')"

  if (customKey == 'B')
  {
    customKey = '0'; pulcnt = 1;
    Serial2.print('A'); Serial2.print('B'); Serial2.print('E'); Serial2.print('8');
    Serial2.print(temp + 48); Serial2.print('{'); Serial2.print('}');
    delay(100); d = 0;
    while (true) {
      if (Serial2.available() > 0) {
        serialinput(); delay(50);
        if (d == 1) { d = 0; lastDisplayRefresh = 0; goto rnloop; }
      }
    }
  }

  if (customKey == 'A')
  {
    customKey = '0'; pulcnt = 1;
    Serial2.print('A'); Serial2.print('B'); Serial2.print('E'); Serial2.print('9');
    Serial2.print(temp + 48); Serial2.print('{'); Serial2.print('}');
    delay(100); d = 0;
    while (true) {
      if (Serial2.available() > 0) {
        serialinput(); delay(50);
        if (d == 1) { d = 0; lastDisplayRefresh = 0; goto rnloop; }
      }
    }
  }

  goto loopv;
}



void tkit()
{ 
 motorstatus = 0;
 unsigned long h1ig = 0;
 byte temp31 = 0; 
 lcd.clear(); 
 lcd.setCursor(0, 0); 
 lcd.print("TKit:-  ");
 lcd.setCursor(0, 1);
 lcd.print("Motor Status:-"); 
 lcd.setCursor(0, 2);
 lcd.print("Sensor.1:-    "); 
 lcd.setCursor(0, 3);
 lcd.print("Sensor.2:-    "); 
 lcd.setCursor(14, 1);
 lcd.print("OFF");
tloop: 
  h1ig = h1ig + 1;
 customKey = customKeypad.getKey(); 

 if (Serial2.available() > 0)
  {
   byte inbyte = (byte)Serial2.read();   
   data3[temp31] = inbyte;   
   temp31 = temp31 + 1;    
  }
  
  if (data3[0] == 67)
    {
     if (data3[1] == 68)
     {      
      
     }
    }
    else
    {
     temp31 = 0;
     data3[0] = '0'; 
    }
   
    if (data3[2] == 48)
      {
        lcd.setCursor(11, 2);
        lcd.print("ON ");
      }
      else
      {
        lcd.setCursor(11, 2);
        lcd.print("OFF");
      }
      
      if (data3[3] == 48)
      {        
        lcd.setCursor(11, 3);
        lcd.print("ON ");
      }
      else
      {
        lcd.setCursor(11, 3);
        lcd.print("OFF");
      }
    
    if (temp31 > 3)
     {
        temp31 = 0;
        data3[0] = '0';
     } 

   if ( h1ig  >= 300)
   {
      h1ig = 0;
      Serial2.print('A');
      delay(1);
      Serial2.print('B');      
      delay(1);
      Serial2.print('T');
      delay(1);
      if (motorstatus == 1)
      {
       Serial2.print('O');
      }
      else
      {       
       Serial2.print('F');  
      }
      delay(1);
      Serial2.print('F');
      delay(1);
      Serial2.print('{');
      delay(1);
      Serial2.print('}');
   }     
 
  if (customKey == 'A')
   {
      lcd.setCursor(14, 1);
      lcd.print("ON ");
      motorstatus = 1;       
   }

  if (customKey == 'B')
   {
      lcd.setCursor(14, 1);
      lcd.print("OFF");
      motorstatus = 0;             
   }
   
   if (customKey == 'E')
   {           
      return;
   }

   if (customKey == '*')
   {
      return;
   }   
   goto tloop;
}

long readeeprom()
{
  for (int i = 0; i <=5; i++)
    {
      pass[i] = EEPROM.read(i);
      EEPROM.commit();
      delay(20);
    }
    tcnt = 0;
    motorstatus = pass[0];
    temp = motorstatus % 48;
    tcnt = temp * 100000;      
    motorstatus = pass[1];
    cnt = motorstatus % 48; //) * 10000);      
    cnt = cnt * 10000;      
    tcnt = tcnt + cnt;       
    motorstatus = pass[2];
    temp = motorstatus % 48; //) * 1000);
    cnt = temp * 1000;
    tcnt = tcnt + cnt;      
    motorstatus = pass[3];
    temp = motorstatus % 48; //) * 100);
    tcnt = tcnt + (temp * 100);      
    motorstatus = pass[4];
    temp = motorstatus % 48; //) * 10);
    tcnt = tcnt + (temp * 10);      
    motorstatus = pass[5];
    temp = motorstatus % 48;
    tcnt = tcnt + temp;    
    return tcnt;    
}

long readpassword(byte cbit)
{    
 byte nt = 0, nt1 = 0;
 motorstatus = 0;
 mcnt = 0; 
 tcnt = 0;
 ch =' ';
 pass[0]='0';  
 pass[1]='0';
 pass[2]='0';
 pass[3]='0';
 pass[4]='0';
 pass[5]='0';
 r = 0;
 cnt = 0;
 m = 0;
 lcd.clear(); 
 lcd.cursor();
 lcd.setCursor(0, 1); 
 lcd.print("Enter Code:-    ");
 lcd.setCursor(7, 2);  
 lcd.print("******");
 lcd.setCursor(7, 2);
rploop: 
 customKey = customKeypad.getKey();

  if (customKey >= 48 && customKey <= 57)
   {
     mcnt = customKey % 48;  
     lcd.setCursor(7+m, 2);
     lcd.print(mcnt);
     lcd.setCursor(7+m, 2);   
     cnt = 0;
     nt1 = 1;
     nt = 0;
     //goto menu;  
   } 
  
  cnt = cnt + 1;  
  nt = nt + 1;
   
   if (cnt >= 100)
   {
    if (cnt <= 110)
    {       
      lcd.setCursor(7+m, 2);
      lcd.print("*");
      lcd.setCursor(7+m, 2);
    }  
   }  
   
   if (nt >= 25)
   {    
    if (nt1 == 1)
    {      
     customKey = 'N';
     nt1 = 0;
    } 
    nt = 0;    
   }
    
   delay(10);
   if (customKey == 'N' || customKey == 'B')
   {
      lcd.setCursor(7+m, 2);
      lcd.print("*");
      lcd.setCursor(7+m, 2);
      motorstatus = mcnt + 48;
      pass[m] = motorstatus;      
      m = m + 1;
      if (m > 5)
      {
        m = 0;
      }
      motorstatus = pass[m];
      mcnt = motorstatus % 48;      
      lcd.setCursor(7+m, 2);
      lcd.print(mcnt);
      lcd.setCursor(7+m, 2);      
      cnt = 0;  
      delay(50);    
   }  

   if (customKey == 'A')
   {
      lcd.setCursor(7+m, 2);
      lcd.print("*");
      lcd.setCursor(7+m, 2);
      motorstatus = mcnt + 48;
      pass[m] = motorstatus;      
      m = m - 1;
      if (m < 0)
      {
        m = 5;
      }
      motorstatus = pass[m];
      mcnt = motorstatus % 48;      
      lcd.setCursor(7+m, 2);
      lcd.print(mcnt);
      lcd.setCursor(7+m, 2);      
      cnt = 0;  
      delay(50);    
   }
      
   
   
   if (customKey == 'E')
   {
      tcnt = 0;
      motorstatus = pass[0];
      temp = motorstatus % 48;
      tcnt = temp * 100000;      
      motorstatus = pass[1];
      cnt = motorstatus % 48; //) * 10000);      
      cnt = cnt * 10000;      
      tcnt = tcnt + cnt;       
      motorstatus = pass[2];
      temp = motorstatus % 48; //) * 1000);
      cnt = temp * 1000;
      tcnt = tcnt + cnt;      
      motorstatus = pass[3];
      temp = motorstatus % 48; //) * 100);
      tcnt = tcnt + (temp * 100);      
      motorstatus = pass[4];
      temp = motorstatus % 48; //) * 10);
      tcnt = tcnt + (temp * 10);      
      motorstatus = pass[5];
      temp = motorstatus % 48;
      tcnt = tcnt + temp;      
      if (cbit == 1)
      {
        for (int i = 0; i <= 5; i++)
        {
          EEPROM.write(i, pass[i]);
          EEPROM.commit();
        }       
        tcnt = 1;
        return tcnt;
      }
      return tcnt;
   }

   if (customKey == '*')
   {
      ch = customKey;
      return tcnt;
   }   
   goto rploop;
}

void admin()
{ 
rptloop:  
 long te =  readeeprom();
 mcnt = 1 ;
 cnt = readpassword(0);
 if (ch == '*')
   {
      return;
   }
 if ((cnt == 1) | (cnt == te))
  {    
    lcd.clear(); 
    lcd.noCursor();
    lcd.setCursor(0, 2); 
    lcd.print(" Logged in.....    ");
    delay(2000);    
  }
  else
  {    
    lcd.clear(); 
    lcd.setCursor(0, 2); 
    lcd.print("    Code Err... ");
    delay(2000);
    goto rptloop;
  } 
  mcnt = 1;
ad1loop:
 lcd.noCursor();  
 motorstatus = 0; 
 lcd.clear(); 
 lcd.setCursor(0, 0);
 lcd.print("Settings            ");    
 lcd.setCursor(0, 1);
 lcd.print("   1.New Password.  ");
 lcd.setCursor(0, 2);
 lcd.print("   2.Invert Design. ");
 lcd.setCursor(0, 3);
 lcd.print("   3.Set Front Back.");
 lcd.setCursor(2, mcnt);
 lcd.write(0); 
 lcd.setCursor(19, 0);
 lcd.print(mcnt);

 lcd.clear(); 
 lcd.setCursor(0, 0);
 lcd.print("Settings            "); 
 if (mcnt < 4)
 {    
   lcd.setCursor(0, 1);
   lcd.print("   1.New Password.  ");  
   lcd.setCursor(0, 2);
   lcd.print("   2.Number of Card.");   
   lcd.setCursor(0, 3);
   lcd.print("   3.Invert Design. ");
   lcd.setCursor(2, mcnt);
   lcd.write(0); 
   lcd.setCursor(19, 0);
   lcd.print(mcnt);
 }
 else
 {    
   lcd.setCursor(0, 1);
   lcd.print("   2.Number of Card.");   
   lcd.setCursor(0, 2);
   lcd.print("   3.Invert Design. ");   
   lcd.setCursor(0, 3);
   lcd.print("   4.Set Front Back.");
   lcd.setCursor(2, 3);
   lcd.write(0);
 }
 lcd.setCursor(19, 0);
 lcd.print(mcnt);     
 
adloop: 
 customKey = customKeypad.getKey(); 
  if (customKey == 'B')
   {
     mcnt = mcnt + 1;
    if (mcnt > 4)
    {
      mcnt = 1;
    }
    lcd.setCursor(4, 1);
    lcd.print(" ");
    lcd.setCursor(4, 2);
    lcd.print(" ");
    lcd.setCursor(4, 3);
    lcd.print(" ");        
    lcd.setCursor(2, mcnt);
    lcd.write(0);          
    goto ad1loop;    
   }

  if (customKey == 'A')
   {
     mcnt = mcnt - 1;
    if (mcnt < 1)
    {
      mcnt = 4;
    }
    lcd.setCursor(4, 1);
    lcd.print(" ");
    lcd.setCursor(4, 2);
    lcd.print(" ");
    lcd.setCursor(4, 3);
    lcd.print(" ");    
    lcd.setCursor(2, mcnt);
    lcd.write(0);          
    goto ad1loop;    
   }
   
   if (customKey == 'E')
   {
      switch (mcnt)
       {        
        case 1:
                cnt = readpassword(1);             
                if (cnt == 1)
                {
                 lcd.clear(); 
                 lcd.setCursor(0, 1); 
                 lcd.print("Password Created... ");                
                 delay(2000);
                }
                mcnt = 1;
                goto ad1loop;
        case 2:
                lcd.clear(); 
                lcd.setCursor(0, 1); 
                numberofcard();
                lcd.noCursor();
                mcnt = 1;
                goto ad1loop;                
        case 3:
                lcd.clear(); 
                lcd.setCursor(0, 1); 
                //files();
                output1direction();
                goto ad1loop;          
        case 4:
                lcd.clear(); 
                Fbsettings();               
                mcnt = 1;
                goto ad1loop;                
       }
   }

   if (customKey == '*')
   {
      return;
   }   
   goto adloop;
}

void valdaycal()
{
 for (int i = 6; i <=13; i++)
    {
      byte tf = EEPROM.read(i);
      if ((tf >= 48) && (tf <= 57)) 
      {
       SysNo[i - 6] = EEPROM.read(i);
      }
      else
      {
       SysNo[i - 6] = '0';
       EEPROM.write(i, 48);
       EEPROM.commit();  
      }
      delay(20);
    }  
 //lcd.clear(); 
 //lcd.setCursor(0, 0); 
 //lcd.print("EEPROM VALUES:");
    
 motorstatus = SysNo[4];
 temp = motorstatus % 48;              
 lockvalue = temp; 
 
 motorstatus = SysNo[2];
 temp = motorstatus % 48;
 temp = temp * 10; 
 lockvalue = lockvalue + temp;

 motorstatus = SysNo[0];
 temp = motorstatus % 48; //) * 10);
 cnt = temp * 100;      
 lockvalue = lockvalue + cnt;
        
 //lcd.setCursor(0, 1);      
 //lcd.print("VALUE: ");
 //lcd.print(lockvalue); 

 motorstatus = SysNo[5];
 temp = motorstatus % 48;              
 lockdays = temp; 
 
 motorstatus = SysNo[3];
 temp = motorstatus % 48;
 temp = temp * 10; 
 lockdays = lockdays + temp;

 motorstatus = SysNo[1];
 temp = motorstatus % 48; //) * 10);
 cnt = temp * 100;      
 lockdays = lockdays + cnt;     
 

 //lcd.setCursor(0, 2);      
 //lcd.print("DAYS: ");
 //lcd.print(lockdays); 

 nextlockvalue = 0;
 
 motorstatus = SysNo[7];
 temp = motorstatus % 48;              
 nextlockvalue = lockvalue + temp;
 
}

void valdaycal1()
{   
 //lcd.clear(); 
 //lcd.setCursor(0, 0); 
 //lcd.print("EEPROM VALUES:");
    
 motorstatus = pass[4];
 temp = motorstatus % 48;              
 lockvalue1 = temp; 
 
 motorstatus = pass[2];
 temp = motorstatus % 48;
 temp = temp * 10; 
 lockvalue1 = lockvalue1 + temp;

 motorstatus = pass[0];
 temp = motorstatus % 48; //) * 10);
 cnt = temp * 100;      
 lockvalue1 = lockvalue1 + cnt;   
     
 //lcd.setCursor(0, 1);      
 //lcd.print("VALUE: ");
 //lcd.print(lockvalue1); 

 motorstatus = pass[5];
 temp = motorstatus % 48;              
 lockdays1 = temp; 
 
 motorstatus = pass[3];
 temp = motorstatus % 48;
 temp = temp * 10; 
 lockdays1 = lockdays1 + temp;

 motorstatus = pass[1];
 temp = motorstatus % 48; //) * 10);
 cnt = temp * 100;      
 lockdays1 = lockdays1 + cnt;     
 

 //lcd.setCursor(0, 2);      
 //lcd.print("DAYS: ");
 //lcd.print(lockdays1);  
}

void systemsettings()
{ 
 valdaycal();  
 delay(1000); 
 lcd.clear(); 
 lcd.setCursor(0, 0); 
 lcd.print("SYSTEM SETTINGS");
 lcd.setCursor(0, 1);
 lcd.print("1.SYS NO: ");
 for (int i = 0; i <=7; i++)
 lcd.print(SysNo[i]); 
 lcd.setCursor(0, 2);
 lcd.print("2.RD NO: ");
 lcd.print(lockdays);
 lcd.setCursor(5, 3);
 lcd.print(">00000000");
 Sysnumber = 0;
 Sysnumber = readvalue(3,6,8,Sysnumber);
 lcd.noCursor();
 lcd.noBlink();
 valdaycal1();
 //lcd.print(pass[7]);
 //longtochar(Sysnumber); 
 if ((nextlockvalue == lockvalue1) || (nextlockvalue == 0)) 
 {
  delay(20); 
  for (int i = 6; i <=13; i++)
  {
   EEPROM.write(i, pass[i - 6]);
   EEPROM.commit();
   delay(20); 
  }
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('L');
  Serial2.print(pass[6]);
  Serial2.print(pass[1]);
  Serial2.print(pass[3]);
  Serial2.print(pass[5]); 
  Serial2.print('{');
  Serial2.print('}');      
 }
 else
 {
  lcd.setCursor(5, 3);
  lcd.print("         ");  
  lcd.setCursor(5, 3);
  lcd.print("ERROR"); 
  delay(5000);
  //goto bb; 
 }
 delay(5000);
}

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
      } else {  //wrong byte reset index back to zero , over write by next time
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
        nc1 = 2;  //get advance index for next byte 
        break;
      } else {
        nc1 = 0;
        return;   // this make issue check>>>>
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
      
      if (nc1 >= (int)sizeof(data3) - 1) {
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
   Serial.print("RECEIVED PACKET: [");
  for (int i = 0; i < nc1 ; i++) {  
        inputString += (char)data3[i];   

  }
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
        if (e != -1) {
          String pick = inputString.substring(e + 1);
          pick.trim();
          filenum_m = pick.toInt();
          Serial.println("filenum RECEIVED");
        }
        goto rout;
      }

      // D 2 - Pick number
      if (data3[3] == 50) { // '2'
        int e = inputString.indexOf('*');
        if (e != -1) {
          String pick = inputString.substring(e + 1);
          pick.trim();
          pickno = pick.toInt();
          Serial.println("pickno RECEIVED");
        }
        goto rout;
      }

      // D 3 - Total pick
      if (data3[3] == 51) { // '3'
        int e = inputString.indexOf('*');
        if (e != -1) {
          String pick = inputString.substring(e + 1);
          pick.trim();
          height = pick.toInt();
          Serial.println("height RECEIVED");
        }
        goto rout;
      }

      // D 4 - Repeat count
      if (data3[3] == 52) { // '4'
        int e = inputString.indexOf('*');
        if (e != -1) {
          String pick = inputString.substring(e + 1);
          pick.trim();
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
        if (e != -1) {
          String pick = inputString.substring(e + 1);
          pick.trim();
          filenum1_m = pick.toInt();
          Serial.println("filenum1 RECEIVED");
        }
        goto rout;
      }

      // D 6 - Pick number 1
      if (data3[3] == 54) { // '6'
        int e = inputString.indexOf('*');
        if (e != -1) {
          String pick = inputString.substring(e + 1);
          pick.trim();
          pickno1 = pick.toInt();
          Serial.println("pickno1 RECEIVED");
        }
        goto rout;
      }

      // D 7 - Height 1
      if (data3[3] == 55) { // '7'
        int e = inputString.indexOf('*');
        if (e != -1) {
          String pick = inputString.substring(e + 1);
          pick.trim();
          height1 = pick.toInt();
          Serial.println("height1 RECEIVED");
        }
        goto rout;
      }

      // D 8 - Repeat count 1
      if (data3[3] == 56) { // '8'
        int e = inputString.indexOf('*');
        if (e != -1) {
          String pick = inputString.substring(e + 1);
          pick.trim();
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
      Serial.print(BtoTmode);
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
      if (e != -1) {
        String pick = inputString.substring(e + 1);
        pick.trim();
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
        if (e != -1) {
          String pick = inputString.substring(e + 1);
          pick.trim();
          totalfolder_m = pick.toInt();
          totalfolder = totalfolder_m;
          Serial.print("total folder:\t");
          Serial.println(totalfolder_m);
        }
      }

      // T L - Total files
      if (data3[3] == 76) { // 'L'
        int e = inputString.indexOf('L');
        if (e != -1) {
          String pick = inputString.substring(e + 1);
          pick.trim();
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
        
        if (idx_K != -1 && idx_star != -1 && idx_star > idx_K) {
          String idxStr = inputString.substring(idx_K + 1, idx_star);
          int folderIdx = idxStr.toInt();
          String folderName = inputString.substring(idx_star + 1 , inputString.length() - 2);
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
        
        if (idx_M != -1 && idx_star != -1 && idx_star > idx_M) {
          String idxStr = inputString.substring(idx_M + 1, idx_star);
          int fileIdx = idxStr.toInt();
          String fileName = inputString.substring(idx_star + 1 , inputString.length() - 2);
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
      if (e != -1) {
        String pick = inputString.substring(e + 1);
        pick.trim();
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
        if (e != -1) {
          //String pick = inputString.substring(e + 1);
          String pick = inputString.substring(e + 1, inputString.length() - 2);
          pick.trim();
          if (lcdcnt >= 4) {
            lcd.clear();
            lcdcnt = 0;
          }
          lcd.setCursor(0, lcdcnt);
          lcd.print(pick);
          lcdcnt = lcdcnt + 1;
          Serial.print("LCD[");
          Serial.print(lcdcnt - 1);
          Serial.print("]: [");
          Serial.print(pick);
          Serial.println("]");
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

void communicationchecking() {
  byte temp1 = 0;
  byte jj1 = 0;
  unsigned long jj = 0;

jk1:
  out[0] = 'A';
  out[1] = 'B';
  out[2] = 'C'; 
  out[3] = '0';
  out[4] = 'F';
  out[5] = '{';
  out[6] = '}';

  // Send command
  for(int m=0; m < 7; m++) {
    Serial.print(m);
    Serial.print(" :");
    Serial2.print((char)out[m]);
    Serial.println((char)out[m]);
  }

  temp1 = 0;
  while (true) {
    // ONLY read if data is actually available in the serial buffer
    if (Serial2.available() > 0) {
      byte inbyte = (byte)Serial2.read(); 
      data3[temp1] = inbyte;
      Serial.println(data3[temp1]);
      temp1 = temp1 + 1;

      // Safe index check: Ensure we have received at least 2 bytes first
      if (temp1 >= 2) {
        if (data3[temp1-2] == 69) {     // 'E'
          if (data3[temp1-1] == 74) {   // 'J'
            Serial.println("Communicate Successfully");
            break; // Exit the loop and continue scanning on next call
          }
        }
      }
    }

    // Timeout counter increments only when waiting
    jj = jj + 1;
    delayMicroseconds(100); // Small delay to prevent runaway counter loops

    if (jj > 5000) {
      jj = 0;
      jj1 = jj1 + 1;
      if (jj1 > 3) {
        jj1 = 0;
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("Switch OFF ");
        lcd.setCursor(9, 2);
        lcd.print("&");
        lcd.setCursor(1, 3);
        lcd.print("Check Signal Cable");
        
        delay(5000); // Wait 5 seconds on error UI before retrying continuous scan
        goto jk1;    // Retry instead of freezing forever
      } else {
        goto jk1;
      }
    }
  }
  return;
}

void cationchecking() {
    static enum { SEND_CMD, WAIT_RESPONSE, COMM_FAILED } state = SEND_CMD;
    static unsigned long timeoutCounter = 0;
    static byte retryCount = 0;
    static byte bufferIndex = 0;

    const byte cmd[] = {'A', 'B', 'C', '0', 'F', '{', '}'};

    switch (state) {
        case SEND_CMD:
            // Send the 7-byte command via HardwareSerial
            for (int m = 0; m < 7; m++) {
                Serial2.write(cmd[m]);
            }
            bufferIndex = 0;
            timeoutCounter = 0;
            state = WAIT_RESPONSE;
            break;

        case WAIT_RESPONSE:
            // Read data only if it is available in the buffer
            while (Serial2.available() > 0 && bufferIndex < sizeof(data3)) {
                data3[bufferIndex] = (byte)Serial2.read();
                
                // Print received byte for debugging
                Serial.println(data3[bufferIndex]);
                bufferIndex++;

                // Check for termination sequence 'E' (69) and 'J' (74)
                if (bufferIndex >= 2) {
                    if (data3[bufferIndex - 2] == 69 && data3[bufferIndex - 1] == 74) {
                        Serial.println("Communicate Successfully");
                        state = SEND_CMD; // Reset for the next continuous scan cycle
                        return;
                    }
                }
            }

            // Non-blocking timeout calculation loop
            timeoutCounter++;
            if (timeoutCounter > 5000) {
                timeoutCounter = 0;
                retryCount++;
                
                if (retryCount > 3) {
                    state = COMM_FAILED; // Switch to alert state
                } else {
                    state = SEND_CMD;    // Retry sending command
                }
            }
            break;

        case COMM_FAILED:
            // Alert user without locking up the entire system
            static unsigned long lcdUpdateTimer = 0;
            if (millis() - lcdUpdateTimer > 1000) { 
                lcd.clear();
                lcd.setCursor(6, 1);
                lcd.print("Switch OFF");
                lcd.setCursor(9, 2);
                lcd.print("&");
                lcd.setCursor(1, 3);
                lcd.print("Check Signal Cable");
                lcdUpdateTimer = millis();
            }
            // To auto-recover later if needed, change state to SEND_CMD here
            break;
    }
}

void numberofcard()
{
 lcd.clear(); 
 lcd.setCursor(0, 0); 
 lcd.print("No Of Cards:-       ");
 lcd.setCursor(0, 1);
 lcd.print("       Card=00    ");
 lcd.setCursor(6, 1);
 lcd.write(0);
 nc = readvalue(1,12,2,nc);
 if (customKey == '*')
  {
   customKey == ' '; 
   lcd.noCursor();
   lcd.noBlink();   
   goto jk12;
  } 
  out[0] = 'A';
  out[1] = 'B';
  out[2] = 'n';  //M for Mode change 
  out[3] = nc;            
  out[4] = 'F';
  out[5] = '{';
  out[6] = '}';
  for(int m=0; m < 7; m++)
  {
   Serial.print(m);  
   Serial.print(" :");               
   Serial2.print(out[m]);
   Serial.println(out[m]);       
  } 
 jk12:
  lcd.noCursor();
  lcd.noBlink(); 
  delay(100);                
}

void output1direction()
{  
d1loop:  
 temp = BtoTmode; 
 mcnt = 1;  
 lcd.clear(); 
 lcd.setCursor(0, 0); 
 lcd.print("DESIGN INVERT       ");
 lcd.setCursor(0, 1);
 lcd.print("       1.ON    ");
 lcd.setCursor(0, 2);
 lcd.print("       2.OFF   ");
 if (temp == 1)
 {
  lcd.setCursor(0, 1);
  lcd.print("       1.ON ");
  lcd.write(1);
 }
 else
 {
  lcd.setCursor(0, 2);
  lcd.print("       2.OFF ");
  lcd.write(1);
 }
 lcd.setCursor(6, mcnt);
 lcd.write(0);
floop: 
 customKey = customKeypad.getKey(); 
  if (customKey == 'B')
   {
    mcnt = mcnt + 1;
    if (mcnt > 2)
    {
     mcnt = 1;
    }
    lcd.setCursor(6, 1);
    lcd.print(" ");
    lcd.setCursor(6, 2);
    lcd.print(" ");    
    lcd.setCursor(6, mcnt);
    lcd.write(0);    
   }

  if (customKey == 'A')
   {
    mcnt = mcnt - 1;
    if (mcnt < 1)
    {
     mcnt = 2;
    }
    lcd.setCursor(6, 1);
    lcd.print(" ");
    lcd.setCursor(6, 2);
    lcd.print(" ");    
    lcd.setCursor(6, mcnt);
    lcd.write(0);    
   }
   
   if (customKey == 'E')
   {
      switch (mcnt)
       {        
        case 1:
              BtoTmode = 1;
              //EEPROM.write(33, 0);
              out[0] = 'A';
              out[1] = 'B';
              out[2] = 'b';  //M for Mode change 
              out[3] = '1';            
              out[4] = 'F';
              out[5] = '{';
              out[6] = '}';
              for(int m=0; m < 7; m++)
              {
               Serial.print(m);  
               Serial.print(" :");               
               Serial2.print(out[m]);
               Serial.println(out[m]);       
              }              
              return;
        case 2:
              BtoTmode = 0;
              //EEPROM.write(33, 1);
              out[0] = 'A';
              out[1] = 'B';
              out[2] = 'b';  //M for Mode change 
              out[3] = '0';            
              out[4] = 'F';
              out[5] = '{';
              out[6] = '}';
              for(int m=0; m < 7; m++)
              {
               Serial.print(m);  
               Serial.print(" :");               
               Serial2.print(out[m]);
               Serial.println(out[m]);       
              }
              return;      
       }
   }

   if (customKey == '*')
   {
      return;
   }   
   goto floop;
}

void runningstatus()
{                             
  out[0] = 'A';
  out[1] = 'B';
  out[2] = 'R';  //M for Mode change 
  out[3] = '1';            
  out[4] = 'F';
  out[5] = '{';
  out[6] = '}';
  for(int m=0; m < 7; m++)
  {
   Serial.print(m);  
   Serial.print(" :");               
   Serial2.print(out[m]);
   Serial.println(out[m]);       
  }           
}

void masterinit()
{  
  out[3] = '1';                           
  out[0] = 'A';
  out[1] = 'B';
  out[2] = 'I';  //I for Master Init             
  out[4] = 'F';
  out[5] = '{';
  out[6] = '}';
  for(int m=0; m < 7; m++)
  {
   Serial.print(m);  
   Serial.print(" :");               
   Serial2.print(out[m]);
   Serial.println(out[m]);    
        

  }           
   Serial.println("masterinitpass\n"); 
}

void files()
{  
f1loop:  
 mcnt = 1;  
 lcd.clear(); 
 lcd.setCursor(0, 0); 
 lcd.print("Running Type       ");
 lcd.setCursor(0, 1);
 lcd.print("       1.Single. ");
 lcd.setCursor(0, 2);
 lcd.print("       2.Dual.   ");
 lcd.setCursor(6, mcnt);
 lcd.write(0);
 if (filerunningmode == 2)
 {
  lcd.setCursor(15, filerunningmode);
  lcd.write(1);   
 }
 else
 {
  lcd.setCursor(17, filerunningmode);
  lcd.write(1);
 }
floop: 
 customKey = customKeypad.getKey(); 
  if (customKey == 'B')
   {
    mcnt = mcnt + 1;
    if (mcnt > 2)
    {
     mcnt = 1;
    }
    lcd.setCursor(6, 1);
    lcd.print(" ");
    lcd.setCursor(6, 2);
    lcd.print(" ");    
    lcd.setCursor(6, mcnt);
    lcd.write(0);
    lcd.setCursor(19, 0);
    lcd.print(mcnt);
   }

  if (customKey == 'A')
   {
      mcnt = mcnt - 1;
      if (mcnt < 1)
      {
        mcnt = 2;
      }
      lcd.setCursor(6, 1);
      lcd.print(" ");
      lcd.setCursor(6, 2);
      lcd.print(" ");    
      lcd.setCursor(6, mcnt);
      lcd.write(0);
      lcd.setCursor(16, 0);
      lcd.print(mcnt);
   }
   
   if (customKey == 'E')
   {
      switch (mcnt)
       {        
        case 1:         
                                         
              out[0] = 'A';
              out[1] = 'B';
              out[2] = 'M';  //M for Mode change    
              out[3] = '1';         
              out[4] = 'F';
              out[5] = '{';
              out[6] = '}';
              for(int m=0; m < 7; m++)
              {
               Serial.print(m);  
               Serial.print(" :");               
               Serial2.print(out[m]);
               Serial.println(out[m]);       
              }           
               return;
        case 2:                                                       
               out[0] = 'A';
               out[1] = 'B';
               out[2] = 'M';  //M for Mode change  
               out[3] = '2';           
               out[4] = 'F';
               out[5] = '{';
               out[6] = '}';
               for(int m=0; m < 7; m++)
               {
                Serial.print(m);  
                Serial.print(" :");               
                Serial2.print(out[m]);
                Serial.println(out[m]);       
               }           
               return;
       }       
   }

   if (customKey == '*')
   {
      return;
   }   
   goto floop;
}

int delfileselection()
{  
 temp = 0;  
 lcd.clear();  
 lcd.setCursor(0, 0);
 lcd.print("Select File  <  /  >");   
 int ck1 = 0;  
 if (totalfile_m > 9)
   {
    lcd.setCursor(18, 0);
    lcd.print("  ");
    lcd.setCursor(17, 0);
   } 
   else
   {
    lcd.setCursor(18, 0);    
   } 
 lcd.print(totalfile_m + 1); 
 lcd.setCursor(6, 2);
 lcd.print(">"); 
 
s1loop1:
   if (ck1 > 9)
   {
    lcd.setCursor(15, 0);
    lcd.print("  ");
    lcd.setCursor(14, 0);
   } 
   else
   {
    lcd.setCursor(15, 0);    
   }
      
   lcd.print(ck1 + 1); 
      
   if (ck1 == 0)
   {
    lcd.setCursor(0, 1);
    lcd.print("                    ");     
   }
   else
   {
    lcd.setCursor(7, 1);
    lcd.print("             ");
    lcd.setCursor(7, 1);
    lcd.print(filelist_m[ck1-1]);
   }
 
  lcd.setCursor(7, 2);
  lcd.print("             ");
  lcd.setCursor(7, 2);
  lcd.print(filelist_m[ck1]);
  
  if (ck1 == totalfile_m)
   {
    lcd.setCursor(0, 3);
    lcd.print("                    "); 
   }
   else
   {
    lcd.setCursor(7, 3);
    lcd.print("             ");
    lcd.setCursor(7, 3);
    lcd.print(filelist_m[ck1+1]);
   }   
 
sloop1: 
 char customKey = customKeypad.getKey(); 
  if (customKey == 'B')
   {
    ck1 += 1;
    if (ck1 >= totalfile_m)
    {
      ck1 = totalfile_m;  
    }
    goto s1loop1;
   }

  if (customKey == 'A')
   {
      ck1 -= 1;
      if (ck1 <= 0)
      {
        ck1 = 0;  
      }
      goto s1loop1;
   }
   
   if (customKey == 'E')
   {    
    return ck1;
   }

   if (customKey == '*')
   {
    temp = 1;
    return ck1;
   }   
   goto sloop1; 
}

void filecopymenu()
{
  delay(100);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Open SDCard.....");  
  lcd.setCursor(0, 2);  
  /*f (!SD.begin(5)) {
    lcd.print("No SDCard in Slot");
    delay(1000);    
   sd:
    //delay(100);
    if (!SD.begin(5)) {
        goto sd;
    }        
  } */
  delay(100);
  lcd.setCursor(0, 0);
  lcd.print("Openning  SDCard"); 
  delay(200); 
  lcd.setCursor(0, 1);
  lcd.print("Loading.....        ");
  delay(100);  
  foldername1 ="";    
 // root3 = SD.open("/");
  totalfile = 0;
  totalfolder = 0;
  readfolderlist(root3); 
  delay(100);    
  folderselection1();  
  if (esc == 1)
  return;
  delay(100);    
  //root = SD.open(String("/" + foldername1 + "/"));   
  //root = SD.open("/");   
  for (uint8_t i = 0; i <= totalfolder; i++) 
  {
   File entry =  root.openNextFile();
   if (foldername1.equalsIgnoreCase(entry.name()))
   { 
    opennext(entry);
    root.close();
    entry.close();
    break;
   }
  }    
  delay(10);
  Serial.print("totalfile: ");
  Serial.println(totalfile);  
  delay(100);    
  //opennext(root);
  delay(100);        
  if (totalfile == 0)
  {
   lcd.clear();
   lcd.setCursor(5, 1);
   lcd.print("No Files.....  ");   
   delay(2000);
   return;
  }
  else
  {
   fileselection1(); 
   //if (pulcnt = 1)      
   //resetFunc();
  }  
}


void Deletemenu()
{  
 mcnt = 1;  
 d = 0;  
 long qo1 = 0;                            
delmnu: 
 lcd.clear(); 
 lcd.setCursor(0, 0); 
 lcd.print("Delete Menu    <1/2>");
 lcd.setCursor(0, 1);
 lcd.print("   1.Delete File.  ");
 lcd.setCursor(0, 2);
 lcd.print("   2.Delete Folder.");
 if (temp == 0)
 {
  lcd.setCursor(0, 1);
  lcd.print("   1.Delete File.  ");
 }
 else
 {
  lcd.setCursor(0, 2);
  lcd.print("   2.Delete Folder.");
 }
 lcd.setCursor(2, mcnt);
 lcd.print(">"); 
floop1: 
 customKey = customKeypad.getKey(); 
  if (customKey == 'B')
   {
      mcnt = mcnt + 1;
      if (mcnt > 2)
      {
        mcnt = 1;
      }
      lcd.setCursor(2, 1);
      lcd.print(" ");
      lcd.setCursor(2, 2);
      lcd.print(" ");    
      lcd.setCursor(2, mcnt);
      lcd.print(">");
      lcd.setCursor(16, 0);
      lcd.print(mcnt);
   }

  if (customKey == 'A')
   {
      mcnt = mcnt - 1;
      if (mcnt < 1)
      {
        mcnt = 2;
      }
      lcd.setCursor(2, 1);
      lcd.print(" ");
      lcd.setCursor(2, 2);
      lcd.print(" ");    
      lcd.setCursor(2, mcnt);
      lcd.print(">");
      lcd.setCursor(16, 0);
      lcd.print(mcnt);
   }
   
   if (customKey == 'E')
   {
      switch (mcnt)
       {        
        case 2:
                tempq = 0;                  
                tempq = folderselection();
                qo1 = 0;
                if (customKey == '*')
                {
                 goto delmnu;
                }
                d = 1; 
                out[0] = 'A';
                out[1] = 'B';
                out[2] = 'd';  //d - Delete Option
                out[3] = '1';  //1 -  Folder Delete Option
                out[4] = tempq + 48;                
                out[5] = '{';
                out[6] = '}';
                for(int m=0; m < 7; m++)
                {
                 Serial.print(m);  
                 Serial.print(" :");               
                 Serial2.print(out[m]);
                 Serial.println(out[m]);       
                }
                
                while (true)
                {
                  qo1 = qo1 + 1;
                 if (Serial2.available() > 0)
                  {
                   byte inbyte = (byte)Serial2.read();          
                   data1[temp1] = inbyte;                      
                   temp1 = temp1 + 1;   
                   if (data1[temp1-2] == 68)
                   {
                    if (data1[temp1-1] == 49)
                    {     
                     lcd.setCursor(0, 1);
                     lcd.print("      ");       
                     lcd.setCursor(0, 1);
                     lcd.print("YES   ");   
                     delay(1000);   
                     goto delmnu;          
                    }
                    if (data1[temp1-1] == 50)
                    {
                     lcd.setCursor(0, 1);
                     lcd.print("      ");       
                     lcd.setCursor(0, 1);
                     lcd.print("XXXXXX");            
                     delay(1000);
                     goto delmnu;     
                    }                    
                   }
                  }                  
                  if (qo1 >= 50000)
                  {
                    goto delmnu;
                  }
                }         
                
        case 1:                 
                tempq = 0;                       
                Serial.println("Delete file menu.");                       
        delmnu1:                
                tempq = delfileselection(); 
                if (temp == 1)
                {
                 d = 0; 
                 temp = 0;
                 goto delmnu;
                }
                qo1 = 0;
                d = 1; 
                out[0] = 'A';
                out[1] = 'B';
                out[2] = 'd';  //d - Delete Option
                out[3] = '2';  //2 -  File Delete Option
                out[4] = tempq + 48;                
                out[5] = '{';
                out[6] = '}';
                for(int m=0; m < 7; m++)
                {
                 Serial.print(m);  
                 Serial.print(" :");               
                 Serial2.print(out[m]);
                 Serial.println(out[m]);       
                }
                
                while (true)
                {
                  qo1 = qo1 + 1;
                  if (Serial2.available() > 0)
                  {
                   byte inbyte = (byte)Serial2.read();          
                   data1[temp1] = inbyte;                      
                   temp1 = temp1 + 1;   
                   if (data1[temp1-2] == 68)
                   {
                    if (data1[temp1-1] == 49)
                    {     
                     lcd.setCursor(0, 1);
                     lcd.print("      ");       
                     lcd.setCursor(0, 1);
                     lcd.print("YES");   
                     delay(1000);   
                     goto delmnu1;          
                    }
                    if (data1[temp1-1] == 50)
                    {
                     lcd.setCursor(0, 1);
                     lcd.print("      ");       
                     lcd.setCursor(0, 1);
                     lcd.print("XXXXXX");            
                     delay(1000);
                     goto delmnu1;     
                    }                    
                   }
                  }
                  delay(10);
                  if (qo1 >= 50000)
                  {
                    goto delmnu1;
                  }
                }              
       }
   }

   if (customKey == '*')
   {    
    if (d == 1)
    resetFunc();       
    d = 0;       
    return;
   }   
   goto floop1;
}



void Editmenu()

{  

    // --- USB folder scan/display (moved out of loop()'s dead test block) ---
  if (!usbWaitForMount())
  {
    return;   // '*' pressed while waiting for USB -> Editmenu returns ->
              // mainmenu() case 3 falls through to "mcnt = 1; goto menu;" anyway
  }
  usbScanFolders();
   //usbPrintFolderListLCD();
    usbBrowseFolders();
  delay(3000);   // pause so you can read the folder list before Copy/Delete menu draws
  // --- end USB block ---



//  mcnt = 1; 
// edmnu:   
//  lcd.clear(); 
//  lcd.setCursor(0, 0); 
//  lcd.print("File Menu           ");
//  lcd.setCursor(0, 1);
//  lcd.print("       1.Copy.      ");
//  lcd.setCursor(0, 2);
//  lcd.print("       2.Delete.    ");
//  if (temp == 0)
//  {
//   lcd.setCursor(0, 1);
//   lcd.print("       1.Copy.      ");
//  }
//  else
//  {
//   lcd.setCursor(0, 2);
//   lcd.print("       2.Delete.    ");
//  }
//  lcd.setCursor(6, mcnt);
//  lcd.write(0); 
 
// while (true)
// {
//  customKey = customKeypad.getKey(); 
//   if (customKey == 'B')
//    {
//       mcnt = mcnt + 1;
//       if (mcnt > 2)
//       {
//         mcnt = 1;
//       }
//       lcd.setCursor(6, 1);
//       lcd.print(" ");
//       lcd.setCursor(6, 2);
//       lcd.print(" ");    
//       lcd.setCursor(6, mcnt);
//       lcd.write(0);
      
//    }

//   if (customKey == 'A')
//    {
//       mcnt = mcnt - 1;
//       if (mcnt < 1)
//       {
//         mcnt = 2;
//       }
//       lcd.setCursor(6, 1);
//       lcd.print(" ");
//       lcd.setCursor(6, 2);
//       lcd.print(" ");    
//       lcd.setCursor(6, mcnt);
//       lcd.write(0);      
//    }
   
//    if (customKey == 'E')
//    {
//       switch (mcnt)
//        {        
//         case 1:
//                 filecopymenu();  
//                 delay(500);
//                 Serial2.print('A');
//                 Serial2.print('B');
//                 Serial2.print('r');
//                 Serial2.print('1');
//                 Serial2.print('*'); 
//                 Serial2.print('1');
//                 Serial2.print('*');                 
//                 Serial2.print('#');
//                 Serial2.print('{');
//                 Serial2.print('}');  
//                 delay(1000);
//                 resetFunc();                           
//         case 2:        
//                 Deletemenu();
//                 delay(500);
//                 Serial2.print('A');
//                 Serial2.print('B');
//                 Serial2.print('r');
//                 Serial2.print('1');
//                 Serial2.print('*'); 
//                 Serial2.print('1');
//                 Serial2.print('*');                 
//                 Serial2.print('#');
//                 Serial2.print('{');
//                 Serial2.print('}');   
//                 delay(1000);
//                 resetFunc();
//        }
//    }

   if (customKey == '*')
   {
    //  break;                   
   }   
  }
void mainmenu()
{  
 byte l1 = 0;   
 mcnt = 1;  
menu: 
 lcd.clear(); 
 lcd.setCursor(0, 0);
 lcd.print("Menu:-              "); 
 if (mcnt < 4)
 {    
   lcd.setCursor(0, 1);
   lcd.print("     1.Type.        ");
   lcd.setCursor(0, 2);
   lcd.print("     2.Folder.      ");
   lcd.setCursor(0, 3);
   lcd.print("     3.File Manager.");
   lcd.setCursor(4, mcnt);      
   lcd.write(0);
 }
 else
 {    
   lcd.setCursor(0, 1);
   lcd.print("     4.Settings.    ");
   lcd.setCursor(0, 2);
   lcd.print("     5.Test Files.  ");
   lcd.setCursor(0, 3);
   lcd.print("     6.Finger.      ");
   l1 = mcnt - 3;
   lcd.setCursor(4, l1);            
   lcd.write(0);
 }
 lcd.setCursor(19, 0);
 lcd.print(mcnt);     
mloop:
 customKey = customKeypad.getKey(); 
 if (customKey == 'B')
 {
    mcnt = mcnt + 1;
    if (mcnt > 6)
    {
      mcnt = 1;
    }
    lcd.setCursor(4, 1);
    lcd.print(" ");
    lcd.setCursor(4, 2);
    lcd.print(" ");
    lcd.setCursor(4, 3);
    lcd.print(" ");      
    if (mcnt < 3)
    {
     lcd.setCursor(4, mcnt);
     lcd.write(0);  
    }
    else
    {
      l1 = mcnt - 3;
      lcd.setCursor(4, l1);
      lcd.write(0);
    }    
  goto menu;   
 }

 if (customKey == 'A')
   {
      mcnt = mcnt - 1;
      if (mcnt < 1)
      {
        mcnt = 6;
      }
      lcd.setCursor(4, 1);
      lcd.print(" ");
      lcd.setCursor(4, 2);
      lcd.print(" ");
      lcd.setCursor(4, 3);
      lcd.print(" ");    
      if (mcnt < 3)
      {
       lcd.setCursor(4, mcnt);
       lcd.write(0);  
      }
      else
      {
       l1 = mcnt - 3;
       lcd.setCursor(4, l1);        
       lcd.write(0);
      }      
    goto menu;   
   }
 
 if (customKey == 'E')
 {
    switch (mcnt)
     {
      case 1:
            lcd.clear(); 
            lcd.setCursor(0, 1); 
            files();
            mcnt = 1;         
            goto menu;


       
      case 2:
          Serial.println("case 2 start");  Serial.println(tempg1);
            tempg1 += 1;             
           
            Serial.println(tempg1);
            ls_name = folderlist_m[foldernum];    //read current folder 
           
              Serial.println("after folderlist_m ");
              Serial.println(tempg1);     
                  
            tempq = folderselection();            //select another folder , i command this function folder read case only 
              Serial.println("after folderselection ");
              Serial.println(tempq);     
                   
            s_name = folderlist_m[tempq];
              
            Serial.println("Selected folder index: ");
              Serial.println(tempq);  
               Serial.println("Target folder name: ");
                Serial.println(s_name); 
            if (customKey == '*')
            {
             return;
            }
            if (ls_name.equalsIgnoreCase(s_name)) //issue is here
            {             
             return;
            } else {
               Serial.println(" PACKED IS SEND TO MAIN ");
               Serial.print("SENDING: ABF");
               Serial.print(tempq);
                Serial.println("#{}"); 
               
               // Send folder change atomically
               char fbuf[50];
           //    snprintf(fbuf, sizeof(fbuf), "ABF%d#&", tempq);
              
              snprintf(fbuf, sizeof(fbuf), "ABF%d#{}", tempq);
               for (int i = 0; fbuf[i] != '\0'; i++) {
                 Serial2.write((byte)fbuf[i]);
               }
               Serial2.flush();
             
            }                        
            return;        
      case 3:
           Editmenu();   
           mcnt = 1;         
           goto menu;                 
      case 4:
           admin(); 
           lcd.noCursor();
           mcnt = 1;           
           goto menu;   
      case 5:
           Testfiles();
           mcnt = 1;          
           goto menu;
      case 6:
           systemsettings();  
           mcnt = 1;          
           goto menu;               
     }      
 }

 if (customKey == '*')
 {
    return;
 }
 goto mloop;   
}

void Fbsettings()
{      
 mcnt = LtoRmode;  
 if (mcnt < 1)
 {
  mcnt = 1;
  LtoRmode = 1;
 } 
menu6: 
 lcd.clear(); 
 lcd.setCursor(0, 0);
 lcd.print("FB Settings..       "); 
 if (mcnt < 4)
 {    
   lcd.setCursor(0, 1);
   lcd.print("    1.Front Left.   ");
   if (LtoRmode == 1)
   {
    lcd.setCursor(18, 1);
    lcd.write(1);
   }
   lcd.setCursor(0, 2);
   lcd.print("    2.Front Right.  ");
   if (LtoRmode == 2)
   {
    lcd.setCursor(19, 2);
    lcd.write(1);
   }
   lcd.setCursor(0, 3);
   lcd.print("    3.Back Left.    ");
   if (LtoRmode == 3)
   {
    lcd.setCursor(17, 3);
    lcd.write(1);
   }
   lcd.setCursor(3, mcnt);
   lcd.write(0);
 }
 else
 {    
   lcd.setCursor(0, 1);
   lcd.print("    2.Front Right.  ");
   if (LtoRmode == 2)
   {
    lcd.setCursor(19, 1);
    lcd.write(1);
   }
   lcd.setCursor(0, 2);
   lcd.print("    3.Back Left.    ");
   if (LtoRmode == 3)
   {
    lcd.setCursor(17, 2);
    lcd.write(1);
   }
   lcd.setCursor(0, 3);
   lcd.print("    4.Back Right.   ");
   if (LtoRmode == 4)
   {
    lcd.setCursor(18, 3);
    lcd.write(1);
   }
   lcd.setCursor(3, 3);
   lcd.write(0);
 }
 lcd.setCursor(19, 0);
 lcd.print(mcnt);     
mloop6:
 customKey = customKeypad.getKey(); 
 if (customKey == 'B')
 {
    mcnt = mcnt + 1;
    if (mcnt > 4)
    {
      mcnt = 1;
    }
    lcd.setCursor(5, 1);
    lcd.print(" ");
    lcd.setCursor(5, 2);
    lcd.print(" ");
    lcd.setCursor(5, 3);
    lcd.print(" ");      
    if (mcnt < 3)
    {
     lcd.setCursor(3, mcnt);
     lcd.write(0);
    }
    else
    {
      lcd.setCursor(3, 3);
      lcd.write(0);
    }    
  goto menu6;   
 }

 if (customKey == 'A')
   {
      mcnt = mcnt - 1;
      if (mcnt < 1)
      {
        mcnt = 4;
      }
      lcd.setCursor(5, 1);
      lcd.print(" ");
      lcd.setCursor(5, 2);
      lcd.print(" ");
      lcd.setCursor(5, 3);
      lcd.print(" ");    
      if (mcnt < 3)
      {
       lcd.setCursor(3, mcnt);
       lcd.write(0);
      }
      else
      {
       lcd.setCursor(3, 3);
       lcd.write(0);
      }      
    goto menu6;   
   }
 
 if (customKey == 'E')
 {
    switch (mcnt)
     {      
      case 1:
              out[0] = 'A';
              out[1] = 'B';
              out[2] = 'i';  //M for Mode change 
              out[3] = '1';            
              out[4] = 'F';
              out[5] = '{';
              out[6] = '}';
              for(int m=0; m < 7; m++)
              {
               Serial.print(m);  
               Serial.print(" :");               
               Serial2.print(out[m]);
               Serial.println(out[m]);       
              }
              LtoRmode = 1;
             mcnt = 1;         
             goto menu6;        
      case 2:
              out[0] = 'A';
              out[1] = 'B';
              out[2] = 'i';  //M for Mode change 
              out[3] = '2';            
              out[4] = 'F';
              out[5] = '{';
              out[6] = '}';
              for(int m=0; m < 7; m++)
              {
               Serial.print(m);  
               Serial.print(" :");               
               Serial2.print(out[m]);
               Serial.println(out[m]);       
              }
              LtoRmode = 2;
             mcnt = 2;         
             goto menu6;        
      case 3:
              out[0] = 'A';
              out[1] = 'B';
              out[2] = 'i';  //M for Mode change 
              out[3] = '3';            
              out[4] = 'F';
              out[5] = '{';
              out[6] = '}';
              for(int m=0; m < 7; m++)
              {
               Serial.print(m);  
               Serial.print(" :");               
               Serial2.print(out[m]);
               Serial.println(out[m]);       
              }
              LtoRmode = 3;
             mcnt = 3;          
             goto menu6;        
      case 4:
              out[0] = 'A';
              out[1] = 'B';
              out[2] = 'i';  //M for Mode change 
              out[3] = '4';            
              out[4] = 'F';
              out[5] = '{';
              out[6] = '}';
              for(int m=0; m < 7; m++)
              {
               Serial.print(m);  
               Serial.print(" :");               
               Serial2.print(out[m]);
               Serial.println(out[m]);       
              }
             lcd.noCursor();
             LtoRmode = 4;
             mcnt = 4;           
             goto menu6;        
     }      
 }

 if (customKey == '*')
 {
    return;
 }
 goto mloop6;   
}

void Testfiles()
{ 
 mcnt = 1;
ad1loop7:
 lcd.noCursor();    
 lcd.clear(); 
 lcd.setCursor(0, 0);
 lcd.print("Manual Test File    ");    
 lcd.setCursor(0, 1);
 lcd.print("   1.Plain File.    ");
 lcd.setCursor(0, 2);
 lcd.print("   2.All Up.        ");
 lcd.setCursor(0, 3);
 lcd.print("   3.All Down.      ");
 lcd.setCursor(2, mcnt);
 lcd.write(0); 
 lcd.setCursor(19, 0);
 lcd.print(mcnt);
adloop7: 
 customKey = customKeypad.getKey(); 
  if (customKey == 'B')
   {
     mcnt = mcnt + 1;
    if (mcnt > 3)
    {
      mcnt = 1;
    }
    lcd.setCursor(4, 1);
    lcd.print(" ");
    lcd.setCursor(4, 2);
    lcd.print(" ");
    lcd.setCursor(4, 3);
    lcd.print(" ");        
    lcd.setCursor(2, mcnt);
    lcd.write(0);          
    goto ad1loop7;    
   }

  if (customKey == 'A')
   {
     mcnt = mcnt - 1;
    if (mcnt < 1)
    {
      mcnt = 3;
    }
    lcd.setCursor(4, 1);
    lcd.print(" ");
    lcd.setCursor(4, 2);
    lcd.print(" ");
    lcd.setCursor(4, 3);
    lcd.print(" ");    
    lcd.setCursor(2, mcnt);
    lcd.write(0);          
    goto ad1loop7;    
   }
   
   if (customKey == 'E')
   {
      switch (mcnt)
       {        
        case 1: 
                out[0] = 'A';
                out[1] = 'B';
                out[2] = 'T';  //M for Mode change 
                out[3] = '1';            
                out[4] = 'F';
                out[5] = '{';
                out[6] = '}';
                for(int m=0; m < 7; m++)
                {
                 Serial.print(m);  
                 Serial.print(" :");               
                 Serial2.print(out[m]);
                 Serial.println(out[m]);       
                } 
                delay(200);
                lcd.noCursor();    
                lcd.clear(); 
                lcd.setCursor(0, 0);
                lcd.print("Manual Test File    ");    
                lcd.setCursor(0, 2);
                lcd.print("  1.Plain Test File.");
                while(true)
                {
                 customKey = customKeypad.getKey();  
                 if (customKey == '*')
                 {
                  out[0] = 'A';
                  out[1] = 'B';
                  out[2] = 'T';  //M for Mode change 
                  out[3] = '4';            
                  out[4] = 'F';
                  out[5] = '{';
                  out[6] = '}';
                  for(int m=0; m < 7; m++)
                  {
                   Serial.print(m);  
                   Serial.print(" :");               
                   Serial2.print(out[m]);
                   Serial.println(out[m]);       
                  }
                  goto ad1loop7;
                 }                
                }
                mcnt = 1;
                goto ad1loop7;
        case 2:
                out[0] = 'A';
                out[1] = 'B';
                out[2] = 'T';  //M for Mode change 
                out[3] = '2';            
                out[4] = 'F';
                out[5] = '{';
                out[6] = '}';
                for(int m=0; m < 7; m++)
                {
                 Serial.print(m);  
                 Serial.print(" :");               
                 Serial2.print(out[m]);
                 Serial.println(out[m]);       
                } 
                delay(200);
                lcd.noCursor();    
                lcd.clear(); 
                lcd.setCursor(0, 0);
                lcd.print("Manual Test File    ");    
                lcd.setCursor(0, 2);
                lcd.print("  2.All Up Test File"); 
                while(true)
                {
                 customKey = customKeypad.getKey();  
                 if (customKey == '*')
                 {
                  out[0] = 'A';
                  out[1] = 'B';
                  out[2] = 'T';  //M for Mode change 
                  out[3] = '4';            
                  out[4] = 'F';
                  out[5] = '{';
                  out[6] = '}';
                  for(int m=0; m < 7; m++)
                  {
                   Serial.print(m);  
                   Serial.print(" :");               
                   Serial2.print(out[m]);
                   Serial.println(out[m]);       
                  }
                  goto ad1loop7;
                 }                
                }     
                mcnt = 2;
                goto ad1loop7;          
        case 3: 
                out[0] = 'A';
                out[1] = 'B';
                out[2] = 'T';  //M for Mode change 
                out[3] = '3';            
                out[4] = 'F';
                out[5] = '{';
                out[6] = '}';
                for(int m=0; m < 7; m++)
                {
                 Serial.print(m);  
                 Serial.print(" :");               
                 Serial2.print(out[m]);
                 Serial.println(out[m]);       
                }
                delay(200);
                lcd.noCursor();    
                lcd.clear(); 
                lcd.setCursor(0, 0);
                lcd.print("Manual Test File    ");    
                lcd.setCursor(0, 2);
                lcd.print("  3.All Down File..."); 
                while(true)
                {
                 customKey = customKeypad.getKey();  
                 if (customKey == '*')
                 {
                  out[0] = 'A';
                  out[1] = 'B';
                  out[2] = 'T';  //M for Mode change 
                  out[3] = '4';            
                  out[4] = 'F';
                  out[5] = '{';
                  out[6] = '}';
                  for(int m=0; m < 7; m++)
                  {
                   Serial.print(m);  
                   Serial.print(" :");               
                   Serial2.print(out[m]);
                   Serial.println(out[m]);       
                  }
                  goto ad1loop7;
                 }                
                }                            
                mcnt = 3;
                goto ad1loop7;                               
       }
   }

   if (customKey == '*')
   {
      return;
   }   
   goto adloop7;
}
uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
      else crc <<= 1;
    }
  }
  return crc;
}