// ----------------------------------------------------
// Pet Feeder Automation System: ESP32 Code
// Microcontroller: ESP32/ESP8266
// ----------------------------------------------------

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <TimeLib.h> // For time management

// --- Configuration ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Pin Definitions
const int SERVO_PIN = 18;  // Servo Motor pin (dispenser mechanism)
const int TRIG_PIN = 5;    // Ultrasonic Sensor Trig pin
const int ECHO_PIN = 17;   // Ultrasonic Sensor Echo pin

// Servo positions
const int DISPENSER_CLOSED_POS = 0;   // Angle for dispenser closed (0 degrees)
const int DISPENSER_OPEN_POS = 120;   // Angle for dispenser open (120 degrees)
const int DISPENSE_DURATION = 1000;   // Time the servo stays open (in ms)

// Bowl Level Configuration
const float BOWL_FULL_DISTANCE_CM = 5.0;  // Distance when bowl is full (cm)
const float BOWL_EMPTY_DISTANCE_CM = 15.0; // Distance when bowl is empty (cm)
const int AUTO_FEED_THRESHOLD = 20;       // Auto-feed if bowl level is below this %

// Scheduling variables (up to 3 daily feeding times)
// Hours (0-23) and Minutes (0-59). -1 means disabled.
int feedSchedule[3][2] = {
  {7, 0},   // 07:00 AM
  {13, 0},  // 01:00 PM
  {20, 0}   // 08:00 PM
};

// State Variables
unsigned long lastFeedTime[3] = {0, 0, 0}; // Track last feed time for each schedule
unsigned long lastEmptyFeedTime = 0; // Track last auto-feed time from empty bowl
const unsigned long MIN_INTERVAL_MS = 60 * 60 * 1000; // Min 1 hour between "empty bowl" feeds

// Hardware Objects
Servo dispenserServo;
WebServer server(80);

// --- Sensor Reading Functions ---

/**
 * Reads the distance from the ultrasonic sensor.
 * Returns the distance in centimeters.
 */
float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distanceCm = duration * 0.0343 / 2;
  return distanceCm;
}

/**
 * Calculates the bowl level percentage.
 * Returns level as an integer 0-100.
 */
int calculateBowlLevel() {
  float distanceCm = readDistanceCm();
  if (distanceCm < 0 || distanceCm > BOWL_EMPTY_DISTANCE_CM * 2) {
    return -1; // Indicate error
  }
  
  // The bowl level is inversely proportional to the distance.
  // When distance is BOWL_FULL_DISTANCE_CM, level is 100%.
  // When distance is BOWL_EMPTY_DISTANCE_CM, level is 0%.
  float distanceRange = BOWL_EMPTY_DISTANCE_CM - BOWL_FULL_DISTANCE_CM;
  float currentLevel = BOWL_EMPTY_DISTANCE_CM - distanceCm;
  
  int levelPercent = map(currentLevel * 100, 0, distanceRange * 100, 0, 100);

  return constrain(levelPercent, 0, 100);
}

// --- Actuator/Control Functions ---

void dispenseFood() {
  if (dispenserServo.read() != DISPENSER_CLOSED_POS) {
      // Don't dispense if it's currently open or moving
      Serial.println("Dispense skipped: Servo is currently active.");
      return; 
  }

  Serial.println("--- Dispensing Food! ---");
  dispenserServo.write(DISPENSER_OPEN_POS);
  delay(DISPENSE_DURATION);
  dispenserServo.write(DISPENSER_CLOSED_POS);
  Serial.println("Dispensing complete.");
}

/**
 * Core scheduling and auto-feed logic.
 */
void checkScheduleAndBowl() {
  time_t currentTime = now();
  int currentHour = hour(currentTime);
  int currentMinute = minute(currentTime);

  // 1. Check Scheduled Feeds
  for (int i = 0; i < 3; i++) {
    int targetHour = feedSchedule[i][0];
    int targetMinute = feedSchedule[i][1];

    if (targetHour != -1) { // Check if schedule is enabled
      // Check if current time matches the schedule and hasn't fed yet today
      if (currentHour == targetHour && currentMinute == targetMinute && lastFeedTime[i] < now() - 23 * 3600) {
        dispenseFood();
        lastFeedTime[i] = now(); // Update last feed time
        Serial.printf("Scheduled Feed %d triggered at %02d:%02d.\n", i + 1, targetHour, targetMinute);
      }
    }
  }

  // 2. Check Auto-Feed (Empty Bowl)
  int bowlLevel = calculateBowlLevel();
  unsigned long currentMillis = millis();

  if (bowlLevel != -1 && bowlLevel <= AUTO_FEED_THRESHOLD) {
    // Check if enough time has passed since the last "empty bowl" feed
    if (currentMillis - lastEmptyFeedTime >= MIN_INTERVAL_MS) {
      dispenseFood();
      lastEmptyFeedTime = currentMillis;
      Serial.printf("Auto Feed triggered: Bowl level is %d%%.\n", bowlLevel);
    } else {
      Serial.println("Bowl is empty, but waiting for minimum interval to pass.");
    }
  }
}

// --- Web Server Handlers ---

String getPage(int bowlLevel) {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="refresh" content="10"> 
<title>Pet Feeder</title>
<style>
body { font-family: 'Tahoma', sans-serif; background: #fffde7; margin: 0; padding: 20px;}
.container { max-width: 550px; margin: auto; background: #fff; padding: 30px; border-radius: 10px; box-shadow: 0 6px 15px rgba(0,0,0,0.1); }
h1 { color: #ff6f00; text-align: center; margin-bottom: 25px; }
.level-gauge { background: #e0e0e0; border: 1px solid #ccc; height: 150px; position: relative; border-radius: 8px; margin-bottom: 25px; overflow: hidden; }
.food-fill { position: absolute; bottom: 0; left: 0; width: 100%; background: #ffb300; transition: height 0.5s; }
.level-text { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); font-size: 2.0em; font-weight: bold; color: #4e342e; text-shadow: 1px 1px 2px white; z-index: 10; }
.status { padding: 10px; border-radius: 6px; margin-bottom: 20px; text-align: center; font-weight: bold; font-size: 1.1em; color: #4e342e; background-color: #ffe082; }
form { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 20px; }
.schedule-group { grid-column: 1 / 3; display: grid; grid-template-columns: 1fr 1fr 0.1fr; gap: 5px; align-items: center; border: 1px solid #eee; padding: 10px; border-radius: 5px; margin-bottom: 10px; }
label { font-weight: bold; font-size: 0.9em; }
input[type="number"] { padding: 8px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
.control button { width: 100%; padding: 10px; background-color: #4caf50; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 1em; }
.control button:hover { background-color: #388e3c; }
</style>
<script>
function sendDispense() {
    fetch('/dispense', { method: 'POST' })
        .then(() => alert('Manual dispense initiated!'));
}
</script>
</head>
<body>
<div class="container">
<h1>Smart Pet Feeder 🐾</h1>

<div class="level-gauge">
    <div class="food-fill" style="height: %d%%;"></div>
    <div class="level-text">%d%%</div>
</div>
<div class="status">
    Current Time: %02d:%02d:%02d (Auto-Feed below %d%%)
</div>

<h2>Set Daily Schedule (24hr)</h2>
<form action="/set" method="POST">
    <label>Schedule 1:</label>
    <div class="schedule-group">
        <input type="number" name="h1" min="-1" max="23" value="%d">
        <input type="number" name="m1" min="0" max="59" value="%d">
        <span>:</span>
    </div>

    <label>Schedule 2:</label>
    <div class="schedule-group">
        <input type="number" name="h2" min="-1" max="23" value="%d">
        <input type="number" name="m2" min="0" max="59" value="%d">
        <span>:</span>
    </div>

    <label>Schedule 3:</label>
    <div class="schedule-group">
        <input type="number" name="h3" min="-1" max="23" value="%d">
        <input type="number" name="m3" min="0" max="59" value="%d">
        <span>:</span>
    </div>
    
    <button type="submit" style="grid-column: 1 / 3;">Update Schedule</button>
</form>

<h2>Manual Control</h2>
<div class="control">
    <button onclick="sendDispense()">Dispense Now!</button>
</div>

<p style="font-size: 0.8em; margin-top: 20px;">Use -1 for the hour to disable a schedule slot.</p>

</div>
</body>
</html>
)rawliteral";

  time_t currentTime = now();
  
  // Format the HTML content
  return String::format(
    html.c_str(),
    bowlLevel,
    bowlLevel,
    hour(currentTime),
    minute(currentTime),
    second(currentTime),
    AUTO_FEED_THRESHOLD,
    feedSchedule[0][0], feedSchedule[0][1],
    feedSchedule[1][0], feedSchedule[1][1],
    feedSchedule[2][0], feedSchedule[2][1]
  );
}

// Handler for the root page ("/")
void handleRoot() {
  int level = calculateBowlLevel();
  // Handle case where ultrasonic reading fails
  if (level == -1) level = 50; 
  server.send(200, "text/html", getPage(level));
}

// Handler for manual dispense
void handleDispense() {
  dispenseFood();
  server.send(200, "text/plain", "Dispense command received.");
}

// Handler for updating the schedule
void handleSet() {
  if (server.hasArg("h1")) {
    feedSchedule[0][0] = server.arg("h1").toInt();
    feedSchedule[0][1] = server.arg("m1").toInt();
    feedSchedule[1][0] = server.arg("h2").toInt();
    feedSchedule[1][1] = server.arg("m2").toInt();
    feedSchedule[2][0] = server.arg("h3").toInt();
    feedSchedule[2][1] = server.arg("m3").toInt();
    
    // Reset last feed times to ensure schedule triggers if a time has passed
    for (int i=0; i<3; i++) lastFeedTime[i] = 0; 

    Serial.println("Schedule Updated.");
    // Redirect back to the main page
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(400, "text/plain", "Bad Request: Missing parameters");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Pin setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Servo setup
  dispenserServo.attach(SERVO_PIN);
  dispenserServo.write(DISPENSER_CLOSED_POS); // Start closed

  // --- WiFi Connection ---
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // --- Time Synchronization (NTP for accurate time) ---
  // 3600 for +1 hr, 3600 for DST offset. Adjust for your timezone
  configTime(3600, 0, "pool.ntp.org"); 
  while (!timeStatus()) {
    Serial.print("Waiting for NTP sync...");
    delay(500);
  }
  Serial.println("Time synchronized.");
  
  // --- Web Server Initialization ---
  server.on("/", HTTP_GET, handleRoot);
  server.on("/set", HTTP_POST, handleSet);
  server.on("/dispense", HTTP_POST, handleDispense);
  server.begin();
  Serial.println("HTTP Server started.");
}

void loop() {
  server.handleClient(); // Handle incoming web requests
  checkScheduleAndBowl(); // Run the feeding logic
  delay(1000); // Check every second
}
