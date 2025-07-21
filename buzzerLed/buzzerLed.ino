int ledPIN = 2;
int ledPIN2 = 3;
int bus = 4;


void setup() 
{
 pinMode(ledPIN, OUTPUT); //definir pin como salida
 pinMode(ledPIN2, OUTPUT); //definir pin como salida
 pinMode(bus, OUTPUT); 
 
 Serial.begin(9600); //iniciar puerto serie 

}

void loop() 
{
  digitalWrite(ledPIN , HIGH); 
  delay(50);

  digitalWrite(bus , HIGH); 
  delay(50);
  digitalWrite(bus, LOW);  
  delay(50);
  
  digitalWrite(ledPIN, LOW);  
  delay(50);
  
  digitalWrite(ledPIN2 , HIGH); 
  delay(50);
  
 digitalWrite(bus , HIGH); 
  delay(50);
  digitalWrite(bus, LOW);  
  delay(50);

  digitalWrite(ledPIN2, LOW);  
  delay(50);
  

  
  
  
  
}
