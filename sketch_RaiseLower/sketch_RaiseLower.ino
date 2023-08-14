/*
Things to still add include:
- Calibrating time for setting raise/lower intervals
  - EEPROM or storage
- Write calibrated distance values to EEPROM
- Timer settings for automation
  - Could use clock or every x minutes
- Moving desk to calibrated values
  - SetDeskHeight(blnDirection)
*/

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
int strSerialInput;
int strBTSerialInput;
const int pinRaise = 22;  // Pin value will likely change once on proto board
const int pinLower = 23;  // Pin value will likely change once on proto board
const int pinTrig = 13;   // Pin value will likely change once on proto board
const int pinEcho = 12;   // Pin value will likely change once on proto board
int intChairHeight = 0;
int intStandHeight = 0;
bool blnTS = false;

void setup()
{
  // Initialize serial
  Serial.begin(115200);
  
  // Configure pins
  pinMode(pinRaise, OUTPUT);
  pinMode(pinLower, OUTPUT);
  digitalWrite(pinRaise, HIGH);
  digitalWrite(pinLower, HIGH);
  pinMode(pinTrig, OUTPUT);
  pinMode(pinEcho, INPUT);

  // Bluetooth initialization
  SerialBT.begin("SmartDesk_Test"); //Bluetooth device name
  Serial.println("The device started, now you can connect it with bluetooth!");
}

void loop()
{
  // Read any serial inputs
  strSerialInput = Serial.read();
  strBTSerialInput = SerialBT.read();
  
  // Check for serial triggers
  switch (strSerialInput)
  {
    case 'C':
      intChairHeight = CalibrateHeight();
      break;
    
    case 'S':
      intStandHeight = CalibrateHeight();
      break;

    case 'R':
      if(blnTS) {Serial.println("Raise for 2 seconds...");}
      digitalWrite(pinRaise, LOW);
      delay(2000);
      digitalWrite(pinRaise, HIGH);

      // SetDeskHeight(true);
      break;
    
    case 'L':
      if(blnTS) {Serial.println("Lower for 2 seconds...");}
      digitalWrite(pinLower, LOW);
      delay(2000);
      digitalWrite(pinLower, HIGH);

      // SetDeskHeight(false);
      break;
    
    case 'T':
      // Toggle troubleshooting mode
      blnTS = !blnTS;
      Serial.println("Troubleshooting mode: " + String(blnTS));
      break;

    default:
      break;
  }

  // Check for bluetooth serial triggers
  switch (strBTSerialInput)
  {
  case 'R':
      SerialBT.println("Raise for 2 seconds...");
      digitalWrite(pinRaise, LOW);
      delay(2000);
      digitalWrite(pinRaise, HIGH);
      break;
    break;
  
  case 'L':
      Serial.println("Lower for 2 seconds...");
      digitalWrite(pinLower, LOW);
      delay(2000);
      digitalWrite(pinLower, HIGH);
      break;
    break;
  
  default:    
    break;
  }
  
  // Minor delay to avoid hiccups
  delay(20);
}

int GetUltrasonicReading()
{
  /*
    Desc:     This method uses the ultrasonic sensor to get the current distance
    Params:   None
    Returns:  Current distance as integer (in cm)
  */

  float duration, distance;

  // Get readings from HC-SR04 ultrasonic sensor
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);

  // Translate readings to cm
  duration = pulseIn(pinEcho, HIGH);
  distance = (duration*.0343)/2;
  return (int) distance;
}

int CalibrateHeight()
{
  /* 
    Desc:     This method uses the ultrasonic sensor to calibrate either the chair or standing value
    Params:   None
    Returns:  Calibrated distance as integer (in cm)
  */
  
  int intCummulative = 0;
  int intAvg = 0;
  int intErrorCheck = 0;
  int intDistance = 0;
  int intHighBoundry = 0;
  int intLowBoundry = 0;

  for(int i = 0; i < 5; i++)
  {
    // Get current US reading
    intDistance = GetUltrasonicReading();    

    // Check distance to make sure it makes sense
    if(i == 0)
    {
      // First time through, just add reading to cummulative
      intCummulative = intDistance;
    }
    else
    {
      // Get high and low boundries (avg distance +/- 2 cm)
      intHighBoundry = (intCummulative/i) + 2;
      intLowBoundry = (intCummulative/i) - 2;

      if(blnTS)
      {
        Serial.println("High boundry: " + String(intHighBoundry));
        Serial.println("Low boundry: " + String(intLowBoundry));
      }

      // Check current reading against boundries - it needs to be between them
      if(intDistance > intLowBoundry && intDistance < intHighBoundry)
      {
        // If reading checks out, add to cummulative and continue
        intCummulative += intDistance;
      }
      else
      {
        if(blnTS) {Serial.println("Iteration " + String(i) + " failed with distance " + String(intDistance));}
        
        // If the reading is not within boundries, repeat loop and increment error check
        i--;
        intErrorCheck++;

        // If error check fails more than three times, restart calibration
        if(intErrorCheck > 3)
        {
          if(blnTS) {Serial.println("Calibration error triggered");}
          
          // Reset variables
          i = -1;
          intCummulative = 0;
          intErrorCheck = 0;
        }
      }
    }

    if(blnTS)
    {
      Serial.println("Iteration: " + String(i));
      Serial.println("Distance: " + String(intDistance));
    }
    
    delay(100);
  }

  // Get average value for calibration setting
  intAvg = intCummulative / 5;

  if(blnTS)
  {
    Serial.println("Cummulative: " + String(intCummulative));
    Serial.println("Average: " + String(intAvg));
    Serial.println("*** Calibration Completed ***");
  }

  return intAvg;
}

void SetDeskHeight(bool blnDirection)
{
  /* 
    Desc:     This method sets the desk height to either the chair or standing calibrated values
    Params:   Boolean value for chair or standing height
    Returns:  None
  */


}