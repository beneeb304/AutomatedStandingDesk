/*
Things to still add include:
- Calibrating time for setting raise/lower intervals
  - EEPROM or storage
- Timer settings for automation
  - Could use clock or every x minutes
- Add bluetooth serial output for troubleshooting mode
- Add bluetooth commands
  - Experiment with adding serial (usb or bt) and only have one switch-case
*/

#include <BluetoothSerial.h>
#include <Preferences.h>

Preferences preferences;

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
  SerialBT.begin("SmartDesk_Test"); // Bluetooth device name
  Serial.println("The device started, now you can connect it with bluetooth!");

  // Create storage space in read/write mode
  preferences.begin("calibrations", false);
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
      // Calibrate the chair height
      SaveDeskHeight(false, CalibrateHeight());
      break;
    
    case 'S':
      // Calibrate the standing height
      SaveDeskHeight(true, CalibrateHeight());
      break;

    case 'R':
      // Move the desk to the standing height
      SetDeskHeight(true);
      break;
    
    case 'L':
      // Move the desk to the chair height
      SetDeskHeight(false);
      break;
    
    case 'T':
      // Toggle troubleshooting mode
      blnTS = !blnTS;
      Serial.println("Troubleshooting mode: " + String(blnTS));
      break;

    default:
      break;
  }

  // // Check for bluetooth serial triggers
  // switch (strBTSerialInput)
  // {
  // case 'R':
  //     SerialBT.println("Raise for 2 seconds...");
  //     digitalWrite(pinRaise, LOW);
  //     delay(2000);
  //     digitalWrite(pinRaise, HIGH);
  //     break;
  //   break;
  
  // case 'L':
  //     Serial.println("Lower for 2 seconds...");
  //     digitalWrite(pinLower, LOW);
  //     delay(2000);
  //     digitalWrite(pinLower, HIGH);
  //     break;
  //   break;
  
  // default:    
  //   break;
  // }
  
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

  float fltDuration, fltDistance;

  // Get readings from HC-SR04 ultrasonic sensor
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);

  // Translate readings to cm
  fltDuration = pulseIn(pinEcho, HIGH);
  fltDistance = (fltDuration*.0343)/2;
  return (int) fltDistance;
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

  // Set a failsafe maximum of 10 seconds for the desk to be in motion
  unsigned long maxSeconds = 10000;
  unsigned long previousMillis = millis();
  unsigned long currentMillis = previousMillis;
  int intRequestedHeight;

  if(blnDirection) {
    // Get the height we need to raise to
    intRequestedHeight = preferences.getUInt("intStandHeight", 1);

    if(blnTS) {
      Serial.println("Starting to raise desk...");
      Serial.println("Requested stand height: " + String(intRequestedHeight));
      Serial.println("Current height: " + String(GetUltrasonicReading()));
    }

    // Only continue if 10 seconds have NOT passed AND the current height is < the standing height
    while((currentMillis - previousMillis < maxSeconds) && (GetUltrasonicReading() < intRequestedHeight)) {
      // Update current time
      currentMillis = millis();

      // Raise the desk
      digitalWrite(pinRaise, LOW);
    }

    // Once loop conditions are not true, stop raising the desk
    digitalWrite(pinRaise, HIGH);

    if(blnTS) {
      Serial.println("Finished raising desk...");
      Serial.println("Current height: " + String(GetUltrasonicReading()));
      Serial.println("Elapsed time to raise: " + String(currentMillis - previousMillis));
    }
  } else {
    // Get the height we need to lower to
    intRequestedHeight = preferences.getUInt("intChairHeight", 150);

    if(blnTS) {
      Serial.println("Starting to lower desk...");
      Serial.println("Requested chair height: " + String(intRequestedHeight));
      Serial.println("Current height: " + String(GetUltrasonicReading()));
    }

    // Only continue if 10 seconds have NOT passed AND the current height is > the chair height
    while((currentMillis - previousMillis < maxSeconds) && (GetUltrasonicReading() > intRequestedHeight)) {
      // Update current time
      currentMillis = millis();

      // Lower the desk
      digitalWrite(pinLower, LOW);
    }

    // Once loop conditions are not true, stop lowering the desk
    digitalWrite(pinLower, HIGH);

    if(blnTS) {
      Serial.println("Finished lowering desk...");
      Serial.println("Current height: " + String(GetUltrasonicReading()));
      Serial.println("Elapsed time to lower: " + String(currentMillis - previousMillis));
    }
  }
}

void SaveDeskHeight(bool blnStanding, int intHeight) {
  /* 
    Desc:     Saves the calibrated values (chair or stand) to preferences library
    Params:   Boolean value for chair or stand, int value for the newly calibrated value
    Returns:  None
  */
  
  // Determine which value we're setting (chair or standing)
  if(blnStanding) {
    // Setting the standing value, get the chair value
    int intChairHeight = preferences.getUInt("intChairHeight", 1);

    // Only set the stand height if the new height is less than the chair height.
    if(intHeight > intChairHeight) {
      preferences.putUInt("intStandHeight", intHeight);
      if(blnTS) {Serial.println("Stand calibration saved!" + String(intHeight));}
    } else {
      if(blnTS) {
        Serial.println("Stand calibration NOT saved!");
        Serial.println("Attempted stand value of " + String(intHeight) + " is less than chair value");
        Serial.println("Stored chair height: " + String(intChairHeight));
        Serial.println("Stored stand height: " + String(preferences.getUInt("intStandHeight", 150)));
      }
    }
  } else {
    // If setting the chair value, get the standing value
    int intStandHeight = preferences.getUInt("intStandHeight", 150);

    // Only set the chair height if the new height is greater than the standing height.
    if(intHeight < intStandHeight) {
      preferences.putUInt("intChairHeight", intHeight);
      if(blnTS) {Serial.println("Chair calibration saved! " + String(intHeight));}
    } else {
      if(blnTS) {
        Serial.println("Chair calibration NOT saved!");
        Serial.println("Attempted chair value of " + String(intHeight) + " is greater than stand value");
        Serial.println("Stored chair height: " + String(preferences.getUInt("intChairHeight", 1)));
        Serial.println("Stored stand height: " + String(intStandHeight));
      }
    }
  }
}