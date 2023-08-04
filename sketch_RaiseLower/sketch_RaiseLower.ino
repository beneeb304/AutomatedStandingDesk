int serialInput;
int pinRed = 22;
int pinYellow = 23;

void setup()
{
  Serial.begin(9600);
  pinMode(pinRed, OUTPUT);
  pinMode(pinYellow, OUTPUT);
  digitalWrite(pinRed, HIGH);
  digitalWrite(pinYellow, HIGH);
}

void loop()
{
  serialInput = Serial.read();
  if(serialInput == 'R')
  {
    Serial.println("Red Pin");
    digitalWrite(pinRed, LOW);
    delay(2000);
    digitalWrite(pinRed, HIGH);
  } 
  else if (serialInput == 'Y')
  {
    Serial.println("Yellow Pin");
    digitalWrite(pinYellow, LOW);
    delay(2000);
    digitalWrite(pinYellow, HIGH);
  }
}