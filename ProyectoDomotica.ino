#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Configuración WiFi
const char* ssid = "MEGACABLE-2.4G-1DA3";          // Cambia por tu red WiFi
const char* password = "Us3R0987654321@";    // Cambia por tu contraseña

// Sensor de humedad del suelo
const int pinSensor = 34; // GPIO 34 para leer señales analógicas

// Sensor DHT11 (temperatura y humedad ambiental)
#define DHTPIN 15       // Pin donde está conectado el DHT11
#define DHTTYPE DHT11   // Tipo de sensor: DHT11
DHT dht(DHTPIN, DHTTYPE);

// Variables para DHT11
float temperature = 0.0;
float humidity = 0.0;

// Servomotor (cerradura)
Servo servoLock;
const int pinServo = 19; // GPIO 19 para el servomotor
bool lockState = false;   // false = cerrado (0°), true = abierto (90°)

// Sensor ultrasónico
const int TRIG_PIN = 2;   // Pin TRIG del sensor ultrasónico
const int ECHO_PIN = 5;   // Pin ECHO del sensor ultrasónico
long duration;
int distance;

// LEDs para iluminación de habitaciones
const int LED_BEDROOM = 18;     // LED Dormitorio
const int LED_BATHROOM = 4;     // LED Baño (cambiado de 16 a 4)
const int LED_LIVING1 = 21;     // LED Sala 1 (cambiado de 17 a 21)
const int LED_LIVING2 = 25;     // LED Sala 2
const int LED_EXTERIOR = 26;    // LED Exterior/Jardín
const int BUZZER = 27;          // Pin del buzzer

// Variables de control de luces
bool bedroomLight = false;
bool bathroomLight = false;
bool livingRoom1Light = false;
bool livingRoom2Light = false;
bool exteriorLight = false;

// Variables de control del sensor
bool proximityAlert = false;
unsigned long lastProximityCheck = 0;

// Servidor web
WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println("Sistema de humedad, cerradura y proximidad iniciando...");
  
  // Inicializar sensor DHT11
  dht.begin();
  Serial.println("Sensor DHT11 inicializado");
  
  // Configurar servomotor
  servoLock.attach(pinServo);
  servoLock.write(0); // Inicialmente cerrado
  Serial.println("Cerradura cerrada (0 grados)");
  
  // Configurar sensor ultrasónico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Configurar LEDs de habitaciones
  pinMode(LED_BEDROOM, OUTPUT);
  pinMode(LED_BATHROOM, OUTPUT);
  pinMode(LED_LIVING1, OUTPUT);
  pinMode(LED_LIVING2, OUTPUT);
  pinMode(LED_EXTERIOR, OUTPUT);
  
  // Configurar buzzer
  pinMode(BUZZER, OUTPUT);
  
  // Inicializar todas las luces apagadas
  digitalWrite(LED_BEDROOM, LOW);
  digitalWrite(LED_BATHROOM, LOW);
  digitalWrite(LED_LIVING1, LOW);
  digitalWrite(LED_LIVING2, LOW);
  digitalWrite(LED_EXTERIOR, LOW);
  digitalWrite(BUZZER, LOW);
  
  Serial.println("Sistema de iluminación y buzzer configurados");
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Configurar rutas del servidor web
  server.on("/", handleRoot);
  server.on("/unlock", handleUnlock);
  server.on("/lock", handleLock);
  server.on("/status", handleStatus);
  server.on("/toggle_bedroom", toggleBedroom);
  server.on("/toggle_bathroom", toggleBathroom);
  server.on("/toggle_living1", toggleLiving1);
  server.on("/toggle_living2", toggleLiving2);
  server.on("/toggle_exterior", toggleExterior);
  server.on("/all_lights_on", allLightsOn);
  server.on("/all_lights_off", allLightsOff);
  
  server.begin();
  Serial.println("Servidor web iniciado");
}

void loop() {
  // Manejar cliente web
  server.handleClient();
  
  // Leer sensor de humedad del suelo cada 5 segundos
  static unsigned long lastSoilHumidityRead = 0;
  if (millis() - lastSoilHumidityRead > 5000) {
    readSoilHumiditySensor();
    lastSoilHumidityRead = millis();
  }
  
  // Leer sensor DHT11 (temperatura y humedad ambiental) cada 3 segundos
  static unsigned long lastDHTRead = 0;
  if (millis() - lastDHTRead > 3000) {
    readDHT11Sensor();
    lastDHTRead = millis();
  }
  
  // Verificar sensor ultrasónico cada 500ms
  if (millis() - lastProximityCheck > 500) {
    checkProximity();
    lastProximityCheck = millis();
  }
  
  delay(50); // Pequeña pausa para no saturar el procesador
}

void readSoilHumiditySensor() {
  int valor = analogRead(pinSensor); // Leer humedad del suelo (0 a 4095)
  Serial.print("Valor del sensor de humedad del suelo: ");
  Serial.println(valor);

  // Interpretación del nivel de humedad del suelo
  if (valor > 3000) {
    Serial.println("La tierra esta muy mojada!");
  } else if (valor > 2000) {
    Serial.println("La tierra esta humeda.");
  } else {
    Serial.println("La tierra esta seca!");
  }
}

void readDHT11Sensor() {
  // Leer temperatura y humedad ambiental
  float newHumidity = dht.readHumidity();
  float newTemperature = dht.readTemperature(); // Celsius por defecto

  // Verificar si la lectura fue exitosa
  if (isnan(newHumidity) || isnan(newTemperature)) {
    Serial.println("Error al leer del sensor DHT11");
    return;
  }

  // Actualizar variables globales
  humidity = newHumidity;
  temperature = newTemperature;

  // Imprimir resultados
  Serial.print("Humedad ambiental: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.println(" C");
}

void checkProximity() {
  // Disparar pulso ultrasónico
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Leer duración del eco
  duration = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout de 30ms
  
  if (duration == 0) {
    Serial.println("Error: No se recibió eco del sensor ultrasónico");
    distance = 999; // Distancia muy alta para indicar error
    return;
  }
  
  // Calcular distancia en cm
  distance = duration / 58.2;
  
  Serial.print("Distancia: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  // Activar alerta si hay proximidad (menos de 15cm)
  if (distance > 0 && distance <= 15) {
    proximityAlert = true;
    activateBuzzer();
    Serial.println("ALERTA DE PROXIMIDAD!");
  } else {
    proximityAlert = false;
    digitalWrite(BUZZER, LOW); // Apagar buzzer
  }
}

void activateBuzzer() {
  // Patrón de buzzer intermitente cuando hay proximidad
  static unsigned long lastBuzzerToggle = 0;
  static bool buzzerState = false;
  
  if (millis() - lastBuzzerToggle > 200) { // Cambiar cada 200ms
    buzzerState = !buzzerState;
    digitalWrite(BUZZER, buzzerState);
    lastBuzzerToggle = millis();
  }
}

// Funciones para control de iluminación individual
void toggleBedroom() {
  bedroomLight = !bedroomLight;
  digitalWrite(LED_BEDROOM, bedroomLight ? HIGH : LOW);
  Serial.println("Luz dormitorio: " + String(bedroomLight ? "ENCENDIDA" : "APAGADA"));
  redirectToHome();
}

void toggleBathroom() {
  bathroomLight = !bathroomLight;
  digitalWrite(LED_BATHROOM, bathroomLight ? HIGH : LOW);
  Serial.println("Luz bano: " + String(bathroomLight ? "ENCENDIDA" : "APAGADA"));
  redirectToHome();
}

void toggleLiving1() {
  livingRoom1Light = !livingRoom1Light;
  digitalWrite(LED_LIVING1, livingRoom1Light ? HIGH : LOW);
  Serial.println("Luz sala 1: " + String(livingRoom1Light ? "ENCENDIDA" : "APAGADA"));
  redirectToHome();
}

void toggleLiving2() {
  livingRoom2Light = !livingRoom2Light;
  digitalWrite(LED_LIVING2, livingRoom2Light ? HIGH : LOW);
  Serial.println("Luz sala 2: " + String(livingRoom2Light ? "ENCENDIDA" : "APAGADA"));
  redirectToHome();
}

void toggleExterior() {
  exteriorLight = !exteriorLight;
  digitalWrite(LED_EXTERIOR, exteriorLight ? HIGH : LOW);
  Serial.println("Luz exterior: " + String(exteriorLight ? "ENCENDIDA" : "APAGADA"));
  redirectToHome();
}

void allLightsOn() {
  bedroomLight = true;
  bathroomLight = true;
  livingRoom1Light = true;
  livingRoom2Light = true;
  exteriorLight = true;
  
  updateAllLightsHardware();
  
  Serial.println("TODAS LAS LUCES ENCENDIDAS");
  redirectToHome();
}

void allLightsOff() {
  bedroomLight = false;
  bathroomLight = false;
  livingRoom1Light = false;
  livingRoom2Light = false;
  exteriorLight = false;
  
  updateAllLightsHardware();
  
  Serial.println("TODAS LAS LUCES APAGADAS");
  redirectToHome();
}

// Helper function to update all LED hardware states
void updateAllLightsHardware() {
  digitalWrite(LED_BEDROOM, bedroomLight ? LOW : HIGH);
  digitalWrite(LED_BATHROOM, bathroomLight ? LOW : HIGH);
  digitalWrite(LED_LIVING1, livingRoom1Light ? LOW : HIGH);
  digitalWrite(LED_LIVING2, livingRoom2Light ? LOW : HIGH);
  digitalWrite(LED_EXTERIOR, exteriorLight ? LOW : HIGH);
}

// Helper function for consistent redirects
void redirectToHome() {
  server.sendHeader("Location", "/");
  server.send(302);
}

void handleRoot() {
  String html = generateHTMLPage();
  server.send(200, "text/html", html);
}

String generateHTMLPage() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Sistema Domotico ESP32</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta charset='UTF-8'>";
  html += generateCSS();
  html += generateJavaScript();
  html += "</head><body>";
  
  html += "<div class='main-container'>";
  html += "<header class='header'>";
  html += "<h1 class='title'>Sistema Domotico ESP32</h1>";
  html += "<div class='subtitle'>Control Inteligente del Hogar</div>";
  html += "</header>";
  
  html += "<div id='status-panel' class='status-panel'>";
  html += getStatusHTML();
  html += "</div>";
  
  html += generateControlPanels();
  
  html += "<footer class='footer'>";
  html += "<p>ESP32 Home Automation System | Actualizacion automatica cada 2s</p>";
  html += "</footer>";
  html += "</div>";
  
  html += "</body></html>";
  return html;
}

void handleUnlock() {
  lockState = true;
  servoLock.write(90); // Abrir cerradura (90 grados)
  Serial.println("Cerradura DESBLOQUEADA (90 grados)");
  redirectToHome();
}

void handleLock() {
  lockState = false;
  servoLock.write(0); // Cerrar cerradura (0 grados)
  Serial.println("Cerradura BLOQUEADA (0 grados)");
  redirectToHome();
}

String generateJavaScript() {
  String js = "<script>";
  js += "function updateStatus() {";
  js += "  fetch('/status').then(response => response.text()).then(data => {";
  js += "    document.getElementById('status-panel').innerHTML = data;";
  js += "  }).catch(err => console.error('Update failed:', err));";
  js += "}";
  js += "setInterval(updateStatus, 2000);";
  js += "document.addEventListener('DOMContentLoaded', function() {";
  js += "  console.log('ESP32 Home Automation System Loaded');";
  js += "});";
  js += "</script>";
  return js;
}

String generateCSS() {
  String css = "<style>";
  
  // Reset and base styles
  css += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  css += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; color: #333; }";
  
  // Main container
  css += ".main-container { max-width: 1200px; margin: 0 auto; padding: 20px; }";
  
  // Header styles
  css += ".header { text-align: center; margin-bottom: 30px; background: rgba(255,255,255,0.95); padding: 25px; border-radius: 15px; box-shadow: 0 8px 32px rgba(0,0,0,0.1); }";
  css += ".title { font-size: 2.5em; color: #2c3e50; margin-bottom: 10px; font-weight: 300; }";
  css += ".subtitle { font-size: 1.1em; color: #7f8c8d; font-weight: 400; }";
  
  // Status panel
  css += ".status-panel { background: rgba(255,255,255,0.95); border-radius: 15px; padding: 25px; margin-bottom: 25px; box-shadow: 0 8px 32px rgba(0,0,0,0.1); }";
  
  // Control panels
  css += ".control-section { background: rgba(255,255,255,0.95); border-radius: 15px; padding: 25px; margin-bottom: 20px; box-shadow: 0 8px 32px rgba(0,0,0,0.1); }";
  css += ".section-title { font-size: 1.5em; color: #2c3e50; margin-bottom: 20px; text-align: center; font-weight: 400; }";
  
  // Button styles
  css += ".btn { display: inline-block; padding: 12px 24px; margin: 8px; border: none; border-radius: 8px; font-size: 1em; font-weight: 500; text-decoration: none; cursor: pointer; transition: all 0.3s ease; text-align: center; min-width: 120px; }";
  css += ".btn:hover { transform: translateY(-2px); box-shadow: 0 6px 20px rgba(0,0,0,0.15); }";
  css += ".btn:active { transform: translateY(0); }";
  
  // Button variants
  css += ".btn-primary { background: linear-gradient(45deg, #3498db, #2980b9); color: white; }";
  css += ".btn-success { background: linear-gradient(45deg, #2ecc71, #27ae60); color: white; }";
  css += ".btn-danger { background: linear-gradient(45deg, #e74c3c, #c0392b); color: white; }";
  css += ".btn-warning { background: linear-gradient(45deg, #f39c12, #e67e22); color: white; }";
  css += ".btn-secondary { background: linear-gradient(45deg, #95a5a6, #7f8c8d); color: white; }";
  css += ".btn-purple { background: linear-gradient(45deg, #9b59b6, #8e44ad); color: white; }";
  css += ".btn-info { background: linear-gradient(45deg, #3498db, #2980b9); color: white; }";
  css += ".btn-dark { background: linear-gradient(45deg, #34495e, #2c3e50); color: white; }";
  
  // Grid layouts
  css += ".room-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }";
  css += ".master-controls { display: flex; gap: 15px; justify-content: center; flex-wrap: wrap; margin-top: 20px; }";
  css += ".lock-controls { display: flex; gap: 15px; justify-content: center; flex-wrap: wrap; }";
  
  // Status cards
  css += ".status-card { background: #f8f9fa; border-radius: 10px; padding: 20px; margin: 10px 0; border-left: 4px solid #3498db; }";
  css += ".status-card.locked { border-left-color: #e74c3c; background: #fdf2f2; }";
  css += ".status-card.unlocked { border-left-color: #2ecc71; background: #f2fdf4; }";
  css += ".status-card.alert { border-left-color: #e74c3c; background: #fdf2f2; animation: pulse 1.5s infinite; }";
  css += ".status-card.soil { border-left-color: #8e44ad; }";
  css += ".status-card.dht { border-left-color: #f39c12; }";
  css += ".status-card.proximity { border-left-color: #e67e22; }";
  css += ".status-card.lights { border-left-color: #2ecc71; }";
  
  // Status text
  css += ".status-title { font-size: 1.2em; font-weight: 600; margin-bottom: 10px; color: #2c3e50; }";
  css += ".status-value { font-size: 1em; color: #7f8c8d; line-height: 1.6; }";
  css += ".status-on { color: #27ae60; font-weight: 600; }";
  css += ".status-off { color: #e74c3c; font-weight: 600; }";
  
  // Light grid for status
  css += ".lights-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin: 15px 0; }";
  css += ".light-item { display: flex; justify-content: space-between; align-items: center; padding: 8px 12px; background: #ecf0f1; border-radius: 6px; }";
  
  // Responsive design
  css += "@media (max-width: 768px) {";
  css += "  .main-container { padding: 10px; }";
  css += "  .title { font-size: 2em; }";
  css += "  .room-grid { grid-template-columns: 1fr 1fr; }";
  css += "  .master-controls { flex-direction: column; align-items: center; }";
  css += "  .lock-controls { flex-direction: column; align-items: center; }";
  css += "  .btn { min-width: 100px; }";
  css += "}";
  
  // Animations
  css += "@keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.7; } 100% { opacity: 1; } }";
  css += "@keyframes fadeIn { from { opacity: 0; transform: translateY(20px); } to { opacity: 1; transform: translateY(0); } }";
  css += ".status-panel, .control-section { animation: fadeIn 0.6s ease-out; }";
  
  // Footer
  css += ".footer { text-align: center; padding: 20px; color: rgba(255,255,255,0.8); font-size: 0.9em; }";
  
  css += "</style>";
  return css;
}

String generateControlPanels() {
  String html = "";
  
  // Security Control Panel
  html += "<div class='control-section'>";
  html += "<h2 class='section-title'>Control de Seguridad</h2>";
  html += "<div class='lock-controls'>";
  html += "<a href='/unlock' class='btn btn-success'>DESBLOQUEAR</a>";
  html += "<a href='/lock' class='btn btn-danger'>BLOQUEAR</a>";
  html += "</div>";
  html += "</div>";
  
  // Lighting Control Panel
  html += "<div class='control-section'>";
  html += "<h2 class='section-title'>Control de Iluminacion</h2>";
  html += "<div class='room-grid'>";
  html += "<a href='/toggle_bedroom' class='btn btn-purple'>DORMITORIO</a>";
  html += "<a href='/toggle_bathroom' class='btn btn-info'>BANO</a>";
  html += "<a href='/toggle_living1' class='btn btn-warning'>SALA 1</a>";
  html += "<a href='/toggle_living2' class='btn btn-warning'>SALA 2</a>";
  html += "</div>";
  html += "<a href='/toggle_exterior' class='btn btn-success' style='width: 100%; margin: 15px 0;'>EXTERIOR/JARDIN</a>";
  html += "<div class='master-controls'>";
  html += "<a href='/all_lights_on' class='btn btn-primary'>ENCENDER TODAS</a>";
  html += "<a href='/all_lights_off' class='btn btn-dark'>APAGAR TODAS</a>";
  html += "</div>";
  html += "</div>";
  
  return html;
}

void handleStatus() {
  server.send(200, "text/html", getStatusHTML());
}

String getStatusHTML() {
  String html = "";
  
  // Security Status Card
  String lockClass = lockState ? "status-card unlocked" : "status-card locked";
  html += "<div class='" + lockClass + "'>";
  html += "<div class='status-title'>Estado de Seguridad</div>";
  html += "<div class='status-value'>";
  html += "Cerradura: " + String(lockState ? "<span class='status-on'>DESBLOQUEADA</span>" : "<span class='status-off'>BLOQUEADA</span>");
  html += "</div></div>";
  
  // Soil Humidity Status Card
  int soilValue = analogRead(pinSensor);
  html += "<div class='status-card soil'>";
  html += "<div class='status-title'>Sensor de Humedad del Suelo</div>";
  html += "<div class='status-value'>";
  html += "Lectura: " + String(soilValue) + " / 4095<br>";
  if (soilValue > 3000) {
    html += "Estado: <span class='status-on'>Muy mojada</span>";
  } else if (soilValue > 2000) {
    html += "Estado: <span style='color: #f39c12; font-weight: 600;'>Humeda</span>";
  } else {
    html += "Estado: <span class='status-off'>Seca</span>";
  }
  html += "</div></div>";
  
  // Environmental Sensor Status Card
  html += "<div class='status-card dht'>";
  html += "<div class='status-title'>Sensor Ambiental DHT11</div>";
  html += "<div class='status-value'>";
  html += "Temperatura: <strong>" + String(temperature, 1) + " C</strong><br>";
  html += "Humedad: <strong>" + String(humidity, 1) + " %</strong><br>";
  
  String tempStatus = "";
  if (temperature > 30) {
    tempStatus = "<span style='color: #e74c3c; font-weight: 600;'>Caluroso</span>";
  } else if (temperature > 20) {
    tempStatus = "<span class='status-on'>Agradable</span>";
  } else if (temperature > 10) {
    tempStatus = "<span style='color: #3498db; font-weight: 600;'>Fresco</span>";
  } else {
    tempStatus = "<span class='status-off'>Frio</span>";
  }
  html += "Estado: " + tempStatus;
  html += "</div></div>";
  
  // Proximity Sensor Status Card
  String proximityCardClass = proximityAlert ? "status-card proximity alert" : "status-card proximity";
  html += "<div class='" + proximityCardClass + "'>";
  html += "<div class='status-title'>Sensor de Proximidad</div>";
  html += "<div class='status-value'>";
  html += "Distancia: <strong>" + String(distance) + " cm</strong><br>";
  
  if (proximityAlert) {
    html += "Estado: <span style='color: #e74c3c; font-weight: 600;'>ALERTA - BUZZER ACTIVO</span>";
  } else if (distance <= 50 && distance > 15) {
    html += "Estado: <span style='color: #f39c12; font-weight: 600;'>Objeto detectado</span>";
  } else {
    html += "Estado: <span class='status-on'>Zona libre</span>";
  }
  html += "</div></div>";
  
  // Lighting Status Card
  html += "<div class='status-card lights'>";
  html += "<div class='status-title'>Sistema de Iluminacion</div>";
  html += "<div class='status-value'>";
  html += "<div class='lights-grid'>";
  
  html += "<div class='light-item'><span>Dormitorio:</span>" + String(bedroomLight ? "<span class='status-on'>ON</span>" : "<span class='status-off'>OFF</span>") + "</div>";
  html += "<div class='light-item'><span>Bano:</span>" + String(bathroomLight ? "<span class='status-on'>ON</span>" : "<span class='status-off'>OFF</span>") + "</div>";
  html += "<div class='light-item'><span>Sala 1:</span>" + String(livingRoom1Light ? "<span class='status-on'>ON</span>" : "<span class='status-off'>OFF</span>") + "</div>";
  html += "<div class='light-item'><span>Sala 2:</span>" + String(livingRoom2Light ? "<span class='status-on'>ON</span>" : "<span class='status-off'>OFF</span>") + "</div>";
  
  html += "</div>";
  html += "<div style='text-align: center; margin-top: 15px;'>";
  html += "<div class='light-item' style='justify-content: center; background: #d5dbdb;'>";
  html += "<span>Exterior:</span>" + String(exteriorLight ? "<span class='status-on'>ON</span>" : "<span class='status-off'>OFF</span>");
  html += "</div>";
  
  int lightsOn = getLightsCount();
  html += "<div style='margin-top: 15px; font-weight: 600; color: #2c3e50;'>";
  html += "Total encendidas: " + String(lightsOn) + "/5";
  html += "</div>";
  html += "</div>";
  html += "</div></div>";
  
  return html;
}

int getLightsCount() {
  int count = 0;
  if (bedroomLight) count++;
  if (bathroomLight) count++;
  if (livingRoom1Light) count++;
  if (livingRoom2Light) count++;
  if (exteriorLight) count++;
  return count;
}
