#include <TM1637.h>
#include <Wire.h>
// Code by Tobias Kuch 2019, Licesed unter GPL 3.0

// Instantiation and pins configurations
// Pin 4 - > DIO
// Pin 5 - > CLK
TM1637 tm1637(4, 5);


#define BUTTON_MINUTEUP_PIN   2    // Digital IO pin connected to the button.  This will be
                          // driven with a pull-up resistor so the switch should
                          // pull the pin to ground momentarily.  On a high -> low
                          // transition the button press logic will execute.
                          // Used for Setting the Clock Time

#define BUTTON_HOURUP_PIN   3    // Digital IO pin connected to the button.  This will be
                          // driven with a pull-up resistor so the switch should
                          // pull the pin to ground momentarily.  On a high -> low
                          // transition the button press logic will execute.
                          // Used for Setting the Clock Time


// interrupt Control
bool SecInterruptOccured = true;
bool A60telSecInterruptOccured = true;
byte A60telSeconds24 = 0;


// Clock Variables
byte Seconds24;
byte Minutes24 ;
byte Hours24;
bool DisableSecondDisplay = false;
bool MinSetQuickTime = false;
bool HourSetQuickTime = false;
bool ButtonDPress = false;
bool ButtonEPress = false;

TwoWire I2CWire = TwoWire(0);

struct BHLightSensorData  
  {
    int Lux = 0 ;          // Lichtstärke in Lux
    int Old_Lux = 0 ;      // Lichtstärke in Lux   
    bool DataValid  = false;
    bool SensorEnabled  = false;
  };

BHLightSensorData BHMeasure;

bool Run_BH1750Sensor (bool Init)   // Runtime Funktion für den BH170 Lichtsensor
{
byte ec;    

  if (Init)
    {
    bool BH1750Detected = false;    
    I2CWire.beginTransmission(35);
    ec=I2CWire.endTransmission(true);
    if(ec==0)
      {
      BH1750Detected = true;
      BH1750I2CAddress = 35; // BH1750 I2C Adresse ist DEC 35
      } else
      {
      I2CWire.beginTransmission(92);
      ec=I2CWire.endTransmission(true);
      if(ec==0)
        {
        BH1750Detected = true;
        BH1750I2CAddress = 92; // BH1750 I2C Adresse ist DEC 92
        }
      }
    if (BH1750Detected) 
      {
      // Intialize Sensor 
      I2CWire.beginTransmission(BH1750I2CAddress);  
      I2CWire.write(0x01);    // Turn it on before we can reset it
      I2CWire.endTransmission();
       
      I2CWire.beginTransmission(BH1750I2CAddress);  
      I2CWire.write(0x07);    // Reset
      I2CWire.endTransmission();

      I2CWire.beginTransmission(BH1750I2CAddress);  
      I2CWire.write(0x10);    // Continuously H-Resolution Mode ( 1 lux Resolution) Weitere Modis möglich, gemäß Datenblatt
      //I2CWire.write(0x11);  // Continuously H-Resolution Mode 2 ( 0.5 lux Resolution)
      //I2CWire.write(0x20);  // One Time H-Resolution Mode  ( 1 lux Resolution)
      //I2CWire.write(0x21);  // One Time H-Resolution Mode2 ( 0.5 lux Resolution)
      I2CWire.endTransmission();
       
      } else       
      {      
      return BH1750Detected; 
      }    
    }
  I2CWire.beginTransmission(BH1750I2CAddress);
  ec=I2CWire.endTransmission(true);
  if(ec==0)
    {
    I2CWire.requestFrom(BH1750I2CAddress, 2);
    BHMeasure.Lux = I2CWire.read();
    BHMeasure.Lux <<= 8;                  // Verschieben der unteren 8 Bits in die höhreren 8 Bits der 16 Bit breiten Zahl
    BHMeasure.Lux |= I2CWire.read();
    BHMeasure.Lux = BHMeasure.Lux / 1.2;
    BHMeasure.DataValid = true;
    if (BHMeasure.Lux != BHMeasure.Old_Lux)
      {
      BHMeasure.Old_Lux = BHMeasure.Lux;        
      Update_Blynk_APP(8,true);  //  Lichtstärkeanzeige in Lux aktualisieren
      #ifdef DEBUG
      Serial.print ("Lichtstärke in Lux :");
      Serial.println (BHMeasure.Lux);
      #endif
      }     
    } else
    {
    BHMeasure.DataValid = false;
    BHMeasure.SensorEnabled = false;  
    }   
  
return true;  
}





//Interrupt Routines




ISR(TIMER1_COMPA_vect)        
{
  A60telSeconds24++;
   tm1637.switchColon();
  if ((A60telSeconds24 > 59) and !(MinSetQuickTime))
    {
       A60telSeconds24 = 0;
      //Calculate Time 24 Stunden Format
      SecInterruptOccured = true;
      Seconds24++;
      if (Seconds24 > 59)
        {
          Seconds24 = 0;
          Minutes24++;
        }
      if (Minutes24 > 59)
        {
          Minutes24 = 0;
          Hours24++;      
        }
      if (Hours24 > 23)
        {
          Hours24 = 0;
        }     
    } 
  
    if  (MinSetQuickTime)
    {
        A60telSeconds24 = 0;
      //Calculate Time 24 h Format
      SecInterruptOccured = true;
      Seconds24++;
      if (Seconds24 > 59)
        {
          Seconds24 = 0;
          Minutes24++;
        }
      if (Minutes24 > 59)
        {
          Minutes24 = 0;
          Hours24++;      
        }
      if (Hours24 > 23)
        {
          Hours24 = 0;
        }    
    }
    
  TCNT1 = 0;      // Register mit 0 initialisieren

  
if  (HourSetQuickTime)
    {
     OCR1A =  200;
    } else
    {
     OCR1A =  33353;      // Output Compare Register vorbelegen 
    }   
  A60telSecInterruptOccured = true;
}




//Interrupts ende

void CheckConfigButtons ()    // InterruptRoutine

{

 bool PressedZ;

PressedZ= digitalRead(BUTTON_MINUTEUP_PIN);
if ((PressedZ == LOW) and (ButtonDPress == false))
   {
    ButtonDPress = true;   
    delay(100);
    Minutes24++;
    Seconds24 = 0;  // Reset Seconds to zero to avoid Randomly time
    DisableSecondDisplay = true ;   // Disable Seconds While Clock Set 
    MinSetQuickTime = true; //Enable Quick Tmime Passby  

   }
if ((PressedZ == HIGH) and (ButtonDPress == true))
   {
   ButtonDPress = false; 
   delay(100);
   DisableSecondDisplay = false ;   // Enable Seconds While Clock Set   
   MinSetQuickTime = false;
   Seconds24 = 0;  // Reset Seconds to zero to avoid Randomly time
   A60telSeconds24 = 0;  
    
   }
   
PressedZ= digitalRead(BUTTON_HOURUP_PIN);
if ((PressedZ == LOW) and (ButtonEPress == false))
   {
      ButtonEPress = true;      
      delay(100);
      DisableSecondDisplay = true ;   // Disable Seconds While Clock Set 
      MinSetQuickTime = true; //Enable Quick Tmime Passby
      HourSetQuickTime = true;
   }
if ((PressedZ == HIGH) and (ButtonEPress == true))
   {  
      noInterrupts();   // deactivate Interrupts 
      ButtonEPress = false;  
      delay(100);
      Minutes24++;
      DisableSecondDisplay = false ;   // Enable Seconds While Clock Set 
      MinSetQuickTime = false; //Enable Quick Tmime Passby
      HourSetQuickTime = false; 
      Seconds24 = 0;  // Reset Seconds to zero to avoid Randomly time
      A60telSeconds24 = 0;    
      interrupts();   // enable all Interrupts 
   }


}




void setup()
{
    tm1637.init();
    tm1637.setBrightness(8); // Highest Brightness
    pinMode(BUTTON_MINUTEUP_PIN, INPUT_PULLUP);
    pinMode(BUTTON_HOURUP_PIN, INPUT_PULLUP);
    digitalWrite(LED_BUILTIN, LOW);
    noInterrupts(); 
    TCCR1A = 0x00;  
    TCCR1B =  0x02;
    TCNT1 = 0;      // Register mit 0 initialisieren
    OCR1A =  33353;      // Output Compare Register vorbelegen 
    TIMSK1 |= (1 << OCIE1A);  // Timer Compare Interrupt aktivieren
    interrupts();   
    Seconds24 = 1;
    Minutes24 = 1;
    Hours24 = 0;
    tm1637.dispNumber(Minutes24 + Hours24 * 100);   
}


void DisplayClockOnLedTM1637()

{
  tm1637.switchColon();
  tm1637.dispNumber(Minutes24 + Hours24 * 100); 
}

void loop()
{
  bool PressedC; 
  if (A60telSecInterruptOccured) 
    {
      A60telSecInterruptOccured = false;    
    }
  if (SecInterruptOccured) 
    {
        SecInterruptOccured = false;
        DisplayClockOnLedTM1637();    
      }
   CheckConfigButtons();   
}
