//Este sensor mide al revez
const int sensorPin = A34; 
int led1Pin = 35;
int led2Pin = 32;

void setup() 
{
  pinMode(sensorPin, INPUT);  
  pinMode(led1Pin, OUTPUT);   
  pinMode(led2Pin, OUTPUT);    

  // Inicialización
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);

  Serial.begin(115000); // Para monitoreo en el Serial Monitor
}

void loop() {
  // Lee el valor analógico del sensor
  int sensorValue = analogRead(sensorPin);

  // Muestra el valor en el Serial Monitor
  Serial.print("Valor del sensor: ");
  Serial.println(sensorValue);

  // Control de los LEDs según el umbral
  if (sensorValue > 500) 
  {
    digitalWrite(led1Pin, HIGH); 
    digitalWrite(led2Pin, LOW);  
  } 
  else 
  {
    digitalWrite(led1Pin, LOW);  
    digitalWrite(led2Pin, HIGH); 
  }

  delay(10); // Espera medio segundo antes de la próxima lectura
}
