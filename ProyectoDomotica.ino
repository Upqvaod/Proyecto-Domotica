/*
 * Sistema Domótico ESP32 con Integración Alexa
 * 
 * INSTALACIÓN DE LIBRERÍAS REQUERIDAS:
 * 1. ESP32Servo (por Kevin Harrington)
 * 2. DHT sensor library (por Adafruit)
 * 3. Adafruit Unified Sensor (por Adafruit)
 * 4. fauxmoESP (por Xose Pérez) - Para integración con Alexa
 * 
 * Para instalar fauxmoESP:
 * - Arduino IDE: Herramientas > Administrar librerías > buscar "fauxmoESP"
 * - O descargar de: https://github.com/vintlabs/fauxmoESP
 * 
 * COMANDOS DE ALEXA:
 * - "Alexa, enciende Bedroom Light"
 * - "Alexa, apaga Bathroom Light"
 * - "Alexa, enciende Living Room One"
 * - "Alexa, enciende Living Room Two"
 * - "Alexa, enciende Exterior Light"
 * - "Alexa, enciende Smart Lock" (desbloquear)
 * - "Alexa, apaga Smart Lock" (bloquear)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <fauxmoESP.h>

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
const int pinServo = 19; 
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

// Servidor web y Alexa
WebServer server(8080);  // Cambiar a puerto 8080 para evitar conflicto con fauxmoESP
fauxmoESP fauxmo;

void setup() {
  Serial.begin(115200);
  Serial.println("Sistema de humedad, cerradura y proximidad iniciando...");
  
  // Inicializar sensor DHT11
  dht.begin();
  Serial.println("Sensor DHT11 inicializado en pin " + String(DHTPIN));
  
  // Verificar conexión inicial del DHT11
  delay(2000); // Esperar estabilización del DHT11
  float testTemp = dht.readTemperature();
  float testHum = dht.readHumidity();
  
  if (isnan(testTemp) || isnan(testHum)) {
    Serial.println("ADVERTENCIA: DHT11 no responde - verificar conexiones");
  } else {
    Serial.println("DHT11 funcionando correctamente");
    Serial.println("Temp inicial: " + String(testTemp) + "°C, Humedad: " + String(testHum) + "%");
  }
  
  // Configurar servomotor
  servoLock.attach(pinServo);
  servoLock.write(0); // Inicialmente cerrado
  Serial.println("Cerradura cerrada (0 grados)");
  
  // Configurar sensor ultrasónico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("Sensor ultrasónico - TRIG: pin " + String(TRIG_PIN) + ", ECHO: pin " + String(ECHO_PIN));
  
  // Configurar sensor de humedad del suelo
  Serial.println("Sensor humedad suelo configurado en pin " + String(pinSensor));
  
  // Verificación inicial de sensores
  Serial.println("=== VERIFICACIÓN INICIAL DE SENSORES ===");
  int soilTest = analogRead(pinSensor);
  Serial.println("Lectura inicial sensor suelo: " + String(soilTest) + " (valores altos = seco)");
  
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
  Serial.println("Servidor web iniciado en puerto 8080");
  Serial.println("Accede desde: http://" + WiFi.localIP().toString() + ":8080");
  
  // Configurar fauxmoESP para Alexa (DEBE ir después del servidor web)
  fauxmo.createServer(true);
  fauxmo.setPort(80);  // fauxmoESP usa puerto 80 para Alexa
  fauxmo.enable(true);
  
  Serial.println("fauxmoESP configurado - Puerto: 80");
  
  // Agregar dispositivos Alexa
  fauxmo.addDevice("Bedroom Light");     // ID 0
  fauxmo.addDevice("Bathroom Light");    // ID 1
  fauxmo.addDevice("Living Room One");   // ID 2
  fauxmo.addDevice("Living Room Two");   // ID 3
  fauxmo.addDevice("Exterior Light");    // ID 4
  fauxmo.addDevice("Smart Lock");        // ID 5
  
  Serial.println("Dispositivos Alexa agregados:");
  Serial.println("- Bedroom Light (ID: 0)");
  Serial.println("- Bathroom Light (ID: 1)");
  Serial.println("- Living Room One (ID: 2)");
  Serial.println("- Living Room Two (ID: 3)");
  Serial.println("- Exterior Light (ID: 4)");
  Serial.println("- Smart Lock (ID: 5)");
  
  // Configurar callback para comandos de Alexa
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    Serial.printf("[ALEXA] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
    Serial.println("[DEBUG] Alexa comando recibido correctamente");
    
    switch(device_id) {
      case 0: // Bedroom Light
        bedroomLight = state;
        digitalWrite(LED_BEDROOM, bedroomLight ? HIGH : LOW);
        Serial.println("Bedroom Light via Alexa: " + String(bedroomLight ? "ON" : "OFF"));
        break;
        
      case 1: // Bathroom Light
        bathroomLight = state;
        digitalWrite(LED_BATHROOM, bathroomLight ? HIGH : LOW);
        Serial.println("Bathroom Light via Alexa: " + String(bathroomLight ? "ON" : "OFF"));
        break;
        
      case 2: // Living Room One
        livingRoom1Light = state;
        digitalWrite(LED_LIVING1, livingRoom1Light ? HIGH : LOW);
        Serial.println("Living Room One via Alexa: " + String(livingRoom1Light ? "ON" : "OFF"));
        break;
        
      case 3: // Living Room Two
        livingRoom2Light = state;
        digitalWrite(LED_LIVING2, livingRoom2Light ? HIGH : LOW);
        Serial.println("Living Room Two via Alexa: " + String(livingRoom2Light ? "ON" : "OFF"));
        break;
        
      case 4: // Exterior Light
        exteriorLight = state;
        digitalWrite(LED_EXTERIOR, exteriorLight ? HIGH : LOW);
        Serial.println("Exterior Light via Alexa: " + String(exteriorLight ? "ON" : "OFF"));
        break;
        
      case 5: // Smart Lock
        lockState = state;
        servoLock.write(lockState ? 90 : 0);
        Serial.println("Smart Lock via Alexa: " + String(lockState ? "UNLOCKED" : "LOCKED"));
        break;
        
      default:
        Serial.println("[ERROR] Device ID no reconocido: " + String(device_id));
        break;
    }
  });
  
  Serial.println("Dispositivos Alexa configurados con fauxmoESP");
  Serial.println("=== INSTRUCCIONES PARA ALEXA ===");
  Serial.println("1. Asegurate de que Alexa y ESP32 esten en la MISMA red WiFi");
  Serial.println("2. Di: 'Alexa, descubre dispositivos'");
  Serial.println("3. Espera 20-30 segundos para el descubrimiento");
  Serial.println("4. Comandos disponibles:");
  Serial.println("   - 'Alexa, enciende Bedroom Light'");
  Serial.println("   - 'Alexa, apaga Bathroom Light'");
  Serial.println("   - 'Alexa, enciende Living Room One'");
  Serial.println("   - 'Alexa, enciende Living Room Two'");
  Serial.println("   - 'Alexa, enciende Exterior Light'");
  Serial.println("   - 'Alexa, enciende Smart Lock' (desbloquear)");
  Serial.println("   - 'Alexa, apaga Smart Lock' (bloquear)");
  Serial.println("================================");
  Serial.println("IP del ESP32: " + WiFi.localIP().toString());
  Serial.println("Puerto Web: 8080");
  Serial.println("Puerto Alexa: 80");
  Serial.println("Red WiFi: " + String(ssid));
  Serial.println("Sistema listo!");
}

void loop() {
  server.handleClient();
  fauxmo.handle();
  
  // Debug fauxmoESP cada 30 segundos
  static unsigned long lastFauxmoDebug = 0;
  if (millis() - lastFauxmoDebug > 30000) {
    Serial.println("[DEBUG] fauxmoESP funcionando - IP: " + WiFi.localIP().toString() + " Puerto: 80");
    lastFauxmoDebug = millis();
  }
  
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

  // Interpretación CORREGIDA del nivel de humedad del suelo
  // Valores ALTOS = seco, valores BAJOS = húmedo
  if (valor < 1500) {
    Serial.println("La tierra esta muy mojada!");
  } else if (valor < 2500) {
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
    Serial.println("ERROR: DHT11 no responde correctamente");
    Serial.println("Verificar: conexiones, alimentación, resistencia pull-up 10kΩ");
    return;
  }

  // Verificar rangos válidos
  if (newTemperature < -40 || newTemperature > 80 || newHumidity < 0 || newHumidity > 100) {
    Serial.println("ERROR: DHT11 valores fuera de rango");
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
  digitalWrite(LED_BEDROOM, bedroomLight ? HIGH : LOW);  // Fixed
  digitalWrite(LED_BATHROOM, bathroomLight ? HIGH : LOW);  // Fixed
  digitalWrite(LED_LIVING1, livingRoom1Light ? HIGH : LOW);  // Fixed
  digitalWrite(LED_LIVING2, livingRoom2Light ? HIGH : LOW);  // Fixed
  digitalWrite(LED_EXTERIOR, exteriorLight ? HIGH : LOW);  // Fixed
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
  html += "<link rel='icon' href='data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMzIiIGhlaWdodD0iMzIiIHZpZXdCb3g9IjAgMCAzMiAzMiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTE2IDJMMjggOEwyOCAyNEwxNiAzMEw0IDI0TDQgOEwxNiAyWiIgZmlsbD0iIzY2N2VlYSIvPgo8L3N2Zz4K'>";
  html += generateCSS();
  html += generateJavaScript();
  html += "</head><body>";
  
  // Navigation bar
  html += "<nav class='navbar'>";
  html += "<div class='nav-brand'>";
  html += "<span class='nav-icon'>🏠</span>";
  html += "<span class='nav-title'>Smart Home</span>";
  html += "</div>";
  html += "<div class='nav-status'>";
  html += "<span class='connection-status online'>● Conectado</span>";
  html += "<span class='device-count'>6 dispositivos</span>";
  html += "</div>";
  html += "</nav>";
  
  html += "<div class='main-container'>";
  
  // Quick action buttons
  html += "<div class='quick-actions'>";
  html += "<h2 class='section-title'>Acciones Rapidas</h2>";
  html += "<div class='quick-buttons'>";
  html += "<a href='/all_lights_on' class='quick-btn lights-on'>";
  html += "<span class='quick-icon'>💡</span>";
  html += "<span class='quick-text'>Todas las Luces</span>";
  html += "<span class='quick-action'>ENCENDER</span>";
  html += "</a>";
  html += "<a href='/all_lights_off' class='quick-btn lights-off'>";
  html += "<span class='quick-icon'>🔌</span>";
  html += "<span class='quick-text'>Todas las Luces</span>";
  html += "<span class='quick-action'>APAGAR</span>";
  html += "</a>";
  html += "<a href='" + String(lockState ? "/lock" : "/unlock") + "' class='quick-btn " + String(lockState ? "lock" : "unlock") + "'>";
  html += "<span class='quick-icon'>" + String(lockState ? "🔒" : "🔓") + "</span>";
  html += "<span class='quick-text'>Cerradura</span>";
  html += "<span class='quick-action'>" + String(lockState ? "BLOQUEAR" : "DESBLOQUEAR") + "</span>";
  html += "</a>";
  html += "</div>";
  html += "</div>";
  
  // Status overview in cards
  html += "<div class='status-overview'>";
  html += "<h2 class='section-title'>Estado del Sistema</h2>";
  html += "<div id='status-panel' class='status-cards'>";
  html += getStatusHTML();
  html += "</div>";
  html += "</div>";
  
  // Control panels with better organization
  html += generateControlPanels();
  
  // Footer with system info
  html += "<footer class='footer'>";
  html += "<div class='footer-content'>";
  html += "<div class='footer-section'>";
  html += "<h4>Sistema ESP32</h4>";
  html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
  html += "<p>Actualización automática</p>";
  html += "</div>";
  html += "<div class='footer-section'>";
  html += "<h4>Control por Voz</h4>";
  html += "<p>Compatible con Alexa</p>";
  html += "<p>6 dispositivos configurados</p>";
  html += "</div>";
  html += "<div class='footer-section'>";
  html += "<h4>Sensores Activos</h4>";
  html += "<p>Temperatura - Humedad</p>";
  html += "<p>Proximidad - Suelo</p>";
  html += "</div>";
  html += "</div>";
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
  
  // Enhanced status update with loading states
  js += "function updateStatus() {";
  js += "  const panel = document.getElementById('status-panel');";
  js += "  if (panel) panel.classList.add('loading');";
  js += "  fetch('/status')";
  js += "    .then(response => response.text())";
  js += "    .then(data => {";
  js += "      if (panel) {";
  js += "        panel.innerHTML = data;";
  js += "        panel.classList.remove('loading');";
  js += "        animateCards();";
  js += "      }";
  js += "    })";
  js += "    .catch(err => {";
  js += "      console.error('Update failed:', err);";
  js += "      if (panel) panel.classList.remove('loading');";
  js += "    });";
  js += "}";
  
  // Animate status cards
  js += "function animateCards() {";
  js += "  const cards = document.querySelectorAll('.status-card');";
  js += "  cards.forEach((card, index) => {";
  js += "    card.style.animationDelay = (index * 0.1) + 's';";
  js += "    card.style.animation = 'fadeIn 0.6s ease-out forwards';";
  js += "  });";
  js += "}";
  
  // Add click feedback to buttons
  js += "function addButtonFeedback() {";
  js += "  const buttons = document.querySelectorAll('.btn, .quick-btn');";
  js += "  buttons.forEach(button => {";
  js += "    button.addEventListener('click', function(e) {";
  js += "      const ripple = document.createElement('span');";
  js += "      ripple.classList.add('ripple');";
  js += "      this.appendChild(ripple);";
  js += "      setTimeout(() => ripple.remove(), 600);";
  js += "    });";
  js += "  });";
  js += "}";
  
  // Connection status monitoring
  js += "function updateConnectionStatus() {";
  js += "  const statusElement = document.querySelector('.connection-status');";
  js += "  if (navigator.onLine) {";
  js += "    if (statusElement) {";
  js += "      statusElement.textContent = '● Conectado';";
  js += "      statusElement.className = 'connection-status online';";
  js += "    }";
  js += "  } else {";
  js += "    if (statusElement) {";
  js += "      statusElement.textContent = '● Desconectado';";
  js += "      statusElement.className = 'connection-status offline';";
  js += "    }";
  js += "  }";
  js += "}";
  
  // Notification system
  js += "function showNotification(message, type = 'info') {";
  js += "  const notification = document.createElement('div');";
  js += "  notification.className = `notification notification-${type}`;";
  js += "  notification.textContent = message;";
  js += "  document.body.appendChild(notification);";
  js += "  setTimeout(() => {";
  js += "    notification.classList.add('show');";
  js += "  }, 100);";
  js += "  setTimeout(() => {";
  js += "    notification.classList.remove('show');";
  js += "    setTimeout(() => notification.remove(), 300);";
  js += "  }, 3000);";
  js += "}";
  
  // Initialize everything when DOM is loaded
  js += "document.addEventListener('DOMContentLoaded', function() {";
  js += "  console.log('[ESP32] Smart Home System Loaded');";
  js += "  animateCards();";
  js += "  addButtonFeedback();";
  js += "  updateConnectionStatus();";
  
  // Set up auto-refresh
  js += "  setInterval(updateStatus, 3000);";
  
  // Monitor online/offline status
  js += "  window.addEventListener('online', updateConnectionStatus);";
  js += "  window.addEventListener('offline', updateConnectionStatus);";
  
  // Add smooth scrolling for better UX
  js += "  document.querySelectorAll('a[href^=\"#\"]').forEach(anchor => {";
  js += "    anchor.addEventListener('click', function (e) {";
  js += "      e.preventDefault();";
  js += "      const target = document.querySelector(this.getAttribute('href'));";
  js += "      if (target) {";
  js += "        target.scrollIntoView({ behavior: 'smooth' });";
  js += "      }";
  js += "    });";
  js += "  });";
  
  js += "});";
  
  // Add CSS for new interactive elements
  js += "const style = document.createElement('style');";
  js += "style.textContent = `";
  js += ".ripple { position: absolute; border-radius: 50%; background: rgba(255, 255, 255, 0.6); transform: scale(0); animation: ripple 0.6s linear; pointer-events: none; }";
  js += "@keyframes ripple { to { transform: scale(4); opacity: 0; } }";
  js += ".notification { position: fixed; top: 20px; right: 20px; padding: 1rem 1.5rem; border-radius: 8px; color: white; font-weight: 500; transform: translateX(100%); transition: transform 0.3s ease; z-index: 1000; }";
  js += ".notification.show { transform: translateX(0); }";
  js += ".notification-info { background: var(--info-color); }";
  js += ".notification-success { background: var(--success-color); }";
  js += ".notification-warning { background: var(--warning-color); }";
  js += ".notification-error { background: var(--danger-color); }";
  js += ".connection-status.offline { color: var(--danger-color); }";
  js += ".btn.active { box-shadow: 0 0 0 2px var(--accent-color); }";
  js += ".room-control { position: relative; }";
  js += ".room-control .btn { flex-direction: column; gap: 0.5rem; min-height: 80px; }";
  js += ".room-control .btn small { font-size: 0.75rem; opacity: 0.8; font-weight: normal; }";
  js += ".command-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 0.75rem; }";
  js += ".command-item { background: rgba(0, 0, 0, 0.05); padding: 0.75rem; border-radius: 8px; font-family: monospace; font-size: 0.875rem; border-left: 3px solid var(--accent-color); }";
  js += "`;";
  js += "document.head.appendChild(style);";
  
  js += "</script>";
  return js;
}

String generateCSS() {
  String css = "<style>";
  
  // Import modern font
  css += "@import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap');";
  
  // CSS Variables for consistent theming
  css += ":root {";
  css += "--primary-color: #667eea;";
  css += "--secondary-color: #764ba2;";
  css += "--accent-color: #f093fb;";
  css += "--success-color: #4ade80;";
  css += "--warning-color: #fbbf24;";
  css += "--danger-color: #f87171;";
  css += "--info-color: #60a5fa;";
  css += "--dark-color: #1f2937;";
  css += "--light-color: #f8fafc;";
  css += "--surface-color: rgba(255, 255, 255, 0.95);";
  css += "--border-radius: 16px;";
  css += "--shadow-sm: 0 1px 3px rgba(0, 0, 0, 0.1);";
  css += "--shadow-md: 0 4px 12px rgba(0, 0, 0, 0.15);";
  css += "--shadow-lg: 0 8px 32px rgba(0, 0, 0, 0.2);";
  css += "--transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);";
  css += "}";
  
  // Reset and base styles
  css += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  css += "body { font-family: 'Inter', -apple-system, BlinkMacSystemFont, sans-serif; background: linear-gradient(135deg, var(--primary-color) 0%, var(--secondary-color) 100%); min-height: 100vh; color: var(--dark-color); line-height: 1.6; }";
  
  // Navigation bar
  css += ".navbar { background: var(--surface-color); backdrop-filter: blur(10px); border-bottom: 1px solid rgba(255, 255, 255, 0.2); padding: 1rem 2rem; display: flex; justify-content: space-between; align-items: center; position: sticky; top: 0; z-index: 100; box-shadow: var(--shadow-sm); }";
  css += ".nav-brand { display: flex; align-items: center; gap: 0.75rem; }";
  css += ".nav-icon { font-size: 1.5rem; }";
  css += ".nav-title { font-size: 1.25rem; font-weight: 700; color: var(--dark-color); }";
  css += ".nav-status { display: flex; flex-direction: column; align-items: flex-end; gap: 0.25rem; }";
  css += ".connection-status { font-size: 0.875rem; font-weight: 500; }";
  css += ".connection-status.online { color: var(--success-color); }";
  css += ".device-count { font-size: 0.75rem; color: #6b7280; }";
  
  // Main container
  css += ".main-container { max-width: 1400px; margin: 0 auto; padding: 2rem; }";
  
  // Section titles
  css += ".section-title { font-size: 1.5rem; font-weight: 600; color: var(--dark-color); margin-bottom: 1.5rem; text-align: center; }";
  
  // Quick actions
  css += ".quick-actions { margin-bottom: 3rem; }";
  css += ".quick-buttons { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 1rem; max-width: 800px; margin: 0 auto; }";
  css += ".quick-btn { background: var(--surface-color); border-radius: var(--border-radius); padding: 1.5rem; text-decoration: none; color: inherit; display: flex; flex-direction: column; align-items: center; gap: 0.75rem; transition: var(--transition); box-shadow: var(--shadow-md); border: 2px solid transparent; }";
  css += ".quick-btn:hover { transform: translateY(-4px); box-shadow: var(--shadow-lg); }";
  css += ".quick-btn.lights-on { border-color: var(--warning-color); }";
  css += ".quick-btn.lights-on:hover { background: linear-gradient(135deg, #fef3c7 0%, #fed7aa 100%); }";
  css += ".quick-btn.lights-off { border-color: var(--dark-color); }";
  css += ".quick-btn.lights-off:hover { background: linear-gradient(135deg, #f1f5f9 0%, #e2e8f0 100%); }";
  css += ".quick-btn.unlock { border-color: var(--success-color); }";
  css += ".quick-btn.unlock:hover { background: linear-gradient(135deg, #dcfce7 0%, #bbf7d0 100%); }";
  css += ".quick-btn.lock { border-color: var(--danger-color); }";
  css += ".quick-btn.lock:hover { background: linear-gradient(135deg, #fef2f2 0%, #fecaca 100%); }";
  css += ".quick-icon { font-size: 2rem; }";
  css += ".quick-text { font-weight: 500; color: #6b7280; }";
  css += ".quick-action { font-weight: 700; font-size: 0.875rem; }";
  
  // Status overview
  css += ".status-overview { margin-bottom: 3rem; }";
  css += ".status-cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 1.5rem; }";
  
  // Status cards redesign
  css += ".status-card { background: var(--surface-color); border-radius: var(--border-radius); padding: 1.5rem; box-shadow: var(--shadow-md); transition: var(--transition); border-left: 4px solid var(--info-color); position: relative; overflow: hidden; }";
  css += ".status-card::before { content: ''; position: absolute; top: 0; right: 0; width: 60px; height: 60px; background: linear-gradient(45deg, transparent, rgba(255, 255, 255, 0.1)); border-radius: 0 0 0 60px; }";
  css += ".status-card:hover { transform: translateY(-2px); box-shadow: var(--shadow-lg); }";
  css += ".status-card.locked { border-left-color: var(--danger-color); }";
  css += ".status-card.unlocked { border-left-color: var(--success-color); }";
  css += ".status-card.alert { border-left-color: var(--danger-color); animation: pulse 2s infinite; }";
  css += ".status-card.soil { border-left-color: #8b5cf6; }";
  css += ".status-card.dht { border-left-color: var(--warning-color); }";
  css += ".status-card.proximity { border-left-color: #f59e0b; }";
  css += ".status-card.lights { border-left-color: var(--success-color); }";
  
  // Status card content
  css += ".status-title { font-size: 1.125rem; font-weight: 600; margin-bottom: 1rem; color: var(--dark-color); display: flex; align-items: center; gap: 0.5rem; }";
  css += ".status-value { font-size: 0.875rem; color: #6b7280; line-height: 1.6; }";
  css += ".status-on { color: var(--success-color); font-weight: 600; }";
  css += ".status-off { color: var(--danger-color); font-weight: 600; }";
  css += ".status-metric { display: flex; justify-content: space-between; align-items: center; margin: 0.75rem 0; padding: 0.5rem; background: rgba(0, 0, 0, 0.02); border-radius: 8px; }";
  
  // Control sections
  css += ".control-section { background: var(--surface-color); border-radius: var(--border-radius); padding: 2rem; margin-bottom: 2rem; box-shadow: var(--shadow-md); }";
  css += ".control-section .section-title { margin-bottom: 2rem; }";
  
  // Enhanced button styles
  css += ".btn { display: inline-flex; align-items: center; justify-content: center; gap: 0.5rem; padding: 0.875rem 1.5rem; border: none; border-radius: 12px; font-size: 0.875rem; font-weight: 600; text-decoration: none; cursor: pointer; transition: var(--transition); min-width: 140px; text-transform: uppercase; letter-spacing: 0.025em; position: relative; overflow: hidden; }";
  css += ".btn::before { content: ''; position: absolute; top: 0; left: -100%; width: 100%; height: 100%; background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent); transition: left 0.5s; }";
  css += ".btn:hover::before { left: 100%; }";
  css += ".btn:hover { transform: translateY(-2px); box-shadow: var(--shadow-md); }";
  css += ".btn:active { transform: translateY(0); }";
  
  // Button variants with modern gradients
  css += ".btn-primary { background: linear-gradient(135deg, var(--info-color), #3b82f6); color: white; }";
  css += ".btn-success { background: linear-gradient(135deg, var(--success-color), #22c55e); color: white; }";
  css += ".btn-danger { background: linear-gradient(135deg, var(--danger-color), #ef4444); color: white; }";
  css += ".btn-warning { background: linear-gradient(135deg, var(--warning-color), #f59e0b); color: white; }";
  css += ".btn-purple { background: linear-gradient(135deg, #a855f7, #8b5cf6); color: white; }";
  css += ".btn-info { background: linear-gradient(135deg, #06b6d4, #0891b2); color: white; }";
  css += ".btn-dark { background: linear-gradient(135deg, var(--dark-color), #374151); color: white; }";
  
  // Grid layouts with better spacing
  css += ".room-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 1rem; margin: 1.5rem 0; }";
  css += ".master-controls { display: flex; gap: 1rem; justify-content: center; flex-wrap: wrap; margin-top: 2rem; }";
  css += ".lock-controls { display: flex; gap: 1rem; justify-content: center; flex-wrap: wrap; }";
  
  // Lights grid for status
  css += ".lights-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 0.75rem; margin: 1rem 0; }";
  css += ".light-item { display: flex; justify-content: space-between; align-items: center; padding: 0.75rem 1rem; background: rgba(0, 0, 0, 0.02); border-radius: 8px; border: 1px solid rgba(0, 0, 0, 0.05); }";
  
  // Footer redesign
  css += ".footer { background: var(--surface-color); margin-top: 3rem; padding: 2rem; border-radius: var(--border-radius); box-shadow: var(--shadow-md); }";
  css += ".footer-content { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 2rem; }";
  css += ".footer-section h4 { font-weight: 600; margin-bottom: 0.5rem; color: var(--dark-color); }";
  css += ".footer-section p { font-size: 0.875rem; color: #6b7280; margin-bottom: 0.25rem; }";
  
  // Responsive design
  css += "@media (max-width: 768px) {";
  css += "  .main-container { padding: 1rem; }";
  css += "  .navbar { padding: 1rem; }";
  css += "  .nav-title { font-size: 1.125rem; }";
  css += "  .quick-buttons { grid-template-columns: 1fr; }";
  css += "  .status-cards { grid-template-columns: 1fr; }";
  css += "  .room-grid { grid-template-columns: repeat(2, 1fr); }";
  css += "  .master-controls, .lock-controls { flex-direction: column; align-items: center; }";
  css += "  .btn { min-width: 120px; }";
  css += "  .footer-content { grid-template-columns: 1fr; text-align: center; }";
  css += "}";
  
  // Animations
  css += "@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.8; } }";
  css += "@keyframes fadeIn { from { opacity: 0; transform: translateY(20px); } to { opacity: 1; transform: translateY(0); } }";
  css += "@keyframes slideIn { from { opacity: 0; transform: translateX(-20px); } to { opacity: 1; transform: translateX(0); } }";
  css += ".quick-actions, .status-overview, .control-section { animation: fadeIn 0.6s ease-out; }";
  css += ".navbar { animation: slideIn 0.5s ease-out; }";
  
  // Loading states
  css += ".loading { position: relative; overflow: hidden; }";
  css += ".loading::after { content: ''; position: absolute; top: 0; left: -100%; width: 100%; height: 100%; background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.4), transparent); animation: loading 1.5s infinite; }";
  css += "@keyframes loading { 0% { left: -100%; } 100% { left: 100%; } }";
  
  css += "</style>";
  return css;
}

String generateControlPanels() {
  String html = "";
  
  // Security Control Panel
  html += "<div class='control-section'>";
  html += "<h2 class='section-title'>Control de Seguridad</h2>";
  html += "<div class='security-status'>";
  html += "<div class='status-metric'>";
  html += "<span>Estado Actual:</span>";
  html += "<span class='" + String(lockState ? "status-on" : "status-off") + "'>";
  html += lockState ? "DESBLOQUEADA" : "BLOQUEADA";
  html += "</span>";
  html += "</div>";
  html += "</div>";
  html += "<div class='lock-controls'>";
  html += "<a href='/unlock' class='btn btn-success'>🔓 DESBLOQUEAR</a>";
  html += "<a href='/lock' class='btn btn-danger'>🔒 BLOQUEAR</a>";
  html += "</div>";
  html += "</div>";
  
  // Lighting Control Panel
  html += "<div class='control-section'>";
  html += "<h2 class='section-title'>Sistema de Iluminacion</h2>";
  
  // Individual room controls
  html += "<div class='room-controls'>";
  html += "<h3 style='margin-bottom: 1rem; color: #6b7280; font-weight: 500;'>Control por Habitacion</h3>";
  html += "<div class='room-grid'>";
  
  // Room buttons with status indicators
  html += "<div class='room-control'>";
  html += "<a href='/toggle_bedroom' class='btn btn-purple " + String(bedroomLight ? "active" : "") + "'>";
  html += "<span>🛏️ DORMITORIO</span>";
  html += "<small>" + String(bedroomLight ? "ENCENDIDA" : "APAGADA") + "</small>";
  html += "</a>";
  html += "</div>";
  
  html += "<div class='room-control'>";
  html += "<a href='/toggle_bathroom' class='btn btn-info " + String(bathroomLight ? "active" : "") + "'>";
  html += "<span>🚿 BANO</span>";
  html += "<small>" + String(bathroomLight ? "ENCENDIDA" : "APAGADA") + "</small>";
  html += "</a>";
  html += "</div>";
  
  html += "<div class='room-control'>";
  html += "<a href='/toggle_living1' class='btn btn-warning " + String(livingRoom1Light ? "active" : "") + "'>";
  html += "<span>🛋️ SALA 1</span>";
  html += "<small>" + String(livingRoom1Light ? "ENCENDIDA" : "APAGADA") + "</small>";
  html += "</a>";
  html += "</div>";
  
  html += "<div class='room-control'>";
  html += "<a href='/toggle_living2' class='btn btn-warning " + String(livingRoom2Light ? "active" : "") + "'>";
  html += "<span>🪑 SALA 2</span>";
  html += "<small>" + String(livingRoom2Light ? "ENCENDIDA" : "APAGADA") + "</small>";
  html += "</a>";
  html += "</div>";
  
  html += "</div>";
  
  // Exterior light (full width)
  html += "<div style='margin: 1.5rem 0;'>";
  html += "<a href='/toggle_exterior' class='btn btn-success " + String(exteriorLight ? "active" : "") + "' style='width: 100%; padding: 1.25rem;'>";
  html += "<span style='font-size: 1.1rem;'>🌳 EXTERIOR/JARDIN</span>";
  html += "<small style='display: block; margin-top: 0.25rem;'>" + String(exteriorLight ? "ENCENDIDA" : "APAGADA") + "</small>";
  html += "</a>";
  html += "</div>";
  
  // Master controls
  html += "<div class='master-controls-section'>";
  html += "<h3 style='margin-bottom: 1rem; color: #6b7280; font-weight: 500; text-align: center;'>Control Maestro</h3>";
  html += "<div class='status-metric' style='margin-bottom: 1rem;'>";
  html += "<span>Luces Encendidas:</span>";
  html += "<span style='font-weight: 700; color: var(--primary-color);'>" + String(getLightsCount()) + "/5</span>";
  html += "</div>";
  html += "<div class='master-controls'>";
  html += "<a href='/all_lights_on' class='btn btn-primary'>💡 ENCENDER TODAS</a>";
  html += "<a href='/all_lights_off' class='btn btn-dark'>🔌 APAGAR TODAS</a>";
  html += "</div>";
  html += "</div>";
  
  html += "</div>";
  html += "</div>";
  
  // Voice Control Info Panel
  html += "<div class='control-section'>";
  html += "<h2 class='section-title'>Control por Voz (Alexa)</h2>";
  html += "<div class='voice-commands'>";
  html += "<p style='text-align: center; margin-bottom: 1rem; color: #6b7280;'>Comandos disponibles:</p>";
  html += "<div class='command-grid'>";
  html += "<div class='command-item'>\"Alexa, enciende Bedroom Light\"</div>";
  html += "<div class='command-item'>\"Alexa, apaga Bathroom Light\"</div>";
  html += "<div class='command-item'>\"Alexa, enciende Living Room One\"</div>";
  html += "<div class='command-item'>\"Alexa, enciende Smart Lock\"</div>";
  html += "</div>";
  html += "<div style='text-align: center; margin-top: 1rem;'>";
  html += "<small style='color: #6b7280;'>Di \"Alexa, descubre dispositivos\" para configurar</small>";
  html += "</div>";
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
  html += "<div class='status-title'>🔐 Estado de Seguridad</div>";
  html += "<div class='status-value'>";
  html += "<div class='status-metric'>";
  html += "<span>Cerradura:</span>";
  html += "<span class='" + String(lockState ? "status-on" : "status-off") + "'>";
  html += lockState ? "DESBLOQUEADA" : "BLOQUEADA";
  html += "</span>";
  html += "</div>";
  html += "<div class='status-metric'>";
  html += "<span>Posicion Servo:</span>";
  html += "<span>" + String(lockState ? "90°" : "0°") + "</span>";
  html += "</div>";
  html += "</div></div>";
  
  // Environmental Sensor Status Card
  html += "<div class='status-card dht'>";
  html += "<div class='status-title'>🌡️ Sensor Ambiental DHT11</div>";
  html += "<div class='status-value'>";
  html += "<div class='status-metric'>";
  html += "<span>Temperatura:</span>";
  html += "<span><strong>" + String(temperature, 1) + "°C</strong></span>";
  html += "</div>";
  html += "<div class='status-metric'>";
  html += "<span>Humedad:</span>";
  html += "<span><strong>" + String(humidity, 1) + "%</strong></span>";
  html += "</div>";
  html += "<div class='status-metric'>";
  html += "<span>Estado:</span>";
  String tempStatus = "";
  if (temperature > 30) {
    tempStatus = "<span style='color: var(--danger-color); font-weight: 600;'>Caluroso</span>";
  } else if (temperature > 20) {
    tempStatus = "<span style='color: var(--success-color); font-weight: 600;'>Agradable</span>";
  } else if (temperature > 10) {
    tempStatus = "<span style='color: var(--info-color); font-weight: 600;'>Fresco</span>";
  } else {
    tempStatus = "<span style='color: var(--danger-color); font-weight: 600;'>Frio</span>";
  }
  html += tempStatus;
  html += "</div>";
  html += "</div></div>";
  
  // Soil Humidity Status Card
  int soilValue = analogRead(pinSensor);
  html += "<div class='status-card soil'>";
  html += "<div class='status-title'>🌱 Sensor de Humedad del Suelo</div>";
  html += "<div class='status-value'>";
  html += "<div class='status-metric'>";
  html += "<span>Lectura:</span>";
  html += "<span>" + String(soilValue) + " / 4095</span>";
  html += "</div>";
  html += "<div class='status-metric'>";
  html += "<span>Estado:</span>";
  // Valores ALTOS = seco, valores BAJOS = húmedo
  if (soilValue < 1500) {
    html += "<span style='color: var(--info-color); font-weight: 600;'>Muy mojada</span>";
  } else if (soilValue < 2500) {
    html += "<span style='color: var(--warning-color); font-weight: 600;'>Humeda</span>";
  } else {
    html += "<span style='color: var(--danger-color); font-weight: 600;'>Seca</span>";
  }
  html += "</div>";
  html += "<div class='status-metric'>";
  html += "<span>Porcentaje:</span>";
  int moisturePercent = map(soilValue, 4095, 0, 0, 100); // Invertir escala
  html += "<span>" + String(moisturePercent) + "%</span>";
  html += "</div>";
  html += "</div></div>";
  
  // Proximity Sensor Status Card
  String proximityCardClass = proximityAlert ? "status-card proximity alert" : "status-card proximity";
  html += "<div class='" + proximityCardClass + "'>";
  html += "<div class='status-title'>📏 Sensor de Proximidad</div>";
  html += "<div class='status-value'>";
  html += "<div class='status-metric'>";
  html += "<span>Distancia:</span>";
  html += "<span><strong>" + String(distance) + " cm</strong></span>";
  html += "</div>";
  html += "<div class='status-metric'>";
  html += "<span>Estado:</span>";
  if (proximityAlert) {
    html += "<span style='color: var(--danger-color); font-weight: 600;'>ALERTA ACTIVA</span>";
  } else if (distance <= 50 && distance > 15) {
    html += "<span style='color: var(--warning-color); font-weight: 600;'>Objeto detectado</span>";
  } else {
    html += "<span style='color: var(--success-color); font-weight: 600;'>Zona libre</span>";
  }
  html += "</div>";
  html += "<div class='status-metric'>";
  html += "<span>Buzzer:</span>";
  html += "<span>" + String(proximityAlert ? "ACTIVO" : "Silencioso") + "</span>";
  html += "</div>";
  html += "</div></div>";
  
  // Lighting Status Card
  html += "<div class='status-card lights'>";
  html += "<div class='status-title'>💡 Sistema de Iluminacion</div>";
  html += "<div class='status-value'>";
  html += "<div class='lights-grid'>";
  
  html += "<div class='light-item'>";
  html += "<span>🛏️ Dormitorio:</span>";
  html += "<span class='" + String(bedroomLight ? "status-on" : "status-off") + "'>";
  html += bedroomLight ? "ON" : "OFF";
  html += "</span>";
  html += "</div>";
  
  html += "<div class='light-item'>";
  html += "<span>🚿 Bano:</span>";
  html += "<span class='" + String(bathroomLight ? "status-on" : "status-off") + "'>";
  html += bathroomLight ? "ON" : "OFF";
  html += "</span>";
  html += "</div>";
  
  html += "<div class='light-item'>";
  html += "<span>🛋️ Sala 1:</span>";
  html += "<span class='" + String(livingRoom1Light ? "status-on" : "status-off") + "'>";
  html += livingRoom1Light ? "ON" : "OFF";
  html += "</span>";
  html += "</div>";
  
  html += "<div class='light-item'>";
  html += "<span>🪑 Sala 2:</span>";
  html += "<span class='" + String(livingRoom2Light ? "status-on" : "status-off") + "'>";
  html += livingRoom2Light ? "ON" : "OFF";
  html += "</span>";
  html += "</div>";
  
  html += "<div class='light-item'>";
  html += "<span>🌳 Exterior:</span>";
  html += "<span class='" + String(exteriorLight ? "status-on" : "status-off") + "'>";
  html += exteriorLight ? "ON" : "OFF";
  html += "</span>";
  html += "</div>";
  
  html += "</div>";
  
  int lightsOn = getLightsCount();
  html += "<div class='status-metric' style='margin-top: 1rem; background: rgba(102, 126, 234, 0.1); border-radius: 8px;'>";
  html += "<span style='font-weight: 600;'>Total Encendidas:</span>";
  html += "<span style='font-weight: 700; color: var(--primary-color); font-size: 1.1rem;'>" + String(lightsOn) + "/5</span>";
  html += "</div>";
  
  // Energy estimation
  float estimatedWatts = lightsOn * 10; // Assuming 10W per LED
  html += "<div class='status-metric'>";
  html += "<span>Consumo Estimado:</span>";
  html += "<span style='color: var(--warning-color);'>⚡ " + String(estimatedWatts, 0) + "W</span>";
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

// Función para sincronizar estado con Alexa
void syncAlexaState(int device_id, bool state) {
  // Esta función puede ser usada para reportar cambios de estado a Alexa
  // fauxmoESP maneja esto automáticamente
}
