/*
Things to still add include:
- Calibrating time for setting raise/lower intervals
  - EEPROM or storage
- Ultrasonic sensor
*/

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
int strSerialInput;
int strBTSerialInput;
int pinRaise = 22;
int pinLower = 23;

void setup()
{
  // Initialize serial
  Serial.begin(115200);
  
  // Configure pins
  pinMode(pinRaise, OUTPUT);
  pinMode(pinLower, OUTPUT);
  digitalWrite(pinRaise, HIGH);
  digitalWrite(pinLower, HIGH);
  
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
    case 'R':
      Serial.println("Raise for 2 seconds...");
      digitalWrite(pinRaise, LOW);
      delay(2000);
      digitalWrite(pinRaise, HIGH);
      break;
    
    case 'L':
      Serial.println("Lower for 2 seconds...");
      digitalWrite(pinLower, LOW);
      delay(2000);
      digitalWrite(pinLower, HIGH);
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
    // Could check for calibration values here?
    break;
  }
  
  // Minor delay to avoid hiccups
  delay(20);
}