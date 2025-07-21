int TRIG = 10;		
int ECO = 9;		
int LED = 3;	
int DURACION;
int DISTANCIA;
int led2 = 4;

void setup()
{
  pinMode(TRIG, OUTPUT);
  pinMode(ECO, INPUT);	
  pinMode(LED, OUTPUT);	
  pinMode(led2, OUTPUT);	
  Serial.begin(9600); 
}
void loop()
{
  led();

  int i = 0;
  for(i = 0; i < 10; i++)
  {
      ultrasonic();
  }
}

void ultrasonic()
{
  digitalWrite(TRIG, HIGH); 
  delay(1);	
  digitalWrite(TRIG, LOW);
  DURACION = pulseIn(ECO, HIGH);
  DISTANCIA = DURACION / 58.2;
  Serial.println(DISTANCIA);		
  delay(200);
  if (DISTANCIA > 0  && DISTANCIA < 20)
  {	
    digitalWrite(LED, HIGH);			
    delay(DISTANCIA * 10);
    digitalWrite(LED, LOW);			
  }
}

void led()
{
  digitalWrite(led2, HIGH);
  delay(5000);
  digitalWrite(led2, LOW);
}











