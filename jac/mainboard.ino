#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>
#include <SD.h>
#include <sd_defines.h>
#include <sd_diskio.h>
#include <SPI.h>
//#include <EEPROM.h>
#include "RTClib.h"
#include <Wire.h> 
#include <stdio.h>
#include "SparkFun_External_EEPROM.h" 

//folder changes worked

RTC_DS1307 rtc;
#define RXD2 16
#define TXD2 17

File root;
File root1;
File root3;
File bMap;
File bMap1;
File bMap3;
File bMap4;

//////////////////////////////////////////////////////////////////////////////////
const int nr = 16; //nr=number of ribbons
int nc = 12; 
//////////////////////////////////////////////////////////////////////////////////

ExternalEEPROM eeprom;

struct jac{
   byte outdata[300];  // ESP32
};

byte tempout[300]; // ESP32

byte kk1, lkk1, d = 0;
int check_bit = 0, baldays = 0, nnr = 0;   //   Check bits
const byte sensor1 = 33, sensor3 = 35;
unsigned long lastpickno = 0, copypick = 0, lastcopypick = 0;
const byte sensor2 = 34, bitmapOffset = 62;
  
String pick = "";
String copyfoldername = "", copyfilename = "";
byte sensor1status = 0, offstage = 0;
byte sensor2status = 0, lockvalue = 0, lockenablestatus = 0;
int lockdays = 0;
byte sensor3status = 0;
long sensor1check = 0;
long sensor2check = 0;
long sensor3check = 0;
byte lastsensorstate = 0;
byte lastsensorstate1 = 0, folderchange = 0;
long previousmillis = 0;
long interval = 61000;
byte filenum = 0;
byte filenum1 = 0;
char file1[15];
unsigned long pickno = 0;
unsigned long pickno1 = 0;
unsigned long height = 0;
unsigned long height1 = 0;
unsigned long repeatcnt = 0;
unsigned long repeatcnt1 = 0;
unsigned int br = 0;
String filelist[50], s_name, foldername1, filename2, folderlist[50], ls_name;
byte filerunningmode = 0, BtoTmode = 0, LtoRmode = 0, foldernum = 0;
byte totalfile = 0, totalfolder = 0;
byte break2, setup1 = 0;
String inputString = "";
int nr1 = 0;
int nc1 = 0;
byte data3[360];
byte break1;
//int odd_even = 0;


int ss = 5;
const int M_CLK = 13;
const int M_DATA = 14;
const int CL_OE = 25;
const int CL_CLK = 26;
const int Loom_Stop = 4;
const int Loom_Stop1 = 2;
 
byte temp = 0;
unsigned long temp2 = 0, temp3 = 0;
byte i = 0;

unsigned int totaldata = ((nr * 32) + 4);
unsigned int copyTotalData = 0; 
byte data1[1030];
byte data2[1030];
byte data4[1030];

jac outdata1[nr];

//must addcrc
void setup() {
  delay(3000);
  rtc.begin();
  delay(200);
  Serial.begin(230400);
  Serial2.setRxBufferSize(2048);
  delay(200);
  Serial2.begin(230400, SERIAL_8N1, RXD2, TXD2);
  delay(200);
  pinMode(M_CLK, OUTPUT);
  pinMode(M_DATA, OUTPUT);
  pinMode(CL_OE, OUTPUT);
  pinMode(CL_CLK, OUTPUT);
  pinMode(Loom_Stop, OUTPUT);
  pinMode(Loom_Stop1, OUTPUT);  
  
  delay(50);
  digitalWrite(CL_OE,HIGH);
  digitalWrite(M_CLK,LOW);
  digitalWrite(CL_CLK,HIGH);
  digitalWrite(Loom_Stop1,LOW);  
  delay(200);     
  //pinMode(40, OUTPUT);
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
  pinMode(sensor3, INPUT);
  //digitalWrite(sensor1, HIGH);
  //digitalWrite(sensor2, HIGH);
  //digitalWrite(sensor3, HIGH);   
  //digitalWrite(loomstop1, LOW);  
  delay(10);
  Serial.println("Electronics Jacquard CPU");   
  delay(100);
  nc = byte(eeprom.read(13));  
  delay(100);
  if ((nc > 32) && (nc <= 0))
  {
   //EEPROM.write(33, 0);
   delay(100);
   eeprom.write(13, 12);
   delay(100);
   nc = 12;
  } 
  delay(100);
  Serial.print("nc:  ");
  delay(10);
  Serial.println(nc);
  delay(10); 
  delay(100);
  totaldata = ((nr * nc) + 4);
  delay(100);
  check_bit = 0; //nc * nr;
  delay(100);
  Serial.print("check_bit:- ");
  delay(100);
  Serial.println(check_bit);
  delay(100);
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    rtc.adjust(DateTime(2023, 3, 17, 18, 45, 0));
  }
  Serial.println("rtc.isrunning");
  delay(10);
  while (true)
  {
    if (Serial2.available() > 0)
    {
      temp3 = 0;
      Serial.println("serial int1");
      rundesign1();
      if (d == 1)
      {
        d = 0;
        goto f1cout1;
      }
    }
  }
f1cout1:

  inbufferclear();
  offstage = 0;
  temp3 = 0;
  delay(20);
  lockvalue = eeprom.read(0);
  Serial.println(lockvalue);
  if ((lockvalue >= 48) && (lockvalue <= 57))
  {
    if (lockvalue == 48)
    {
      lockenablestatus = 0;
      Serial.println("LOCK DISABLE");
      goto gh;
    }
    temp = eeprom.read(5);
    Serial.println(temp);
    delay(20);
    if ((temp >= 48) && (temp <= 57))
    {
      if ((lockvalue == 49) && (temp == 49) )
      {
        lockenablestatus = 1;
        Serial.println("LOCK ENABLED");
      }
      else if (lockvalue == 49)
      {
        byte ct1 = 0, ct2 = 0;
        DateTime now0 = rtc.now();
        ct1 = now0.second();
        delay(3000);
        DateTime now1 = rtc.now();
        ct2 = now1.second();
        if (ct1 == ct2)
        {
          delay(10);
          lockenablestatus = 1;
          Serial.println("RTC CRYSTAL ERROR");
          delay(1000);
          goto gh;
        }
        lockread();
      }
      else
      {
        lockenablestatus = 0;
      }
    }
  }
  else
  {
    lockenablestatus = 0;
    lockdays = 0;
    delay(10);
    eeprom.write(5, lockdays);
    delay(10);
  }
gh:
   for (int r = 0; r < nr; r++)             //ESP32
  {
    for (int i = 0; i < 300; i++)
   {
    outdata1[r].outdata[i] = 0; 
   }
  }
  delay(10);
  Serial.println("Start to run");
  delay(10);
  sensor1check =  rtc.readnvram(21);//eeprom.readByte(8); //EEPROM.read(8);
  delay(10);
  sensor1check = sensor1check % 48;
  if (sensor1check > 1)
  {
    sensor1check = 1;
    delay(10);
    //EEPROM.write(8, sensor1check);
    //eeprom.writeByte(8, sensor1check);
    rtc.writenvram(21, sensor1check);
    delay(10);
  }
  sensor2check = rtc.readnvram(22); //eeprom.readByte(9); //EEPROM.read(9);
  sensor2check = sensor2check % 48;
  if (sensor2check > 1)
  {
    sensor2check = 0;
    delay(10);
    //EEPROM.write(9, sensor2check);
    //eeprom.writeByte(9, sensor2check);
    rtc.writenvram(22, sensor2check);
    delay(10);
  }
  down();
  delay(10);
  onepulse2();
  delay(10);
  outshifter();
  digitalWrite(CL_OE, LOW);
  colorselector();
  delay(1000);
}

void loop()
{
read1:
  if (Serial2.available() > 0)
  {
    temp3 = 0;
    Serial.println("serial int");
    rundesign1();
  }
  
  sensor1status = digitalRead(sensor1);
  if (sensor1status == LOW)
  {
    temp3 += 1;
    if ((lastsensorstate == 1) || (offstage == 0))
    {
      offstage = 1;
      lastsensorstate = 0;
      if ((sensor1check > sensor2check))  // || (offstage == 0))
      {
        Serial.println("pulse on");
        temp3 = 0;
        sensor2check = sensor1check;
        //EEPROM.write(8, sensor1check);         
        //eeprom.writeByte(8, sensor1check);
        rtc.writenvram(21, sensor1check);
         
        //EEPROM.write(9, sensor2check);
        //eeprom.writeByte(9, sensor2check);
        rtc.writenvram(22, sensor2check);        
        onepulse2();       
        //if (lockenablestatus == 0)
        //{
        digitalWrite(Loom_Stop1, LOW);
        outshifter();
        colorselector();
        //}
        digitalWrite(CL_OE, LOW);
        valuedisplay();
      }
      goto read1;
    }
  }

  if (sensor1status == HIGH)
  {
    if (lastsensorstate == 0)
    {
      temp3 = 50;
      lastsensorstate = 1;
    }
  }

  sensor2status = digitalRead(sensor2);

  if (sensor2status == LOW)
  {
    temp3 += 1;
    if (lastsensorstate1 == 1) //|| (offstage == 0))
    {
      lastsensorstate1 = 0;

      if (sensor1check == sensor2check)
      {
        temp3 = 0;
        //sensor1check = sensor1check + 1;
        sensor1check = 1;
        sensor2check = 0;
        //EEPROM.write(8, sensor1check);         
        //eeprom.writeByte(8, sensor1check);
        rtc.writenvram(21, sensor1check);        
        //EEPROM.write(9, sensor2check);
        //eeprom.writeByte(9, sensor2check);
        rtc.writenvram(22, sensor2check);         
        onepulse2();       
        //if (lockenablestatus == 0)
        //{
        digitalWrite(Loom_Stop1, LOW);
        outshifter();
        colorselector();
        //}
        digitalWrite(CL_OE, LOW);
        valuedisplay();
        //digitalWrite(CL_OE, HIGH); 
      }
      goto read1;
    }
  }

  if (sensor2status == HIGH)
  {
    if (lastsensorstate1 == 0)
    {
      temp3 = 50;
      lastsensorstate1 = 1;
    }
  }

  if ((sensor2status == LOW) ^ (sensor1status == LOW))
  {
    if ((temp3 >= 50) && (temp3 <= 37500000))

    {
      digitalWrite(CL_OE, HIGH);       
    }
    else
    {       
      digitalWrite(CL_OE, LOW);      
    }
  }
  else
  {
    digitalWrite(CL_OE, LOW);
  } 

  sensor3status = digitalRead(sensor3);

  if (sensor3status == LOW)
  {
    if (sensor3check == 1)
    {
      sensor3check = 0;
      //colorselector();
    }
  }

  if (sensor3status == HIGH)
  {
    if (sensor3check == 0)
    {
      sensor3check = 1;
    }
  }
    
}

void outshifter()
{
 if(LtoRmode == 1) 
  {
   shift21();
   Serial.println("shift21");
  }
  if(LtoRmode == 4)
  {
   shift2();
   Serial.println("shift2");
  }
  if(LtoRmode == 3)
  {
    shift12();
    Serial.println("shift12");
  }
  if(LtoRmode == 2)
  {
    shift1();
    Serial.println("shift1");
  }  
}

void inbufferclear()
{
  for (byte r = 0; r < 100; r++)
  {
    data3[r] = 0;
  }
}

void colorselector()
{ 
  int csl = 0;   
  Serial.print("Color Selector Value: ");
  Serial.println(data1[0]);
  if(nc <= 8)
  csl = 16;
  if(nc >= 8 && nc <= 16)
  csl = 24;
 digitalWrite(Loom_Stop1, LOW);    
 for (int m = 0; m < csl; m++)
  {
   if (m <= 7)
   {      
    digitalWrite(M_DATA, bitRead(data1[0], m));           
   }
   else
   {
    digitalWrite(M_DATA,LOW);
   }
   delayMicroseconds(15); 
   digitalWrite(M_CLK, HIGH);            
   delayMicroseconds(15);
   digitalWrite(M_CLK, LOW);
  }
  digitalWrite(Loom_Stop1, HIGH);  
}

void shift21()
{
  unsigned int ik = 0;
  int pinState[nr];

  ik = nc * 8;
  //clear everything out just in case to
  //prepare shift register for bit shifting
  //for (int m = 0; m < nr; m++)
    //digitalWrite(dsl[m], 0);   
  digitalWrite(CL_CLK, HIGH);  
  digitalWrite(M_CLK, LOW); 
  digitalWrite(Loom_Stop, LOW);
  digitalWrite(Loom_Stop1, LOW);
  for (int n =0 ; n < ik; n++)
  {     
      //Sets the pin to HIGH or LOW depending on pinState
      for (int m = 0; m < nr; m++)
      {      
       digitalWrite(M_DATA, outdata1[m].outdata[n]);           
       delayMicroseconds(1); 
       digitalWrite(M_CLK, HIGH);         
       delayMicroseconds(1);
       digitalWrite(M_CLK, LOW);
      }  
      digitalWrite(Loom_Stop, HIGH); 
      delayMicroseconds(1);       
      digitalWrite(CL_CLK, LOW);        
      delayMicroseconds(1);
      digitalWrite(CL_CLK, HIGH);
      digitalWrite(Loom_Stop, LOW);
      
      //zero the data pin after shift to prevent bleed through
      //for (int m = 0; m < nr; m++)
      //digitalWrite(dsl[m], 0);
    
  }
  
  /*if (bitRead(data1[check_bit],6) == 0)
  {
    digitalWrite(loomstop2, HIGH); //loomstop2
    digitalWrite(loomstop1, HIGH); //loomstop1
    delay(500);
    digitalWrite(loomstop1, LOW);//loomstop1
    digitalWrite(loomstop2, LOW);//loomstop2
  }
  else
  {
    digitalWrite(loomstop1, LOW); //loomstop1
    digitalWrite(loomstop2, LOW); //loomstop2
  }*/
}


void shift2()
{
  unsigned int ik = 0;
  //int pinState[nr];

  ik = nc * 8;
  //clear everything out just in case to
  //prepare shift register for bit shifting
  //for (int m = 0; m < nr; m++)
    //digitalWrite(dsl[m], 0);   
  digitalWrite(CL_CLK, HIGH);  
  digitalWrite(M_CLK, LOW); 
  digitalWrite(Loom_Stop, LOW);
  
  for (int n = (ik - 1); n >= 0; n--)
  {     
      //Sets the pin to HIGH or LOW depending on pinState
      for (int m = 0; m < nr; m++)
      {      
       digitalWrite(M_DATA, outdata1[m].outdata[n]);           
       delayMicroseconds(1); 
       digitalWrite(M_CLK, HIGH);         
       delayMicroseconds(1);
       digitalWrite(M_CLK, LOW);
      }  
      digitalWrite(Loom_Stop, HIGH); 
      delayMicroseconds(1);       
      digitalWrite(CL_CLK, LOW);        
      delayMicroseconds(1);
      digitalWrite(CL_CLK, HIGH);
      digitalWrite(Loom_Stop, LOW);
      
      //zero the data pin after shift to prevent bleed through
      //for (int m = 0; m < nr; m++)
      //digitalWrite(dsl[m], 0);
    
  }
  
  /*if (bitRead(data1[check_bit],6) == 0)
  {
    digitalWrite(loomstop2, HIGH); //loomstop2
    digitalWrite(loomstop1, HIGH); //loomstop1
    delay(500);
    digitalWrite(loomstop1, LOW);//loomstop1
    digitalWrite(loomstop2, LOW);//loomstop2
  }
  else
  {
    digitalWrite(loomstop1, LOW); //loomstop1
    digitalWrite(loomstop2, LOW); //loomstop2
  }*/
}

void shift12()
{
  unsigned int ik = 0;
  int pinState[nr];

  ik = nc * 8;
  //clear everything out just in case to
  //prepare shift register for bit shifting
  //for (int m = 0; m < nr; m++)
    //digitalWrite(dsl[m], 0);   
  digitalWrite(CL_CLK, HIGH);  
  digitalWrite(M_CLK, LOW); 
  digitalWrite(Loom_Stop, LOW);
  
  for (int n = (ik - 1); n >= 0; n--)
  {     
      //Sets the pin to HIGH or LOW depending on pinState
      for (int m = (nr - 1); m >= 0; m--)
      {      
       digitalWrite(M_DATA, outdata1[m].outdata[n]);           
       //delayMicroseconds(10); 
       digitalWrite(M_CLK, HIGH);         
       delayMicroseconds(15);
       digitalWrite(M_CLK, LOW);
      }  
      digitalWrite(Loom_Stop, HIGH); 
      //delayMicroseconds(10);       
      digitalWrite(CL_CLK, LOW);        
      delayMicroseconds(15);
      digitalWrite(CL_CLK, HIGH);
      digitalWrite(Loom_Stop, LOW);
      
      //zero the data pin after shift to prevent bleed through
      //for (int m = 0; m < nr; m++)
      //digitalWrite(dsl[m], 0);
    
  }
  
  /*if (bitRead(data1[check_bit],6) == 0)
  {
    digitalWrite(loomstop2, HIGH); //loomstop2
    digitalWrite(loomstop1, HIGH); //loomstop1
    delay(500);
    digitalWrite(loomstop1, LOW);//loomstop1
    digitalWrite(loomstop2, LOW);//loomstop2
  }
  else
  {
    digitalWrite(loomstop1, LOW); //loomstop1
    digitalWrite(loomstop2, LOW); //loomstop2
  }*/
}

void shift1()
{
  unsigned int ik = 0;
  int pinState[nr];

  ik = nc * 8;
  //clear everything out just in case to
  //prepare shift register for bit shifting
  //for (int m = 0; m < nr; m++)
    //digitalWrite(dsl[m], 0);   
  digitalWrite(CL_CLK, HIGH);  
  digitalWrite(M_CLK, LOW); 
  digitalWrite(Loom_Stop, LOW);
  
  for (int n = 0; n < ik; n++)
  {     
      //Sets the pin to HIGH or LOW depending on pinState
      for (int m = (nr - 1); m >= 0; m--)
      {      
       digitalWrite(M_DATA, outdata1[m].outdata[n]);           
       delayMicroseconds(1); 
       digitalWrite(M_CLK, HIGH);         
       delayMicroseconds(1);
       digitalWrite(M_CLK, LOW);
      }  
      digitalWrite(Loom_Stop, HIGH); 
      delayMicroseconds(1);       
      digitalWrite(CL_CLK, LOW);        
      delayMicroseconds(1);
      digitalWrite(CL_CLK, HIGH);
      digitalWrite(Loom_Stop, LOW);
      
      //zero the data pin after shift to prevent bleed through
      //for (int m = 0; m < nr; m++)
      //digitalWrite(dsl[m], 0);
    
  }
  
  /*if (bitRead(data1[check_bit],6) == 0)
  {
    digitalWrite(loomstop2, HIGH); //loomstop2
    digitalWrite(loomstop1, HIGH); //loomstop1
    delay(500);
    digitalWrite(loomstop1, LOW);//loomstop1
    digitalWrite(loomstop2, LOW);//loomstop2
  }
  else
  {
    digitalWrite(loomstop1, LOW); //loomstop1
    digitalWrite(loomstop2, LOW); //loomstop2
  }*/
}

void lockread()
{
  byte tt1 = 0;
  int tt2 = 0;
  delay(20);
  DateTime now = rtc.now();
  lockvalue = eeprom.read(0);
  Serial.println(lockvalue);
  delay(20);
  if ((lockvalue >= 48) && (lockvalue <= 57))
  {
    if (lockvalue == 49)
    {
      delay(20);
      temp = eeprom.read(1);
      if ((temp >= 48) && (temp <= 57))
      {
        temp = temp % 48;
        lockdays = temp * 100;
      }
      else
      {
        lockdays = 0;
        delay(10);
        eeprom.write(1, lockdays);
        delay(10);
      }
      delay(10);
      temp = eeprom.read(2);
      if ((temp >= 48) && (temp <= 57))
      {
        temp = (temp % 48) * 10;
        lockdays = lockdays + temp;
      }
      else
      {
        lockdays = 0;
        delay(10);
        eeprom.write(2, lockdays);
        delay(10);
      }
      temp = eeprom.read(3);
      Serial.println(temp);
      if ((temp >= 48) && (temp <= 57))
      {
        temp = temp % 48;
        lockdays = lockdays + temp;
      }
      else
      {
        lockdays = 0;
        delay(10);
        eeprom.write(3, lockdays);
        delay(10);
      }
      //Serial.print("LOCKDAYS: ");
      //Serial.println(lockdays);

      temp = eeprom.read(4);
      Serial.println(temp);
      if ((temp >= 1) && (temp <= 31))
      {
        //lockdays = lockdays + temp;
        tt1 = now.day();
        Serial.println("DAY:");
        Serial.println(temp);
        if (temp != tt1)
        {
          lockdays = lockdays - 1;
          if (lockdays <= 0)
          {
            tt1 = 49;
            delay(10);
            eeprom.write(5, tt1);
            delay(10);
            lockenablestatus = 1;
            Serial.println("LOCK ENABLED");
            lockdays = 0;
            delay(10);
            eeprom.write(1, lockdays);
            delay(10);
            eeprom.write(2, lockdays);
            delay(10);
            eeprom.write(3, lockdays);
            delay(10);
          }
          else
          {
            tt2 = lockdays / 100;
            tt1 = tt2 + 48;
            delay(10);
            eeprom.write(1, tt1);
            delay(10);
            tt2 = lockdays / 10;
            tt2 = tt2 % 10;
            tt1 = tt2 + 48;
            delay(10);
            eeprom.write(2, tt1);
            delay(10);
            tt2 = lockdays % 10;
            tt1 = tt2 + 48;
            delay(10);
            eeprom.write(3, tt1);
            delay(10);
          }
          delay(10);
          tt1 = now.day();
          delay(10);
          eeprom.write(4, tt1);
          delay(10);
        }
        else
        {
          Serial.println("SAME DAY:");
        }
      }
      else
      {
        temp = now.day();
        Serial.println("DAY:");
        Serial.println(temp);
        delay(10);
        eeprom.write(4, temp);
        delay(10);
      }
    }
    else
    {
      lockdays = 0;
      delay(10);
      eeprom.write(0, lockdays);
      delay(10);
      eeprom.write(1, lockdays);
      delay(10);
      eeprom.write(2, lockdays);
      delay(10);
      eeprom.write(3, lockdays);
      delay(10);
      //Serial.print("LOCKDAYS1: ");
      //Serial.println(lockdays);
    }
  }
}


void masterint()
{
  delay(100);
  pick = "Initiative SDCard";
  Serial.println(pick);
  intreplay(pick);
  delay(200);
  if (!SD.begin(ss)) {
    pick = "SD Card failed";
    Serial.println(pick);
    intreplay(pick);
    delay(200);
    pick = "Please Insert SDCard";
    Serial.println(pick);
    intreplay(pick);
sd:
    delay(200);
    if (!SD.begin(ss)) {
      goto sd;
    }
  }
  delay(200);
  pick = "Initiated....        ";
  Serial.println(pick);
  intreplay(pick);
  delay(200);
  pick = "Loading SD Card....";
  Serial.println(pick);
  intreplay(pick);
  delay(100);
  root3 = SD.open("/");
  delay(100);  
  BtoTmode = eeprom.read(33); //EEPROM.read(33);
  BtoTmode = BtoTmode % 48;
  Serial.println("BtoTmode:  ");
  Serial.print(BtoTmode);
  if (BtoTmode > 1)
  {
    //EEPROM.write(33, 0);
    delay(100);
    eeprom.write(33, 0);
    delay(100);
    BtoTmode = 0;
  }

  LtoRmode = eeprom.read(12); //EEPROM.read(12);
  LtoRmode = LtoRmode % 48;
  if (LtoRmode > 4)
  {
    LtoRmode = 4;
    delay(100);
    //EEPROM.write(12, LtoRmode);
    eeprom.write(12, LtoRmode);
    delay(100);
  }

  temp = eeprom.read(10); //EEPROM.read(10);
  temp = temp % 48;
  if (temp > 2)
  {
    temp = 2;
    delay(100);
    //EEPROM.write(10, 2);
    eeprom.write(10, 2);
    delay(100);
  }
  filerunningmode = temp;
  setup1 = 0;
  totalfile = 0;
  totalfolder = 0;
  delay(200);
  pick = "Loading Folder&Files";   
  Serial.println(pick);
  //intreplay(pick);   
  readfolderlist(root3);   // this below 3 fuction are printing the folder name 
  delay(200);
  setupoffolder();
  delay(100);
  sendtotalfolder();
  delay(100);
  sendtotalfile();
  delay(100);
  pick = "Loading Folders.....";
  Serial.println(pick);
  intreplay(pick);
  delay(100);
  sendfolderlist();
  delay(100);
  sendfilelist();
  delay(100);
  pick = "Loading Files";
  Serial.println(pick);
  intreplay(pick);
  delay(100);
  currentfoldernumber();
  delay(100);
  //check_bit = nc * nr;  
  pick = "Jacquard Capacity..";
  Serial.println(pick);
  intreplay(pick);
  delay(100);
  nctype();
  delay(100);
  BtTtype(); 
  delay(100);
  LtRtype();
  delay(100);
  machinetype();
  delay(500);
  runninginit();
  delay(50);
  d = 1;
}


void TKitmenu()
{
  delay(10);
  sensor1status = digitalRead(sensor1);
  delay(10);
  sensor2status = digitalRead(sensor2);
  delay(10);
  Serial2.print('C');
  delay(1);
  Serial2.print('E');
  if (sensor1status == LOW)
  {
    Serial2.print('0');
  }
  else
  {
    Serial2.print('1');
  }
  delay(1);
  if (sensor2status == LOW)
  {
    Serial2.print('0');
  }
  else
  {
    Serial2.print('1');
  }
}

void filerunningmode1()
{
  // Serial2.print('A');
  // Serial2.print('B');
  // Serial2.print('M');
  // Serial2.print(filerunningmode);
  // Serial2.print('1');
  // Serial2.print('#');
  // Serial2.print('&');
     char packet[10];
  snprintf(packet, sizeof(packet), "ABM%d1#&", filerunningmode);
  sendPacket(packet);   // writes all bytes in one loop + Serial2.flush()

}



void currentfoldernumber()
{
  // Send current folder number in format: ABF<foldernum>{}
  char packet[50];
  //snprintf(packet, sizeof(packet), "ABF%d{}", foldernum);
  snprintf(packet, sizeof(packet), "ABF%dK#&", foldernum);
  // Send atomically
  for (int i = 0; packet[i] != '\0'; i++) {
    Serial2.write((byte)packet[i]);
  }
  Serial2.flush();
  
  Serial.print("SENT CURRENT FOLDER: ");
  Serial.println(packet);
}

void f1fileno()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('D');
  Serial2.print('K');
  Serial2.print(filenum);
  Serial2.print('#');
  Serial2.print('&');
}

void f1pickno()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('D');
  Serial2.print('2');
  Serial2.print('*');
  Serial2.print(pickno);
  Serial2.print('#');
  Serial2.print('&');
}

void f1hightno()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('D');
  Serial2.print('3');
  Serial2.print('*');
  Serial2.print(height);
  Serial2.print('#');
  Serial2.print('&');
}

void f1repeatno()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('D');
  Serial2.print('4');
  Serial2.print('*');
  Serial2.print(repeatcnt);
  Serial2.print('#');
  Serial2.print('&');
}

void f2fileno()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('D');
  Serial2.print('M');
  Serial2.print(filenum1);
  Serial2.print('#');
  Serial2.print('&');
}

void f2pickno()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('D');
  Serial2.print('6');
  Serial2.print('*');
  Serial2.print(pickno1);
  Serial2.print('#');
  Serial2.print('&');
}

void f2hightno()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('D');
  Serial2.print('7');
  Serial2.print('*');
  Serial2.print(height1);
  Serial2.print('#');
  Serial2.print('&');
}

void f2repeatno()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('D');
  Serial2.print('8');
  Serial2.print('*');
  Serial2.print(repeatcnt1);
  Serial2.print('#');
  Serial2.print('&');
}

void sendtotalfolder()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('T');
  Serial2.print('K');
  Serial2.print(totalfolder);
  Serial2.print('#');
  Serial2.print('&');
}

void sendtotalfile()
{

  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('T');
  Serial2.print('L');
  Serial2.print(totalfile);
  Serial2.print('#');
  Serial2.print('&');
}

void intreplay(String pick1 )
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('I');
  Serial2.print('1');
  Serial2.print('*');
  Serial2.print(pick1);
  Serial2.print('#');
  Serial2.print('&');
}

void machinetype()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('I');
  Serial2.print('1');
  Serial2.print('*');  
  Serial2.print(nr);
  Serial.print(nr);
  Serial2.print('x');
  Serial.print('x');
  Serial2.print(nc);
  Serial.print(nc);
  Serial2.print('#');
  Serial2.print('&');
}

void BtTtype()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('b');
  Serial2.print(BtoTmode);  
  Serial2.print('#');
  Serial2.print('&');
}

void nctype()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('n');
  Serial2.print(nc);  
  Serial2.print('#');
  Serial2.print('&');
}

void LtRtype()
{
  Serial2.print('A');
  Serial2.print('B');
  Serial2.print('i');
  Serial2.print(LtoRmode);  
  Serial2.print('#');
  Serial2.print('&');
}

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

void sendfolderlist()
{
  if (totalfolder == 0) {
    Serial.println("ERROR: No folders to send");
    return;
  }
  
  Serial.print("SENDING ");
  Serial.print(totalfolder);
  Serial.println(" FOLDERS");
  
  char packet[300];
  
  // Send each folder
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

void sendfilelist()
{
  if (totalfile == 0) {
    Serial.println("ERROR: No files to send");
    return;
  }
  
  Serial.print("SENDING ");
  Serial.print(totalfile);
  Serial.println(" FILES");
  
  char packet[300];
  
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


void valuedisplay()
{
  Serial.print("F1:");
  s_name = filelist[filenum];
  s_name.trim();
  kk1 = s_name.length();
  kk1 = kk1 % 48;
  kk1 = kk1 - 4;
  s_name.remove(kk1);
  Serial.println(s_name);
  Serial.print(pickno);
  Serial.print('\t');
  Serial.print(height);
  Serial.print('\t');
  Serial.println(repeatcnt);
  delay(10);
  f1fileno();
  delay(10);
  f1pickno();
  delay(10);
  f1hightno();
  delay(10);
  f1repeatno();
  delay(10);

  if (filerunningmode == 2)
  {
    Serial.print("F2:");
    s_name = filelist[filenum1];
    delay(10);
    f2fileno();
    s_name.trim();
    kk1 = s_name.length();
    kk1 = kk1 % 48;
    kk1 = kk1 - 4;
    s_name.remove(kk1);
    Serial.println(s_name);
    Serial.print(pickno1);
    Serial.print('\t');
    Serial.print(height1);
    Serial.print('\t');
    Serial.println(repeatcnt1);
    delay(10);
    f2pickno();
    delay(10);
    f2hightno();
    delay(10);
    f2repeatno();
    delay(0);
  }
}

void(* resetFunc) (void) = 0;

void rundesign1()
{
  inbufferclear();
  inputString = "";
  temp3 = 0;
  nc1 = 0;
  int nc2 = 0;
  while (true)
  {
    if (Serial2.available() > 0)
    {
      byte inbyte = (byte)Serial2.read();
      data3[nc1] = inbyte;
      //nc1 = nc1 + 1;
      if (data3[0] == 65)          // for header 'A'
      {
        nc1 = nc1 + 1;
        if (data3[nc1 - 2] == 66)       // for header 'B'
        {
          if (data3[nc1 - 1] == 68)     // for header 'D'
          {
            nc2 = 0;
            while (true)
            {
              if (Serial2.available() > 0)
              {
                byte inbyte = (byte)Serial2.read();
                char inChar = (char)inbyte;
                data3[nc1] = inbyte;
                nc1 = nc1 + 1;
                if (data3[3] == 111)   // 'o' - Offset/header packet
{
  Serial.println("RX HEADER CMD: ABDo detected, expecting 62 bytes...");
  nc2 = 0;
  unsigned long offsetTimeout = millis();

  while (nc2 < 62) {
    if (Serial2.available() > 0) {
      data1[nc2] = (byte)Serial2.read();
      Serial.print("RX HB["); Serial.print(nc2); Serial.print("]="); Serial.println(data1[nc2]);
      nc2++;
      offsetTimeout = millis();
    }
    if (millis() - offsetTimeout > 1000) {
      Serial.print("ERROR: Offset packet timeout at byte ");
      Serial.println(nc2);
      Serial2.print('D'); Serial2.print('0');
      Serial2.flush();
      return;
    }
  }

  Serial.println("RX HEADER COMPLETE (62 bytes received)");
  bMap4.seek(0);
  for (int i = 0; i < 62; i++) {
    bMap4.write(data1[i]);
  }
  bMap4.flush();

  lastcopypick = 0;
  copypick = 0;
  Serial.println("Offset Created (62 bytes) — sending D1 ACK");
  Serial2.print('D'); Serial2.print('1');
  Serial2.flush();
  return;
}
           /*    if (data3[3] == 111)        //Offset DATA
                {
                  
                  data1[nc2] = inbyte;
                  nc2 = nc2 + 1;
                  if (nc2 > 64)
                  {
                    bMap4.seek(1);
                    for (int i = 1; i <= 62; i++)
                      bMap4.write(data1[i]);
                    delay(20);
                    lastcopypick = 0;
                    copypick = 0;
                    Serial.println("Offset Created");
                    Serial.println("DEBUG ESP32: SENDING FILE OPEN ACK D1");
                    Serial2.print('D');
                    Serial2.print('1');
                    Serial2.flush();
                    return;
                  }
                } */
                if ((data3[3] == 101) || (data3[3] == 115))   // 'e' or 's' - card data
{
  nc2 = 0;
  unsigned long cardTimeout = millis();

  while (nc2 < copyTotalData + 2) {  // +2 for CRC bytes 
         if (Serial2.available() > 0) {
      data1[nc2] = (byte)Serial2.read();
      nc2++;
      cardTimeout = millis();
    }
    if (millis() - cardTimeout > 2000) {
      Serial.print("ERROR: Card data timeout at byte ");
      Serial.print(nc2);
      Serial.print(" / expected pick ");
      Serial.println(copypick);
      Serial2.print('D'); Serial2.print('0');
      Serial2.flush();
      return;
    }
  }

  Serial.print("RX CARD COMPLETE: pick=");
  Serial.print(copypick);
  Serial.print(" lastcopypick=");
  Serial.println(lastcopypick);
 // --- CRC CHECK (new block) ---
  uint16_t recvCrc = ((uint16_t)data1[copyTotalData] << 8) | data1[copyTotalData + 1];
  uint16_t calcCrc = crc16_ccitt(data1, copyTotalData);
  bool crcOk = (recvCrc == calcCrc);
  Serial.print("CRC CHECK: card ");
  Serial.print(copypick);
  Serial.print(": received=0x");
  Serial.print(recvCrc, HEX);
  Serial.print(" calculated=0x");
  Serial.println(calcCrc, HEX);
  if (!crcOk) {
    Serial.println("CRC RESULT: FAIL - TX D0 NACK");
    Serial.print("CRC FAILED: rejected card payload; bit position not identified");
    Serial.println();
  } else {
    Serial.println("CRC RESULT: PASS - DATA VALID");
  }
  if (!crcOk) {
    Serial.print("ERROR: CRC mismatch, expected ");
    Serial.print(calcCrc, HEX);
    Serial.print(" got ");
    Serial.println(recvCrc, HEX);
  }
  // --- END CRC CHECK ---
  if (crcOk && (lastcopypick + 1 )== copypick)
  {
    bMap4.seek(((copypick - 1) * copyTotalData) + bitmapOffset);
    for (int i = 0; i < copyTotalData; i++) {          // 0..totaldata-1, correct range
      bMap4.write(data1[i]);
    }
    bMap4.flush();
    lastcopypick = copypick;

    Serial.print("Data written OK : ");
    Serial.println(copypick);
    Serial2.print('D'); Serial2.print('1');
    Serial.println("ACK SENT: D1 - CARD WRITTEN");
  }
  else
  {
    if (crcOk)
    {
      Serial.print("ERROR: card sequence mismatch, expected ");
      Serial.print(lastcopypick + 1);
      Serial.print(" got ");
      Serial.println(copypick);
    }
    Serial2.print('D'); Serial2.print('0');   // NACK instead of silence, so RP2040 retries promptly
    Serial.println("NACK SENT: D0 - CARD REJECTED");
  }
  Serial2.flush();

  if (data3[3] == 115) {
    Serial.println("FILE Closed");
    bMap4.close();
    lastcopypick = 0;
    copypick = 0;
     copyTotalData = 0; 
  }
  return;
}
              /*  if ((data3[3] == 101) || (data3[3] == 115))       //CARD DATA
                {
                  data1[nc2] = inbyte;
                  nc2 = nc2 + 1;
                  if (nc2 > (totaldata + 2))
                  {
                    if ((lastcopypick + 1) == copypick)
                    {  
                      delayMicroseconds(50);
                      bMap4.seek(((copypick - 1) * totaldata) + bitmapOffset); 
                      delayMicroseconds(50);                    
                      for (int i = 1; i <= totaldata; i++)
                      {
                        delayMicroseconds(10);
                        bMap4.write(data1[i]);
                        delayMicroseconds(10);                       
                      }  
                      delay(20);
                      lastcopypick = copypick;                      
                      Serial2.print('D');
                      Serial2.print('1');
                      Serial.print("Data : ");
                      Serial.println(copypick);
                      Serial2.flush();
                    }
                    if (data3[3] == 115)
                    {
                      Serial.print("FILE Close");
                      bMap4.close();
                      lastcopypick = 0;
                      copypick = 0;
                    }
                    return;
                  }
                }  */
              }
            }

            Serial.println("CARD DATA RECEIVED");
            //goto ooout;
          }
          else
          {
            while (true)
            {
              if (Serial2.available() > 0)
              {
                byte inbyte = (byte)Serial2.read();
                char inChar = (char)inbyte;
                inputString += inChar;
                data3[nc1] = inbyte;
                nc1 = nc1 + 1;
                if (data3[nc1 - 2] == 123)
                {
                  if (data3[nc1 - 1] == 125)
                  {
                    if (data3[2] == 84)        //TKit DATA
                    {
                      if (data3[3] == 79)    //Motor Control Data
                      {
                        //digitalWrite(53, HIGH);
                        //digitalWrite(49, HIGH);
                        //delay(500);
                        //digitalWrite(49, LOW);
                      }
                      else
                      {
                        //digitalWrite(53, LOW);
                        //digitalWrite(49, LOW);
                      }

                      if (data3[3] == 49)    //1 for PLAIN TEST FILE
                      {
                       //Serial.println("PLAIN TEST FILE DATA RECEIVED");
                       while(true)
                       {                       
                        plaintestfiletesting();
                        if (Serial2.available() > 0)
                        {
                         return;  
                        }
                       }
                       return; 
                      }

                      if (data3[3] == 50)    //2 for ALL UP TEST FILE
                      {
                       //Serial.println("ALL UP FILE DATA RECEIVED"); 
                       AllUp();
                       while(true)
                       {
                        testfiletesting();
                        if (Serial2.available() > 0)
                        {
                         return;  
                        }
                       }
                       return; 
                      }

                      if (data3[3] == 51)    //2 for ALL DOWN TEST FILE
                      {
                       //Serial.println("ALL DOWN FILE DATA RECEIVED");  
                       AllDown();
                       while(true)
                       {
                        testfiletesting();

                        if (Serial2.available() > 0)
                        {
                         return;  
                        }
                       }
                       return; 
                      }
                      
                      //TKitmenu();
                      Serial.println("TKit DATA RECEIVED");
                      return;
                    }

                    if (data3[2] == 110)       // Configuration Settings
                    {
                      nc = byte(data3[3]);
                      delay(100);
                      //EEPROM.write(33, BtoTmode);
                      eeprom.write(13, nc);
                      delay(100);
                      Serial.print("No of Cards = ");
                      Serial.println(nc);
                      return;
                    }

                    if (data3[2] == 98)       // Configuration Settings
                    {
                      BtoTmode = data3[3] % 48;
                      delay(100);
                      //EEPROM.write(33, BtoTmode);
                      eeprom.write(33, BtoTmode);
                      delay(100);
                      Serial.print("BtoTmode = ");
                      Serial.println(BtoTmode);
                      return;
                    }

                    if (data3[2] == 105)       // Configuration Settings
                    {
                      LtoRmode = data3[3] % 48;
                      delay(100);
                      //EEPROM.write(12, LtoRmode);
                      eeprom.write(12, LtoRmode);
                      delay(100);
                      Serial.print("LtoRmode = ");
                      Serial.println(LtoRmode);
                      return;
                    }

                    if (data3[2] == 76)       // Configuration Settings
                    {

                      //nr = data1[3] % 48;
                      //nc = data1[4] % 48;
                      //delay(100);
                      //EEPROM.write(34, nr);
                      //delay(100);
                      //EEPROM.write(35, nc);
                      //delay(100);
                      //designsettings();
                      if (data3[3] == 50)
                      {
                        Serial2.print('A');
                        Serial2.print('B');
                        Serial2.print('l');
                        Serial2.print(lockdays);
                        Serial2.print('#');
                        Serial2.print('&');
                      }

                      if ((data3[3] == 49) || (data3[3] == 48))
                      {
                        lockvalue = data3[3];
                        delay(100);
                        eeprom.write(0, lockvalue);
                        delay(100);
                        Serial.print("lockvalue = ");
                        Serial.println(lockvalue);
                        Serial.print("lockdays = ");
                        lockvalue = data3[4];
                        delay(100);
                        eeprom.write(1, lockvalue);
                        delay(100);
                        Serial.print(lockvalue);
                        lockvalue = data3[5];
                        delay(100);
                        eeprom.write(2, lockvalue);
                        delay(100);
                        Serial.print(lockvalue);
                        lockvalue = data3[6];
                        delay(100);
                        eeprom.write(3, lockvalue);
                        delay(100);
                        Serial.println(lockvalue);
                        temp = 48;
                        delay(10);
                        eeprom.write(5, temp);
                        delay(100);
                        lockread();
                        delay(100);
                      }
                      return;
                    }

                    if (data3[2] == 77)
                    {
                      temp = data3[3] - 48;
                      delay(20);
                      //EEPROM.write(10, temp);
                      eeprom.write(10, temp);
                      delay(20);
                      Serial.print("file running mode:");
                      Serial.print('\t');
                      Serial.println(temp);
                      delay(50);
                      runninginit();
                      delay(50);
                      return;
                    }

                    if (data3[2] == 67)        //Communication Checking
                    {
                      Serial2.print('E');
                      Serial2.print('J');
                      Serial.println("Communication Checking");
                      //delay(100);
                      //sendtotalfolder();
                      //delay(100);
                      //sendtotalfile();
                      //delay(100);
                      //sendfolderlist();
                      //delay(100);
                      //sendfilelist();
                      //delay(100);
                      //currentfoldernumber();
                      delay(50);
                      return;
                    }

                    if (data3[2] == 82)        //R - Running Status
                    {
                      if (folderchange == 1)
                      {
                        folderchange = 0;
                        delay(100);
                        sendtotalfile();
                        delay(100);
                        sendfilelist();
                        delay(100);
                        currentfoldernumber();
                        delay(50);
                      }
                      delay(20);
                      filerunningmode1();
                      delay(20);
                      valuedisplay();
                      Serial.println("Running status");
                      return;
                    }

                    if (data3[2] == 114)        //r - reset mode
                    {
                      Serial.println("Reset Mode");
                      delay(200);
                      resetFunc();
                    }

                    if (data3[2] == 70)        //F - Folder Change , first the update folder index store in eeprom then its goes update ,if epprom is miss the index data ,file changs failed.
                    {     Serial.print("RECEIVED PACKET: [");
                          Serial.print(inputString);
                          Serial.println("]");
                       int tempx = 0;
                        String pick = inputString;      // inputString is already "1#{}" — starts right at the digit
                        pick.trim();
                        tempx = pick.toInt();           // stops at the first non-digit '#', so this is safe
                        temp = (byte)tempx;
                        Serial.print("Folder Change:");
                        Serial.print('\t');
                        Serial.println(temp);
                        totalfile = 0;
                        folderchange = 1;
                        s_name = folderlist[temp];
                        foldernum = temp;
                        Serial.print("DEBUG: temp=");
                        Serial.print(temp);
                        Serial.print(" folderlist[temp]=[");
                        Serial.print(s_name);
                        Serial.println("]");
                      s_name.trim();
                      kk1 = s_name.length();
                      delay(10);
                      rtc.writenvram(47, kk1);
                      delay(10);
                      writenvram1(s_name, 48, kk1);
                      delay(10);
                      NVRAMWritelong(12, 0);
                      //NVRAMWritelong(18, 0);
                      NVRAMWrite_Rpt(18, 0);
                      NVRAMWritelong(23, 0);
                      //NVRAMWritelong(28, 0);
                      NVRAMWrite_Rpt(28, 0);
                      delay(100);
                      setupoffolder();
                      delay(100);
                      runninginit();
                      delay(50);
                      return; 
                    }
        //  inpit string is the running accumulation of every byte resived since header A B (build in the big while(true) loop above this )
        // its fing the * that comes right after the 1 ,and the # that terminates the payload , then inbetwenn is raw folder name text ,
                    if (data3[2] == 102)        //F - Copy mode
                    {
                      if (data3[3] == 49)        //1 - Folder Selection
                      {
                        Serial.print("Copy Folder Name: ");   
                        int e = inputString.indexOf('*'); 
                        int  f = inputString.indexOf('#');   
                        String pick = inputString.substring(e + 1, f);
                        copyfoldername = pick;
                        copyfoldername.trim(); //strips any whitesspace/CR-LF. this is store in the globale copyfoldername 
                        Serial.print("COPY FOLDER REQUEST: /");
                        Serial.println(copyfoldername);
                        Serial.println(copyfoldername);                                               
                        if (SD.exists(String("/" + copyfoldername)))
                        {
                          Serial.println("exists.");

                          Serial2.print('D');
                          Serial2.print('1');
                          Serial.println("DEBUG ESP32: SENDING OFFSET ACK D1");  
                        }
                        else
                        {
                          delay(100);
                          SD.mkdir(String("/" + copyfoldername));
                          delay(100);
                          Serial.println("doesn't exist."); 
                          Serial2.print('D');
                          Serial2.print('2');
                        }
                       }

                         if (data3[3] == 50)  // 2 - file open
{
  int e = inputString.indexOf('*');
  int f = inputString.indexOf('#');
  if (e == -1 || f == -1) return;
  String pick = inputString.substring(e + 1, f);
  pick.trim();
  copyfilename = pick;
  copyfoldername.trim();
  String filedata = "/" + copyfoldername + "/" + copyfilename;   // <-- this line was missing

  Serial.print("Copy File Name: ");
  Serial.println(copyfilename);
  Serial.print("COPY FILE PATH: ");
  Serial.println(filedata);

  // Never replace an existing destination file. The normal f4 check already
  // skips it, and this guard also protects against a race or stale request.
  if (SD.exists(filedata)) {
    Serial.println("COPY FILE RESULT: EXISTS - LEFT UNTOUCHED");
    Serial2.print('D');
    Serial2.print('1');
    Serial2.flush();
    return;
  }

  Serial.println("Newly Created");
  bMap4 = SD.open(filedata, FILE_WRITE);
  if (!bMap4) {
    Serial.println("COPY FILE RESULT: OPEN FAILED - D0");
    Serial2.print('D');
    Serial2.print('0');
    Serial2.flush();
  } else {
    Serial.print("Opened fresh, initial size = ");
    Serial.println(bMap4.size());
    Serial.println("COPY FILE RESULT: OPENED - D2");
    Serial2.print('D');
    Serial2.print('2');
    Serial2.flush();
  }
  return;
}
         if (data3[3] == 52)        //4 - Check if file exists (READ-ONLY, no delete/create)
                      {
                        Serial.print("Check File Exists: ");
                        int e = inputString.indexOf('*');
                        int  f = inputString.indexOf('#');
                        String pick = inputString.substring(e + 1, f);
                        String checkfilename = pick;
                        checkfilename.trim();
                        copyfoldername.trim();
                        Serial.println(checkfilename);
                        String filedata = "/" + copyfoldername + "/" + checkfilename;
                        filedata.trim();
                        if (SD.exists(filedata))
                        {
                          Serial.println("exists.");
                          Serial2.print('D');
                          Serial2.print('1');
                        }
                        else
                        {
                          Serial.println("doesn't exist.");
                          Serial2.print('D');
                          Serial2.print('2');
                        }
                      }
                           if (data3[3] == 51)        //3 - Refresh folder list after USB copy
                      {
                        Serial.println("Refreshing folder list after USB copy...");
                        totalfolder = 0;
                        root3 = SD.open("/");
                        readfolderlist(root3);
                        delay(50);
                        sendtotalfolder();
                        delay(50);
                        sendfolderlist();
                        Serial.println("Folder list refresh sent.");
                      }
                        if (data3[3] == 53)        //5 - Check file size on SD (confirms bytes written, no data loss)
                      {
                        Serial.print("Check File Size: ");
                        int e = inputString.indexOf('*');
                        int  f = inputString.indexOf('#');
                        String pick = inputString.substring(e + 1, f);
                        String checkfilename = pick;
                        checkfilename.trim();
                        copyfoldername.trim();
                        Serial.println(checkfilename);
                        String filedata = "/" + copyfoldername + "/" + checkfilename;
                        filedata.trim();
                        if (SD.exists(filedata))
                        {
                          File chkFile = SD.open(filedata, FILE_READ);
                          unsigned long fsize = chkFile.size();
                          chkFile.close();
                          Serial.print("File size on SD: ");
                          Serial.println(fsize);
                          char packet[60];
                          snprintf(packet, sizeof(packet), "ABDz*%lu#&", fsize);
                          sendPacket(packet);
                        }
                        else
                        {
                          Serial.println("File not found for size check.");
                          sendPacket("ABDz*0#&");
                        }
                      }
                      if (data3[3] == 55)        //7 - Abort failed copy and close destination file
                      {
                        Serial.println("COPY ABORT REQUEST: closing incomplete destination file");
                        if (bMap4) {
                          unsigned long fileSize = bMap4.size();
                          if (fileSize > 0) {
                            bMap4.seek(fileSize - 1);
                            int lastByte = bMap4.read();
                            Serial.print("COPY ABORT LAST BYTE: 0x");
                            Serial.println(lastByte, HEX);
                          } else {
                            Serial.println("COPY ABORT LAST BYTE: none");
                          }
                          bMap4.close();
                        }
                        lastcopypick = 0;
                        copypick = 0;
                        copyTotalData = 0;
                        Serial2.print('D');
                        Serial2.print('1');
                        Serial2.flush();
                        Serial.println("COPY ABORT COMPLETE: file closed; ready for next file");
                      }

                      return;
                    }

                    if (data3[2] == 99)        //c - Copy pick no
                    {
                      if (data3[3] == 116)        //t - pick no
                      {
                        int e = inputString.indexOf('*');
                          int f2 = inputString.lastIndexOf('*');
                        int  f = inputString.indexOf('#');

                        String pick = inputString.substring(e + 1, (f2 > e) ? f2 : f);
                        // String pick = inputString.substring(e + 1, f);
                        copypick = pick.toInt();
                         if (f2 > e) {
                          String td = inputString.substring(f2 + 1, f);
                          copyTotalData = td.toInt();
                          Serial.print("copyTotalData set: ");
                          Serial.println(copyTotalData);
                        }
                                            
                        Serial.print("Copypickno : ");
                        Serial.println(copypick);
                        return;
                      }
                    }

                    if (data3[2] == 69)        //E - Edit in running mode
                    {
                      if (data3[3] == 75)        //1 - F1: file change
                      {
                        int tempy = 0;
                        int e = inputString.indexOf('K');
                        int  f = inputString.indexOf('{');
                        String pick = inputString.substring(e + 1, f);
                        tempy = pick.toInt();
                        //temp = data3[4] - 48;
                        Serial.print("F1:File Change:");
                        Serial.print('\t');
                        Serial.println(tempy);
                        bMap.close();
                        s_name = filelist[tempy];
                        s_name.trim();
                        kk1 = s_name.length();
                        delay(10);
                        rtc.writenvram(45, (byte)kk1);
                        delay(10);
                        writenvram1(s_name, 0, kk1);
                        temp = (byte)tempy;
                        //EEPROM.write(11, temp);
                        eeprom.write(11, temp);
                        NVRAMWritelong(12, 0);
                        //NVRAMWritelong(18, 0);
                        NVRAMWrite_Rpt(18, 0);
                        delay(50);
                        runninginit();
                        delay(50);
                        return;
                      }

                      if (data3[3] == 50)        //2 - F1: Pickno change
                      {
                        lastpickno = pickno;
                        int e = inputString.indexOf('*');
                        int  f = inputString.indexOf('#');
                        String pick = inputString.substring(e + 1, f);
                        pickno = pick.toInt();
                        Serial.print("F1: Pickno Change:");
                        Serial.print('\t');
                        Serial.println(pickno);
                        if (filerunningmode == 2)
                        {
                          if (pickno > lastpickno)
                          {
                            jump(pickno, lastpickno);
                            delay(10);
                            pickno = NVRAMReadlong(12);
                            delay(10);
                            pickno1 = NVRAMReadlong(23);
                            delay(10);
                            return;
                          }

                          if (pickno  < lastpickno)
                          {
                            jump1(pickno, lastpickno);
                            delay(10);
                            pickno = NVRAMReadlong(12);
                            delay(10);
                            pickno1 = NVRAMReadlong(23);
                            delay(10);
                            return;
                          }
                        }
                        else
                        {
                          NVRAMWritelong(12, pickno);
                        }

                        Serial.println("pickno RECEIVED");
                        return;
                      }

                      if (data3[3] == 51)        //3 - F1: repeatcnt change
                      {
                        int e = inputString.indexOf('*');
                        int  f = inputString.indexOf('#');
                        String pick = inputString.substring(e + 1, f);
                        repeatcnt = pick.toInt();
                        Serial.print("F1: repeatcnt Change:");
                        Serial.print('\t');
                        Serial.println(repeatcnt);
                        delay(10);
                        //NVRAMWritelong(18, repeatcnt);
                        NVRAMWrite_Rpt(18, repeatcnt);
                        delay(10);
                        return;
                      }

                      if (data3[3] == 76)        //5 - F2: file change
                      {
                        int tempz = 0;
                        int e = inputString.indexOf('L');
                        int  f = inputString.indexOf('{');
                        String pick = inputString.substring(e + 1, f);
                        tempz = pick.toInt();

                        //temp = data3[4] - 48;
                        Serial.print("F2:File Change:");
                        Serial.print('\t');
                        Serial.println(tempz);
                        bMap1.close();
                        s_name = filelist[tempz];
                        s_name.trim();
                        kk1 = s_name.length();
                        delay(10);
                        rtc.writenvram(46, (byte)kk1);
                        delay(10);
                        writenvram1(s_name, 33, kk1);
                        delay(10);
                        temp = (byte)tempz;
                        //EEPROM.write(32, temp);
                        eeprom.write(32, temp);
                        delay(10);
                        NVRAMWritelong(23, 0);
                        delay(10);
                        //NVRAMWritelong(28, 0);
                        NVRAMWrite_Rpt(28, 0);
                        delay(10);
                        runninginit();
                        delay(10);
                        return;
                      }

                      if (data3[3] == 54)        //6 - F2: pickno1 change
                      {
                        int e = inputString.indexOf('*');
                        int  f = inputString.indexOf('#');
                        String pick = inputString.substring(e + 1, f);
                        pickno1 = pick.toInt();
                        Serial.print("F2: Pickno1 Change:");
                        Serial.print('\t');
                        Serial.println(pickno1);
                        delay(10);
                        NVRAMWritelong(23, pickno1);
                        delay(10);
                        return;
                      }

                      if (data3[3] == 55)        //7 - F2: repeatcnt1 change
                      {
                        int e = inputString.indexOf('*');
                        int  f = inputString.indexOf('#');
                        String pick = inputString.substring(e + 1, f);
                        repeatcnt1 = pick.toInt();
                        Serial.print("F2: repeatcnt1 Change:");
                        Serial.print('\t');
                        Serial.println(repeatcnt1);
                        delay(10);
                        //NVRAMWritelong(28, repeatcnt1);
                        NVRAMWrite_Rpt(28, repeatcnt1);
                        delay(10);
                        return;
                      }

                      if (data3[3] == 56)        //8 - One Pulse Down
                      {
                        down();
                        delay(10);
                        outshifter();
                        delay(10);
                        valuedisplay();
                        delay(10);
                        return;
                      }

                      if (data3[3] == 57)        //9 - One Pulse Count
                      {
                        onepulse2();
                        delay(10);
                        outshifter();
                        delay(10);
                        valuedisplay();
                        delay(10);
                        return;
                      }

                    }

                    if (data3[2] == 73)        //I - Master inite 
                    {
                      masterint();
                      delay(10);
                      valuedisplay();
                      delay(10);
                      d = 1;
                      return;
                    }

                    if (data3[2] == 100)        //d - Delete Option
                    { /*
                      if (data3[3] == 49)        //1 - Folder Delete Option
                      {
                        Serial.println("Delete folder menu.");
                        kk1 = 0;
                        kk1 = data3[4] % 48;
                        s_name = folderlist[kk1];
                        s_name.trim();
                        delay(200);
                        if (SD.exists(s_name))
                        {
                          SD.rmdir(s_name);
                          Serial.println(s_name);
                          Serial.println("exists.");
                          Serial2.print('D');
                          Serial2.print('1');
                        }
                        else
                        {
                          Serial.println(s_name);
                          Serial.println("doesn't exist.");
                          Serial2.print('D');
                          Serial2.print('2');
                        }
                      } */if (data3[3] == 49)  // 1 - folder create
{
  int e = inputString.indexOf('*');
  int f = inputString.indexOf('#');
  if (e == -1 || f == -1) return;
  String pick = inputString.substring(e + 1, f);
  pick.trim();
  copyfoldername = pick;

  bool folderExisted = SD.exists("/" + copyfoldername);

  Serial.print("Copy Folder Name: ");
  Serial.println(copyfoldername);
  Serial.println(folderExisted ? "exists." : "doesn't exist.");
  Serial.print("SENDING FOLDER-CREATE ACK: D");
  Serial.println(folderExisted ? '1' : '2');

  // SEND ACK FIRST (before slow SD operations)
  Serial2.print('D');
  Serial2.print(folderExisted ? '1' : '2');
  Serial2.flush();

  // THEN do mkdir if needed
  if (!folderExisted) {
    SD.mkdir("/" + copyfoldername);
  }
  return;
}

                      if (data3[3] == 50)        //2 -  File Delete Option
                      {
                        Serial.println("Delete file menu.");
                        kk1 = 0;
                        kk1 = data3[4] % 48;
                        s_name = filelist[kk1];
                        s_name.trim();
                        delay(200);
                        if (SD.exists(foldername1 + "/" + s_name))
                        {
                          SD.remove(foldername1 + "/" + s_name);
                          Serial.println(s_name);
                          Serial.println("exists.");
                          Serial2.print('D');
                          Serial2.print('1');
                        }
                        else
                        {
                          Serial.println(s_name);
                          Serial.println("doesn't exist.");
                          Serial2.print('D');
                          Serial2.print('2');
                        }
                      }
                      return;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
ooout:
  Serial.println("C");
}

void jump1(unsigned long j11, unsigned long j112)
{
  pickno = j112;
  while (true)
  {
    pickno = pickno - 1;
    if (pickno < j11)
    {
      NVRAMWritelong(23, pickno1);
      return;
    }
    NVRAMWritelong(12, pickno);
    if (pickno == height)
    {
      bMap.seek(bitmapOffset + check_bit);
    }
    else
    {
      bMap.seek((pickno * totaldata) + bitmapOffset + check_bit);
    }
    data1[0] = (byte)bMap.read();
    if (bitRead(data1[check_bit], 7) == 0)
    {

    }
    else
    {
      pickno1 = pickno1 - 1;
      if (pickno1 < 1)
      {
        pickno1 = height1;
      }
    }
  }
}

void jump(unsigned long j1, unsigned long j12)
{
  pickno = j12;
j12:
  pickno = pickno + 1;
  if (pickno > j1)
  {
    NVRAMWritelong(23, pickno1);
    return;
  }
  NVRAMWritelong(12, pickno);
  bMap.seek(((pickno - 1) * totaldata) + bitmapOffset);
  for (int m = 0; m < totaldata; m++)
  {
    data1[m] = (byte)bMap.read();
  }

  if (bitRead(data1[check_bit], 7) == 0)
  {

  }
  else
  {
    pickno1 = pickno1 + 1;
    if (pickno1 > height1)
    {
      pickno1 = 1;
    }
  }
  goto j12;
}



void AllUp()
{
  int kl1 = nc * 8;
  for (int m = 0; m < nr; m++)
  {
    for (int n = 0; n < kl1; n++)
    {      
     outdata1[m].outdata[n] = 0;     
    }
  }    
}

void AllDown()
{
  int kl1 = nc * 8;
  for (int m = 0; m < nr; m++)
  {
    for (int n = 0; n < kl1; n++)
    {      
     outdata1[m].outdata[n] = 1;     
    }
  }    
}

void runninginit()
{  
  temp = eeprom.read(10); //EEPROM.read(10);
  temp = temp % 48;
  filerunningmode = temp;
  temp = eeprom.read(11); //EEPROM.read(11);
  filenum = temp % 48;
  delay(10);
  if (filenum > totalfile)
    filenum = 0;   
  bMap = SD.open(String("/" +foldername1 + "/" + filelist[filenum]), FILE_READ); 
  delay(10);
  height = FindHeight(1);
  delay(10);
  pickno = NVRAMReadlong(12);
  if ((pickno > height) | (pickno < 0))
    pickno = 0;
  //repeatcnt = NVRAMReadlong(18);
  repeatcnt = NVRAMRead_Rpt(18);
  if ((repeatcnt > 999) | (repeatcnt < 0))
    repeatcnt = 0;

  if (filerunningmode == 2)
  {
    temp = eeprom.read(32);  //EEPROM.read(32);
    filenum1 = temp % 48;
    delay(10);
    if (filenum1 > totalfile)
      filenum1 = 0;
    bMap1 = SD.open(String("/" +foldername1 + "/" + filelist[filenum1]), FILE_READ);    
    delay(10);
    height1 = FindHeight(2);
    delay(10);
    pickno1 = NVRAMReadlong(23);
    if ((pickno1 > height1) | (pickno1 < 0))
      pickno1 = 0;
    //repeatcnt1 = NVRAMReadlong(28);
    repeatcnt1 = NVRAMRead_Rpt(28);
    if ((repeatcnt1 > 999) | (repeatcnt1 < 0))
      repeatcnt1 = 0;
  }
}
/*
void readfolderlist(File root2)
{
  s_name = "";
  while (true)
  {
    bMap3 = root2.openNextFile();
    if (!bMap3)
    {
      if (totalfolder != 0) 
        totalfolder = totalfolder ;  // -1 
      root2.rewindDirectory();
      bMap3.close();
      root2.close();
      Serial.print("totalfolder");
      Serial.print('\t');
      Serial.println(totalfolder );  //+1
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
        Serial.print("folders name");
        Serial.print('\t');
        Serial.println(s_name);
      }
    }
    delay(20);
  }  
}
*/ 
void readfolderlist(File root2)
{
  s_name = "";
  while (true)
  {
    bMap3 = root2.openNextFile();
    if (!bMap3)
    {
      if (totalfolder != 0) 
        totalfolder = totalfolder;  // -1 
      root2.rewindDirectory();
      bMap3.close();
      root2.close();

      // ==================== ALPHABETICAL SORTING BLOCK ====================
      // Sorts the Arduino String array alphabetically to match the USB board
      if (totalfolder > 1) {
        for (int i = 0; i < totalfolder - 1; i++) {
          for (int j = 0; j < totalfolder - i - 1; j++) {
            // Compare strings. if folder j should come after folder j+1, swap them
            if (folderlist[j].compareTo(folderlist[j + 1]) > 0) {
              String tempStr = folderlist[j];
              folderlist[j] = folderlist[j + 1];
              folderlist[j + 1] = tempStr;
            }
          }
        }
        Serial.println("DEBUG: folderlist sorted alphabetically successfully.");
      }
      // ====================================================================

      Serial.print("totalfolder");
      Serial.print('\t');
      Serial.println(totalfolder);  //+1
      return;
    }
    else
    {
      s_name = bMap3.name();
      s_name.trim();
      if (((s_name.indexOf('.') >= 0) || (s_name.startsWith("SYSTEM"))) || (s_name.startsWith("System")))
      {
         // Skip files and system folders
      }
      else
      {
        folderlist[totalfolder] = s_name;
        folderlist[totalfolder].trim();
        totalfolder += 1;
        Serial.print("folders name");
        Serial.print('\t');
        Serial.println(s_name);
      }
    }
    delay(20);
  }  
}

void setupoffolder()
{
  if (totalfolder < 1)
  {
    pick = "No Folders";
    Serial.println(pick);
    intreplay(pick);
    pick = "In SDCard";
    Serial.println(pick);
    intreplay(pick);
    delay(2000);
    goto ii2;
  }
  delay(10);
  kk1 = rtc.readnvram(47);
  delay(10);
  kk1 = kk1 % 48;
  Serial.print("DEBUG: NVRAM length readback kk1=");
Serial.println(kk1);
  if (kk1 > 15)
  {
    s_name = folderlist[0];
    foldernum = 0;
    s_name.trim();
    kk1 = s_name.length();
    delay(10);
    rtc.writenvram(47, kk1);
    delay(10);
    writenvram1(s_name, 48, kk1);
    foldername1 = s_name;
    foldername1.trim();
  }
  else
  {
    s_name = readnvram1(48, kk1);
    Serial.print("DEBUG: NVRAM name readback=[");
    Serial.print(s_name);
    Serial.println("]");
    for (int m = 0; m <= totalfolder; m++)
    {
      if (folderlist[m].equalsIgnoreCase(s_name))
      {
        foldername1 = s_name;
        foldername1.trim();
        foldernum = m;
        goto ii12;
      }
    }
  }
  s_name = folderlist[0];
  foldernum = 0;
  s_name.trim();
  kk1 = s_name.length();
  delay(10);
  rtc.writenvram(47, kk1);
  delay(10);
  writenvram1(s_name, 48, kk1);
  foldername1 = s_name;
  foldername1.trim();  
ii12:  
  delay(10);
  Serial.print("folders name:");  
  Serial.println(foldername1);
  delay(10);
  root = SD.open("/");   
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
  delay(10);
  if (totalfile < 1)
  {
    pick = "File Error!";
    Serial.println(pick);
    intreplay(pick);
    pick = "Keep Minimum 2 Files";
    Serial.println(pick);
    intreplay(pick);
    delay(2000);
    goto ii2;
  }
  delay(10);
  kk1 = rtc.readnvram(45);
  delay(10);
  kk1 = kk1 % 48;
  if (kk1 > 12)
  {
    s_name = filelist[0];
    s_name.trim();
    kk1 = s_name.length();
    delay(10);
    rtc.writenvram(45, kk1);
    delay(10);
    writenvram1(s_name, 0, kk1);
    //EEPROM.write(11, (byte)0);
    eeprom.write(11, (byte)0);
  }
  else
  {
    s_name = readnvram1(0, kk1);
    for (int m = 0; m <= totalfile; m++)
    {
      if (filelist[m].equalsIgnoreCase(s_name))
      {
        temp = m;
        //EEPROM.write(11, temp);
        eeprom.write(11, temp);
        goto ii1;
      }
    }
  }
  s_name = filelist[0];
  s_name.trim();
  kk1 = s_name.length();
  delay(10);
  rtc.writenvram(45, kk1);
  delay(10);
  writenvram1(s_name, 0, kk1);
  //EEPROM.write(11, 0);
  eeprom.write(11, 0);
ii1:
  delay(10);
  kk1 = rtc.readnvram(46);
  delay(10);
  kk1 = kk1 % 48;
  if (kk1 > 12)
  {
    s_name = filelist[0];
    s_name.trim();
    kk1 = s_name.length();
    delay(10);
    rtc.writenvram(46, kk1);
    delay(10);
    writenvram1(s_name, 33, kk1);
    //EEPROM.write(32, 0);
    eeprom.write(32, 0);
  }
  else
  {
    s_name = readnvram1(33, kk1);
    s_name.trim();
    for (int m = 0; m <= totalfile; m++)
    {
      if (filelist[m].equalsIgnoreCase(s_name))
      {
        temp = m;
        //EEPROM.write(32, temp);
        eeprom.write(32, temp);
        goto ii2;
      }
    }
  }
  s_name = filelist[0];
  s_name.trim();
  kk1 = s_name.length();
  delay(10);
  rtc.writenvram(46, kk1);
  delay(10);
  writenvram1(s_name, 33, kk1);
  //EEPROM.write(32, 0);
  eeprom.write(32, 0);
ii2:
  temp = temp;
  Serial.print("Selected folders name");
  Serial.print('\t');
  Serial.println(foldername1);
}



void writenvram1(String inputString, byte add, int cbytes)
{
  inputString.toCharArray(file1, 15);
  for (int m = 0; m < cbytes; m++)
  {
    delay(10);
    rtc.writenvram(add, (byte)file1[m]);
    delay(10);
    add ++;
  }
}


String readnvram1(byte add1, int cbytes1)
{
  byte vvalue = 0;
  s_name = " ";
  for (int m = 0; m < cbytes1; m++)
  {
    delay(10);
    vvalue = rtc.readnvram(add1);
    delay(10);
    s_name += (char)(vvalue);
    add1 ++;
  }
  s_name.trim();
  return s_name;
}

void opennext(File root1)
{
  s_name = "";
  while (true)
  {
    bMap = root1.openNextFile();
    if (!bMap)
    {
     // if (totalfile != 0)
     //   totalfile = totalfile - 1;
      root1.rewindDirectory();
      bMap.close();
      return;
    }
    else
    {
      s_name = bMap.name();
      s_name.trim();
      if (s_name.endsWith(".BMP") || s_name.endsWith(".bmp"))
      {
        filelist[totalfile] = s_name;
        filelist[totalfile].trim();
        Serial.print(totalfile);
        Serial.print('\t');
        Serial.println(filelist[totalfile]);
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
  if (ht > 99999)
  ht = 99999;
  return ht;
}

long NVRAMReadlong(long address)
{
  long five = rtc.readnvram(address);
  long four = rtc.readnvram(address + 1);
  long three = rtc.readnvram(address + 2);
  long two = rtc.readnvram(address + 3);
  long one = rtc.readnvram(address + 4);
  return (((five << 0) & 0xFF) + ((four << 8) & 0xFFFF) + ((three << 16) & 0xFFFFFF) + ((two << 24) & 0xFFFFFFFF) + ((one << 32) & 0xFFFFFFFFFF));
}

long NVRAMRead_Rpt(long address)
{
  long five = rtc.readnvram(address);
  long four = rtc.readnvram(address + 1);
  long three = rtc.readnvram(address + 2);   
  return((five*100)+(four*10)+three);
}

void onepulse2()
{
  Serial.println("totaldata");
  Serial.println(totaldata);
  pickno = pickno + 1;
  if (pickno > height)
  {
    pickno = 1;
    repeatcnt = repeatcnt + 1;
    if (repeatcnt > 999)
      repeatcnt = 0;
    //NVRAMWritelong(18, repeatcnt);
    NVRAMWrite_Rpt(18, repeatcnt);
  }
  NVRAMWritelong(12, pickno);
  bMap.seek(((pickno - 1) * totaldata) + bitmapOffset);
  for (int m = 0; m < totaldata; m++)
  {
    delayMicroseconds(2);
    data1[m] = (byte)bMap.read();
  }

  if (filerunningmode == 1)
  {
    file1output();
  }
  else
  {
    if (bitRead(data1[check_bit], 7) == 0)
    {
      file1output();
    }
    else
    {
      pickno1 = pickno1 + 1;
      if (pickno1 > height1)
      {
        pickno1 = 1;
        repeatcnt1 = repeatcnt1 + 1;
        if (repeatcnt1 > 999)
          repeatcnt1 = 0;
        //NVRAMWritelong(28, repeatcnt1);
        NVRAMWrite_Rpt(28, repeatcnt1);
      }
      NVRAMWritelong(23, pickno1);
      bMap1.seek(((pickno1 - 1) * totaldata) + bitmapOffset);
      for (int m = 0; m < totaldata; m++)
      {
        delayMicroseconds(2);
        data2[m] = (byte)bMap1.read();
      }
      delay(1);
      file2output();
    }
  }
}

void down()
{
  if (filerunningmode == 1)
  {
    if (pickno > 0)
      pickno = pickno - 1;
    if (pickno < 1)
    {
      pickno = height;
      if (repeatcnt > 0)
        repeatcnt = repeatcnt - 1;
      if (repeatcnt < 1)
      {
        repeatcnt = 0;
      }
      //NVRAMWritelong(18, repeatcnt);
      NVRAMWrite_Rpt(18, repeatcnt);
    }
    NVRAMWritelong(12, pickno);
    bMap.seek(((pickno - 1) * totaldata) + bitmapOffset);
    for (int m = 0; m < totaldata; m++)
    {
      data1[m] = (byte)bMap.read();
    }
    file1output();
  }
  else
  {
    if (pickno > 0)
      pickno = pickno - 1;
    if (pickno < 1)
    {
      pickno = height;
      if (repeatcnt > 0)
        repeatcnt = repeatcnt - 1;
      if (repeatcnt < 1)
      {
        repeatcnt = 0;
      }
      //NVRAMWritelong(18, repeatcnt);
      NVRAMWrite_Rpt(18, repeatcnt);
    }
    NVRAMWritelong(12, pickno);
    if (pickno == height)
    {
      bMap.seek(bitmapOffset + check_bit);
    }
    else
    {
      bMap.seek((pickno * totaldata) + bitmapOffset + check_bit);
    }
    data1[check_bit] = (byte)bMap.read();
    if (bitRead(data1[check_bit], 7) == 0)
    {
      bMap.seek(((pickno - 1) * totaldata) + bitmapOffset);
      for (int m = 0; m < totaldata; m++)
      {
        data1[m] = (byte)bMap.read();
      }
      file1output();
    }
    else
    {
      if (pickno1 > 0)
        pickno1 = pickno1 - 1;
      if (pickno1 < 1)
      {
        pickno1 = height1;
        if (repeatcnt1 > 0)
          repeatcnt1 = repeatcnt1 - 1;
        if (repeatcnt1 < 1)
        {
          repeatcnt1 = 0;
        }
        //NVRAMWritelong(28, repeatcnt1);
        NVRAMWrite_Rpt(28, repeatcnt1);
      }
      NVRAMWritelong(23, pickno1);

      bMap.seek(((pickno - 1) * totaldata) + bitmapOffset);
      for (int m = 0; m < totaldata; m++)
      {
        data1[m] = (byte)bMap.read();
      }
      bMap1.seek(((pickno1 - 1) * totaldata) + bitmapOffset);
      for (int m = 0; m < totaldata; m++)
      {
        data2[m] = (byte)bMap1.read();
      }
      file2output();
    }
  }
}


void NVRAMWritelong(int address, long value)
{
  byte five = (value & 0xFF);
  byte four = ((value >> 8) & 0xFF);
  byte three = ((value >> 16) & 0xFF);
  byte two = ((value >> 24) & 0xFF);
  byte one = ((value >> 32) & 0xFF);
  rtc.writenvram(address, five);
  rtc.writenvram(address + 1, four);
  rtc.writenvram(address + 2, three);
  rtc.writenvram(address + 3, two);
  rtc.writenvram(address + 4, one);
}

void NVRAMWrite_Rpt(int address, long value)
{  
  byte three = ((value/100));
  byte two = ((value/10) % 10 );
  byte one = ((value % 10));
  rtc.writenvram(address, three);
  rtc.writenvram(address + 1, two);
  rtc.writenvram(address + 2, one);  
}

void file1output()
{
  nr1 = 0;
  nc1 = 0;
  temp = 0;  
  
  for (int m = 0; m < nr; m++)  
  {     
    br = m * nc;
    byte bb = 0;
    for (int n = 0; n < nc; n++)
    {
     bb = n*8; 
     tempout[bb] = bitRead(data1[(br + n) + 1], 7);     
     //Serial.print(tempout[bb]);
     tempout[bb + 1] = bitRead(data1[(br + n) + 1], 6);
     //Serial.print(tempout[bb + 1]);
     tempout[bb + 2] = bitRead(data1[(br + n) + 1], 5);
     //Serial.print(tempout[bb + 2]);
     tempout[bb + 3] = bitRead(data1[(br + n) + 1], 4);
     //Serial.print(tempout[bb + 3]);
     tempout[bb + 4] = bitRead(data1[(br + n) + 1], 3);
     //Serial.print(tempout[bb + 4]);
     tempout[bb + 5] = bitRead(data1[(br + n) + 1], 2);
     //Serial.print(tempout[bb + 5]);
     tempout[bb + 6] = bitRead(data1[(br + n) + 1], 1);
     //Serial.print(tempout[bb + 6]);
     tempout[bb + 7] = bitRead(data1[(br + n) + 1], 0);
     //Serial.println(tempout[bb + 7]);
    }
   
    if (BtoTmode == 1)
    {
     if((LtoRmode == 2) || (LtoRmode == 4))
     { 
      for (int n = 0; n < nc; n++)
      {
       bb = n*8;
       outdata1[m].outdata[bb] = (tempout[n + (nc*7)] == 1) ? 0 : 1;    //~(tempout[n + (nc*7)]);   
       outdata1[m].outdata[bb+1] = (tempout[n + (nc*6)] == 1) ? 0 : 1;  //~(tempout[n + (nc*6)]);
       outdata1[m].outdata[bb+2] = (tempout[n + (nc*5)] == 1) ? 0 : 1;  //~(tempout[n + (nc*5)]);
       outdata1[m].outdata[bb+3] = (tempout[n + (nc*4)] == 1) ? 0 : 1;  //~(tempout[n + (nc*4)]);
       outdata1[m].outdata[bb+4] = (tempout[n + (nc*3)] == 1) ? 0 : 1;  //~(tempout[n + (nc*3)]);
       outdata1[m].outdata[bb+5] = (tempout[n + (nc*2)] == 1) ? 0 : 1;  //~(tempout[n + (nc*2)]);
       outdata1[m].outdata[bb+6] = (tempout[n + (nc*1)] == 1) ? 0 : 1;  //~(tempout[n + (nc*1)]);
       outdata1[m].outdata[bb+7] = (tempout[n] == 1) ? 0 : 1;           //~(tempout[n]);
      }
     } 

     if((LtoRmode == 1) || (LtoRmode == 3))
     { 
      for (int n = 0; n < nc; n++)
      {
       bb = n*8;
       outdata1[m].outdata[bb] = (tempout[n] == 1) ? 0 : 1;           //~(tempout[n]);   
       outdata1[m].outdata[bb+1] = (tempout[n + (nc*1)] == 1) ? 0 : 1;  //~(tempout[n + (nc*1)]);
       outdata1[m].outdata[bb+2] = (tempout[n + (nc*2)] == 1) ? 0 : 1;  //~(tempout[n + (nc*2)]);
       outdata1[m].outdata[bb+3] = (tempout[n + (nc*3)] == 1) ? 0 : 1;  //~(tempout[n + (nc*3)]);
       outdata1[m].outdata[bb+4] = (tempout[n + (nc*4)] == 1) ? 0 : 1;  //~(tempout[n + (nc*4)]);
       outdata1[m].outdata[bb+5] = (tempout[n + (nc*5)] == 1) ? 0 : 1;  //~(tempout[n + (nc*5)]);
       outdata1[m].outdata[bb+6] = (tempout[n + (nc*6)] == 1) ? 0 : 1;  //~(tempout[n + (nc*6)]);
       outdata1[m].outdata[bb+7] = (tempout[n + (nc*7)] == 1) ? 0 : 1;    //~(tempout[n + (nc*7)]);
      }
     }          
    }
    else
    {
      
     if((LtoRmode == 2) || (LtoRmode == 4))
     { 
      for (int n = 0; n < nc; n++)
      {
       bb = n*8;
       outdata1[m].outdata[bb] = tempout[n + (nc*7)];   
       outdata1[m].outdata[bb+1] = tempout[n + (nc*6)];
       outdata1[m].outdata[bb+2] = tempout[n + (nc*5)];
       outdata1[m].outdata[bb+3] = tempout[n + (nc*4)];
       outdata1[m].outdata[bb+4] = tempout[n + (nc*3)];
       outdata1[m].outdata[bb+5] = tempout[n + (nc*2)];
       outdata1[m].outdata[bb+6] = tempout[n + (nc*1)];
       outdata1[m].outdata[bb+7] = tempout[n];
      }
     }

     if((LtoRmode == 1) || (LtoRmode == 3))
     { 
      for (int n = 0; n < nc; n++)
      {
       bb = n*8;
       outdata1[m].outdata[bb] = tempout[n];    
       outdata1[m].outdata[bb+1] = tempout[n + (nc*1)];
       outdata1[m].outdata[bb+2] = tempout[n + (nc*2)];
       outdata1[m].outdata[bb+3] = tempout[n + (nc*3)];
       outdata1[m].outdata[bb+4] = tempout[n + (nc*4)];
       outdata1[m].outdata[bb+5] = tempout[n + (nc*5)];
       outdata1[m].outdata[bb+6] = tempout[n + (nc*6)];
       outdata1[m].outdata[bb+7] = tempout[n + (nc*7)];
      }
     }
      
    }           
  }
}

void file2output()
{
  nr1 = 0;
  nc1 = 0;
  temp = 0;
  for (int m = 0; m < totaldata; m++)
  {
    byte f2 = (~data1[m]) | (~data2[m]);
    //delayMicroseconds(1);
    //if (BtoTmode == 1)
    //{
      //data4[m] = f2;
    //}
    //else
    //{
      delayMicroseconds(2);
      data4[m] = (~f2);
    //}        
    f2 = 0;
  }
  
  for (int m = 0; m < nr; m++)  
  {     
    br = m * nc;
    byte bb = 0;
    for (int n = 0; n < nc; n++)
    {
     bb = n*8; 
     tempout[bb] = bitRead(data4[(br + n) + 1], 7);     
     //Serial.print(tempout[bb]);
     tempout[bb + 1] = bitRead(data4[(br + n) + 1], 6);
     //Serial.print(tempout[bb + 1]);
     tempout[bb + 2] = bitRead(data4[(br + n) + 1], 5);
     //Serial.print(tempout[bb + 2]);
     tempout[bb + 3] = bitRead(data4[(br + n) + 1], 4);
     //Serial.print(tempout[bb + 3]);
     tempout[bb + 4] = bitRead(data4[(br + n) + 1], 3);
     //Serial.print(tempout[bb + 4]);
     tempout[bb + 5] = bitRead(data4[(br + n) + 1], 2);
     //Serial.print(tempout[bb + 5]);
     tempout[bb + 6] = bitRead(data4[(br + n) + 1], 1);
     //Serial.print(tempout[bb + 6]);
     tempout[bb + 7] = bitRead(data4[(br + n) + 1], 0);
     //Serial.println(tempout[bb + 7]);
    } 

     if((LtoRmode == 2) || (LtoRmode == 4))
     { 
      for (int n = 0; n < nc; n++)
      {
       bb = n*8;
       outdata1[m].outdata[bb] = tempout[n + (nc*7)];   
       outdata1[m].outdata[bb+1] = tempout[n + (nc*6)];
       outdata1[m].outdata[bb+2] = tempout[n + (nc*5)];
       outdata1[m].outdata[bb+3] = tempout[n + (nc*4)];
       outdata1[m].outdata[bb+4] = tempout[n + (nc*3)];
       outdata1[m].outdata[bb+5] = tempout[n + (nc*2)];
       outdata1[m].outdata[bb+6] = tempout[n + (nc*1)];
       outdata1[m].outdata[bb+7] = tempout[n];
      }
     }


     if((LtoRmode == 1) || (LtoRmode == 3))
     { 
      for (int n = 0; n < nc; n++)
      {
       bb = n*8;
       outdata1[m].outdata[bb] = tempout[n];    
       outdata1[m].outdata[bb+1] = tempout[n + (nc*1)];
       outdata1[m].outdata[bb+2] = tempout[n + (nc*2)];
       outdata1[m].outdata[bb+3] = tempout[n + (nc*3)];
       outdata1[m].outdata[bb+4] = tempout[n + (nc*4)];
       outdata1[m].outdata[bb+5] = tempout[n + (nc*5)];
       outdata1[m].outdata[bb+6] = tempout[n + (nc*6)];
       outdata1[m].outdata[bb+7] = tempout[n + (nc*7)];
      }
     }       
  }
}

void testfiletesting()
{ 
  sensor1status = digitalRead(sensor1);
  if (sensor1status == LOW)
  {
   if (lastsensorstate == 1)
    {       
      lastsensorstate = 0;
      //Serial.println("TEST FILE SENSOR_2");
      outshifter();        
      digitalWrite(CL_OE, LOW);
    }    
  }

  if (sensor1status == HIGH)
  {
   if (lastsensorstate == 0)
   {      
    lastsensorstate = 1;  
        
   }
  } 


  sensor2status = digitalRead(sensor2);

  if (sensor2status == LOW)
  {    
   if (lastsensorstate1 == 1)
   {
    lastsensorstate1 = 0;
    //Serial.println("TEST FILE SENSOR_1");    
    digitalWrite(CL_OE, HIGH);     
   }
  }

  if (sensor2status == HIGH)
  {
   if (lastsensorstate1 == 0)
   {      
    lastsensorstate1 = 1;      
   }
  }   
}

void plaintestfiletesting()
{ 
  sensor1status = digitalRead(sensor1);
  if (sensor1status == LOW)
  {
   if (lastsensorstate == 1)
    {       
      lastsensorstate = 0;
      int kl = nc * 8;
      byte kg = 0, kg1 = 0;
      for (int m = 0; m < nr; m++)
      {
        for (int n = 0; n < kl; n++)
        {      
         if (kg1 == 0)
         {
          outdata1[m].outdata[n] = 1; 
         }
         else
         {
          outdata1[m].outdata[n] = 0; 
         }
         kg = kg + 1; 
         if(kg >= 8)
         {
          kg = 0;
          if (kg1 == 0)
          {
           kg1 = 1;    
          }
          else
          {
           kg1 = 0;
          }
         }
        }
      }
      //Serial.println("PLAIN TEST FILE SENSOR_1");
      outshifter();        
      digitalWrite(CL_OE, LOW);
    }    
  }

  if (sensor1status == HIGH)
  {
   if (lastsensorstate == 0)
   {      
    lastsensorstate = 1;  
    
   }
  } 


  sensor2status = digitalRead(sensor2);

  if (sensor2status == LOW)
  {    
   if (lastsensorstate1 == 1)
   {
    lastsensorstate1 = 0;
    int kl = nc * 8;
    byte kg = 0, kg1 = 0;
    for (int m = 0; m < nr; m++)
    {
      for (int n = 0; n < kl; n++)
      {      
       if (kg1 == 0)
       {
        outdata1[m].outdata[n]= 0; 
       }
       else
       {
        outdata1[m].outdata[n]= 1; 
       }
       kg = kg + 1; 
       if(kg >= 8)
       {
        kg = 0;
        if (kg1 == 0)
        {
         kg1 = 1;    
        }
        else
        {
         kg1 = 0;
        }
       }
      }
    }
    //Serial.println("PLAIN TEST FILE SENSOR_2");    
    digitalWrite(CL_OE, HIGH);  
   }
  }

  if (sensor2status == HIGH)
  {
   if (lastsensorstate1 == 0)
   {      
    lastsensorstate1 = 1;     
   }
  }   
}

void writeFile(fs::FS &fs, const char * path)
{
    Serial.printf("Writing file: %s\n", path);
    bMap4 = fs.open(path, FILE_WRITE);
    if(!bMap4)
    {
     Serial.println("Failed to open file for writing");
     return;
    }    
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