# 🏠 ESP32 Smart Home (Domotics) System

## Overview
A comprehensive home automation system built with ESP32, featuring environmental monitoring and intelligent lighting control through a responsive web interface.

## 🌟 Key Features

### 📊 Environmental Monitoring
- **Temperature Monitoring** - LM35 sensor with alert system
- **Humidity/Water Detection** - Soil moisture sensor for leak detection
- **Motion Detection** - HC-SR04 ultrasonic sensor for security
- **Real-time Alerts** - Audio buzzer notifications for critical conditions

### 💡 Smart Lighting Control
- **9 Independent LED Controls** - Individual room lighting
- **Remote Web Control** - Toggle lights from any device
- **Master Controls** - All ON/OFF functionality
- **Room-based Organization** - Named lighting zones

### 🌐 Web Interface
- **Real-time Dashboard** - Live sensor data updates
- **Responsive Design** - Works on mobile, tablet, desktop
- **Interactive Controls** - Click to toggle individual lights
- **Status Indicators** - Visual feedback for all systems

## 🏡 Smart Home Rooms

| LED | Room Name | Function |
|-----|-----------|----------|
| LED1 | Living Room | Main area lighting |
| LED2 | Kitchen | Cooking area lighting |
| LED3 | Bedroom | Rest area lighting |
| LED4 | Bathroom | Bathroom lighting |
| LED5 | Garden | Outdoor lighting |
| LED6 | Garage | Storage area lighting |
| LED7 | Office | Work area lighting |
| LED8 | Hallway | Corridor lighting |
| LED9 | Security | Motion-activated security light |

## 🔧 Hardware Requirements

### Main Components
- ESP32 Development Board
- HC-SR04 Ultrasonic Sensor (Motion Detection)
- LM35 Temperature Sensor
- Soil Moisture/Humidity Sensor
- 9 LEDs (different colors recommended)
- 1 Buzzer (for alerts)
- 9x 220Ω Resistors (for LEDs)
- Breadboard and jumper wires

### Pin Connections

#### Sensors
| Component | ESP32 Pin | Function |
|-----------|-----------|----------|
| Ultrasonic TRIG | GPIO 18 | Motion detection trigger |
| Ultrasonic ECHO | GPIO 19 | Motion detection echo |
| Temperature | GPIO 34 (ADC) | LM35 analog input |
| Humidity | GPIO 35 (ADC) | Moisture sensor input |
| Buzzer | GPIO 21 | Alert notifications |

#### Smart LEDs (with 220Ω resistors)
| Room | ESP32 Pin | Suggested LED Color |
|------|-----------|-------------------|
| Living Room | GPIO 2 | Warm White |
| Kitchen | GPIO 4 | Cool White |
| Bedroom | GPIO 5 | Soft Yellow |
| Bathroom | GPIO 12 | Bright White |
| Garden | GPIO 13 | Green |
| Garage | GPIO 14 | Orange |
| Office | GPIO 15 | Blue |
| Hallway | GPIO 16 | Purple |
| Security | GPIO 17 | Red |

## 🚀 Setup Instructions

### 1. Install Libraries
In Arduino IDE, install:
```
- WiFi (ESP32 Core - Built-in)
- WebServer (ESP32 Core - Built-in)
- ArduinoJson (by Benoit Blanchon)
```

### 2. Configure WiFi
Update these lines in the code:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 3. Upload Code
- Select "ESP32 Dev Module" as board
- Upload `merged_sensors.ino`
- Open Serial Monitor (115200 baud)

### 4. Access Dashboard
- Note the IP address from Serial Monitor
- Open web browser and go to: `http://[ESP32_IP_ADDRESS]`

## 🖥️ Web Interface Features

### Environmental Dashboard
- **Real-time Sensor Data** - Updates every second
- **Temperature Monitoring** - Current reading with alert status
- **Water Leak Detection** - Humidity sensor with critical alerts
- **Motion Detection** - Distance measurement and motion status
- **Alert System** - Visual and audio notifications

### Smart Lighting Control
- **Individual Room Control** - Toggle each room's light independently
- **Master Controls** - Turn all lights ON or OFF instantly
- **Visual Feedback** - LED status indicators show current state
- **Room Labels** - Clear identification of each lighting zone

### Responsive Design
- **Mobile Friendly** - Optimized for smartphones and tablets
- **Desktop Compatible** - Full-featured desktop experience
- **Auto-refresh** - Continuous updates without manual refresh
- **Modern UI** - Glassmorphism design with smooth animations

## 🏠 Smart Home Features

### Automation Logic
- **Security Lighting** - Motion detection auto-activates security light
- **Temperature Alerts** - Buzzer sounds for extreme temperatures (>35°C or <10°C)
- **Water Leak Detection** - Immediate alert for moisture readings >3000
- **Motion Logging** - Serial monitor tracks all motion events

### Alert System
- **Audio Alerts** - Buzzer patterns for different emergencies
- **Visual Indicators** - Color-coded status displays
- **Real-time Notifications** - Instant web interface updates

### Energy Management
- **Individual Control** - Turn off unused room lights
- **Master Controls** - Quick all-off for energy saving
- **Status Monitoring** - Always know which lights are on

## 🔧 Customization

### Modify Alert Thresholds
```cpp
// Temperature alerts
temperatureAlert = (temperatureC > 35 || temperatureC < 10);

// Humidity/water leak alert
humidityAlert = (sensorValue > 3000);

// Motion detection range
motionDetected = (DISTANCIA > 0 && DISTANCIA < 30);
```

### Change Room Names
```cpp
String roomNames[9] = {
  "Living Room", "Kitchen", "Bedroom", "Bathroom", "Garden", 
  "Garage", "Office", "Hallway", "Security"
};
```

### Adjust Update Intervals
```cpp
// Sensor reading frequency
if (currentMillis - previousMillis >= 2000) // 2 seconds

// Motion detection frequency  
if (currentMillis - ultrasonicPreviousMillis >= 500) // 0.5 seconds
```

## 📱 Mobile App Integration

The web interface is fully responsive and acts like a native mobile app:
- **Add to Home Screen** - Create app-like shortcut
- **Touch-friendly Controls** - Large buttons for easy interaction
- **Real-time Updates** - Always current status information

## 🔒 Security Features

- **Motion Detection** - Monitor entry points
- **Automatic Security Lighting** - Motion-activated illumination
- **Water Leak Alerts** - Prevent water damage
- **Temperature Monitoring** - Fire/freeze prevention

## 🌐 Network Integration

### API Endpoints
- `GET /` - Main dashboard interface
- `GET /data` - JSON sensor data
- `GET /toggle?led=X` - Toggle specific LED
- `GET /alloff` - Turn all lights OFF
- `GET /allon` - Turn all lights ON

### JSON Data Format
```json
{
  "temperature": 25.6,
  "humidity": 1250,
  "distance": 45,
  "motion": false,
  "tempAlert": false,
  "humidAlert": false,
  "leds": [true, false, true, false, true, false, true, false, true]
}
```

## 🚨 Troubleshooting

### Common Issues
1. **WiFi Connection Failed**
   - Verify SSID and password
   - Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
   - Check network range

2. **Sensors Not Reading**
   - Verify wiring connections
   - Check power supply (3.3V for sensors)
   - Test individual components

3. **LEDs Not Responding**
   - Check resistor values (220Ω)
   - Verify GPIO pin connections
   - Test LED polarity

4. **Web Interface Not Loading**
   - Check IP address in Serial Monitor
   - Ensure device is on same network
   - Try different browser

### Serial Monitor Output
Monitor for:
- WiFi connection status
- Sensor readings
- Motion detection events
- Temperature/humidity alerts
- Light control commands

## 🔮 Future Enhancements

### Planned Features
- **Schedule-based Lighting** - Time-based automation
- **Voice Control Integration** - Alexa/Google Assistant
- **Energy Usage Monitoring** - Power consumption tracking
- **Mobile Push Notifications** - Remote alert system
- **MQTT Integration** - IoT platform connectivity

### Advanced Options
- **Machine Learning** - Automated lighting based on usage patterns
- **Geo-fencing** - Location-based automation
- **Integration APIs** - Connect with other smart home systems
- **Data Logging** - Historical sensor data storage

## 📄 License
Open source project - modify and distribute freely!
