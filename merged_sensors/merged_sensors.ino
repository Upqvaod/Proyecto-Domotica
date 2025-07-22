// ESP32 Smart Home (Domotics) Project with Web Interface
// Sensors: Ultrasonic, DHT11 (Temperature & Humidity) + 5 Independent Smart LEDs

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>

// DHT11 sensor setup
#define DHT_PIN 34
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// WiFi credentials
const char* ssid = "MEGACABLE-2.4G-1DA3";
const char* password = "Us3R0987654321@";

WebServer server(80);

// Ultrasonic sensor pins
int TRIG = 13;		
int ECO = 19;		
int DURACION;
int DISTANCIA;

// Humidity sensor pins (keeping for water leak detection)
int humiditySensorPin = 35;

// 5 Smart LED pins for Home Automation
int led1 = 15;   // Bathroom/W.C. Light
int led2 = 2;   // Bedroom 1 Light
int led3 = 4;   // Exterior/Garden Light
int led4 = 5;  // Living Room Light
int led5 = 18;  // Garden Light (moved from pin 18 to avoid conflict)

// Buzzer pin for alerts
int buzzerPin = 21;

// LED States (for smart home control)
bool ledStates[5] = {false, false, false, false, false};

// Timing variables
unsigned long previousMillis = 0;
unsigned long ultrasonicPreviousMillis = 0;
unsigned long alertPreviousMillis = 0;

// Sensor data variables
float currentTemperature = 0;
float currentHumidity = 0;
int currentWaterSensor = 0;  // For water leak detection
int currentDistance = 0;
bool motionDetected = false;
bool temperatureAlert = false;
bool humidityAlert = false;
bool waterLeakAlert = false;

// Smart home room names
String roomNames[5] = {
  "Living Room", "Kitchen", "Bedroom", "Bathroom", "Garden"
};

void setup()
{
  Serial.begin(115200);
  
  // Initialize DHT sensor
  dht.begin();
  
  // Initialize all LED pins for smart home lighting
  pinMode(led1, OUTPUT);  
  pinMode(led2, OUTPUT);  
  pinMode(led3, OUTPUT);  
  pinMode(led4, OUTPUT);  
  pinMode(led5, OUTPUT);  
  
  // Ultrasonic setup
  pinMode(TRIG, OUTPUT);
  pinMode(ECO, INPUT);
  
  // Buzzer setup for alerts
  pinMode(buzzerPin, OUTPUT);
  
  // Initialize all LEDs to OFF
  turnOffAllLeds();
  
  // Initialize WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("Smart Home System IP: ");
  Serial.println(WiFi.localIP());
  
  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleSensorData);
  server.on("/control", HTTP_POST, handleLEDControl);
  server.on("/toggle", HTTP_GET, handleToggleLED);
  server.on("/alloff", HTTP_GET, handleAllOff);
  server.on("/allon", HTTP_GET, handleAllOn);
  
  server.begin();
  Serial.println("Smart Home Web Server Started");
  Serial.println("Access your home dashboard at: http://" + WiFi.localIP().toString());
}

void loop()
{
  unsigned long currentMillis = millis();
  
  // Handle web server requests
  server.handleClient();
  
  // Read ultrasonic sensor every 500ms
  if (currentMillis - ultrasonicPreviousMillis >= 500) {
    ultrasonicPreviousMillis = currentMillis;
    ultrasonicSensor();
  }
  
  // Read temperature and humidity sensors every 2000ms
  if (currentMillis - previousMillis >= 2000) {
    previousMillis = currentMillis;
    readDHT11Sensor();
    waterLeakSensor();
  }
  
  // Handle alerts every 3000ms
  if (currentMillis - alertPreviousMillis >= 3000) {
    alertPreviousMillis = currentMillis;
    handleAlerts();
  }
}

void ultrasonicSensor()
{
  digitalWrite(TRIG, HIGH); 
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  DURACION = pulseIn(ECO, HIGH);
  DISTANCIA = DURACION / 58.2;
  currentDistance = DISTANCIA;
  
  // Motion detection (object within 30cm)
  motionDetected = (DISTANCIA > 0 && DISTANCIA < 30);
  
  Serial.print("Distance: ");
  Serial.print(DISTANCIA);
  Serial.print(" cm - Motion: ");
  Serial.println(motionDetected ? "DETECTED" : "None");
}

void readDHT11Sensor() 
{
  // Read temperature and humidity from DHT11
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  // Check if readings are valid
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT11 sensor!");
    return;
  }
  
  currentTemperature = temperature;
  currentHumidity = humidity;
  
  // Temperature alert if too hot (>35°C) or too cold (<10°C)
  temperatureAlert = (temperature > 35 || temperature < 10);
  
  // Humidity alert if too high (>85%) or too low (<30%)
  humidityAlert = (humidity > 85 || humidity < 30);

  Serial.print("DHT11 - Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.print(" % - Temp Alert: ");
  Serial.print(temperatureAlert ? "YES" : "No");
  Serial.print(", Humidity Alert: ");
  Serial.println(humidityAlert ? "YES" : "No");
}

void waterLeakSensor() 
{
  int sensorValue = analogRead(humiditySensorPin);
  currentWaterSensor = sensorValue;
  
  // Water leak alert if too high (>3000) - indicates water presence
  waterLeakAlert = (sensorValue > 3000);

  Serial.print("Water Sensor: ");
  Serial.print(sensorValue);
  Serial.print(" - Water Leak Alert: ");
  Serial.println(waterLeakAlert ? "LEAK DETECTED!" : "Normal");
}

void handleAlerts() 
{
  // Sound buzzer for critical alerts
  if (temperatureAlert || humidityAlert || waterLeakAlert) {
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    delay(100);
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
  }
  
  // Auto-turn on garden light if motion detected at night
  // (You can add light sensor logic here)
  if (motionDetected) {
    ledStates[4] = true; // Garden light (LED5)
    updateLEDState(4);
  }
}

void turnOffAllLeds() {
  for (int i = 0; i < 5; i++) {
    ledStates[i] = false;
  }
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  digitalWrite(led4, LOW);
  digitalWrite(led5, LOW);
}

void updateLEDState(int ledIndex) {
  int pins[5] = {led1, led2, led3, led4, led5};
  digitalWrite(pins[ledIndex], ledStates[ledIndex] ? HIGH : LOW);
}

void updateAllLEDs() {
  for (int i = 0; i < 5; i++) {
    updateLEDState(i);
  }
}

// Smart Home Web Server Functions
void handleRoot() {
  // Send the external HTML file
  String html = readHTMLFile();
  server.send(200, "text/html", html);
}

String readHTMLFile() {
  // Since ESP32 doesn't have filesystem by default, we'll include a simplified version
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Smart Home Dashboard</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { font-family: 'Segoe UI', sans-serif; background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%); color: white; min-height: 100vh; padding: 20px; }";
  html += ".container { max-width: 1200px; margin: 0 auto; }";
  html += "h1 { text-align: center; font-size: 2.5em; margin-bottom: 30px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }";
  html += ".section { background: rgba(255,255,255,0.1); backdrop-filter: blur(10px); border-radius: 20px; padding: 25px; margin-bottom: 25px; box-shadow: 0 8px 32px rgba(0,0,0,0.1); border: 1px solid rgba(255,255,255,0.2); }";
  html += ".section h2 { margin-bottom: 20px; color: #FFD700; text-align: center; }";
  html += ".sensor-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; }";
  html += ".sensor-card { background: rgba(255,255,255,0.1); border-radius: 15px; padding: 20px; text-align: center; border: 1px solid rgba(255,255,255,0.2); }";
  html += ".sensor-card h3 { margin-bottom: 10px; font-size: 1.2em; }";
  html += ".value { font-size: 1.8em; font-weight: bold; margin: 10px 0; color: #FFD700; }";
  html += ".status { font-size: 1em; margin-top: 10px; font-weight: bold; }";
  html += ".control-buttons { text-align: center; margin-bottom: 25px; }";
  html += ".btn-all { padding: 12px 30px; margin: 0 10px; border: none; border-radius: 25px; font-size: 1.1em; font-weight: bold; cursor: pointer; transition: all 0.3s ease; }";
  html += ".btn-all.on { background: #4CAF50; color: white; }";
  html += ".btn-all.off { background: #f44336; color: white; }";
  html += ".btn-all:hover { transform: translateY(-2px); box-shadow: 0 5px 15px rgba(0,0,0,0.2); }";
  html += ".light-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px; }";
  html += ".light-card { background: rgba(255,255,255,0.1); border-radius: 15px; padding: 15px; text-align: center; border: 1px solid rgba(255,255,255,0.2); transition: transform 0.3s ease; }";
  html += ".light-card:hover { transform: translateY(-3px); }";
  html += ".light-card h4 { margin: 10px 0; font-size: 0.9em; }";
  html += ".led { width: 40px; height: 40px; border-radius: 50%; margin: 0 auto 10px; border: 3px solid #333; transition: all 0.4s ease; }";
  html += ".led.on { background: linear-gradient(45deg, #00ff00, #32cd32); box-shadow: 0 0 25px #00ff00, inset 0 0 15px #00cc00; animation: pulse 2s infinite; }";
  html += ".led.off { background: #333; box-shadow: inset 0 0 10px #111; }";
  html += ".btn-toggle { padding: 8px 20px; border: none; border-radius: 20px; background: #2196F3; color: white; font-weight: bold; cursor: pointer; transition: all 0.3s ease; width: 100%; }";
  html += ".btn-toggle:hover { background: #1976D2; transform: translateY(-1px); }";
  html += ".footer { text-align: center; margin-top: 30px; opacity: 0.8; font-size: 0.9em; }";
  html += "@keyframes pulse { 0% { box-shadow: 0 0 25px #00ff00, inset 0 0 15px #00cc00; } 50% { box-shadow: 0 0 35px #00ff00, inset 0 0 20px #00cc00; } 100% { box-shadow: 0 0 25px #00ff00, inset 0 0 15px #00cc00; } }";
  html += "@media (max-width: 768px) { h1 { font-size: 2em; } .sensor-grid { grid-template-columns: 1fr; } .light-grid { grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); } .btn-all { padding: 10px 20px; margin: 5px; } }";
  html += "</style>";
  html += "<script>";
  html += "const roomNames = ['Living Room', 'Kitchen', 'Bedroom', 'Bathroom', 'Garden'];";
  html += "function initializeLightGrid() {";
  html += "  const lightGrid = document.getElementById('lightGrid');";
  html += "  lightGrid.innerHTML = '';";
  html += "  for(let i = 0; i < 5; i++) {";
  html += "    const lightCard = document.createElement('div');";
  html += "    lightCard.className = 'light-card';";
  html += "    lightCard.innerHTML = `<div class='led off' id='led${i}'></div><h4>${roomNames[i]}</h4><button onclick='toggleLED(${i})' class='btn-toggle' id='btn${i}'>ON</button>`;";
  html += "    lightGrid.appendChild(lightCard);";
  html += "  }";
  html += "}";
  html += "function updateData() {";
  html += "  fetch('/data').then(response => response.json()).then(data => {";
  html += "    document.getElementById('temp').innerHTML = data.temperature.toFixed(1) + ' &deg;C';";
  html += "    document.getElementById('humidity').innerHTML = data.humidity.toFixed(1) + ' %';";
  html += "    document.getElementById('waterSensor').innerHTML = data.waterSensor;";
  html += "    document.getElementById('distance').innerHTML = data.distance + ' cm';";
  html += "    document.getElementById('motion').innerHTML = data.motion ? 'DETECTED' : 'Clear';";
  html += "    document.getElementById('tempAlert').innerHTML = data.tempAlert ? 'ALERT' : 'Normal';";
  html += "    document.getElementById('humidAlert').innerHTML = data.humidAlert ? 'ALERT' : 'Normal';";
  html += "    document.getElementById('waterLeakAlert').innerHTML = data.waterLeakAlert ? 'LEAK!' : 'Normal';";
  html += "    for(let i = 0; i < 5; i++) {";
  html += "      document.getElementById('led' + i).className = data.leds[i] ? 'led on' : 'led off';";
  html += "      document.getElementById('btn' + i).textContent = data.leds[i] ? 'OFF' : 'ON';";
  html += "    }";
  html += "  }).catch(error => console.error('Error:', error));";
  html += "}";
  html += "function toggleLED(id) { fetch('/toggle?led=' + id).then(() => updateData()); }";
  html += "function allOff() { fetch('/alloff').then(() => updateData()); }";
  html += "function allOn() { fetch('/allon').then(() => updateData()); }";
  html += "window.onload = function() { initializeLightGrid(); updateData(); setInterval(updateData, 1000); };";
  html += "</script></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>Smart Home Control Center</h1>";
  
  // Sensor Dashboard
  html += "<div class='section'>";
  html += "<h2>Environmental Monitoring</h2>";
  html += "<div class='sensor-grid'>";
  html += "<div class='sensor-card'><h3>Temperature (DHT11)</h3><div class='value' id='temp'>Loading...</div><div class='status' id='tempAlert'>Loading...</div></div>";
  html += "<div class='sensor-card'><h3>Humidity (DHT11)</h3><div class='value' id='humidity'>Loading...</div><div class='status' id='humidAlert'>Loading...</div></div>";
  html += "<div class='sensor-card'><h3>Water Leak Sensor</h3><div class='value' id='waterSensor'>Loading...</div><div class='status' id='waterLeakAlert'>Loading...</div></div>";
  html += "<div class='sensor-card'><h3>Motion Sensor</h3><div class='value' id='distance'>Loading...</div><div class='status' id='motion'>Loading...</div></div>";
  html += "</div></div>";
  
  // Light Control
  html += "<div class='section'>";
  html += "<h2>Smart Lighting Control</h2>";
  html += "<div class='control-buttons'>";
  html += "<button onclick='allOn()' class='btn-all on'>ALL ON</button>";
  html += "<button onclick='allOff()' class='btn-all off'>ALL OFF</button>";
  html += "</div>";
  html += "<div class='light-grid' id='lightGrid'></div>";
  html += "</div>";
  
  html += "<div class='footer'><p>Auto-refresh | ESP32 Smart Home System</p></div>";
  html += "</div></body></html>";
  
  return html;
}

void handleSensorData() {
  DynamicJsonDocument doc(1024);
  
  doc["temperature"] = currentTemperature;
  doc["humidity"] = currentHumidity;
  doc["waterSensor"] = currentWaterSensor;
  doc["distance"] = currentDistance;
  doc["motion"] = motionDetected;
  doc["tempAlert"] = temperatureAlert;
  doc["humidAlert"] = humidityAlert;
  doc["waterLeakAlert"] = waterLeakAlert;
  
  JsonArray ledsArray = doc.createNestedArray("leds");
  for (int i = 0; i < 5; i++) {
    ledsArray.add(ledStates[i]);
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  server.send(200, "application/json", jsonString);
}

void handleLEDControl() {
  if (server.hasArg("led") && server.hasArg("state")) {
    int ledIndex = server.arg("led").toInt();
    bool state = server.arg("state") == "1";
    
    if (ledIndex >= 0 && ledIndex < 5) {
      ledStates[ledIndex] = state;
      updateLEDState(ledIndex);
      Serial.println("LED " + String(ledIndex) + " (" + roomNames[ledIndex] + ") set to " + (state ? "ON" : "OFF"));
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleToggleLED() {
  if (server.hasArg("led")) {
    int ledIndex = server.arg("led").toInt();
    
    if (ledIndex >= 0 && ledIndex < 5) {
      ledStates[ledIndex] = !ledStates[ledIndex];
      updateLEDState(ledIndex);
      Serial.println(roomNames[ledIndex] + " light " + (ledStates[ledIndex] ? "ON" : "OFF"));
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleAllOff() {
  for (int i = 0; i < 5; i++) {
    ledStates[i] = false;
    updateLEDState(i);
  }
  Serial.println("All lights turned OFF");
  server.send(200, "text/plain", "All OFF");
}

void handleAllOn() {
  for (int i = 0; i < 5; i++) {
    ledStates[i] = true;
    updateLEDState(i);
  }
  Serial.println("All lights turned ON");
  server.send(200, "text/plain", "All ON");
}
