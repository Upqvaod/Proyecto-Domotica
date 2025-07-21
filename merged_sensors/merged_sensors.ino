// ESP32 Smart Home (Domotics) Project with Web Interface
// Sensors: Ultrasonic, Temperature, Humidity + 9 Independent Smart LEDs

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

// Ultrasonic sensor pins
int TRIG = 18;		
int ECO = 19;		
int DURACION;
int DISTANCIA;

// Temperature sensor pins (ESP32 analog pins)
int temperatureSensorPin = 34;

// Humidity sensor pins
int humiditySensorPin = 35;

// 9 Smart LED pins for Home Automation
int led1 = 2;   // Living Room Light
int led2 = 4;   // Kitchen Light
int led3 = 5;   // Bedroom Light
int led4 = 12;  // Bathroom Light
int led5 = 13;  // Garden Light
int led6 = 14;  // Garage Light
int led7 = 15;  // Office Light
int led8 = 16;  // Hallway Light
int led9 = 17;  // Security Light

// Buzzer pin for alerts
int buzzerPin = 21;

// LED States (for smart home control)
bool ledStates[9] = {false, false, false, false, false, false, false, false, false};

// Timing variables
unsigned long previousMillis = 0;
unsigned long ultrasonicPreviousMillis = 0;
unsigned long alertPreviousMillis = 0;

// Sensor data variables
float currentTemperature = 0;
int currentHumidity = 0;
int currentDistance = 0;
bool motionDetected = false;
bool temperatureAlert = false;
bool humidityAlert = false;

// Smart home room names
String roomNames[9] = {
  "Living Room", "Kitchen", "Bedroom", "Bathroom", "Garden", 
  "Garage", "Office", "Hallway", "Security"
};

void setup()
{
  Serial.begin(115200);
  
  // Initialize all LED pins for smart home lighting
  pinMode(led1, OUTPUT);  
  pinMode(led2, OUTPUT);  
  pinMode(led3, OUTPUT);  
  pinMode(led4, OUTPUT);  
  pinMode(led5, OUTPUT);  
  pinMode(led6, OUTPUT);  
  pinMode(led7, OUTPUT);  
  pinMode(led8, OUTPUT);  
  pinMode(led9, OUTPUT);  
  
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
  server.on("/style.css", HTTP_GET, handleCSS);
  
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
    temperatureSensor();
    humiditySensor();
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

void temperatureSensor() 
{
  int sensorValue = analogRead(temperatureSensorPin);
  
  // ESP32 ADC conversion (12-bit, 3.3V reference)
  float voltage = sensorValue * (3.3 / 4095.0);
  float temperatureC = voltage * 100; // Assuming LM35 sensor
  currentTemperature = temperatureC;
  
  // Temperature alert if too hot (>35°C) or too cold (<10°C)
  temperatureAlert = (temperatureC > 35 || temperatureC < 10);

  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.print(" °C - Alert: ");
  Serial.println(temperatureAlert ? "YES" : "No");
}

void humiditySensor() 
{
  int sensorValue = analogRead(humiditySensorPin);
  currentHumidity = sensorValue;
  
  // Humidity alert if too high (>3000) - indicates water leak
  humidityAlert = (sensorValue > 3000);

  Serial.print("Humidity: ");
  Serial.print(sensorValue);
  Serial.print(" - Water Alert: ");
  Serial.println(humidityAlert ? "LEAK DETECTED!" : "Normal");
}

void handleAlerts() 
{
  // Sound buzzer for critical alerts
  if (temperatureAlert || humidityAlert) {
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    delay(100);
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
  }
  
  // Auto-turn on security light if motion detected at night
  // (You can add light sensor logic here)
  if (motionDetected) {
    ledStates[8] = true; // Security light (LED9)
    updateLEDState(8);
  }
}

void turnOffAllLeds() {
  for (int i = 0; i < 9; i++) {
    ledStates[i] = false;
  }
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  digitalWrite(led4, LOW);
  digitalWrite(led5, LOW);
  digitalWrite(led6, LOW);
  digitalWrite(led7, LOW);
  digitalWrite(led8, LOW);
  digitalWrite(led9, LOW);
}

void updateLEDState(int ledIndex) {
  int pins[9] = {led1, led2, led3, led4, led5, led6, led7, led8, led9};
  digitalWrite(pins[ledIndex], ledStates[ledIndex] ? HIGH : LOW);
}

void updateAllLEDs() {
  for (int i = 0; i < 9; i++) {
    updateLEDState(i);
  }
}

// Smart Home Web Server Functions
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>🏠 Smart Home Dashboard</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<link rel='stylesheet' type='text/css' href='/style.css'>";
  html += "<script>";
  html += "function updateData() {";
  html += "  fetch('/data')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      document.getElementById('temp').innerHTML = data.temperature.toFixed(1) + ' °C';";
  html += "      document.getElementById('humidity').innerHTML = data.humidity;";
  html += "      document.getElementById('distance').innerHTML = data.distance + ' cm';";
  html += "      document.getElementById('motion').innerHTML = data.motion ? '🚨 DETECTED' : '✅ Clear';";
  html += "      document.getElementById('tempAlert').innerHTML = data.tempAlert ? '🚨 ALERT' : '✅ Normal';";
  html += "      document.getElementById('humidAlert').innerHTML = data.humidAlert ? '💧 LEAK!' : '✅ Normal';";
  html += "      for(let i = 0; i < 9; i++) {";
  html += "        document.getElementById('led' + i).className = data.leds[i] ? 'led on' : 'led off';";
  html += "        document.getElementById('btn' + i).textContent = data.leds[i] ? 'OFF' : 'ON';";
  html += "      }";
  html += "    });";
  html += "}";
  html += "function toggleLED(id) {";
  html += "  fetch('/toggle?led=' + id).then(() => updateData());";
  html += "}";
  html += "function allOff() {";
  html += "  fetch('/alloff').then(() => updateData());";
  html += "}";
  html += "function allOn() {";
  html += "  fetch('/allon').then(() => updateData());";
  html += "}";
  html += "setInterval(updateData, 1000);";
  html += "window.onload = updateData;";
  html += "</script></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>🏠 Smart Home Control Center</h1>";
  
  // Sensor Dashboard
  html += "<div class='section'>";
  html += "<h2>📊 Environmental Monitoring</h2>";
  html += "<div class='sensor-grid'>";
  html += "<div class='sensor-card'>";
  html += "<h3>🌡️ Temperature</h3>";
  html += "<div class='value' id='temp'>Loading...</div>";
  html += "<div class='status' id='tempAlert'>Loading...</div>";
  html += "</div>";
  
  html += "<div class='sensor-card'>";
  html += "<h3>💧 Humidity/Water</h3>";
  html += "<div class='value' id='humidity'>Loading...</div>";
  html += "<div class='status' id='humidAlert'>Loading...</div>";
  html += "</div>";
  
  html += "<div class='sensor-card'>";
  html += "<h3>� Motion Sensor</h3>";
  html += "<div class='value' id='distance'>Loading...</div>";
  html += "<div class='status' id='motion'>Loading...</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  // Light Control
  html += "<div class='section'>";
  html += "<h2>💡 Smart Lighting Control</h2>";
  html += "<div class='control-buttons'>";
  html += "<button onclick='allOn()' class='btn-all on'>🔆 ALL ON</button>";
  html += "<button onclick='allOff()' class='btn-all off'>🌙 ALL OFF</button>";
  html += "</div>";
  
  html += "<div class='light-grid'>";
  for (int i = 0; i < 9; i++) {
    html += "<div class='light-card'>";
    html += "<div class='led off' id='led" + String(i) + "'></div>";
    html += "<h4>" + roomNames[i] + "</h4>";
    html += "<button onclick='toggleLED(" + String(i) + ")' class='btn-toggle' id='btn" + String(i) + "'>ON</button>";
    html += "</div>";
  }
  html += "</div>";
  html += "</div>";
  
  html += "<div class='footer'>";
  html += "<p>🔄 Auto-refresh | 🏡 ESP32 Smart Home System</p>";
  html += "</div>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleSensorData() {
  DynamicJsonDocument doc(1024);
  
  doc["temperature"] = currentTemperature;
  doc["humidity"] = currentHumidity;
  doc["distance"] = currentDistance;
  doc["motion"] = motionDetected;
  doc["tempAlert"] = temperatureAlert;
  doc["humidAlert"] = humidityAlert;
  
  JsonArray ledsArray = doc.createNestedArray("leds");
  for (int i = 0; i < 9; i++) {
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
    
    if (ledIndex >= 0 && ledIndex < 9) {
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
    
    if (ledIndex >= 0 && ledIndex < 9) {
      ledStates[ledIndex] = !ledStates[ledIndex];
      updateLEDState(ledIndex);
      Serial.println(roomNames[ledIndex] + " light " + (ledStates[ledIndex] ? "ON" : "OFF"));
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleAllOff() {
  for (int i = 0; i < 9; i++) {
    ledStates[i] = false;
    updateLEDState(i);
  }
  Serial.println("All lights turned OFF");
  server.send(200, "text/plain", "All OFF");
}

void handleAllOn() {
  for (int i = 0; i < 9; i++) {
    ledStates[i] = true;
    updateLEDState(i);
  }
  Serial.println("All lights turned ON");
  server.send(200, "text/plain", "All ON");
}

void handleCSS() {
  String css = "* { margin: 0; padding: 0; box-sizing: border-box; }";
  css += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%); color: white; min-height: 100vh; padding: 20px; }";
  css += ".container { max-width: 1200px; margin: 0 auto; }";
  css += "h1 { text-align: center; font-size: 2.5em; margin-bottom: 30px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }";
  css += ".section { background: rgba(255,255,255,0.1); backdrop-filter: blur(10px); border-radius: 20px; padding: 25px; margin-bottom: 25px; box-shadow: 0 8px 32px rgba(0,0,0,0.1); border: 1px solid rgba(255,255,255,0.2); }";
  css += ".section h2 { margin-bottom: 20px; color: #FFD700; text-align: center; }";
  
  // Sensor Grid
  css += ".sensor-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; }";
  css += ".sensor-card { background: rgba(255,255,255,0.1); border-radius: 15px; padding: 20px; text-align: center; border: 1px solid rgba(255,255,255,0.2); }";
  css += ".sensor-card h3 { margin-bottom: 10px; font-size: 1.2em; }";
  css += ".value { font-size: 1.8em; font-weight: bold; margin: 10px 0; color: #FFD700; }";
  css += ".status { font-size: 1em; margin-top: 10px; font-weight: bold; }";
  
  // Light Control
  css += ".control-buttons { text-align: center; margin-bottom: 25px; }";
  css += ".btn-all { padding: 12px 30px; margin: 0 10px; border: none; border-radius: 25px; font-size: 1.1em; font-weight: bold; cursor: pointer; transition: all 0.3s ease; }";
  css += ".btn-all.on { background: #4CAF50; color: white; }";
  css += ".btn-all.off { background: #f44336; color: white; }";
  css += ".btn-all:hover { transform: translateY(-2px); box-shadow: 0 5px 15px rgba(0,0,0,0.2); }";
  
  css += ".light-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px; }";
  css += ".light-card { background: rgba(255,255,255,0.1); border-radius: 15px; padding: 15px; text-align: center; border: 1px solid rgba(255,255,255,0.2); transition: transform 0.3s ease; }";
  css += ".light-card:hover { transform: translateY(-3px); }";
  css += ".light-card h4 { margin: 10px 0; font-size: 0.9em; }";
  
  // LED Styles
  css += ".led { width: 40px; height: 40px; border-radius: 50%; margin: 0 auto 10px; border: 3px solid #333; transition: all 0.4s ease; }";
  css += ".led.on { background: linear-gradient(45deg, #00ff00, #32cd32); box-shadow: 0 0 25px #00ff00, inset 0 0 15px #00cc00; animation: pulse 2s infinite; }";
  css += ".led.off { background: #333; box-shadow: inset 0 0 10px #111; }";
  
  css += ".btn-toggle { padding: 8px 20px; border: none; border-radius: 20px; background: #2196F3; color: white; font-weight: bold; cursor: pointer; transition: all 0.3s ease; width: 100%; }";
  css += ".btn-toggle:hover { background: #1976D2; transform: translateY(-1px); }";
  
  css += ".footer { text-align: center; margin-top: 30px; opacity: 0.8; font-size: 0.9em; }";
  
  // Animations
  css += "@keyframes pulse { 0% { box-shadow: 0 0 25px #00ff00, inset 0 0 15px #00cc00; } 50% { box-shadow: 0 0 35px #00ff00, inset 0 0 20px #00cc00; } 100% { box-shadow: 0 0 25px #00ff00, inset 0 0 15px #00cc00; } }";
  
  // Responsive Design
  css += "@media (max-width: 768px) { h1 { font-size: 2em; } .sensor-grid { grid-template-columns: 1fr; } .light-grid { grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); } .btn-all { padding: 10px 20px; margin: 5px; } }";
  
  server.send(200, "text/css", css);
}
