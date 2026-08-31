#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>
#include <sd_defines.h>
#include <sd_diskio.h>
#include <Keypad.h>
#include <Wire.h>
#include <stdio.h>
#include <EEPROM.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include "EspUsbHost.h"  // USB Host for reading USB flash drives

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

// ===== USB HOST VARIABLES =====
EspUsbHost usb;
EspUsbHostMscFS usbMassStorage;
bool is_mounted = false;
volatile bool usbRemovedDuringBrowse = false;

// ===== USB FILE MANAGEMENT =====
String usbFolderList[50];
byte   usbFolderCount = 0;
String usbFileList[50];
byte   usbFileCount = 0;
String usbCurrentFolderName = "";

// ===== UI CONSTANTS =====
#define UI_COLS 20
#define UI_ROWS 4
#define UI_TEXT_SIZE 1
#define UI_CHAR_W 6

// ===== UI HELPER FUNCTIONS =====
String uiTrimText(const String &text, uint8_t maxChars = UI_COLS)
{
  if (text.length() <= maxChars) return text;
  return text.substring(0, maxChars);
}

int uiRowY(uint8_t row) { return row * (tft.height() / UI_ROWS); }
int uiRowH() { return tft.height() / UI_ROWS; }

void uiClear()
{
  tft.fillScreen(TFT_BLACK);
}

void uiLine(uint8_t row, const String &text, uint16_t color = TFT_WHITE)
{
  int y = uiRowY(row);
  tft.fillRect(0, y, tft.width(), uiRowH(), TFT_BLACK);
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextSize(UI_TEXT_SIZE);
  tft.setCursor(2, y + 2);
  tft.print(uiTrimText(text));
}

void uiLineAt(uint8_t col, uint8_t row, const String &text, uint16_t color = TFT_WHITE)
{
  int y = uiRowY(row);
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextSize(UI_TEXT_SIZE);
  tft.setCursor(col * UI_CHAR_W, y + 2);
  tft.print(uiTrimText(text, UI_COLS - col));
}

void uiProgress(uint8_t row, int percent, unsigned long cur, unsigned long total)
{
  int y = uiRowY(row);
  tft.fillRect(0, y, tft.width(), uiRowH(), TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(UI_TEXT_SIZE);
  tft.setCursor(2, y + 2);

  char progressText[UI_COLS + 1];
  snprintf(progressText, sizeof(progressText), "%3d%% %lu/%lu", percent, cur, total);
  tft.print(uiTrimText(String(progressText)));

  int barX = 2;
  int barY = y + uiRowH() - 6;
  int barW = tft.width() - 4;
  int barH = 4;
  tft.drawRect(barX, barY, barW, barH, TFT_DARKGREY);
  int fillW = (int)((long)(barW - 2) * percent / 100);
  tft.fillRect(barX + 1, barY + 1, fillW, barH - 2, TFT_GREEN);
}

void uiTransferProgress(int percent, unsigned long cur, unsigned long total)
{
  int rowY = uiRowY(2);
  int rowH = uiRowH();
  static int lastBarWidth = -1;
  if (cur == 0) lastBarWidth = -1;

  tft.startWrite();

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(UI_TEXT_SIZE);
  tft.setCursor(2, rowY + 2);

  char progressText[UI_COLS + 1];
  snprintf(progressText, sizeof(progressText), "%3d%% %lu/%lu", percent, cur, total);
  tft.print(progressText);
  tft.print("                    ");

  int barX = 2;
  int barY = rowY + rowH - 6;
  int barW = tft.width() - 4;
  int barH = 4;
  int fillW = (int)((long)(barW - 2) * percent / 100);
  if (lastBarWidth < 0)
  {
    tft.drawRect(barX, barY, barW, barH, TFT_DARKGREY);
  }
  if (fillW > lastBarWidth)
  {
    int startX = barX + 1 + (lastBarWidth < 0 ? 0 : lastBarWidth);
    tft.fillRect(startX, barY + 1, fillW - (lastBarWidth < 0 ? 0 : lastBarWidth), barH - 2, TFT_GREEN);
  }
  lastBarWidth = fillW;

  tft.setCursor(2, rowY + rowH + 2);
  String cardText = "Card " + String(cur) + "/" + String(total);
  tft.print(uiTrimText(cardText));
  tft.print("                    ");
  tft.endWrite();
}

void drawScrollableMenu(const String &title, String items[], int itemCount, int selectedIndex, int topIndex, int visibleRows)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("TILT TECHNOLOGIES", 130, 10, 4);
  tft.drawLine(10, 40, 470, 40, TFT_CYAN);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(title, 35, 60, 4);

  int x = 90;
  int y = 110;
  int rowHeight = 35;

  for (int i = 0; i < visibleRows; i++)
  {
    int idx = topIndex + i;
    if (idx >= itemCount) break;

    String text = items[idx];
    if (text.length() > 18) text = text.substring(0, 18);

    int rowY = y + i * rowHeight;

    if (idx == selectedIndex)
    {
      tft.fillRect(x - 10, rowY - 5, 280, 30, TFT_BLUE);
      tft.setTextColor(TFT_YELLOW, TFT_BLUE);
    }
    else
    {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }

    tft.drawString(String(idx + 1) + ". " + text, x, rowY, 4);
  }
}

// ===== END UI HELPER FUNCTIONS =====

#define RXD2 16
#define TXD2 17
#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // four columns

#define TFT_GREY 0x5AEB // New colour

File root;
File root3;
File bMap;
File bMap1;
File bMap3;

int nr = 16; //nr=number of ribbons  
int nc = 12; //nc=number of cards per ribbon

static int pos = 0;
static unsigned long lastMove = 0;
char customKey = ' ';


byte kk1,lkk1,nc1,d=0;
byte pulcnt = 0; 
byte esc = 0;
byte data2[360];
byte lcdcnt = 0, temp1 = 0;
byte data1[30];
char pass[8];
char SysNo[8];
char out[11];
char file1[15];
String filelist[50],s_name,foldername1,filename2,folderlist[50],ls_name;
String filelist_m[50],foldername_m,filename1_m,folderlist_m[50],filename2_m; 
char ch;
unsigned long height = 0,temp3 = 0;
unsigned long height1 = 0;
int totaldata = 0,nextlockvalue = 0,lockvalue = 0,lockdays = 0;
int nextlockvalue1 = 0,lockvalue1 = 0,lockdays1 = 0;
String inputString = "", passStr;
byte check_bit = 0,tempq = 0;     //   Check bits
byte filereadcnt = 0,foldernum = 0;
byte data3[50];
byte pluseupcheck,lastpickno1;
unsigned long lastpickno;
byte filerunningmode = 1,BtoTmode = 0,LtoRmode = 0;
byte break2,setup1 = 0;
String str;
int mcnt = 0,r,c,m;
unsigned int bitmapOffset = 62;
byte Menu2 = 0,Enter2 = 0,F12 = 0,F22 = 0,Up2 = 0,Down2 = 0,Left2 = 0,Right2 = 0;
byte motorstatus = 0,temp = 0;
byte totalfile = 0,totalfolder = 0;
int totalfile_m = 0,totalfolder_m = 0;
int filenum_m = 0,filenum1_m = 0;
byte lockid = 0;
byte tempg1 = 0;
unsigned long cnt = 0,tcnt = 0; 
char Direction = 0;
unsigned long pickno = 0,Sysnumber = 0;
unsigned long pickno1 = 0;
unsigned long repeatcnt = 0;
unsigned long repeatcnt1 = 0;
byte R_mode = 0;

String menuItems[] = {"Running Mode.", "File Manager.", "Settings", "Test Files.", "Finger."};
String rn_Mode[] = {"Body only.", "Body & Border."};
String edit_Menu[] = {"Folder Selection.", "Folder Copy.", "Folder Delete.", "File Delete."};
String admin_Menu[] = {"New Password.", "Number of Ribbon.", "Number of Card.", "Invert Design.", "Jac Profile."};
String testFile_Menu[] = {"Plain File.", "AllUp File.", "AllDown File.", "Ciel Card Plain"};
String profile_Menu[] = {"Profile->1.", "Profile->2.", "Profile->3.", "Profile->4."};

char keys[ROW_NUM][COLUMN_NUM] = {
  // {'2','6','0','D'},
  // {'1','5','9','B'},
  // {'3','7','*','E'},
  // {'4','8','C','A'}
  // row 0:
  {'1', '2', '3', '4'},
  // row 1:
  {'5', '6', '7', '8'},
  // row 2:
  {'9', '0', '*', 'L'},
  // row 3:
  {'B', 'R', 'E', 'A'}

};

byte pin_rows[ROW_NUM]      = {47, 18, 5, 4}; 
byte pin_column[COLUMN_NUM] = { 38, 39, 3, 15};
Keypad customKeypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );


void setup() 
{Serial.begin(230400);
  delay(1000);
  Serial.println("BEFORE tft.init()");
 // delay(3000);  
 tft.init();
 Serial.println("AFTER tft.init()");
 tft.setRotation(1);
 Serial.println("AFTER tft.setRotation()");
  Serial2.setRxBufferSize(2048);
  Serial2.begin(230400, SERIAL_8N1, RXD2, TXD2);
  delay(300);
 // Serial.begin(230400);
  delay(300);
//EEPROM.begin(255);
  delay(300);
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
  
  //root3 = SD.open("/");  
  setup1 = 0;  
  totalfile = 0;
  totalfolder = 0;
  Serial.println("log start:");
  inbufferclear();
  //readfolderlist(root3);    
  //setupoffolder();
  delay(500);        
  tft.fillScreen(random(0xFFFF));
  delay(200);
  Loading_Page();
  delay(500);
 communicationchecking();      
  delay(500);  
  masterinit();
  delay(100);  
 masterverify();
  delay(100);
  runningstatus();
  delay(100);   
}

void loop() 
{   
  Running_Page();
  delay(200);
  Main_menu();  
}

void inbufferclear()
{
  for (byte r = 0; r < 50; r++)
  {
    data3[r] = 0;
  }
}

void masterinit()
{                             
  out[0] = 'A';
  out[1] = 'B';
  out[2] = 'I';  //I for Master Init 
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

void serialinput() {
  nc1 = 0;
  inbufferclear();
  inputString = "";       
  while (true)
  {           
   if (Serial2.available() > 0)
    {
     byte inbyte = (byte)Serial2.read();
     if (nc1 >= sizeof(data3)) {
       inbufferclear();
       nc1 = 0;
       inputString = "";
       return;
     }
     data3[nc1] = inbyte;
     nc1 = nc1 + 1;
     if (data3[0] == 65)          // for header 'A'
     {      
      if (data3[1] == 66)         // for header 'B'
       {        
        while(true)
         {
         if (Serial2.available() > 0)
          {
           byte inbyte = (byte)Serial2.read();
           char inChar = (char)inbyte;
           inputString += inChar;
           if (nc1 >= sizeof(data3)) {
             inbufferclear();
             nc1 = 0;
             inputString = "";
             return;
           }
           data3[nc1] = inbyte;
           nc1 = nc1 + 1;
           if (data3[nc1-2] == 35)
           {            
            if (data3[nc1-1] == 38)
            {
              
             if (data3[2] == 68)       // for 'D'-Display             
             {
              if (data3[3] == 75)     // for 'D'-K_file mumber
              {
                int e = inputString.indexOf('K');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                filenum_m = pick.toInt();
                //filenum_m = data3[4] - 48;    
                Serial.println("filenum RECEIVED");
                goto rout;
              }
              
              if (data3[3] == 50)     // for 'D'-2_Pick mumber
              {
                int e = inputString.indexOf('*');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);                
                pickno = pick.toInt();
                Serial.println("pickno RECEIVED");
                Serial.println(pickno);
                goto rout;
              }
              if (data3[3] == 51)     // for 'D'-3_TotalPick mumber
              {
                int e = inputString.indexOf('*');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                height = pick.toInt();
                Serial.println("height RECEIVED");                
                goto rout;
              }      
              if (data3[3] == 52)     // for 'D'-4_repeatcnt mumber
              {
                int e = inputString.indexOf('*');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                repeatcnt = pick.toInt();
                Serial.println("repeatcnt RECEIVED");
                if (filerunningmode == 1)
                d = 1;      
                goto rout;
              }
              if (data3[3] == 77)     // for 'D'-M_filenum1
              {
                int e = inputString.indexOf('M');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                filenum1_m = pick.toInt();
                //filenum1_m = data3[4] - 48;                    
                Serial.println("filenum1 RECEIVED");
                goto rout;
              }
              if (data3[3] == 54)     // for 'D'-6_pickno1
              {
                int e = inputString.indexOf('*');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                pickno1 = pick.toInt();
                Serial.println("pickno1 RECEIVED");
                goto rout;
              }

              if (data3[3] == 55)     // for 'D'-7_height1
              {
                int e = inputString.indexOf('*');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                height1 = pick.toInt();
                Serial.println("height1 RECEIVED");
                goto rout;
              }

              if (data3[3] == 56)     // for 'D'-8_repeatcnt1
              {
                int e = inputString.indexOf('*');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                repeatcnt1 = pick.toInt();
                Serial.println("repeatcnt1 RECEIVED");
                if (filerunningmode == 2)
                d = 1;
                goto rout;
              }
             }

             if (data3[2] == 98)       // Configuration Settings
              {
                BtoTmode = data3[3] % 48;
                delay(100);                
                Serial.print("BtoTmode = ");
                Serial.println(BtoTmode);
                return;
              }

             if (data3[2] == 105)       // Configuration Settings
              {
                LtoRmode = data3[3] % 48;
                delay(100);                
                Serial.print("LtoRmode = ");
                Serial.println(LtoRmode);
                return;
              }

             if (data3[2] == 108)       // for 'l'-lock value             
             {
                int e = inputString.indexOf('l');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                //totalfolder_m = (byte)inputString.substring(e + 1, f);
                lockdays1 = pick.toInt();
                goto rout;
             }
             
             if (data3[2] == 84)       // for 'T'-total file and folder             
             {
              if (data3[3] == 75)     // for 'K'-Folder
              {

                int e = inputString.indexOf('K');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                //totalfolder_m = (byte)inputString.substring(e + 1, f);
                totalfolder_m = pick.toInt();
                                
                Serial.print("total folder:");
                Serial.print('\t');
                Serial.println(totalfolder_m);
              }

              if (data3[3] == 76)     // for 'L'-File
              {
                int e = inputString.indexOf('L');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                //totalfolder_m = (byte)inputString.substring(e + 1, f);
                totalfile_m = pick.toInt();
                //totalfile_m = data3[4] - 48;
                Serial.println("total file:");
                Serial.print('\t');
                Serial.println(totalfile_m);
              }                            
              goto rout;
             }

             if (data3[2] == 110)       // Configuration Settings
              {
                int e = inputString.indexOf('n');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                nc = pick.toInt();
                //nc = (data3[3]%48)*10;
                //nc = (data3[4]%48) + nc;
                delay(100);               
                Serial.print("No of Cards = ");
                Serial.println(nc);
                return;
              }

              if (data3[2] == 78)       // Configuration Settings
              {
                int e = inputString.indexOf('N');
                int  f = inputString.indexOf('#');
                String pick = inputString.substring(e + 1, f);
                nr = pick.toInt();
                //nr = (data3[3]%48)*10;
                //nr = (data3[4]%48) + nr;
                delay(100);               
                Serial.print("No of Ribbon = ");
                Serial.println(nr);
                return;
              }

             if (data3[2] == 76)       // for 'L'-List of file and folder             
             {
              if (data3[3] == 75)     // for 'K'-Folder
              {
                int e = inputString.indexOf('K');
                int  f = inputString.indexOf('*');
                String pick = inputString.substring(e + 1, f);
                //totalfolder_m = (byte)inputString.substring(e + 1, f);
                int tempx = pick.toInt();                
                //int tempx = data3[4] - 48;                
                e = inputString.indexOf('*');
                f = inputString.indexOf('#');
                pick = inputString.substring(e + 1, f);
                folderlist_m[tempx] = pick;
                Serial.print(tempx);
                Serial.print('\t');
                Serial.println(folderlist_m[tempx]);
              }

              if (data3[3] == 77)     // for 'M'-File
              {
                int e = inputString.indexOf('M');
                int  f = inputString.indexOf('*');
                String pick = inputString.substring(e + 1, f);
                //totalfolder_m = (byte)inputString.substring(e + 1, f);
                int tempy = pick.toInt();
                
                //int tempy = data3[4] - 48;                
                 e = inputString.indexOf('*');
                 f = inputString.indexOf('#');
                 pick = inputString.substring(e + 1, f);
                filelist_m[tempy] = pick;
                Serial.print(tempy);
                Serial.print('\t');
                Serial.println(filelist_m[tempy]);
              }                            
              goto rout;
             }
                                     
             if (data3[2] == 77)       // for 'M'-File running mode             
             {
              filerunningmode = data3[3] - 48;                
              Serial.print("File running mode");
              Serial.print('\t');
              Serial.println(filerunningmode);
              R_mode = 1;
              goto rout;
             }

             if (data3[2] == 70)       // for 'F'-Folder numbe             
             {
              int e = inputString.indexOf('F');
              int  f = inputString.indexOf('k');
              String pick = inputString.substring(e + 1, f);   
              int tempy = pick.toInt();
              foldernum = (byte)tempy;
              //data3[3] - 48;              
              Serial.print("Current Folder Number");
              Serial.print('\t');
              Serial.println(tempy); //foldernum);
              goto rout;
             }                          
            }
           }
          }
         }              
       }
     }
    }    
  } 
 rout:  
 Serial.println("out");
}

void communicationchecking()
{
      byte temp1 = 0, jj1 = 0;
      unsigned long jj = 0;
      Serial.println("Serial Communication Testing");
  jk1:    
      out[0] = 'A';
      out[1] = 'B';
      out[2] = 'C';  //C for communicationchecking      
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
      temp1 = 0;      
      while (true)
      {
       jj = jj + 1; 
       byte inbyte = (byte)Serial2.read();          
       data3[temp1] = inbyte;
       Serial.println(data3[temp1]);                      
       temp1 = temp1 + 1;   
       if (data3[temp1-2] == 69)
       {
        if (data3[temp1-1] == 74)
        {
         Serial.println("Communicate Successfully");       
         break; 
        }
       } 
       if ( jj > 5000)
       {
         jj = 0;
         jj1 = jj1 + 1;
         if (jj1 > 3)
         {
          jj1 = 0;
          tft.fillScreen(TFT_BLACK);  
          tft.setTextColor(TFT_CYAN);  
          tft.drawString("TILT TECHNOLOGIES", 130, 120, 4);
          tft.drawString("ELECTRONICS JACQUARD", 100, 150, 4);
          tft.drawString("Communication Err", 130, 200, 4);
          while (true)
          {
            
          }
         }
         else
         {
          goto jk1;
         }
       }
      }
     return;
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
 R_mode = 0;  
 while(true)
 {
  if (Serial2.available() > 0)
  {     
   serialinput(); 
   delay(50);  
   if ((d == 1) && (R_mode == 1))
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
      totalfolder = totalfolder - 1;
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
   //lcd.clear();
   //lcd.setCursor(3, 1);
   //lcd.print("Folder Error!");
   //lcd.setCursor(0, 2);
   //lcd.print("Keep 2 Folders");
   //lcd.setCursor(10, 3);
   //lcd.print("In SDCard");   
   //delay(2000); 
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
      totalfile = totalfile - 1;
      root1.rewindDirectory();  
      bMap.close();      
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

void tilt_tech()
{
 tft.fillScreen(TFT_BLACK);
 //delay(200);  
 tft.setTextColor(TFT_CYAN, TFT_BLACK);
 tft.drawString("TILT TECHNOLOGIES", 140, 10, 4);
 tft.drawLine(10, 40, 475, 40, TFT_CYAN);
}

void drawLabelWithCursor(int x, int y, int w, int h, uint16_t bgColor, uint16_t fontColor, long value, bool cursorEnabled = false, int cursorPos = -1, bool blink = false) 
{
  int fontNum = 7;
  int padding = 5;

  // Convert long to string
  String text = String(value);
  
  // Draw background rectangle
  tft.fillRect(x, y, w, h, bgColor);

  // Draw text (left-aligned)
  tft.setTextColor(fontColor, bgColor);
  tft.setTextDatum(TL_DATUM);
  int textY = y + (h - tft.fontHeight(fontNum)) / 2;
  tft.drawString(text, x + padding, textY, fontNum);

  // === Draw cursor if enabled ===
  if (cursorEnabled && cursorPos >= 0) {
    // Calculate cursor X position based on character width
    int cursorX = x + padding;
    for (int i = 0; i < cursorPos && i < text.length(); i++) {
      cursorX += tft.textWidth(String(text[i]), fontNum);
    }

    int cursorY = textY;
    int cursorHeight = tft.fontHeight(fontNum);

    // Handle blinking
    bool visible = true;
    static unsigned long lastToggle = 0;
    static bool blinkState = true;

    if (blink) {
      unsigned long now = millis();
      if (now - lastToggle > 500) {
        blinkState = !blinkState;
        lastToggle = now;
      }
      visible = blinkState;
    }

    if (visible) {
      tft.fillRect(cursorX, cursorY, 3, cursorHeight, fontColor);
    }
  }
}

void drawLabel(int x, int y, int w, int h, uint16_t bgColor, uint16_t fontColor, long value, byte font) {
  // Draw background rectangle
  tft.fillRect(x, y, w, h, bgColor);

  // Convert long to string
  String text = String(value);

  // Set text properties
  tft.setTextColor(fontColor, bgColor);
  tft.setTextDatum(TL_DATUM); 
  // Draw text centered inside the label box
  tft.drawString(text, x, y, font); // Font size 4 (can change 1–8)
}

void Loading_Page()
{
  int cal = 310; 
  for(int c = 6;c >= 1; c--)
  {
   tft.fillScreen(TFT_BLACK);   
   tft.setTextColor(TFT_CYAN);
   if(c <= 1)
   {
    tft.drawString("Generating Main Page....", 10, (cal-(((1-c)+1)*25)), 4);
   }
   if(c <= 1)
   {
    tft.drawString("Generating Main Page....", 10, (cal-(((1-c)+1)*25)), 4);
   }
   if(c <= 2)
   {
    tft.drawString("Bluetooth 5.0 BLE.....", 10, (cal-(((2-c)+1)*25)), 4);
   }
   if(c <= 3)
   {
    tft.drawString("Wi-Fi 4.0 (802.11 b/g/n).....", 10, (cal-(((3-c)+1)*25)), 4);
   }
   if(c <= 4)
   {
    tft.drawString("GD25Q32 QSPI 16MB flash.....", 10, (cal-(((4-c)+1)*25)), 4);
   }
   if(c <= 5)
   {
    tft.drawString("Zephyr 4.1.0[RTOS]....", 10, (cal-(((5-c)+1)*25)), 4);
   }
   if(c <= 6)
   {    
    tft.drawString("Starting....", 10, (cal-(((6-c)+1)*25)), 4);
   } 
   delay(2000);   
  }
  tft.fillScreen(TFT_BLACK);  
  tft.setTextColor(TFT_CYAN);  
  tft.drawString("TILT TECHNOLOGIES", 130, 120, 4);
  tft.drawString("ELECTRONICS JACQUARD", 100, 150, 4);
  delay(200);
  tft.setTextSize(1);
  tft.drawString("Version 2.0", 340, (cal-20), 4);
  delay(1000);  
}

void drawMenu(int x, int y, uint16_t bgColor, uint16_t textColor, String menuItems[], int itemCount, int selected, byte last1) 
{
  //tft.fillScreen(bgColor);
  String itemText ="";
  tft.fillRect((x - 15), y - 10, 300, 200, bgColor);
  tft.setTextDatum(TL_DATUM);
  //tft.setTextSize(3);

  int lineHeight = 35;

  for (int i = 0; i < itemCount; i++) {
    if (i == last1)
    {
      itemText = String(i + 1) + ". " + menuItems[i] + "  <@>";
    }
    else
    {
      itemText = String(i + 1) + ". " + menuItems[i];
    }

    if (i == selected) {
      tft.fillRect(x - 10, y - 5, 280, 30, TFT_BLUE);
      tft.setTextColor(TFT_YELLOW, TFT_BLUE);
    } else {
      tft.setTextColor(textColor, bgColor);
    }

    tft.drawString(itemText, x, y, 4);
    y += lineHeight;
  }
}

void drawActiveMenu(int x, int y, uint16_t bgColor, uint16_t textColor, String menuItems[], int itemCount, int selected, int active1) 
{
  tft.fillRect((x - 15), y - 10, 300, 200, bgColor);
  tft.setTextDatum(TL_DATUM);
  //tft.setTextSize(2);

  int lineHeight = 35;

  for (int i = 0; i < itemCount; i++) {
    String itemText = String(i + 1) + ". " + menuItems[i];

    if (i == selected) {
      // Currently selected
      tft.fillRect(x - 10, y - 5, 280, 30, TFT_BLUE);
      tft.setTextColor(TFT_YELLOW, TFT_BLUE);
    }
    else if (i == active1) {
      // Previously selected (active item)
      tft.fillRect(x - 10, y - 5, 280, 30, TFT_DARKGREEN);
      tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    }
    else {
      // Normal
      tft.setTextColor(textColor, bgColor);
    }

    tft.drawString(itemText, x, y, 4);

    if (i == active1) {
      tft.setTextColor(TFT_YELLOW, (i == selected) ? TFT_BLUE : TFT_DARKGREEN);
      tft.drawString("$", x + 250, y, 4);   // Position checkmark on the right
    }
    y += lineHeight;
  }
}

void drawLabelBlinkCharColor(int x, int y, int w, int h, uint16_t bgColor, uint16_t fontColor, uint16_t blinkColor, byte fontNum, const String &text, bool cursorEnabled = false, int cursorPos = -1, bool blink = false) 
{
  //int fontNum = 7;
  int padding = 0;  
  
  // Background box
  tft.fillRect((x - 5), y, w, h, bgColor);

  // Set text alignment
  tft.setTextDatum(TL_DATUM);
  int textY = y + (h - tft.fontHeight(fontNum)) / 2;
  int cursorX = x + padding;

  // Draw each character
  for (int i = 0; i < text.length(); i++) 
  {
    String c = String(text[i]);
    int charWidth = tft.textWidth(c, fontNum);

    uint16_t color = fontColor;
    if (cursorEnabled && i == cursorPos) {
      color = blinkColor; // active color for blinking handled in loop()
    }

    tft.setTextColor(color, bgColor);
    tft.drawString(c, cursorX, textY, fontNum);
    cursorX += charWidth;
  }
}

unsigned long readvalue(unsigned int r1,unsigned int c1,byte sz,unsigned long pic, byte runmode, byte fontNum)
{ 
rd:  
 String inputText = String(pic);
 inputText.trim();
 customKey = ' ';
 //tft.drawString(c, cursorX, textY, fontNum);
 tft.setTextColor(TFT_CYAN);
 if(runmode == 0)
 {
  tft.fillRect(80, 50, 150, 30, TFT_BLACK); 
  tft.drawString(inputText, 80, 50, 4);
 }
 else
 {
  tft.fillRect(320, 50, 150, 30, TFT_BLACK); 
  tft.drawString(inputText, 320, 50, 4);
 }
 drawLabelBlinkCharColor(r1, c1, 195, 55,TFT_WHITE,TFT_BLACK,TFT_BLUE,fontNum,inputText,true,m,true);
while (true)
{ 
 customKey = customKeypad.getKey();  

  if (customKey) 
  {
    if (customKey >= '0' && customKey <= '9') {
      if (inputText.length() < sz) {   // limit to 8 digits
        inputText += customKey;
        m = inputText.length();
      }
    }

    if (customKey == '*') { // backspace
      if (inputText.length() == 0)
      return pic;
      if (inputText.length() > 0) {
        inputText.remove(inputText.length() - 1);
        m = inputText.length();
      }
    } 
    
    if (customKey == 'E')    // confirm/enter
    {
      inputText.trim(); 
      unsigned long value = inputText.toInt();      
      if (runmode == 0)
      {     
        if ((value > height) || (value < 0))
        {
         Serial.print("Pick1 Err.."); 
         tft.fillRect(80, 50, 150, 30, TFT_BLACK); 
         tft.drawString("T_Error", 80, 50, 4);
         delay(3000);
         goto rd; 
        }
        else
        {
          Serial.print("Entered number1: ");
          Serial.println(value);
          return value;
        }
      }
      else
      {
       if ((value > height1) || (value < 0))
        {
         Serial.print("Pick2 Err.."); 
         tft.fillRect(320, 50, 150, 30, TFT_BLACK); 
         tft.drawString("T_Error", 320, 50, 4);
         delay(3000); 
         goto rd;
        }
        else
        {
         Serial.print("Entered number2: ");
         Serial.println(value); 
         return value;  
        }
      }         
    }
   drawLabelBlinkCharColor(r1, c1, 200, 55,TFT_WHITE,TFT_BLACK,TFT_BLUE,fontNum,inputText,true,m,true); 
  }   
 }   
}

unsigned long readrptvalue(unsigned int r1,unsigned int c1,byte sz,unsigned long pic, byte runmode, byte fontNum)
{ 
  unsigned int r2 = 50;
  unsigned int c2 = 25;
rd:  
 String inputText = String(pic);
 inputText.trim();
 customKey = ' ';
 //tft.drawString(c, cursorX, textY, fontNum);
 tft.setTextColor(TFT_CYAN);
 if(runmode == 0)
 {
  tft.fillRect(80, 50, 150, 30, TFT_BLACK); 
  tft.drawString(inputText, 80, 50, 4);
 }
 else
 {
  tft.fillRect(320, 50, 150, 30, TFT_BLACK); 
  tft.drawString(inputText, 320, 50, 4);
 }
 drawLabelBlinkCharColor(r1, c1, r2, c2,TFT_WHITE,TFT_BLACK,TFT_BLUE,fontNum,inputText,true,m,true);
while (true)
{ 
 customKey = customKeypad.getKey();  

  if (customKey) 
  {
    if (customKey >= '0' && customKey <= '9') {
      if (inputText.length() < sz) {   // limit to 8 digits
        inputText += customKey;
        m = inputText.length();
      }
    }

    if (customKey == '*') { // backspace
      if (inputText.length() == 0)
      return pic;
      if (inputText.length() > 0) {
        inputText.remove(inputText.length() - 1);
        m = inputText.length();
      }
    } 
    
    if (customKey == 'E')    // confirm/enter
    {
      inputText.trim(); 
      unsigned long value = inputText.toInt();      
      if (runmode == 0)
      {     
        if ((value > 999) || (value < 0))
        {
         Serial.print("Rpt Err.."); 
         tft.fillRect(80, 50, 150, 30, TFT_BLACK); 
         tft.drawString("Rpt Err", 80, 50, 4);
         delay(3000);
         goto rd; 
        }
        else
        {
          Serial.print("Repeat1 number: ");
          Serial.println(value);
          return value;
        }
      }
      else
      {
       if ((value > 999) || (value < 0))
        {
         Serial.print("Rpt Err.."); 
         tft.fillRect(320, 50, 150, 30, TFT_BLACK); 
         tft.drawString("Rpt Err", 320, 50, 4);
         delay(3000); 
         goto rd;
        }
        else
        {
         Serial.print("Repeat2 number: ");
         Serial.println(value); 
         return value;  
        }
      }         
    }
   drawLabelBlinkCharColor(r1, c1, r2, c2,TFT_WHITE,TFT_BLACK,TFT_BLUE,fontNum,inputText,true,m,true); 
  }   
 }   
}

void runningdisplay()
{
  tilt_tech();
  tft.drawString("BODY", 80, 50, 4);
  tft.drawString("BORDER", 320, 50, 4);
  tft.drawLine(10, 80, 475, 80, TFT_CYAN);
  tft.drawLine(240, 40, 240, 290, TFT_CYAN);
  tft.drawLine(265, 150, 460, 150, TFT_CYAN);
  tft.drawLine(10, 220, 475, 220, TFT_CYAN);
  tft.setTextFont(7);  
  tft.setCursor(30, 90);
  if (pickno > height)
  pickno = height; 
  tft.print(pickno);                          //F1-Pick Number
  tft.drawLine(30, 150, 225, 150, TFT_CYAN);  
  tft.setCursor(30, 160);
  tft.print(height);                          //F1-Total Pick 
  s_name = filelist_m[filenum_m];
  s_name.trim();
  tft.drawString(s_name, 10, 230, 4);
  tft.drawLine(10, 260, 475, 260, TFT_CYAN);
  tft.setTextFont(4);
  tft.setCursor(30, 265);  
  tft.print("RPT - ");
  tft.setCursor(100, 265);
  tft.print(repeatcnt);
  if (filerunningmode == 2)
  {
    tft.setTextFont(7);  
    tft.setCursor(270, 90);
    if (pickno1 > height1)
    pickno1 = height1; 
    tft.print(pickno1);                         //F2-Pick Number    
    tft.setCursor(270, 160);
    tft.print(height1);                         //F2-Total Pick 
    Serial.print("Height1 - ");
    Serial.println(height1);    
    s_name = filelist_m[filenum1_m];
    s_name.trim();
    tft.drawString(s_name, 250, 230, 4);
    tft.setTextFont(4);
    tft.setCursor(270, 265);    
    tft.print("RPT - ");
    tft.setCursor(340,265);
    tft.print(repeatcnt1);
  }
  
  tft.drawLine(10, 290, 475, 290, TFT_CYAN);
  tft.setTextFont(4);    
  tft.setCursor(10, 295);
  tft.print("R-");
  tft.print(nr);  
  tft.print("/");
  tft.print("C-");
  tft.print(nc);  
}

void running_out()
{
 tft.setTextFont(7);
 tft.fillRect(30, 90, 200, 60, TFT_BLACK); 
 tft.setCursor(30, 90);
 tft.println(pickno); 
 tft.fillRect(100, 265, 50, 20, TFT_BLACK);
 tft.setTextFont(4);
 tft.setCursor(100, 265);
 tft.print(repeatcnt);  
 if (filerunningmode == 2)
  { 
   tft.setTextFont(7); 
   tft.fillRect(270, 90, 200, 60, TFT_BLACK); 
   tft.setCursor(270, 90);
   tft.println(pickno1);
   tft.setTextFont(4);
   tft.fillRect(340,265, 50, 20, TFT_BLACK);
   tft.setCursor(340,265);       
   tft.print(repeatcnt1); 
  }  
  tft.drawLine(10, 290, 475, 290, TFT_CYAN); 
}


byte fileselection(int x, int y,byte filenum_m1)
{
  int ck = filenum_m1;
s1loop: 
  tft.fillRect(x - 5, y - 7, 200, 35, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE); 
  s_name = filelist_m[ck];
  s_name.trim();
  tft.drawString(s_name, x, y, 4);
sloop: 
  customKey = customKeypad.getKey();
  if (customKey == 'B')
   {
    ck += 1;
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
        ck = totalfile_m - 1;
      }
      goto s1loop;
   }
   
   if (customKey == 'E')
   {    
    return byte(ck);
   }

   if (customKey == '*')
   {
    customKey == '*';
    return filenum_m1;
   }   
   goto sloop;
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

byte findsize(unsigned long hig)
 {
  byte wck12 = 0;
  if (hig <= 999999) 
  wck12 = 6;  
  if (hig <= 99999) 
  wck12 = 5;  
  if (hig <= 9999)
  wck12 = 4; 
  if (hig <= 999)
  wck12 = 3; 
  if (hig <= 99)
  wck12 = 2; 
  if (hig <= 9)
  wck12 = 1;
  return wck12;
 }


void Running_Page()
{      
 rnmain:            
      runningstatus();           
      delay(10);      
      byte ck12 = 0;
      byte ck13 = 0; 
      byte key = 0;
      byte sen_set = 0;
 rnd:       
      runningdisplay();    
 rnloop:         
      running_out();
      
      if (sen_set == 0)
      {
       sen_set = 1;   
       tft.drawString("S1", 310, 295, 4);
       tft.fillCircle(365, 305, 15, TFT_WHITE);
       tft.drawString("S2", 395, 295, 4);
       tft.fillCircle(450, 305, 15, TFT_GREEN);
      }
      else
      {
       sen_set = 0; 
       tft.drawString("S1", 310, 295, 4);
       tft.fillCircle(365, 305, 15, TFT_GREEN);
       tft.drawString("S2", 395, 295, 4);
       tft.fillCircle(450, 305, 15, TFT_WHITE); 
      }          
      //runningdisplay();
      tft.fillCircle(240, 305, 15, TFT_WHITE);
      //delay(50); 
      byte temp1 = 0;
 loopv:
      if (Serial2.available() > 0)
      {
       Serial.println("ok");
       d=0; 
       serialinput();  
       delay(10); 
       if (d == 1)
       {    
        if(pulcnt == 0)
        {
          temp3 = 0;
        }
        else
        {
         pulcnt = 0;     
         temp3 = 425010;     
        }
        goto rnloop;    
       }   
      }

      temp3 = temp3 + 1;
  
      if (temp3 <= 425000)
      {
       if(key == 0)
       {
        key = 1;
        tft.fillCircle(240, 305, 15, TFT_WHITE); 
       }
       goto loopv;      
      } 
      else
      {
       temp3 = 425010;
       if(key == 1)
       {
        key = 0;
        tft.fillCircle(240, 305, 15, TFT_GREEN);
       }         
      }

      customKey = customKeypad.getKey();
      if (customKey) {
        Serial.print("RAW KEY: '");
        Serial.print(customKey);
        Serial.print("' (code=");
        Serial.print((int)customKey);
        Serial.println(")");
      }

     if (customKey == 'E')
     {
      pulcnt = 0;
      temp3 = 0;
      return;  
     } 

     if (customKey == '1')
     {
      ck12 = findsize(height);
      lastpickno = pickno; 
      pickno = readvalue(30, 90, ck12, pickno,0,7);
      if (pickno == 0)
      pickno = 1;
      Serial.print("Pick1 :-");
      Serial.println(pickno);

      if (lastpickno != pickno)
      {
       Serial2.print('A');
       Serial2.print('B');
       Serial2.print('E');
       Serial2.print('2');
       Serial2.print('*');
       Serial2.print(pickno);
       Serial2.print('#');
       Serial2.print('{');
       Serial2.print('}');       
      }
      else
      {
        goto rnmain;
      }
      
       delay(500);               
       runningstatus();
       delay(100);            
       d = 0;
       while(true)
       {
        if (Serial2.available() > 0)
        {     
         serialinput();                 
         delay(50);  
         if (d == 1)
         {
          d = 0;
          //runningdisplay(); 
          delay(100);                 
          break;
         }                
        }               
       }    
      goto rnmain;  
     }

     if (customKey == '2')
     {        
      temp = fileselection(10,230,filenum_m);
      if (filenum_m != temp)
      {               
        Serial2.print('A'); //out[0] = 'A';
        Serial2.print('B'); //out[1] = 'B';
        Serial2.print('E'); //out[2] = 'E';  //E for Edit the current running design             
        Serial2.print('K'); //out[3] = 'K';  // K- for F1 file change                                         
        Serial2.print(temp); //out[4] = temp + 48;                
        Serial2.print('{'); //out[5] = '{';
        Serial2.print('}'); //out[6] = '}'; 
        Serial.println("File1 num:- ");
        Serial.println(temp);
        filenum_m = temp;                            
      }      
      delay(500);               
       runningstatus();            
       d = 0;
       while(true)
       {
        if (Serial2.available() > 0)
        {     
         serialinput();                 
         delay(50);  
         if (d == 1)
         {
          d = 0;
          //runningdisplay(); 
          delay(100);                 
          break;
         }                
        }               
       }     
      goto rnmain;
     }
     
     if (customKey == '3')
     {
      
      lastpickno = repeatcnt;
      repeatcnt = readrptvalue(100,265,3,repeatcnt,0,4);

      if (lastpickno != repeatcnt)
      {
       Serial2.print('A');
       Serial2.print('B');
       Serial2.print('E');
       Serial2.print('3');
       Serial2.print('*');
       Serial2.print(repeatcnt);
       Serial2.print('#');
       Serial2.print('{');
       Serial2.print('}');       
      }
      else
      {
        goto rnmain;
      }
      delay(500);               
      runningstatus();
      delay(100);            
      d = 0;
      while(true)
      {
        if (Serial2.available() > 0)
        {     
          serialinput();                 
          delay(50);  
          if (d == 1)
          {
          d = 0;
          //runningdisplay(); 
          delay(100);                 
          break;
          }                
        }               
      }
      goto rnmain;
     } 

     
     if (customKey == '4')
     {
      ck12 = findsize(height1);
      lastpickno = pickno1;
      pickno1 = readvalue(270, 90, ck12, pickno1,1,7); 
      if (pickno1 == 0)
      pickno1 = 1;
      if (lastpickno != pickno1)
      {
        Serial2.print('A');
        Serial2.print('B');
        Serial2.print('E');
        Serial2.print('6');
        Serial2.print('*');
        Serial2.print(pickno1);
        Serial2.print('#');
        Serial2.print('{');
        Serial2.print('}');
      }      
      delay(500);               
       runningstatus();            
       d = 0;
       while(true)
       {
        if (Serial2.available() > 0)
        {     
         serialinput();                 
         delay(50);  
         if (d == 1)
         {
          d = 0;
          //runningdisplay(); 
          delay(100);                 
          break;
         }                
        }               
       }    
      goto rnmain;  
     }

     if (customKey == '5')
     {
      temp = fileselection(250,230,filenum1_m);
      if (filenum1_m != temp)
      {
        Serial2.print('A'); //out[0] = 'A';
        Serial2.print('B'); //out[1] = 'B';
        Serial2.print('E'); //out[2] = 'E';  //E for Edit the current running design             
        Serial2.print('L'); //out[3] = 'L';  // L- for F2 file change                                         
        Serial2.print(temp); //out[4] = temp + 48;                
        Serial2.print('{'); //out[5] = '{';
        Serial2.print('}'); 
        Serial.println("File1 num:- ");
        Serial.println(temp);
        filenum1_m = temp;               
      }
      
      delay(500);               
       runningstatus();            
       d = 0;
       while(true)
       {
        if (Serial2.available() > 0)
        {     
         serialinput();                 
         delay(50);  
         if (d == 1)
         {
          d = 0;
          //runningdisplay(); 
          delay(100);                 
          break;
         }                
        }               
       }     
      goto rnmain;  
     } 

     if (customKey == '6')
     {             
      lastpickno = repeatcnt1; 
      repeatcnt1 = readrptvalue(340,265,3,repeatcnt1,1,4);
      if (lastpickno != repeatcnt)
      {
       Serial2.print('A');
       Serial2.print('B');
       Serial2.print('E');
       Serial2.print('7');
       Serial2.print('*');
       Serial2.print(repeatcnt1);
       Serial2.print('#');
       Serial2.print('{');
       Serial2.print('}');       
      }
      else
      {
        goto rnmain;
      }
      delay(500);               
      runningstatus();
      delay(100);            
      d = 0;
      while(true)
      {
        if (Serial2.available() > 0)
        {     
          serialinput();                 
          delay(50);  
          if (d == 1)
          {
          d = 0;
          //runningdisplay(); 
          delay(100);                 
          break;
          }                
        }               
      }      
      goto rnmain;
     } 

     if (customKey == 'B')
    {      
      customKey = '0';   
      pulcnt = 1;               
      out[0] = 'A';
      out[1] = 'B';
      out[2] = 'E';  //E for Edit the current running design             
      out[3] = '8';  // 1- for F1 file change                                         
      out[4] = temp + 48;                
      out[5] = '{';
      out[6] = '}';
      for(int m=0; m < 7; m++)
      {
       Serial.print(m);  
       Serial.print(" :");               
       Serial2.print(out[m]);
       Serial.println(out[m]);       
      }
      delay(100);  
      d = 0;
      while(true)
      {
        if (Serial2.available() > 0)
         {     
          serialinput(); 
          delay(50);  
          if (d == 1)
          {
            d = 0;   
            //runningdisplay(); 
            temp3 = 425010;
            key = 1;              
            goto rnloop;
          }                
         }               
      }
      goto rnloop;
    }

  if (customKey == 'A')
    {      
      customKey = '0';     
      //onepulse2();
      pulcnt = 1; 
      out[0] = 'A';
      out[1] = 'B';
      out[2] = 'E';  //E for Edit the current running design             
      out[3] = '9';  // 1- for F1 file change                                         
      out[4] = temp + 48;                
      out[5] = '{';
      out[6] = '}';
      for(int m=0; m < 7; m++)
      {
       Serial.print(m);  
       Serial.print(" :");               
       Serial2.print(out[m]);
       Serial.println(out[m]);       
      }
      delay(100); 
      d = 0;
      while(true)
      {
        if (Serial2.available() > 0)
         {     
          serialinput(); 
          delay(50);  
          if (d == 1)
          {
            d = 0;   
            //runningdisplay();  
            temp3 = 425010;
            key = 1;             
            goto rnloop;
          }                
         }               
      }
      goto rnloop;
    }
  goto loopv;       
}

void drawFolderList(
    int x, int y,
    int width, int rowHeight,    
    int count,
    int selected
) {
  for (int i = 0; i < count; i++) {
    uint16_t bg = (i == selected) ? TFT_BLUE : TFT_BLACK;
    uint16_t fg = (i == selected) ? TFT_YELLOW : TFT_WHITE;

    tft.fillRect(x, y + i * rowHeight, width, rowHeight, bg);
    tft.setTextColor(fg, bg);
    tft.drawString(folderlist_m[i], x + 10, y + i * rowHeight + 8, 4);
  }
}

// NOTE: folderCount must be the TRUE folder count (not count-1).
byte folder_selection(int folderCount)
{
  if (folderCount <= 0)
  {
    Serial.println("No folders available");
    return 255;
  }

  int selectedIndex = (foldernum < folderCount) ? foldernum : 0;
  int topIndex = 0;
  const int VISIBLE_ROWS = 5;
  const int x = 90;
  const int y = 110;
  const int width = 280;
  const int rowHeight = 35;

  auto drawList = [&]() {
    tft.fillScreen(TFT_BLACK);
    tilt_tech();
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("SELECT FOLDER", 35, 60, 4);

    for (int i = 0; i < VISIBLE_ROWS; i++)
    {
      int idx = topIndex + i;
      if (idx >= folderCount) break;

      String text = folderlist_m[idx];
      if (text.length() > 18) text = text.substring(0, 18);

      int rowY = y + i * rowHeight;
      if (idx == selectedIndex)
      {
        tft.fillRect(x - 10, rowY - 5, width, 30, TFT_BLUE);
        tft.setTextColor(TFT_YELLOW, TFT_BLUE);
      }
      else
      {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }

      String marker = (idx == foldernum) ? "@ " : "  ";
      tft.drawString(String(idx + 1) + ". " + marker + text, x, rowY, 4);
    }
  };

  drawList();

  while (true)
  {
    char key = customKeypad.getKey();
    if (!key) continue;

    if (key == 'A')
    {
      selectedIndex = (selectedIndex > 0) ? selectedIndex - 1 : folderCount - 1;
    }
    else if (key == 'B')
    {
      selectedIndex = (selectedIndex < folderCount - 1) ? selectedIndex + 1 : 0;
    }
    else if (key == 'E')
    {
      return selectedIndex;
    }
    else if (key == '*')
    {
      return 255;
    }

    if (selectedIndex < topIndex) topIndex = selectedIndex;
    else if (selectedIndex >= topIndex + VISIBLE_ROWS) topIndex = selectedIndex - VISIBLE_ROWS + 1;

    drawList();
  }
}

// ===== Wait for a mainboard reply that sets d==1 (e.g. after 'R'/'F') =====
bool waitForRunningStatus(unsigned long timeout)
{
  unsigned long statusStart = millis();
  d = 0;
  while (millis() - statusStart <= timeout)
  {
    if (Serial2.available() > 0)
    {
      serialinput();
      if (d == 1)
      {
        d = 0;
        return true;
      }
    }
  }
  Serial.println("Running status timeout");
  return false;
}

// ===== DELETE: confirmation / result screens =====
bool confirmDelete(const String &itemType, const String &itemName)
{
  tilt_tech();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("DELETE " + itemType + "?", 35, 60, 4);

  String displayName = itemName;
  if (displayName.length() > 24) displayName = displayName.substring(0, 24);
  tft.drawString(displayName, 35, 105, 4);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("E: YES", 55, 175, 4);
  tft.drawString("*: NO", 300, 175, 4);

  while (true)
  {
    customKey = customKeypad.getKey();
    if (customKey == 'E') return true;
    if (customKey == '*') return false;
  }
}

void showDeleteResult(const String &itemType, bool deleted)
{
  tilt_tech();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(itemType + " DELETE", 35, 60, 4);
  tft.setTextColor(deleted ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.drawString(deleted ? "SUCCESS" : "FAILED", 95, 120, 4);
  delay(1200);
}

// ===== DELETE: send ABd1*/ABd2*{index}#{} and wait for D1/D2 =====
byte requestDelete(byte deleteType, byte itemIndex)
{
  while (Serial2.available() > 0) Serial2.read();

  Serial.print("TX DELETE: ABd");
  Serial.print(deleteType);
  Serial.print('*');
  Serial.print(itemIndex);
  Serial.println("#{}");

  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('d');
  Serial2.print(deleteType);
  Serial2.print('*');
  Serial2.print(itemIndex);
  Serial2.print('#');
  Serial2.print('{');
  Serial2.print('}');
  Serial2.flush();

  byte previousByte = 0;
  unsigned long start = millis();
  while (millis() - start < 3000)
  {
    if (Serial2.available() > 0)
    {
      byte receivedByte = (byte)Serial2.read();
      if (previousByte == 'D')
      {
        if (receivedByte == '1') return 1;
        if (receivedByte == '2') return 2;
      }
      previousByte = receivedByte;
    }
  }

  Serial.println("ERROR: delete response timeout");
  return 0;
}

void deleteFolder()
{
  if (totalfolder_m <= 0)
  {
    showDeleteResult("FOLDER", false);
    return;
  }

  byte selected = folder_selection(totalfolder_m);
  if (selected == 255) return;

  String folderName = folderlist_m[selected];
  folderName.trim();
  if (!confirmDelete("FOLDER", folderName)) return;

  byte result = requestDelete(1, selected);
  showDeleteResult("FOLDER", result == 1);
  if (result == 1)
  {
    usbRefreshMainboardFolderList();
  }
}

byte selectFileForDelete()
{
  if (totalfile_m <= 0) return 255;

  int selectedIndex = 0;
  int topIndex = 0;
  const int VISIBLE_ROWS = 6;
  bool needsRedraw = true;

  while (true)
  {
    if (needsRedraw)
    {
      if (selectedIndex < topIndex) topIndex = selectedIndex;
      else if (selectedIndex >= topIndex + VISIBLE_ROWS) topIndex = selectedIndex - VISIBLE_ROWS + 1;
      drawScrollableMenu("DELETE FILE", filelist_m, totalfile_m, selectedIndex, topIndex, VISIBLE_ROWS);
      needsRedraw = false;
    }

    customKey = customKeypad.getKey();
    if (!customKey) { delay(10); continue; }

    if (customKey == 'A')
    {
      selectedIndex = (selectedIndex > 0) ? selectedIndex - 1 : totalfile_m - 1;
      needsRedraw = true;
    }
    else if (customKey == 'B')
    {
      selectedIndex = (selectedIndex + 1 < totalfile_m) ? selectedIndex + 1 : 0;
      needsRedraw = true;
    }
    else if (customKey == 'E')
    {
      return (byte)selectedIndex;
    }
    else if (customKey == '*')
    {
      return 255;
    }
  }
}

// File Delete acts on the mainboard's currently active folder, so switch to
// the chosen folder first (same 'F' command Folder Selection already uses).
void deleteFile()
{
  if (totalfolder_m <= 0)
  {
    showDeleteResult("FILE", false);
    return;
  }

  byte folderIndex = folder_selection(totalfolder_m);
  if (folderIndex == 255) return;

  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('F');
  Serial2.print(folderIndex);
  Serial2.print('#');
  Serial2.print('{');
  Serial2.print('}');
  Serial2.flush();

  delay(50);
  runningstatus();
  if (!waitForRunningStatus(3000))
  {
    showDeleteResult("FILE", false);
    return;
  }

  if (totalfile_m <= 0)
  {
    showDeleteResult("FILE", false);
    return;
  }

  byte selected = selectFileForDelete();
  if (selected == 255) return;
  String fileName = filelist_m[selected];
  fileName.trim();
  if (!confirmDelete("FILE", fileName)) return;

  byte result = requestDelete(2, selected);
  showDeleteResult("FILE", result == 1);
  if (result == 1)
  {
    usbRefreshMainboardFolderList();
  }
}

byte menu_List(String menu_Caption, String m_list[], int menuCount1, byte last, byte last1)
{ 
 unsigned int x = 0;
 unsigned int y = 0;
 int selectedIndex = last; 
 //tilt_tech();
 tft.drawString(menu_Caption, 35, 60, 4);
 x = 90;
 y = 110;
 drawMenu(x, y, TFT_BLACK, TFT_WHITE, m_list, menuCount1, selectedIndex, last1);
 menuLoop:
  customKey = customKeypad.getKey();
  if (customKey) 
  {    
    if (customKey == 'A')                                     // Up 
    { 
      if (selectedIndex > 0) selectedIndex--;
      else selectedIndex = menuCount1 - 1;
      drawMenu(x, y, TFT_BLACK, TFT_WHITE, m_list, menuCount1, selectedIndex, last1);
    }
    else if (customKey == 'B')                                // Down
    { 
      if (selectedIndex < menuCount1 - 1) selectedIndex++;
      else selectedIndex = 0;
      drawMenu(x, y, TFT_BLACK, TFT_WHITE, m_list, menuCount1, selectedIndex, last1);
    }
    else if (customKey == 'E')                               // Select
    {       
      delay(500);
      return selectedIndex;          
    }
    else if (customKey == '*')                              // Esc
    {
      delay(500);
      return customKey;
    }
  }
 goto menuLoop;   
}

void finger_selection()
{
  tilt_tech();
  tft.drawString("Coming in next version", 140, 10, 4);
  delay(2000);  
}


void Main_menu()
{ 
 byte selectedIndex1 = 0;  
 mm:
 tilt_tech(); 
 const int menuCount = sizeof(menuItems) / sizeof(menuItems[0]); 
 selectedIndex1 = menu_List("MENU LIST", menuItems, menuCount, selectedIndex1, 10);
 switch (selectedIndex1)
  {        
    case 0:                      // Running mode Menu
           running_Mode();
           delay(100);  
           masterinit();
           delay(100);  
           masterverify();
           delay(100);
           break;                      
    case 1:                      // File copy Menu
           edit_Mode();
           goto mm;
    case 2:                      // Settings Menu
           admin_Mode();
           goto mm;
    case 3:                      // Test File Menu
           test_file();
           goto mm;
    case 4:                      // Finger Selection Menu for Rapier Jacquard
           finger_selection();
           goto mm;
  }
}

void Jac_Profile()
{ 
 tilt_tech();
 byte selectedIndex1 = LtoRmode - 1;  
 const int menuCount = sizeof(profile_Menu) / sizeof(profile_Menu[0]); 
 selectedIndex1 = menu_List("PROFILE SELECTION", profile_Menu, menuCount, selectedIndex1, (LtoRmode - 1));
 switch (selectedIndex1)
  {        
    case 0:                      // Profile 1
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
           break;
    case 1:                      // Profile 2
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
           break;
    case 2:                      // Profile 3
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
           break;
    case 3:                      // Profile 4
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
           LtoRmode = 4;
          break;  
  }              
}

void plain_file()
{ 
 tilt_tech();
 tft.drawString("PLAIN TEST FILE", 35, 60, 4);
 tft.drawString("FILE RUNNING....", 60, 100, 4);
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
  while(true)
  {
    customKey = customKeypad.getKey();  
    if (customKey == '*')
    {
      out[0] = 'A';
      out[1] = 'B';
      out[2] = 'T';  //M for Mode change 
      out[3] = '5';            
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
      break;
    }                
  }  
 delay(2000);  
}

void allup_file()
{ 
 tilt_tech();
 tft.drawString("ALL UP TEST FILE", 35, 60, 4);
 tft.drawString("FILE RUNNING....", 60, 100, 4);
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
  while(true)
  {
    customKey = customKeypad.getKey();  
    if (customKey == '*')
    {
      out[0] = 'A';
      out[1] = 'B';
      out[2] = 'T';  //M for Mode change 
      out[3] = '5';            
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
      break;
    }                
  }       
 delay(2000);  
}

void alldown_file()
{ 
 tilt_tech();
 tft.drawString("ALL DOWN TEST FILE", 35, 60, 4);
 tft.drawString("FILE RUNNING....", 60, 100, 4);
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
  while(true)
  {
    customKey = customKeypad.getKey();  
    if (customKey == '*')
    {
    out[0] = 'A';
    out[1] = 'B';
    out[2] = 'T';  //M for Mode change 
    out[3] = '5';            
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
    break;
    }                
  }                              
 delay(2000);  
}

void Ciel4x4_file()
{ 
 tilt_tech();
 tft.drawString("CIEL CARD PLAIN", 35, 60, 4);
 tft.drawString("FILE RUNNING....", 60, 100, 4);
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
  delay(200);  
  while(true)
  {
    customKey = customKeypad.getKey();  
    if (customKey == '*')
    {
    out[0] = 'A';
    out[1] = 'B';
    out[2] = 'T';  //M for Mode change 
    out[3] = '5';            
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
    break;
    }                
  }                              
 delay(2000);  
}

void test_file()
{ 
 byte selectedIndex1 = 0; 
 tf:
 tilt_tech();  
 const int menuCount = sizeof(testFile_Menu) / sizeof(testFile_Menu[0]); 
 selectedIndex1 = menu_List("TESTING FILE", testFile_Menu, menuCount, selectedIndex1, 10);
 switch (selectedIndex1)
  {        
    case 0:                      // Plain File
           plain_file();
           goto tf;
    case 1:                      // Allup File
           allup_file();
           goto tf;
    case 2:                      // AllDown File   
           alldown_file();
           goto tf;
    case 3:                      // Ciel Card Plain File   
           Ciel4x4_file();       
           goto tf;                   
  }              
}

void running_Mode()
{  
 tilt_tech(); 
 const int menuCount = sizeof(rn_Mode) / sizeof(rn_Mode[0]);
 Serial.print("Run Mode:-");
 Serial.println(filerunningmode);
 byte selectedIndex1 = menu_List("LOOM RUNNING MODE", rn_Mode, menuCount, 0, (filerunningmode - 1)); 
 selectedIndex1++; 
 switch (selectedIndex1)
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
        Serial.println("Body File");
        delay(500);          
        break;
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
         Serial.println("Body & Border File");
         delay(500);           
         break;
 }
    
}

void edit_Mode()
{
 tilt_tech();   
 const int menuCount = sizeof(edit_Menu) / sizeof(edit_Menu[0]);
 unsigned int x = 0;
 unsigned int y = 0;
 int selectedIndex = 0; 
 tft.drawString("FILE MANAGER", 35, 60, 4);
 x = 90;
 y = 110;
 //drawMenu(x, y, TFT_BLACK, TFT_WHITE, rn_Mode, menuCount, selectedIndex);
 drawMenu(x, y, TFT_BLACK, TFT_WHITE, edit_Menu, menuCount, selectedIndex, 10);
 while (true)
 {
  customKey = customKeypad.getKey();
  if (customKey) 
  {
    if (customKey == 'A')           // Up
    { 
      if (selectedIndex > 0) selectedIndex--;
      else selectedIndex = menuCount - 1;      
      drawMenu(x, y, TFT_BLACK, TFT_WHITE, edit_Menu, menuCount, selectedIndex, 10);
    }
    
    if (customKey == 'B')     // Down
    { 
      if (selectedIndex < menuCount - 1) selectedIndex++;
      else selectedIndex = 0;
      drawMenu(x, y, TFT_BLACK, TFT_WHITE, edit_Menu, menuCount, selectedIndex, 10);
    }
    
    if (customKey == 'E')     // Select
    {
     switch (selectedIndex + 1)
     {        
      case 1:               
            ls_name =folderlist_m[foldernum];
            tempq = folder_selection(totalfolder_m);
            if (tempq == '*')
            break;                                
            s_name = folderlist_m[tempq];
            if (ls_name.equalsIgnoreCase(s_name))
              {             
              //goto menu; 
              break;
              }     
              else       
              {              
                Serial2.print('A'); //out[0] = 'A';
                Serial2.print('B'); //out[1] = 'B';
                Serial2.print('F'); //out[2] = 'F';  //F for Folder Change Mode            
                Serial2.print(tempq); //out[3] = (byte)tempq + 48;                           
                Serial2.print('#'); //out[4] = '#';
                Serial2.print('{'); //out[5] = '{';
                Serial2.print('}'); //out[6] = '}'; 
                delay(1000);                          
              }

              delay(500);               
              runningstatus();
              delay(100);            
              d = 0;
              while(true)
              {
                if (Serial2.available() > 0)
                {     
                  serialinput();                 
                  delay(50);  
                  if (d == 1)
                  {
                    d = 0;
                    //runningdisplay(); 
                    delay(100);                 
                    break;
                  }                
                }               
              } 
              break;           
      case 2:
             USB();
             break;
      case 3:
             deleteFolder();
             break;
      case 4:
             deleteFile();
             break;
     }
     break;     
    }
    
    if (customKey == '*')                              // Esc
    {
      delay(500);
      break;
    }
  }
 }  
}

int readTwoDigitNumber(TFT_eSPI &tft,Keypad &keypad,const char *title,const char *caption, int value) 
{
  char digits[2] = { '0', '0' };   
  digits[0] = (value / 10) + '0';  
  digits[1] = (value % 10) + '0';
  uint8_t cursor = 0;     // 0 = left digit, 1 = right digit
  char key;
  
  // ---- UI Layout ----
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);

  tft.drawString(title, 140, 10, 4);
  tft.drawLine(10, 40, 475, 40, TFT_CYAN);
  tft.drawString(caption, 60, 80, 4);

  // Input box
  tft.drawRect(170, 130, 140, 80, TFT_WHITE);

  while (true) {
    // ---- Draw digits ----
    for (int i = 0; i < 2; i++) {
      uint16_t bg = (i == cursor) ? TFT_BLUE : TFT_BLACK;
      uint16_t fg = TFT_WHITE;

      tft.setTextColor(fg, bg);
      tft.drawChar(digits[i], 195 + (i * 40), 150, 6);
    }

    key = keypad.getKey();
    if (!key) continue;

    // ---- Digit replace ----
    if (key >= '0' && key <= '9') {
      digits[cursor] = key;
      cursor = (cursor == 0) ? 1 : 0;
    }

    // ---- Cursor LEFT (C) ----
    else if (key == 'C') {
      cursor = (cursor == 0) ? 1 : 0;
    }

    // ---- Cursor RIGHT (D) ----
    else if (key == 'D') {
      cursor = (cursor == 1) ? 0 : 1;
    }

    // ---- Clear digit (A) ----
    else if (key == 'A') {
      digits[cursor] = '0';
    }

    // ---- Confirm ----
    else if (key == 'E') {
      delay(200);
      int value = (digits[0] - '0') * 10 + (digits[1] - '0');
      return value;
    }

    // ---- Cancel ----
    else if (key == '*') {
      return -1;
    }

    delay(120); // debounce
  }
}

void Number_of_Card(byte cb)
{
  int cardCount = 0;
  if(cb == 0)
  { 
   cardCount = readTwoDigitNumber(tft,customKeypad,"TILT TECHNOLOGIES","NO OF RIBBON DETAILS", nr);
  }
  else
  {
   cardCount = readTwoDigitNumber(tft,customKeypad,"TILT TECHNOLOGIES","NO OF CARD DETAILS", nc); 
  } 
  if (cardCount == -1) 
  {
  // User pressed *
  //showMainMenu();
  } 
  else 
  {
   if(cb == 1)
   { 
    Serial.print("No of Cards = ");
    Serial.println(cardCount);
    nc = cardCount;
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
   }
   else
   {
    Serial.print("No of Ribbon = ");
    Serial.println(cardCount);
    nr = cardCount;
    out[0] = 'A';
    out[1] = 'B';
    out[2] = 'N';  //M for Mode change
    out[3] = nr;
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

   unsigned long confirmedNr = 0, confirmedNc = 0;
   bool synced = waitForRibbonCardSync(confirmedNr, confirmedNc, 2000);
   uiClear();
   tilt_tech();
   if (!synced) {
     uiLine(1, "Main board");
     uiLine(2, "Sync FAILED");
     uiLine(3, "(no reply)");
   } else if (confirmedNr != (unsigned long)nr || confirmedNc != (unsigned long)nc) {
     uiLine(1, "Sync MISMATCH");
     uiLine(2, "Board R:" + String(confirmedNr) + " C:" + String(confirmedNc));
     uiLine(3, "Panel R:" + String(nr) + " C:" + String(nc));
   } else {
     uiLine(1, "Main board synced");
     uiLine(2, "R:" + String(confirmedNr) + " C:" + String(confirmedNc));
   }
   delay(2000);
  }
}


String readPassword(TFT_eSPI &tft,Keypad &keypad,const char *title,const char *caption,uint8_t maxLength) 
{
  String password = "";
  char key;
  byte can = 1;

  // ---- Screen layout ----
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);

  tft.drawString(title, 140, 10, 4);
  tft.drawLine(10, 40, 475, 40, TFT_CYAN);
  tft.drawString(caption, 35, 70, 4);
  tft.drawLine(30, 95, 250, 95, TFT_CYAN);

  // Password box
  tft.drawRect(80, 140, 320, 50, TFT_WHITE);
  tft.drawString("ENTER PASSWORD", 130, 110, 4);

  while (true) {
    key = keypad.getKey();
    if (!key) continue;

    // ---- Digits ----
    if (key >= '0' && key <= '9') {
      if (password.length() < maxLength) {
        password += key;
      }
    }

    // ---- Backspace ----
    else if (key == 'C') {
      if (password.length() > 0) {
        password.remove(password.length() - 1);
      }
    }

    // ---- Enter ----
    else if (key == 'E') {
      ch = key;
      delay(200);
      return password;
    }

    // ---- Cancel ----
    else if (key == '*') {
      ch = key;
      return "";   // empty = cancelled
    }

    // ---- Update masked display ----
    tft.fillRect(90, 150, 300, 30, TFT_BLACK);

    String masked = "";
    for (uint8_t i = 0; i < password.length(); i++) {
      masked += "*";
    }

    tft.drawString(masked, 100, 150, 4);

    delay(120); // debounce
  }
 }

unsigned long New_Password(byte t)
{
 unsigned long passss = 0; 
 String pass = " ";
 if(t == 1)
 {
  pass = readPassword(tft,customKeypad,"TILT TECHNOLOGIES","CREATE PASSWORD",6);
 }
 else
 {
  pass = readPassword(tft,customKeypad,"TILT TECHNOLOGIES","SECURITY LOGIN",6);
 } 
 
 if ((t == 1) && (ch == 'E'))
 {
  int len = pass.length(); 
  EEPROM.write(0, len);          // store length first
  for (int i = 0; i < len; i++) {
    EEPROM.write(i + 1, pass[i]);
  }
  EEPROM.commit();   
  tcnt = 1;
  return tcnt;
 }
 passss = pass.toInt();
 return passss; 
}

void drawInvertList(int x,int y,const char *items[],int count,int selectedIndex,bool df) 
{
  const int lineHeight = 40;

  for (int i = 0; i < count; i++) 
  {
    uint16_t bg = (i == selectedIndex) ? TFT_BLUE : TFT_BLACK;
    uint16_t fg = (i == selectedIndex) ? TFT_YELLOW : TFT_CYAN;

    // Clear line
    tft.fillRect(x, y + i * lineHeight, 260, lineHeight, bg);

    tft.setTextColor(fg, bg);
    if ((i == 0) && (df == 1))
    {
      String sd = items[i];
      sd = sd + "  <@>"; 
     tft.drawString(sd, x + 10, y + i * lineHeight + 8, 4);
    }
    else
    {
     tft.drawString(items[i], x + 10, y + i * lineHeight + 8, 4); 
    }
    if ((i == 1) && (df == 0))
    {
     String sd1 = items[i]; 
     sd1 = sd1 + "  <@>"; 
     tft.drawString(sd1, x + 10, y + i * lineHeight + 8, 4);
    }
    else
    {
     tft.drawString(items[i], x + 10, y + i * lineHeight + 8, 4); 
    }
        
  }
}

int readInvertOption(TFT_eSPI &tft,Keypad &keypad,const char *title,const char *caption,bool defaultValue) 
{
  const char *items[] = {"1.INVERT ON","2.INVERT OFF"};
  const int itemCount = 2;
  int selectedIndex = defaultValue == 1 ? 0 : 1;
  char key;
  // ---- Static UI ----
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);

  tft.drawString(title, 140, 10, 4);
  tft.drawLine(10, 40, 475, 40, TFT_CYAN);
  tft.drawString(caption, 60, 80, 4);

  // Initial draw
  drawInvertList(120, 140, items, itemCount, selectedIndex, defaultValue);

  while (true) {
    key = keypad.getKey();
    if (!key) continue;

    // UP
    if (key == 'A') {
      selectedIndex =
        (selectedIndex == 0) ? itemCount - 1 : selectedIndex - 1;
      drawInvertList(120, 140, items, itemCount, selectedIndex, defaultValue);
    }

    // DOWN
    else if (key == 'B') {
      selectedIndex =(selectedIndex == itemCount - 1) ? 0 : selectedIndex + 1;
      drawInvertList(120, 140, items, itemCount, selectedIndex, defaultValue);
    }

    // SELECT
    else if (key == 'E') {
      delay(150);
      return (selectedIndex == 0) ? 1 : 0;
    }

    // CANCEL
    else if (key == '*') {
      return -1;
    }

    delay(120); // debounce
  }
}

// ============================================================
// ===== USB HOST FUNCTIONS (EspUsbHost Implementation) =====
// ============================================================

void usbhostsetup() {
  // Initialize USB Host with device callbacks
  usb.onDeviceConnected([](const EspUsbHostDeviceInfo &device) {
    Serial.println("USB device connected");
  });
  
  usb.onDeviceDisconnected([](const EspUsbHostDeviceInfo &device) {
    Serial.println("USB device disconnected");
    usbRemovedDuringBrowse = true;
    is_mounted = false; 
  });
  
  if (!usb.begin()) {
    Serial.printf("USB init failed: %s\n", usb.lastErrorName());
  }
  Serial.println("USB Host initialized");
}

void usbhostloop() {
  // Check if mounted, mount if needed
  if (!usbMassStorage.mounted()) {
    static uint32_t lastAttempt = 0;
    if (millis() - lastAttempt >= 1000) {
      lastAttempt = millis();
      if (usbMassStorage.begin(usb, "/usb")) {
        Serial.println("USB filesystem mounted");
        is_mounted = true;
      }
    }
  } else {
    is_mounted = true;
  }
}

void USB() {
  static bool usbFlowActive = false;
  if (usbFlowActive) return;

  usbFlowActive = true;
  usbhostsetup();
  delay(1000);

  // Wait for USB mount
  uiClear();
  tilt_tech();
  uiLine(1, "Insert USB Drive...");
  
  unsigned long startWait = millis();
  while (millis() - startWait < 30000) {
    usbhostloop();
    if (is_mounted) {
      uiClear();
      tilt_tech();
      uiLine(1, "USB Ready");
      delay(500);
      break;
    }
    customKey = customKeypad.getKey();
    if (customKey == '*') {
      usbFlowActive = false;
      return;
    }
    delay(100);
  }

  if (!is_mounted) {
    uiClear();
    tilt_tech();
    uiLine(1, "USB Mount Failed");
    delay(2000);
    usbFlowActive = false;
    return;
  }

  usbScanFolders();
  usbBrowseFolders();
  delay(300);

  usbFlowActive = false;
}

// ===== SCAN USB FOLDERS =====
void usbScanFolders() {
  usbFolderCount = 0;
  
  File usbroot = usbMassStorage.open("/");
  if (!usbroot || !usbroot.isDirectory()) {
    Serial.println("ERROR: Could not open USB root directory");
    return;
  }

  while (true) {
    File entry = usbroot.openNextFile();
    if (!entry) break;

    if (entry.isDirectory()) {
      String name = entry.name();
      int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      if (!name.startsWith(".") && !name.startsWith("SYSTEM") && !name.startsWith("System")) {
        if (usbFolderCount < 50) {
          usbFolderList[usbFolderCount] = name;
          usbFolderCount++;
          Serial.print("FOUND FOLDER: ");
          Serial.println(name);
        }
      }
    }
    entry.close();
  }
  usbroot.close();

  Serial.print("usbFolderCount = ");
  Serial.println(usbFolderCount);
}

// ===== SCAN FILES IN FOLDER =====
void usbScanFilesInFolder(String folderName) {
  usbCurrentFolderName = folderName;
  usbFileCount = 0;
  
  String path = "/" + folderName;
  File dir = usbMassStorage.open(path.c_str());
  if (!dir || !dir.isDirectory()) {
    Serial.print("ERROR: Could not open folder: ");
    Serial.println(path);
    return;
  }
  
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.endsWith(".BMP") || name.endsWith(".bmp") ||
          name.endsWith(".EJC") || name.endsWith(".ejc")) {
        if (usbFileCount < 50) {
          usbFileList[usbFileCount] = name;
          usbFileCount++;
          Serial.print("FOUND FILE: ");
          Serial.println(name);
        }
      }
    }
    entry.close();
  }
  dir.close();

  Serial.print("usbFileCount = ");
  Serial.println(usbFileCount);
}

// ===== CRC16-CCITT (must match Main_Board.ino's crc16_ccitt) =====
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

// ===== FIND BMP HEIGHT =====
unsigned long usbFindHeight(File &f) {
  unsigned long ht = 0;
  f.seek(0x16);
  for (int i = 0; i < 4; i++) {
    ht += ((unsigned long)f.read() << (8 * i));
  }
  if (ht > 99999) ht = 99999;
  return ht;
}

// ===== DERIVE TOTAL DATA =====
bool usbDeriveTotalData(File &f, unsigned long srcSize,
                        unsigned long &heightOut, unsigned int &totalDataOut) {
  heightOut = usbFindHeight(f);
  if (heightOut == 0) return false;

  unsigned long payloadBytes = (srcSize > bitmapOffset) ? (srcSize - bitmapOffset) : 0;
  if (payloadBytes % heightOut != 0) {
    Serial.println("WARNING: payload not exact multiple of height");
    return false;
  }

  totalDataOut = (unsigned int)(payloadBytes / heightOut);
  return true;
}

// ===== CHECK FILE FORMAT =====
bool usbCheckFileFormatMatch(String folderName, String fileName) {
  String path = "/" + folderName + "/" + fileName;
  File checkFile = usbMassStorage.open(path.c_str(), FILE_READ);
  if (!checkFile) {
    uiClear();
    tilt_tech();
    uiLine(1, "Cannot read file");
    uiLine(2, "Wrong format");
    delay(2000);
    return false;
  }

  unsigned long fileSize = checkFile.size();
  unsigned long fileHeight = usbFindHeight(checkFile);
  unsigned long payloadBytes = (fileSize > bitmapOffset) ? fileSize - bitmapOffset : 0;
  bool validShape = fileHeight > 0 && payloadBytes > 0 && payloadBytes % fileHeight == 0;
  unsigned long fileTotalData = validShape ? payloadBytes / fileHeight : 0;
  unsigned long expectedTotalData = ((unsigned long)nr * (unsigned long)nc) + 4UL;
  bool validFormat = validShape && fileTotalData >= 4 && fileTotalData <= 1028 &&
                     fileTotalData == expectedTotalData;
  checkFile.close();

  if (!validFormat) {
    uiClear();
    tilt_tech();
    uiLine(1, "Wrong file format");
    if (!validShape) {
      uiLine(2, "Invalid card data");
    } else {
      uiLine(2, "Expected: " + String(expectedTotalData));
    }
    delay(2500);
  }

  return validFormat;
}

// ===== CHECK FILE EXISTS =====
byte usbCheckFileExists(String fileName) {
  while (Serial2.available() > 0) { Serial2.read(); }
  
  Serial.print("TX: ABf4*");
  Serial.print(fileName);
  Serial.println("#{}");

  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('4');
  Serial2.print('*'); Serial2.print(fileName);
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();
  
  byte b0 = 0, b1 = 0;
  unsigned long start = millis();
  while (millis() - start < 3000) {
    if (Serial2.available() > 0) {
      byte inbyte = (byte)Serial2.read();
      b0 = b1;
      b1 = inbyte;
      if (b0 == 68) {   // 'D'
        if (b1 == 49) { Serial.println("RX: D1 (exists)"); return 1; }
        if (b1 == 50) { Serial.println("RX: D2 (doesn't exist)"); return 2; }
      }
    }
  }

  Serial.println("ERROR: usbCheckFileExists timeout");
  return 0;
}

// ===== CHECK FILE SIZE =====
unsigned long usbCheckFileSize(String fileName) {
  Serial.print("TX COMMAND: ABf5*");
  Serial.print(fileName);
  Serial.println("#{}");
  
  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('5');
  Serial2.print('*'); Serial2.print(fileName);
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();

  String resp = "";
  unsigned long start = millis();
  while (millis() - start < 3000) {
    if (Serial2.available() > 0) {
      resp += (char)Serial2.read();
      if (resp.length() > 60) resp.remove(0, resp.length() - 60);
      int zIdx = resp.indexOf("Dz*");
      if (zIdx != -1) {
        int hashIdx = resp.indexOf('#', zIdx);
        if (hashIdx != -1) {
          String sizeStr = resp.substring(zIdx + 3, hashIdx);
          sizeStr.trim();
          return (unsigned long)sizeStr.toInt();
        }
      }
    }
  }
  return 0xFFFFFFFF;
}

// ===== WAIT FOR MAIN BOARD RIBBON/CARD SYNC CONFIRMATION =====
// After sending an 'n'/'N' (card/ribbon count) change, the main board replies
// with "ABDy*{nr}*{nc}#&" carrying the values it actually applied.
bool waitForRibbonCardSync(unsigned long &confirmedNr, unsigned long &confirmedNc, unsigned long timeoutMs) {
  String resp = "";
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (Serial2.available() > 0) {
      resp += (char)Serial2.read();
      if (resp.length() > 40) resp.remove(0, resp.length() - 40);
      int idx = resp.indexOf("ABDy*");
      if (idx != -1) {
        int hashIdx = resp.indexOf('#', idx);
        if (hashIdx != -1) {
          String nums = resp.substring(idx + 5, hashIdx);
          int starIdx = nums.indexOf('*');
          if (starIdx != -1) {
            confirmedNr = (unsigned long)nums.substring(0, starIdx).toInt();
            confirmedNc = (unsigned long)nums.substring(starIdx + 1).toInt();
            return true;
          }
        }
      }
    }
  }
  return false;
}

// ===== SEND FOLDER CREATE =====
byte usbSendFolderCreate(String folderName) {
  while (Serial2.available() > 0) { Serial2.read(); }

  Serial.print("TX: ABf1*");
  Serial.print(folderName);
  Serial.println("#{}");

  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('1');
  Serial2.print('*'); Serial2.print(folderName);
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();

  byte state = 0;
  unsigned long start = millis();
  while (millis() - start < 3000) {
    if (Serial2.available() > 0) {
      byte inbyte = (byte)Serial2.read();
      Serial.print("RX byte: ");
      Serial.println(inbyte);
      
      if (state == 0) {
        if (inbyte == 68) state = 1;   // 'D'
      } else {
        if (inbyte == 49) { Serial.println("RX: D1 (exists)"); return 1; }
        if (inbyte == 50) { Serial.println("RX: D2 (created)"); return 2; }
        if (inbyte == 51) { Serial.println("RX: D3 (mkdir failed)"); return 3; }
        state = 0;
      }
    }
  }
  Serial.println("ERROR: usbSendFolderCreate timeout");
  return 0;
}

// ===== ABORT FILE COPY =====
void usbAbortFileCopy() {
  Serial.println("TX COMMAND: ABf7#{}");
  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('7');
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();
}

// ===== SEND FILE OPEN =====
byte usbSendFileOpen(String fileName) {
  while (Serial2.available() > 0) { Serial2.read(); }

  char packet[256];
  snprintf(packet, sizeof(packet), "ABf2*%s#{}", fileName.c_str());
  Serial.print("TX COMMAND: ");
  Serial.println(packet);

  for (int i = 0; packet[i] != '\0'; i++) Serial2.write((byte)packet[i]);
  Serial2.flush();

  byte b0 = 0, b1 = 0;
  unsigned long start = millis();
  while (millis() - start < 3000) {
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

// ===== REFRESH FOLDER LIST =====
bool usbRefreshMainboardFolderList() {
  Serial.println("Requesting mainboard folder-list refresh...");

  while (Serial2.available() > 0) { Serial2.read(); }

  Serial2.print('A'); Serial2.print('B'); Serial2.print('f'); Serial2.print('3');
  Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
  Serial2.flush();

  // The main board sends the whole list as a fast burst (~10ms between
  // items) with no explicit "end of list" marker, so we treat 300ms of
  // silence as "done" instead of blocking for a fixed worst-case timeout.
  const unsigned long QUIET_MS = 300;
  const unsigned long MAX_WAIT_MS = 3000;
  unsigned long overallStart = millis();
  unsigned long lastByteAt = millis();
  bool gotAnyData = false;
  while (millis() - overallStart < MAX_WAIT_MS) {
    if (Serial2.available() > 0) {
      serialinput();
      lastByteAt = millis();
      gotAnyData = true;
    } else if (gotAnyData && (millis() - lastByteAt > QUIET_MS)) {
      break;
    }
  }
  Serial.println("Folder list refresh completed");
  return true;
}

// ===== CHECK USB REMOVED =====
bool usbCheckRemoved() {
  usbhostloop();
  if (usbRemovedDuringBrowse) {
    usbRemovedDuringBrowse = false;
    delay(1000);
    return true;
  }
  return false;
}

// ===== BROWSE FOLDERS =====
void usbBrowseFolders() {
  if (usbFolderCount == 0) {
    tft.fillScreen(TFT_BLACK);
    tilt_tech();
    uiLine(1, "No folders on USB");
    delay(2000);
    return;
  }

  const int VISIBLE_ROWS = 6;
  int ck1 = 0;
  int top1 = 0;
  bool needsRedraw = true;

  while (true) {
    if (needsRedraw) {
      if (ck1 < top1) top1 = ck1;
      else if (ck1 >= top1 + VISIBLE_ROWS) top1 = ck1 - VISIBLE_ROWS + 1;

      drawScrollableMenu("USB FOLDERS", usbFolderList, usbFolderCount, ck1, top1, VISIBLE_ROWS);
      needsRedraw = false;
    }

    usbhostloop();
    customKey = customKeypad.getKey();
    if (!customKey) {
      delay(20);
      continue;
    }

    if (customKey == 'B') {
      ck1 += 1;
      if (ck1 >= usbFolderCount) ck1 = usbFolderCount - 1;
      needsRedraw = true;
    } else if (customKey == 'A') {
      ck1 -= 1;
      if (ck1 < 0) ck1 = 0;
      needsRedraw = true;
    } else if (customKey == 'E') {
      usbScanFilesInFolder(usbFolderList[ck1]);
      usbBrowseFiles();
      return;
    } else if (customKey == 'R') {
      usbCopyFolder(usbFolderList[ck1]);
      usbScanFolders();
      needsRedraw = true;
    } else if (customKey == '*') {
      return;
    }
  }
}

// ===== BROWSE FILES =====
void usbBrowseFiles() {
  if (usbFileCount == 0) {
    tft.fillScreen(TFT_BLACK);
    tilt_tech();
    uiLine(1, "No matching files");
    delay(2000);
    return;
  }

  const int VISIBLE_ROWS = 6;
  int ck2 = 0;
  int top2 = 0;
  bool needsRedraw = true;

  while (true) {
    if (needsRedraw) {
      if (ck2 < top2) top2 = ck2;
      else if (ck2 >= top2 + VISIBLE_ROWS) top2 = ck2 - VISIBLE_ROWS + 1;

      drawScrollableMenu("FOLDER FILES", usbFileList, usbFileCount, ck2, top2, VISIBLE_ROWS);
      needsRedraw = false;
    }

    customKey = customKeypad.getKey();
    if (!customKey) {
      delay(20);
      continue;
    }

    if (customKey == 'R') {
      needsRedraw = true;
    } else if (customKey == 'B') {
      ck2 += 1;
      if (ck2 >= usbFileCount) ck2 = usbFileCount - 1;
      needsRedraw = true;
    } else if (customKey == 'A') {
      ck2 -= 1;
      if (ck2 < 0) ck2 = 0;
      needsRedraw = true;
    } else if (customKey == 'E') {
      usbCopySingleFile(usbFileList[ck2]);
      usbScanFilesInFolder(usbCurrentFolderName);
      needsRedraw = true;
    } else if (customKey == '*') {
      return;
    }
  }
}

// ===== COPY SINGLE FILE =====
void usbCopySingleFile(String sourceFileName) {
  if (!usbCheckFileFormatMatch(usbCurrentFolderName, sourceFileName)) {
    return;
  }

  usbRefreshMainboardFolderList();

  if (totalfolder_m == 0) {
    uiClear();
    tilt_tech();
    uiLine(1, "No folders on");
    uiLine(2, "main board SD");
    delay(2000);
    return;
  }

  byte destIndex = folder_selection(totalfolder_m);
  if (destIndex == 255) {
    return;  // user cancelled with '*'
  }
  String destFolderName = folderlist_m[destIndex];
  destFolderName.trim();

  uiClear();
  tilt_tech();
  uiLine(1, "Copying file...");
  uiLine(2, sourceFileName);
  uiLine(3, "-> " + destFolderName);
  delay(500);

  byte fr = usbSendFolderCreate(destFolderName);
  if (fr == 0) {
    uiClear();
    tilt_tech();
    uiLine(1, "No reply from main");
    delay(2000);
    return;
  }
  if (fr == 3) {
    uiClear();
    tilt_tech();
    uiLine(1, "Mkdir failed");
    delay(2000);
    return;
  }

  byte existsCheck = usbCheckFileExists(sourceFileName);
  if (existsCheck == 0) {
    uiClear();
    tilt_tech();
    uiLine(1, "Main board");
    uiLine(2, "No response");
    delay(2000);
    return;
  }
  if (existsCheck == 1) {
    uiClear();
    tilt_tech();
    uiLine(1, "File exists");
    uiLine(2, "Skip copy");
    delay(2000);
    return;
  }

  bool ok = usbCopyOneFile(usbCurrentFolderName, sourceFileName);

  uiClear();
  tilt_tech();
  uiLine(1, "File copy");
  uiLine(2, ok ? "Success" : "Failed");
  delay(2000);

  usbRefreshMainboardFolderList();
}

// ===== COPY FOLDER =====
bool usbCopyFolder(String folderName) {
  uiClear();
  tilt_tech();
  uiLine(1, "Copy Folder:");
  uiLine(2, folderName);
  delay(500);

  byte fr = usbSendFolderCreate(folderName);
  if (fr == 0) {
    uiClear();
    tilt_tech();
    uiLine(1, "No reply from main");
    delay(2000);
    return false;
  }
  if (fr == 1) {
    uiClear();
    tilt_tech();
    uiLine(1, "Folder already");
    uiLine(2, "Exists - skip");
    delay(2000);
    return false;
  }
  if (fr == 3) {
    uiClear();
    tilt_tech();
    uiLine(1, "Mkdir failed");
    delay(2000);
    return false;
  }

  usbScanFilesInFolder(folderName);

  if (usbFileCount == 0) {
    uiClear();
    tilt_tech();
    uiLine(1, "No files to copy");
    delay(2000);
    return false;
  }

  byte copiedOk = 0, copiedFail = 0;

  for (byte i = 0; i < usbFileCount; i++) {
    uiClear();
    tilt_tech();
    uiLine(1, "Folder copy");
    uiLine(2, "File " + String(i + 1) + "/" + String(usbFileCount));
    uiLine(3, usbFileList[i]);

    if (usbCheckRemoved()) {
      uiLine(3, "Copy aborted");
      delay(2000);
      break;
    }

    bool r = usbCopyOneFile(folderName, usbFileList[i]);
    if (r) copiedOk++; else copiedFail++;
    delay(50);
  }

  uiClear();
  tilt_tech();
  uiLine(1, "Folder copy done");
  uiLine(2, "OK: " + String(copiedOk));
  uiLine(3, "Failed: " + String(copiedFail));
  delay(2000);

  usbRefreshMainboardFolderList();

  return (copiedFail == 0);
}

// ===== COPY ONE FILE (CORE FUNCTION) =====
bool usbCopyOneFile(String folderName, String fileName) {
  if (!usbCheckFileFormatMatch(folderName, fileName)) {
    return false;
  }

  String path = "/" + folderName + "/" + fileName;
  Serial.print("COPY FILE START: ");
  Serial.println(path);

  File usbSrcFile = usbMassStorage.open(path.c_str(), FILE_READ);
  if (!usbSrcFile) {
    Serial.print("ERROR: cannot open USB file: ");
    Serial.println(path);
    return false;
  }

  unsigned long srcSize = usbSrcFile.size();
  Serial.print("SENDING FILE: ");
  Serial.print(fileName);
  Serial.print(" SIZE: ");
  Serial.print(srcSize);
  Serial.println(" bytes");

  byte fr = usbSendFileOpen(fileName);
  if (fr == 0) {
    Serial.println("ERROR: no reply on file-open");
    usbSrcFile.close();
    return false;
  }

  if (fr == 1) {
    Serial.println("File exists - skipping (no overwrite)");
    delay(1000);
    usbSrcFile.close();
    return true;
  }

  if (fr == 2) {
    char pkt[64];
    snprintf(pkt, sizeof(pkt), "ABf6*%lu#{}", srcSize);
    for (int i = 0; pkt[i] != '\0'; i++) Serial2.write((byte)pkt[i]);
    Serial2.flush();
    Serial.print("TX: ");
    Serial.println(pkt);
    delay(10);
  }

  unsigned long height;
  unsigned int fileTotalData;
  if (!usbDeriveTotalData(usbSrcFile, srcSize, height, fileTotalData)) {
    Serial.println("ERROR: could not derive height/totaldata");
    usbSrcFile.close();
    usbAbortFileCopy();
    return false;
  }

  uiClear();
  tilt_tech();
  uiLine(1, "File copy:");
  uiLine(2, fileName);
  uiTransferProgress(0, 0, height);

  // Send header (62 bytes)
  Serial.println("TX HEADER START");
  Serial2.print('A'); Serial2.print('B'); Serial2.print('D'); Serial2.print('o');
  
  usbSrcFile.seek(0);
  unsigned long bytesSent = 0;
  for (int m = 0; m < bitmapOffset; m++) {
    byte headerByte = (byte)usbSrcFile.read();
    Serial2.write(headerByte);
    bytesSent++;
    delayMicroseconds(50);
  }
  Serial2.print('{'); Serial2.print('}');
  Serial2.flush();
  Serial.print("TX HEADER COMPLETE: ");
  Serial.print(bytesSent);
  Serial.println(" bytes sent");

  // Wait for header ACK
  unsigned long ackStart = millis();
  byte ackbuf[4] = {0};
  byte n = 0;
  bool ok = false;
  while (millis() - ackStart < 10000) {
    if (Serial2.available() > 0) {
      byte inbyte = (byte)Serial2.read();
      if (n < 4) ackbuf[n] = inbyte;
      n++;
      if (n >= 2 && ackbuf[0] == 68 && ackbuf[1] == 49) {
        ok = true;
        Serial.println("RX HEADER ACK D1");
        break;
      }
    }
  }

  if (!ok) {
    Serial.println("ERROR: offset ack timeout");
    usbSrcFile.close();
    usbAbortFileCopy();
    return false;
  }

  // Send per-card loop
  bool allOk = true;
  for (unsigned long ttotal = 1; ttotal <= height; ttotal++) {
    int percent = (int)(((unsigned long)ttotal * 100UL) / height);
    uiTransferProgress(percent, ttotal, height);

    if (usbCheckRemoved()) {
      allOk = false;
      break;
    }

    // Read the card once; a retry must resend this exact same payload.
    usbSrcFile.seek(((ttotal - 1) * fileTotalData) + bitmapOffset);
    unsigned long bytesRemaining = srcSize - (bitmapOffset + ((ttotal - 1) * fileTotalData));
    size_t toSend = (bytesRemaining > (unsigned long)fileTotalData) ? fileTotalData : (size_t)bytesRemaining;

    uint8_t pickBuf[1030];
    for (size_t m = 0; m < toSend; m++) {
      pickBuf[m] = (byte)usbSrcFile.read();
      delayMicroseconds(10);
    }
    uint16_t crc = crc16_ccitt(pickBuf, toSend);

    bool cardAcked = false;
    bool formatMismatch = false;
    String mismatchInfo = "";
    byte retries = 0;

    while (!cardAcked && retries < 5) {
      if (usbCheckRemoved()) { allOk = false; break; }

      // Send card parameters
      Serial2.print('A'); Serial2.print('B'); Serial2.print('c'); Serial2.print('t');
      Serial2.print('*'); Serial2.print(ttotal);
      Serial2.print('*'); Serial2.print(fileTotalData);
      Serial2.print('#'); Serial2.print('{'); Serial2.print('}');
      Serial2.flush();

      // Wait for D2 (accept) or D3 (main board format mismatch) ACK
      unsigned long cardAckStart = millis();
      byte prevByte = 0;
      bool controlAcked = false;
      while (millis() - cardAckStart < 10000) {
        if (Serial2.available() > 0) {
          byte b = (byte)Serial2.read();
          if (prevByte == 'D' && b == '2') {
            controlAcked = true;
            break;
          }
          if (prevByte == 'D' && b == '3') {
            unsigned long mmStart = millis();
            while (millis() - mmStart < 500) {
              if (Serial2.available() > 0) {
                char c = (char)Serial2.read();
                mismatchInfo += c;
                if (c == '#') break;
              }
            }
            formatMismatch = true;
            break;
          }
          prevByte = b;
        }
      }

      if (formatMismatch) break;

      if (!controlAcked) {
        retries++;
        Serial.print("Card parameter ACK timeout for card ");
        Serial.println(ttotal);
        continue;
      }

      // Send card data
      Serial2.print('A'); Serial2.print('B'); Serial2.print('D');
      Serial2.print((ttotal == height) ? 's' : 'e');
      for (size_t m = 0; m < toSend; m++) Serial2.write(pickBuf[m]);
      Serial2.write((byte)(crc >> 8));
      Serial2.write((byte)(crc & 0xFF));
      Serial2.flush();

      // Wait for D1 (success) or D0 (NACK)
      unsigned long dataAckStart = millis();
      prevByte = 0;
      bool receiverNack = false;
      while (millis() - dataAckStart < 10000) {
        if (Serial2.available() > 0) {
          byte b = (byte)Serial2.read();
          if (prevByte == 'D' && b == '1') { cardAcked = true; break; }
          if (prevByte == 'D' && b == '0') { receiverNack = true; break; }
          prevByte = b;
        }
      }

      if (!cardAcked) {
        retries++;
        Serial.print("Retrying card "); Serial.print(ttotal);
        Serial.print(" (attempt "); Serial.print(retries + 1);
        Serial.print(") - ");
        Serial.println(receiverNack ? "receiver NACK D0" : "ACK timeout");
      }
    }

    if (formatMismatch) {
      int s1 = mismatchInfo.indexOf('*');
      int s2 = mismatchInfo.indexOf('*', s1 + 1);
      int hashIdx = mismatchInfo.indexOf('#');
      String gotStr = (s1 != -1 && s2 != -1) ? mismatchInfo.substring(s1 + 1, s2) : "?";
      String expStr = (s2 != -1 && hashIdx != -1) ? mismatchInfo.substring(s2 + 1, hashIdx) : "?";
      Serial.print("MAIN BOARD FORMAT MISMATCH: file card size=");
      Serial.print(gotStr);
      Serial.print(" board expects=");
      Serial.println(expStr);
      uiClear();
      tilt_tech();
      uiLine(1, "Main board mismatch");
      uiLine(2, "File:" + gotStr + " Board:" + expStr);
      delay(3000);
      usbSrcFile.close();
      usbAbortFileCopy();
      return false;
    }

    if (!allOk) break;

    if (!cardAcked) {
      Serial.print("Card "); Serial.print(ttotal); Serial.println(" failed after all retries");
      allOk = false;
      break;
    }
  }

  usbSrcFile.close();

  if (allOk) {
    Serial.println("COPY FILE RESULT: SUCCESS");
    return true;
  } else {
    Serial.println("COPY FILE RESULT: FAILED");
    usbAbortFileCopy();
    return false;
  }
}

void Invert_Design()
{
  int invertMode = readInvertOption(tft,customKeypad,"TILT TECHNOLOGIES","INVERT DESIGN",BtoTmode);
  delay(100);
 if (invertMode == 1) 
 {
  BtoTmode = 1;
  Serial.println("Invert: ON");
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
 }
 else if (invertMode == 0) 
 {
  Serial.println("Invert: OFF");
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
 }
 else 
 {
  Serial.println("Invert selection cancelled");
 } 
}

long readeeprom()
{
  pass[0]='0';  
  pass[1]='0';
  pass[2]='0';
  pass[3]='0';
  pass[4]='0';
  pass[5]='0';
  int len = EEPROM.read(0);
  if (len > 6)
  return -1; 
  Serial.print("EEPROM_Pass:-");
  Serial.println(len);
  for (int i = 1; i <= len; i++)
  {
    pass[i - 1] = EEPROM.read(i);      
    delay(20);
    Serial.println(pass[i-1]);
    pass[i] = '\0';
  }     
  String ed = String(pass);
  Serial.print("ED:-");
  Serial.println(ed); 
  tcnt = ed.toInt();     
  return tcnt; 
}

void admin_Mode()
{
 rptloop: 
 long te = readeeprom();
 if (te == -1)
 {
  Serial.print(" First time Password:");
  cnt = New_Password(1);
  goto rptloop; 
 }
 Serial.print("Password eeprom :");
 Serial.println(te);
 cnt = New_Password(0);
 Serial.print("Password read :");
 Serial.println(cnt);
 if (ch == '*')
 {
  return;
 }
 tilt_tech();
 if ((cnt == 1234) | (cnt == te))  
 {
    tft.drawString("ACCESS GRANTED", 120, 150, 4);
    delay(2000);
    //showMainMenu();
  } 
  else 
  {    
    tft.drawString("WRONG PASSWORD", 120, 150, 4);
    delay(2000);
    goto rptloop;
  }


 int selectedIndex = 0;  
 adm: 
 tilt_tech();
 const int menuCount = sizeof(admin_Menu) / sizeof(admin_Menu[0]);
 unsigned int x = 0;
 unsigned int y = 0;   
 tft.drawString("SETTINGS MENU", 35, 60, 4);
 x = 90;
 y = 110;  
 drawMenu(x, y, TFT_BLACK, TFT_WHITE, admin_Menu, menuCount, selectedIndex, 10);
 admodeLoop:
  customKey = customKeypad.getKey();
  if (customKey) {
    if (customKey == 'A')                               // Up 
    { 
      if (selectedIndex > 0) selectedIndex--;
      else selectedIndex = menuCount - 1;
      drawMenu(x, y, TFT_BLACK, TFT_WHITE, admin_Menu, menuCount, selectedIndex, 10);
    }
    else if (customKey == 'B')                           // Down 
    { 
      if (selectedIndex < menuCount - 1) selectedIndex++;
      else selectedIndex = 0;
      drawMenu(x, y, TFT_BLACK, TFT_WHITE, admin_Menu, menuCount, selectedIndex, 10);
    }
    else if (customKey == 'E')                            // Select 
    {       
      delay(1000);      
      switch (selectedIndex)
      {        
        case 0:
               New_Password(1);
               goto adm;
        case 1:
               Number_of_Card(0);               
               goto adm;       
        case 2:
               Number_of_Card(1);               
               goto adm;
        case 3:
               Invert_Design();
               goto adm;        
        case 4:
               Jac_Profile();
               goto adm;                
      }     
      return;      
    }
    else if (customKey == '*')                              // Esc
    {
      delay(500);
      return;
    }
  }
 goto admodeLoop;  
}
