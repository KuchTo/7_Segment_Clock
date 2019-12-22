#include <TM1637.h>
#include <SoftwareSerial.h>
// Code by Tobias Kuch 2019, Licesed unter GPL 3.0

// Instantiation and pins configurations
// Pin 4 - > DIO
// Pin 5 - > CLK
TM1637 tm1637(4, 5);

// software serial #1: RX = digital pin 6, TX = digital pin 7
SoftwareSerial GPSModule(6, 7);


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
    Serial.begin(9600);
      while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
     }
    GPSModule.begin(9600);
    while (!GPSModule) {
        ; // wait for serial port to connect. Needed for native USB port only
     }
   
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
    GPSModule.listen();
}


void DisplayClockOnLedTM1637()

{
  tm1637.switchColon();
  tm1637.dispNumber(Minutes24 + Hours24 * 100); 
}

void loop()
{
  bool PressedC; 
  
 // Serial.println("Data from GPSModule:");
  // while there is data coming in, read it
  // and send to the hardware serial port:
  while (GPSModule.available() > 0) {
    char inByte = GPSModule.read();
    Serial.write(inByte);
  }
  
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
