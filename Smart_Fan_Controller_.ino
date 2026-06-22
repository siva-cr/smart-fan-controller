#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>

// --- Hardware Pins ---
#define DHTPIN D4
#define DHTTYPE DHT11
#define RELAY_PIN D5
#define TRIG_PIN D7
#define ECHO_PIN D8

// --- WiFi Credentials ---
const char* ssid = "FLOWER HOUSE";
const char* password = "AMMA@1234";

// --- System States ---
enum ControlMode { MODE_AUTO, MODE_FORCE_ON, MODE_FORCE_OFF };
ControlMode currentMode = MODE_AUTO;

enum OccupancyState { OCC_NO, OCC_YES, OCC_RECENTLY_SEEN };
OccupancyState personStatus = OCC_NO;

// --- Global Variables ---
float currentTemperature = 0.0;
float currentHumidity = 0.0;
long currentDistance = 0;
bool fanState = false;

float maxTemp = -999.0;
float minTemp = 999.0;

// --- Timers and Thresholds ---
unsigned long lastSeenMillis = 0;
const unsigned long OCCUPANCY_TIMEOUT = 120000; // 2 minutes in milliseconds
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 1000;     // Read sensors every 1 second

// --- Web Server & DHT Instances ---
ESP8266WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

// --- Function Declarations ---
void readSensors();
void updateFanLogic();
void handleRoot();
void handleSetMode();

void setup() {
  Serial.begin(115200);
  Serial.println("\nInitializing Smart Fan Controller...");

  // Initialize Pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Ensure fan is off initially
  digitalWrite(RELAY_PIN, LOW);

  // Initialize DHT Sensor
  dht.begin();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Setup Web Server Routes
  server.on("/", handleRoot);
  server.on("/setMode", handleSetMode);
  server.begin();
  Serial.println("HTTP Server Started.");
}

void loop() {
  server.handleClient();

  // Periodic sensor reading and logic updates
  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentMillis;
    readSensors();
    updateFanLogic();
  }
}

// --- Sensor Processing Logic ---
void readSensors() {
  // 1. Read DHT11 Temperature & Humidity
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) {
    currentHumidity = h;
    currentTemperature = t;

    // Track Min/Max Temperatures
    if (currentTemperature > maxTemp) maxTemp = currentTemperature;
    if (currentTemperature < minTemp) minTemp = currentTemperature;
  }

  // 2. Read HC-SR04 Ultrasonic Distance Sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout (~5m max range)
  
  if (duration == 0) {
    currentDistance = 999; // Out of range/no echo
  } else {
    currentDistance = duration * 0.034 / 2; // Calculate distance in cm
  }

  // 3. Process Occupancy State Machine
  if (currentDistance < 150) {
    personStatus = OCC_YES;
    lastSeenMillis = millis(); // Reset/update the timer
  } else {
    if (personStatus == OCC_YES || personStatus == OCC_RECENTLY_SEEN) {
      if (millis() - lastSeenMillis < OCCUPANCY_TIMEOUT) {
        personStatus = OCC_RECENTLY_SEEN;
      } else {
        personStatus = OCC_NO;
      }
    }
  }
}

// --- Automation Control Core ---
void updateFanLogic() {
  if (currentMode == MODE_FORCE_ON) {
    fanState = true;
  } 
  else if (currentMode == MODE_FORCE_OFF) {
    fanState = false;
  } 
  else { // MODE_AUTO
    bool personPresent = (personStatus == OCC_YES || personStatus == OCC_RECENTLY_SEEN);

    if (personPresent && currentTemperature > 30.0) {
      fanState = true;
    } 
    else if (currentTemperature < 28.0) {
      fanState = false;
    } 
    else if (!personPresent) {
      fanState = false;
    }
  }

  // Apply state to the active-high relay
  digitalWrite(RELAY_PIN, fanState ? HIGH : LOW);
}

// --- Web Server Handlers ---
void handleRoot() {
  // Convert enum states to printable strings for HTML delivery
  String modeStr = (currentMode == MODE_AUTO) ? "AUTO" : ((currentMode == MODE_FORCE_ON) ? "FORCE ON" : "FORCE OFF");
  String statusStr = (personStatus == OCC_YES) ? "YES" : ((personStatus == OCC_RECENTLY_SEEN) ? "RECENTLY SEEN" : "NO");
  String fanStr = fanState ? "ON" : "OFF";

  // Build the dynamic Responsive CSS Dark Web UI
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta http-equiv='refresh' content='3'>"; // Auto-refresh string every 3 seconds
  html += "<title>Smart Fan Controller</title>";
  html += "<style>";
  html += "body { font-family: -apple-system, sans-serif; background-color: #121212; color: #e0e0e0; text-align: center; margin:0; padding:20px; }";
  html += "h1 { color: #ffffff; margin-bottom: 20px; font-size: 1.8rem; }";
  html += ".container { max-width: 600px; margin: 0 auto; }";
  html += ".grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; margin-bottom: 25px; }";
  html += ".card { background: #1e1e1e; padding: 15px; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); border: 1px solid #2d2d2d; }";
  html += ".card-full { grid-column: span 2; }";
  html += ".label { font-size: 0.85rem; color: #aaaaaa; text-transform: uppercase; letter-spacing: 0.5px; }";
  html += ".value { font-size: 1.4rem; font-weight: bold; margin-top: 5px; color: #00adb5; }";
  html += ".btn-group { display: flex; flex-direction: column; gap: 10px; margin-top: 15px; }";
  html += ".btn { padding: 12px; font-size: 1rem; font-weight: bold; border: none; border-radius: 8px; cursor: pointer; transition: 0.2s; color: white; text-decoration: none; text-align: center; }";
  html += ".btn-auto { background-color: #2b7a78; } .btn-on { background-color: #388e3c; } .btn-off { background-color: #d32f2f; }";
  html += ".btn:hover { opacity: 0.9; transform: scale(0.99); }";
  html += ".highlight-on { color: #4caf50 !important; } .highlight-off { color: #f44336 !important; }";
  html += "</style></head><body>";

  html += "<div class='container'>";
  
  // Emoji removed from the line below
  html += "<h1>Smart Fan Controller</h1>";
  
  html += "<div class='grid'>";
  
  // Row 1: Temp & Humidity
  html += "<div class='card'><div class='label'>Temperature</div><div class='value'>" + String(currentTemperature, 1) + " °C</div></div>";
  html += "<div class='card'><div class='label'>Humidity</div><div class='value'>" + String(currentHumidity, 1) + " %</div></div>";
  
  // Row 2: Distance & Occupancy
  html += "<div class='card'><div class='label'>Distance</div><div class='value'>" + String(currentDistance) + " cm</div></div>";
  html += "<div class='card'><div class='label'>Person Present</div><div class='value'>" + statusStr + "</div></div>";
  
  // Row 3: Fan status & Mode
  html += "<div class='card'><div class='label'>Fan Status</div><div class='value " + String(fanState ? "highlight-on" : "highlight-off") + "'>" + fanStr + "</div></div>";
  html += "<div class='card'><div class='label'>Active Mode</div><div class='value'>" + modeStr + "</div></div>";

  // Row 4: Min/Max Record Bounds
  html += "<div class='card'><div class='label'>Min Temperature</div><div class='value' style='color:#64b5f6;'>" + (minTemp == 999.0 ? "--" : String(minTemp, 1) + " °C") + "</div></div>";
  html += "<div class='card'><div class='label'>Max Temperature</div><div class='value' style='color:#e57373;'>" + (maxTemp == -999.0 ? "--" : String(maxTemp, 1) + " °C") + "</div></div>";

  // Control Center Interface Card
  html += "<div class='card card-full'>";
  html += "<div class='label' style='margin-bottom:10px;'>System Overrides</div>";
  html += "<div class='btn-group'>";
  html += "<a href='/setMode?m=auto' class='btn btn-auto'>AUTO MODE</a>";
  html += "<a href='/setMode?m=on' class='btn btn-on'>FORCE ON</a>";
  html += "<a href='/setMode?m=off' class='btn btn-off'>FORCE OFF</a>";
  html += "</div></div>";

  html += "</div></div></body></html>";

  server.send(200, "text/html", html);
}

void handleSetMode() {
  // Read URL query arguments to swap operational modes
  if (server.hasArg("m")) {
    String modeArg = server.arg("m");
    if (modeArg == "auto") currentMode = MODE_AUTO;
    else if (modeArg == "on") currentMode = MODE_FORCE_ON;
    else if (modeArg == "off") currentMode = MODE_FORCE_OFF;
    
    // Execute logic state instantly on input changes
    updateFanLogic();
  }
  
  // Redirect back to root dashboard immediately
  server.sendHeader("Location", "/");
  server.send(303);
}