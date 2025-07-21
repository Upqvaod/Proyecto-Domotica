int sensorPin = 35;
int led1Pin = 34;
int led2Pin = 32;
void setup() 
{
  pinMode(sensorPin, INPUT); 
  pinMode(led1Pin, OUTPUT); 
  pinMode(led2Pin, OUTPUT); 
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  Serial.begin(9600);
}

void loop() 
{
  int sensorValue = analogRead(sensorPin);

  // Conversión a voltaje
  float voltage = sensorValue * (5.0 / 1023.0);
  // Conversión a temperatura en grados Celsius
  float temperatureC = voltage * 100;

  // Conversión a Fahrenheit
  //float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;

  // Muestra las temperaturas en el Serial Monitor
  Serial.print("Temperatura en °C: ");
  Serial.println(temperatureC);
  //Serial.print(" | Temperatura en °F: ");
  //Serial.println(temperatureF);


  if (temperatureC > 40) 
  {
    digitalWrite(led1Pin, HIGH);
    digitalWrite(led2Pin, LOW);
  } 
  else 
  {
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, HIGH);
  }
  delay(100); 
}


