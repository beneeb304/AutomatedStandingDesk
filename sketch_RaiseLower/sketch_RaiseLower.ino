// ****************************************************************************************************
// File:          sketch_RaiseLower.ino
// Project:       Automated Standing Desk
// Created:       8/3/2023
// Author:        Ben Neeb
// Purpose:       Automatically raise and lower SteelCase standing desks
// Input:         NA, IoT project
// Output:        NA, IoT project
// Requirements:  ESP32-WROOM-DA board and an ultrasonic module HC-SR04
// Revised:       
// Change Notes:  
// ****************************************************************************************************

#include <BluetoothSerial.h>
#include <Preferences.h>

Preferences preferences;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
char chrSerialInput;
const int pinRaise = 22;  // Pin value will likely change once on proto board
const int pinLower = 23;  // Pin value will likely change once on proto board
const int pinTrig = 13;   // Pin value will likely change once on proto board
const int pinEcho = 12;   // Pin value will likely change once on proto board
bool blnVerbose = false;
bool blnAutoChange = true;
unsigned long lngLastChange = 0;
String strMinutes = "";

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
  SerialBT.begin("SmartDesk"); // Bluetooth device name
  Serial.println("The device started, now you can connect it with bluetooth!");

  // Create storage space in read/write mode
  preferences.begin("calibrations", false);
}

void loop()
{
  // Loop while either bluetooth or USB serial is available
  while (SerialBT.available() || Serial.available())
  {
    // Read bt serial, if not available read usb serial
    if(SerialBT.available()) 
    {
      chrSerialInput = SerialBT.read(); 
      if(blnVerbose && isalnum(chrSerialInput)) { VerboseOutput("Recieved BT serial: " + String(chrSerialInput)); }
    }
    else if(Serial.available())
    {
      chrSerialInput = Serial.read();
      if(blnVerbose && isalnum(chrSerialInput)) { VerboseOutput("Recieved USB serial: " + String(chrSerialInput)); }
    }

    // Check for serial triggers
    switch (chrSerialInput)
    {
      case 'A':
        // Toggle desk auto height change
        blnAutoChange = !blnAutoChange;
        if(blnVerbose) { VerboseOutput("Auto change: " + String(blnAutoChange)); }
        break;
        
      case 'M':
        SaveTimeInterval(strMinutes.toInt());
        break;

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
      
      case 'V':
        // Toggle verbose mode for Bluetooth
        blnVerbose = !blnVerbose;
        VerboseOutput("Verbose mode for Bluetooth: " + String(blnVerbose));
        break;

      case 'v':
        // Toggle verbose mode for USB
        blnVerbose = !blnVerbose;
        VerboseOutput("Verbose mode for USB: " + String(blnVerbose));
        break;

      default:
        // Assume we are recieving a numeric value in char form and append to string
        strMinutes += chrSerialInput;
        break;
    }
  }

  // Clear minutes variable when not recieving serial data
  strMinutes = "";

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

      if(blnVerbose) { VerboseOutput("High boundry: " + String(intHighBoundry) + ", Low boundry: " + String(intLowBoundry)); }

      // Check current reading against boundries - it needs to be between them
      if(intDistance > intLowBoundry && intDistance < intHighBoundry)
      {
        // If reading checks out, add to cummulative and continue
        intCummulative += intDistance;
      }
      else
      {
        if(blnVerbose) { VerboseOutput("Iteration " + String(i) + " failed with distance " + String(intDistance)); }

        // If the reading is not within boundries, repeat loop and increment error check
        i--;
        intErrorCheck++;

        // If error check fails more than three times, restart calibration
        if(intErrorCheck > 3)
        {
          if(blnVerbose) { VerboseOutput("Calibration error triggered"); }
          
          // Reset variables
          i = -1;
          intCummulative = 0;
          intErrorCheck = 0;
        }
      }
    }

    if(blnVerbose) { VerboseOutput("Iteration: " + String(i) + ", Distance: " + String(intDistance)); }

    delay(100);
  }

  // Get average value for calibration setting
  intAvg = intCummulative / 5;

  if(blnVerbose)
  {
    VerboseOutput("Cummulative: " + String(intCummulative));
    VerboseOutput("Average: " + String(intAvg));
    VerboseOutput("*** Calibration Completed ***");
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

  if(blnDirection)
  {
    // Get the height we need to raise to
    intRequestedHeight = preferences.getUInt("intStandHeight", 150);

    if(blnVerbose)
    {
      VerboseOutput("Starting to raise desk...");
      VerboseOutput("Requested stand height: " + String(intRequestedHeight));
      VerboseOutput("Current height: " + String(GetUltrasonicReading()));
    }

    // Only continue if 10 seconds have NOT passed AND the current height is < the standing height
    while((currentMillis - previousMillis < maxSeconds) && (GetUltrasonicReading() < intRequestedHeight))
    {
      // Update current time
      currentMillis = millis();

      // Raise the desk
      digitalWrite(pinRaise, LOW);
    }

    // Once loop conditions are not true, stop raising the desk
    digitalWrite(pinRaise, HIGH);

    if(blnVerbose)
    {
      VerboseOutput("Finished raising desk...");
      VerboseOutput("Current height: " + String(GetUltrasonicReading()));
      VerboseOutput("Elapsed time to raise: " + String(currentMillis - previousMillis));
    }
  }
  else
  {
    // Get the height we need to lower to
    intRequestedHeight = preferences.getUInt("intChairHeight", 1);

    if(blnVerbose)
    {
      VerboseOutput("Starting to lower desk...");
      VerboseOutput("Requested chair height: " + String(intRequestedHeight));
      VerboseOutput("Current height: " + String(GetUltrasonicReading()));
    }

    // Only continue if 10 seconds have NOT passed AND the current height is > the chair height
    while((currentMillis - previousMillis < maxSeconds) && (GetUltrasonicReading() > intRequestedHeight))
    {
      // Update current time
      currentMillis = millis();

      // Lower the desk
      digitalWrite(pinLower, LOW);
    }

    // Once loop conditions are not true, stop lowering the desk
    digitalWrite(pinLower, HIGH);

    if(blnVerbose)
    {
      VerboseOutput("Finished lowering desk...");
      VerboseOutput("Current height: " + String(GetUltrasonicReading()));
      VerboseOutput("Elapsed time to lower: " + String(currentMillis - previousMillis));
    }
  }
}

void SaveDeskHeight(bool blnStanding, int intHeight)
{
  /* 
    Desc:     Saves the calibrated values (chair or stand) to preferences library
    Params:   Boolean value for chair or stand, int value for the newly calibrated value
    Returns:  None
  */
  
  // Determine which value we're setting (chair or standing)
  if(blnStanding)
  {
    // Setting the standing value, get the chair value
    int intChairHeight = preferences.getUInt("intChairHeight", 1);

    // Only set the stand height if the new height is less than the chair height.
    if(intHeight > intChairHeight)
    {
      preferences.putUInt("intStandHeight", intHeight);
      if(blnVerbose) { VerboseOutput("Stand calibration saved! " + String(intHeight)); }
    }
    else
    {
      if(blnVerbose)
      {
        VerboseOutput("Stand calibration NOT saved!");
        VerboseOutput("Attempted stand value of " + String(intHeight) + " is less than chair value");
        VerboseOutput("Stored chair height: " + String(intChairHeight));
        VerboseOutput("Stored stand height: " + String(preferences.getUInt("intStandHeight", 150)));
      }
    }
  }
  else
  {
    // If setting the chair value, get the standing value
    int intStandHeight = preferences.getUInt("intStandHeight", 150);

    // Only set the chair height if the new height is greater than the standing height.
    if(intHeight < intStandHeight)
    {
      preferences.putUInt("intChairHeight", intHeight);
      if(blnVerbose) { VerboseOutput("Chair calibration saved! " + String(intHeight)); }
    }
    else
    {
      if(blnVerbose)
      {
        VerboseOutput("Chair calibration NOT saved!");
        VerboseOutput("Attempted chair value of " + String(intHeight) + " is greater than stand value");
        VerboseOutput("Stored chair height: " + String(preferences.getUInt("intChairHeight", 1)));
        VerboseOutput("Stored stand height: " + String(intStandHeight));
      }
    }
  }
}

void SaveTimeInterval(int intMinutes)
{
  /* 
    Desc:     Saves the user defined time interval (in minutes) to persistent storage
    Params:   Int of minutes to wait before auto change
    Returns:  None
  */
  
  preferences.putUInt("intTimeInterval", intMinutes);
  if(blnVerbose) { VerboseOutput("Time interval in minutes saved! " + String(intMinutes)); }
}

void CompareTime()
{
  /* 
    Desc:     Compare current time with last moved time. If the interval is >= the set interval time, then trigger a state change
    Params:   None
    Returns:  None
  */

  // Only continue if auto change is enabled
  if(blnAutoChange)
  {
    // Get current time
    unsigned long lngCurrentTime = millis();

    //Get saved interval time and convert to milliseconds
    unsigned long lngInterval = preferences.getUInt("intTimeInterval", 30) * 60000;

    int intABSchange = lngCurrentTime - lngLastChange;
    if(abs(intABSchange) >= lngInterval)
    {
      // Get current US reading and saved presets
      int intCurrentHeight = GetUltrasonicReading();
      int intStandHeight = preferences.getUInt("intStandHeight", 150);
      int intChairHeight = preferences.getUInt("intChairHeight", 1);

      if(blnVerbose)
      {
        VerboseOutput("Current US: " + String(intCurrentHeight) + 
        ", Saved stand: " + String(intStandHeight) + 
        ", Saved chair: " + String(intChairHeight));
      }

      int intABSstand = intCurrentHeight - intStandHeight;
      int intABSchair = intCurrentHeight - intChairHeight;
      // Determine if we're currently standing or sitting
      if(abs(intABSstand) > abs(intABSchair))
      {
        // Move the desk to the standing height
        SetDeskHeight(true);
      }
      else
      {
        // Move the desk to the chair height
        SetDeskHeight(false);
      }

      // Reset last change to the current time
      lngLastChange = lngCurrentTime;
    }
  }
}

void VerboseOutput(String strMessage)
{
  /* 
    Desc:     Outputs verbose message to usb serial and/or bluetooth serial
    Params:   String of ts message to send
    Returns:  None
  */
  
  SerialBT.println(strMessage);
  Serial.println(strMessage);
}