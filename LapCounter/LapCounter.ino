// for LCD screen
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// for talkie
#include <Arduino.h>
#include "Talkie.h"
#include "Vocab_US_Large.h"
//#include "Vocab_US_TI99.h"

// for IR receiver
#include <irmpSelectMain15Protocols.h> 
#include <irmp.hpp>
#define IRMP_SUPPORT_NEC_PROTOCOL        1 

// init LCD screen
// SDA : A4
// SCL : A5
LiquidCrystal_I2C lcd(0x27,20,4); 

// init talkie
// L : 3
// B : 11
Talkie voice;

// init IR
IRMP_DATA irmp_data;

// select potentiometer : pin A3

// init buttons
// start/enter : pin 12
// cancel/back : pin 13
int pinStart = 12;
int pinCancel = 13;

//status variables
boolean mainMenu = true;
boolean raceMenu = false;
boolean fastestLapMenu = false;
boolean raceMode = false;
boolean raceLapSelectMode = false;
boolean fastestLapSelectMode = false;
boolean selectingLaps = false;
boolean raceStarted = false;
boolean fastestLapStarted = false;
int totalRaceLaps = 3;
int totalFastestLaps = 10;

// players IR code
int mrWhite = 247;
int mrRed = 55;

// score best lap
int playerLapRecord = 0;
int lapRecord = 0;
int currentPlayer=0;
int currentBestLap=0;

// lap data
unsigned long currentTime = 0;
unsigned long previousTime = 0;
unsigned long lapTimes[20];
unsigned int currentLap = 0;
unsigned int fastest = 0;
unsigned int overallFastest = 99999;
unsigned int overallFastestPlayer = 0;

void setup() 
{
  // buttons
  digitalWrite(pinStart, INPUT);
  digitalWrite(pinCancel, INPUT);

  // LCD
  lcd.init();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Lap counter");
  lcd.setCursor(0,1);
  lcd.print("C.Jaspart 2023");

  // Voice 
  //delay(2000);
  //voice.say(sp2_DEVICE);
  //voice.say(sp2_READY);

  // for serial debugging
  Serial.begin(9600);

  // Init loop done
  lcd.clear();
}

void loop() 
{
  // display menu controller
  if (mainMenu)
  {
    displayMainMenu();
  }
  else if (raceMenu)
  {
    displayRaceMenu();
  }
  else if (fastestLapMenu)
  {
    displayFastestLapMenu();
  }
  else if (selectingLaps)
  {
    selectLaps();
  }
  else if (fastestLapStarted)
  {
    readFastestLap();
  }

  // enter/select button controller
  if (digitalRead(pinStart))
  {
    if (selectingLaps)
    {
      selectingLaps = false;
    }
    else if (mainMenu && raceMode)
    {
      mainMenu = false;
      raceMenu = true;
      displayRaceMenu();
    }
    else if (mainMenu && !raceMode)
    {
      mainMenu = false;
      fastestLapMenu = true;
      displayFastestLapMenu();
    }
    else if (raceMenu && raceLapSelectMode)
    {
        selectingLaps = true;
        selectLaps();
    }
    else if (raceMenu && !raceLapSelectMode)
    {
        raceMenu=false;
        startRace();
    }
    else if (fastestLapMenu && fastestLapSelectMode)
    {
        selectingLaps = true;
        selectLaps();
    }
    else if (fastestLapMenu && !fastestLapSelectMode)
    {
        fastestLapMenu=false;
        startRace();
    }

    delay(100);
  }

  // cancel/back button controller
  if (digitalRead(pinCancel))
  {
    if (raceMenu && !selectingLaps)
    {
      raceMenu = false;
      mainMenu = true;
    }
    else if (fastestLapMenu && !selectingLaps)
    {
      fastestLapMenu = false;
      mainMenu = true;
    }
    delay(100);
  }
}

void displayMainMenu()
{
  lcd.setCursor(2,0);
  lcd.print("Race          ");
  lcd.setCursor(2,1);
  lcd.print("Fastest lap   ");

  int cursorValue = map(analogRead(A3),0,510,0,1);
  if (cursorValue)
  {
    lcd.setCursor(0,0);
    lcd.print(" ");    
    lcd.setCursor(0,1);
    lcd.print(">");
    raceMode = false;
  }
  else
  {
    lcd.setCursor(0,1);
    lcd.print(" ");    
    lcd.setCursor(0,0);
    lcd.print(">");
    raceMode = true;
  }
}

void displayRaceMenu()
{
  lcd.setCursor(2,0);
  lcd.print("Total laps:");
  lcd.setCursor(14,0);
  lcd.print(totalRaceLaps);
  lcd.setCursor(2,1);
  lcd.print("Start race !");

  if (!selectingLaps)
  {
    int cursorValue = map(analogRead(A3),0,510,0,1);
    Serial.println(analogRead(A3));
    if (cursorValue)
    {
      lcd.setCursor(0,0);
      lcd.print(" ");    
      lcd.setCursor(0,1);
      lcd.print(">");
      raceLapSelectMode = false;
    }
    else
    {
      lcd.setCursor(0,1);
      lcd.print(" ");    
      lcd.setCursor(0,0);
      lcd.print(">");
      raceLapSelectMode = true;
    }
  }
}

void displayFastestLapMenu()
{
  lcd.setCursor(2,0);
  lcd.print("Total laps:");
  lcd.setCursor(14,0);
  lcd.print(totalFastestLaps);
  lcd.setCursor(2,1);
  lcd.print("Start laps !");

  if (!selectingLaps)
  {
    int cursorValue = map(analogRead(A3),0,510,0,1);
    Serial.println(analogRead(A3));
    if (cursorValue)
    {
      lcd.setCursor(0,0);
      lcd.print(" ");    
      lcd.setCursor(0,1);
      lcd.print(">");
      fastestLapSelectMode = false;
    }
    else
    {
      lcd.setCursor(0,1);
      lcd.print(" ");    
      lcd.setCursor(0,0);
      lcd.print(">");
      fastestLapSelectMode = true;
    }
  }
}

 void selectLaps()
 {
  lcd.setCursor(14,0);
  lcd.print("  ");
  int lapsValue = map(analogRead(A3),0,1000,1,20);
  lcd.setCursor(14,0);
  lcd.print(lapsValue);
  
  if (raceLapSelectMode)
  {
    totalRaceLaps = lapsValue;
  }
  else if (fastestLapSelectMode)
  {
    totalFastestLaps = lapsValue;
  }
  
  delay(200);
 }

void startRace()
{
  startIntro();
}


void startFastestLap()
{
  resetLapTimes();
  currentLap = 0;
  currentPlayer = 0;
  startIntro();
  previousTime = millis();
  delay(2000);
  raceStarted = true;  
}

 void startIntro()
 {
  lcd.noBacklight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Race starting");
  lcd.setCursor(0,1);
  lcd.print("in 5 seconds");
  delay(2000);
  voice.say(sp2_THREE);
  delay(1000);
  voice.say(sp2_TWO);
  delay(1000);
  voice.say(sp2_ONE);
  delay(1000);
  voice.say(sp2_GO);
  
 }

 void readFastestLap()
 {
  if (currentLap < totalFastestLaps && irmp_get_data(&irmp_data)) 
  { 
    if (irmp_data.command == mrWhite || irmp_data.command == mrRed)
    {
      // time detection
      currentTime = millis(); 
      
      // player detection
      if (currentLap == 0 && irmp_data.command == mrWhite)
      {
        currentPlayer = 1;
      }
      else if (currentLap == 0 && irmp_data.command == mrRed)
      {
        currentPlayer = 2;
      }
       
     // only process if the car wasn't detected twice within a 4s time frame
     if ((currentTime - previousTime) > 4000)
     {
       lapTimes[currentLap] = currentTime - previousTime;
       previousTime = currentTime;
       if (currentLap > 0 && isNewLapRecord(currentLap))
       {
         newLapRecord();
         fastest = lapTimes[currentLap];
       }
  
       currentLap++;
  
       if (currentLap >= totalFastestLaps)
       {
        endFastestLap();
       }
     }
    } 
  }
 }


void resetLapTimes()
{
  for(int i=0;i<20;i++)
  {
    lapTimes[i] = 0;
  }
}

boolean isNewLapRecord(int lapNumber)
{
  int i=0;
  boolean isFastest = true;
  for(i=0;i<lapNumber;i++)
  {
    if (lapTimes[lapNumber] > lapTimes[i])
    {
      isFastest = false;
    }
  }

  return isFastest;
}

boolean newLapRecord()
{
  voice.say(sp4_BRAVO);
}

void endFastestLap()
{
  // status ended
  fastestLapStarted = false;
  
  // todo end the race
  voice.say(sp3_OVER);

  // check for overall new lap record
  if (fastest < overallFastest)
  {
    overallFastest = fastest;
    overallFastestPlayer = currentPlayer;
  }

  // Display player result
  lcd.setCursor(0,0);
  if (currentPlayer == 1)
  {
    lcd.print("Mr W: ");
  }
  else
  {
    lcd.print("Mr R: ");
  }
  
  lcd.setCursor(0,6);
  lcd.print(fastest);

  // display overall fastest
  lcd.setCursor(0,0);
  if (overallFastestPlayer == 1)
  {
    lcd.print("Mr W: ");
  }
  else
  {
    lcd.print("Mr R: ");
  }
  lcd.setCursor(0,6);
  lcd.print(overallFastest);
}
