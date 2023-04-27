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
int btnStart = 12;
int btnCancel = 13;
int buttonReleaseDelay = 500; // ms

//status variables
enum menuItems { MAIN, RACE, GOFAST, SELECTLAP, TESTCODE };
int statut = MAIN;
int menuPosition = -1;

// race variables
int numberLaps = 3;
unsigned int startTime = 0;
unsigned int endTime = 0;
unsigned int lapTimes[20];
unsigned int mrWhiteNumberLaps=0;
unsigned int mrRedNumberLaps=0;
unsigned int currentLap = 0;
unsigned int bestGoFastScore = 0;
int currentCarCode = 0;

// players IR code
unsigned int mrWhite = 247;
unsigned int mrRed = 55;

void setup() 
{
  // buttons
  digitalWrite(btnStart, INPUT);
  digitalWrite(btnCancel, INPUT);

  // LCD
  lcd.init();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Lap counter");
  lcd.setCursor(0,1);
  lcd.print("C.Jaspart 2023");

  // Voice 
  delay(2000);
  voice.say(sp2_READY);

  // init IR
  irmp_init();

  // for serial debugging
  //Serial.begin(9600);

  // Init loop done
  lcd.clear();
}

void loop() 
{
  if (statut == MAIN)
  {
    displayMenu();
  }
  else if (statut == RACE)
  {
    startRace();
  }
  else if (statut == GOFAST)
  {
    startGoFast();
  }
  else if (statut == SELECTLAP)
  {
    selectNumberLaps();
  }
  else if (statut == TESTCODE)
  {
    testCode();
  }

  // gestion du bouton enter/start
  if (digitalRead(btnStart))
  {
    //Serial.println("button start pushed");
    if (statut == MAIN)
    {
      statut = menuPosition;
    }
  }
}

void displayMenu()
{
  // selection du menu
  int cursorValue = map(analogRead(A3),0,1024,1,5);
  
  //Serial.println(cursorValue);
  
  // position the cursor
  if (cursorValue != menuPosition)
  {
    // affichage du menu
    lcd.setCursor(1,0);
    lcd.print("Race     GoFast");
    lcd.setCursor(1,1);
    lcd.print("Laps     Test  ");
    
    switch(menuPosition)
    {
      case RACE:      lcd.setCursor(0,0);break;
      case GOFAST:    lcd.setCursor(9,0);break;
      case SELECTLAP: lcd.setCursor(0,1);break;
      case TESTCODE:  lcd.setCursor(9,1);break;
      default: break;
    }
  
    lcd.print(" ");  

    switch(cursorValue)
    {
      case RACE:      lcd.setCursor(0,0);break;
      case GOFAST:    lcd.setCursor(9,0);break;
      case SELECTLAP: lcd.setCursor(0,1);break;
      case TESTCODE:  lcd.setCursor(9,1);break;
      default: break;
    }

    lcd.print(">"); 
    menuPosition = cursorValue;
  }
}

// start race
void startRace()
{
    // init players number laps
    mrWhiteNumberLaps=0;
    mrRedNumberLaps=0;
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(" Race: ");
    lcd.print(numberLaps);
    lcd.print(" laps");

    // countdown 10s before start
    for(int t=10;t>=0;t--)
    {
      lcd.setCursor(0,1);
      lcd.print(" Starts in ");
      lcd.print(t);
      lcd.print("s      ");
  
      switch(t)
      {
        case 5: voice.say(sp2_FIVE);break;
        case 4: voice.say(sp2_FOUR);break;
        case 3: voice.say(sp2_THREE);break;
        case 2: voice.say(sp2_TWO);break;
        case 1: voice.say(sp2_ONE);break;
        case 0: voice.say(sp2_GO);break;
      }
      delay(1000);
    }

    lcd.setCursor(0,1);
    lcd.print(" Race started !    ");
    delay(1500); // be sure no detection from car at start position
    irmp_init();
    boolean cancelRace = false;
  
    // race loop
    while( mrWhiteNumberLaps < numberLaps && mrRedNumberLaps < numberLaps && !cancelRace)
    {
        if (irmp_get_data(&irmp_data)) 
        { 
          unsigned int carCode = irmp_data.command;
          if ( carCode == mrWhite)
          {
            mrWhiteNumberLaps++;
          }
          else if ( carCode == mrRed)
          {
            mrRedNumberLaps++;
          }

          lcd.setCursor(0,1);
          lcd.print(" White:");
          lcd.print(mrWhiteNumberLaps);
          lcd.print(" Red:");
          lcd.print(mrRedNumberLaps);
          lcd.print("      ");
        }
       
        // cancel the race
        if (digitalRead(btnCancel))
        {
          lcd.setCursor(0,1);
          lcd.print(" Race canceled    ");
          voice.say(sp2_CANCEL);
          cancelRace = true;
          menuPosition = -1;
          statut = MAIN;
          delay(1500);
        }

       delay(200);
    }

    // race ended, display result
    if (!cancelRace)
    {   
       
        lcd.setCursor(0,1);
        if (mrWhiteNumberLaps == numberLaps)
        { 
          lcd.print(" MrWhite wins!    ");
          voice.say(sp5_INNER);
          voice.say(sp3_WHITE);
        }
        else
        {
          lcd.print(" MrRed wins!     ");
          voice.say(sp5_INNER);
          voice.say(sp3_RED);
        }
        
    }

    boolean quitRace = false;
    while(!quitRace)
    {
      if (digitalRead(btnStart) || digitalRead(btnCancel))
      {
        quitRace = true;
        menuPosition = -1;
        statut = MAIN;
        delay(buttonReleaseDelay);
      }
    }
    
}

// start go fast
void startGoFast()
{
  clearLapTimes();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Go Fast:");
  lcd.print(numberLaps);
  lcd.print(" laps");

  // countdown 10s before start
  for(int t=10;t>=0;t--)
  {
    lcd.setCursor(0,1);
    lcd.print(" Starts in ");
    lcd.print(t);
    lcd.print("s      ");

    switch(t)
    {
      case 5: voice.say(sp2_FIVE);break;
      case 4: voice.say(sp2_FOUR);break;
      case 3: voice.say(sp2_THREE);break;
      case 2: voice.say(sp2_TWO);break;
      case 1: voice.say(sp2_ONE);break;
      case 0: voice.say(sp2_GO);
              startTime = millis();
              break;
    }
    delay(1000);
  }

  // display current lap
  delay(1500); // be sure no detection from car at start position
  irmp_init();
  currentLap = 1;
  displayLap(currentLap);
  boolean cancelRace = false;
  
  // race loop
  while((currentLap < (numberLaps+1)) && !cancelRace)
  {
    // car detected
    if (irmp_get_data(&irmp_data)) 
    { 
      endTime = millis();
      unsigned int carCode = irmp_data.command;
      if ( carCode == mrWhite)
      {
          sayLap(currentLap);
          lapTimes[currentLap-1] = endTime - startTime;
          startTime = endTime;
          currentLap++;
            
          // be sure car is long gone to avoid double detection 
          if (currentLap < numberLaps+1)
          {
            displayLap(currentLap);
            delay(2000);
            irmp_init();
          }
      }
    }
      
    // cancel the race
    if (digitalRead(btnCancel))
    {
      lcd.setCursor(0,1);
      lcd.print(" Race canceled    ");
      voice.say(sp2_CANCEL);
      cancelRace = true;
      menuPosition = -1;
      statut = MAIN;
      delay(1500);
    }
  }

  // race ended, display result
  if (!cancelRace)
  {
    voice.say(sp4_WAY);
    voice.say(sp2_OVER);
    lcd.setCursor(0,0);
    lcd.print(" Best: ");
    unsigned int result = getBestLapTime();
    lcd.print((float)result/1000);
    lcd.print("s     ");

    // New go fast record or not ?
    lcd.setCursor(0,1);
    if (isAGoFastRecord(result))
    {
      lcd.print("** NEW RECORD **   ");
    }
    else
    {
      lcd.print(" To beat: ");
      lcd.print((float)bestGoFastScore/1000);
      lcd.print("s     ");
    }
    
    boolean quitGoFast = false;
    while(!quitGoFast)
    {
      if (digitalRead(btnStart) || digitalRead(btnCancel))
      {
        quitGoFast = true;
        menuPosition = -1;
        statut = MAIN;
        delay(buttonReleaseDelay);
      }
    }
  }
}

boolean isAGoFastRecord(unsigned int res)
{
  boolean recordBroken = false;
  
  if (bestGoFastScore == 0)
  {
    bestGoFastScore = res;
    recordBroken = false;
  }
  else if (res < bestGoFastScore)
  {
    recordBroken = true;
  }

  return recordBroken;
}

unsigned int getBestLapTime()
{
  int best=lapTimes[0];

  if (numberLaps > 1)
  {
    for(int i=1;i<numberLaps;i++)
    {
      if (best > lapTimes[i])
      {
        best = lapTimes[i];
      }
    }
  }

  return best;
}

void sayLap(int lap)
{
  //voice.say(sp5_FLAPS);
  switch(lap)
  {
      case 20: voice.say(sp3_TWENTY);break;
      case 19: voice.say(sp3_NINETEEN);break;
      case 18: voice.say(sp3_EIGHTEEN);break;
      case 17: voice.say(sp3_SEVENTEEN);break;
      case 16: voice.say(sp3_SIXTEEN);break;
      case 15: voice.say(sp3_FIFTEEN);break;
      case 14: voice.say(sp3_FOURTEEN);break;
      case 13: voice.say(sp3_THIRTEEN);break;
      case 12: voice.say(sp3_TWELVE);break;
      case 11: voice.say(sp3_ELEVEN);break;
      case 10: voice.say(sp3_TEN);break;
      case 9: voice.say(sp3_NINE);break;
      case 8: voice.say(sp3_EIGHT);break;
      case 7: voice.say(sp3_SEVEN);break;
      case 6: voice.say(sp3_SIX);break;
      case 5: voice.say(sp3_FIVE);break;
      case 4: voice.say(sp3_FOUR);break;
      case 3: voice.say(sp3_THREE);break;
      case 2: voice.say(sp3_TWO);break;
      case 1: voice.say(sp3_ONE);break;
  }
}

void displayLap(int lapNumber)
{
  lcd.setCursor(0,1);
  lcd.print(" Lap ");
  lcd.print(lapNumber);
  lcd.print("/");
  lcd.print(numberLaps);
  lcd.print("     ");
}

void clearLapTimes()
{
  for (int i=0;i<20;i++)
    lapTimes[i] = 0;
}

// select number total laps
void selectNumberLaps()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Number laps: ");
  lcd.print(numberLaps);

  boolean selectingValue = true;
  int selectedLaps = 0;
  int previousValue = -1;

  // avoid button enter pushed twice
  delay(buttonReleaseDelay);

  while(selectingValue)
  {
    // display the new selected value
    selectedLaps = map(analogRead(A3),0,1000,1,20);
    if (previousValue != selectedLaps)
    {
      lcd.setCursor(1,1);
      lcd.print("New value: ");
      lcd.print(selectedLaps);
      if (selectedLaps < 10)
        lcd.print(" ");
        
      previousValue = selectedLaps;
    }

    if (digitalRead(btnStart))
    {
      numberLaps = selectedLaps;
      selectingValue = false;
      menuPosition = -1;
      statut = MAIN;
      delay(buttonReleaseDelay);
    }

    if (digitalRead(btnCancel))
    {
      selectingValue = false;
      menuPosition = -1;
      statut = MAIN;
      delay(buttonReleaseDelay);
    }
  }
}

// Display the code sent from the transponder
void testCode()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Code testing");

  boolean testing = true;

  // avoid button enter pushed twice
  delay(buttonReleaseDelay);

  while (testing)
  {
    if (irmp_get_data(&irmp_data)) 
    { 
      int receivedCode = irmp_data.command;
      //Serial.println(receivedCode);
      
      lcd.setCursor(1,1);
      lcd.print("                ");
      lcd.setCursor(1,1);
      lcd.print(receivedCode);
      
      if ( receivedCode == mrWhite)
      {
        lcd.print(" : MrWhite");
      }
      else if (receivedCode == mrRed)
      {
        lcd.print(" : MrRed");
      }

      delay(2000);
    }
    else
    {
      lcd.setCursor(1,1);
      lcd.print("Waiting ...   ");
    }

    if (digitalRead(btnStart) || digitalRead(btnCancel))
    {
      testing = false;
      menuPosition = -1;
      statut = MAIN;
      delay(buttonReleaseDelay);
    }
  }
}
