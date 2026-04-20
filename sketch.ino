/**
 * ╔══════════════════════════════════════════════════════════════╗
 * ║   SC-IES — Smart Campus Intelligent Environment System      ║
 * ║   Sensor Node Firmware — Room 101, UR-CST Block B           ║
 * ║                                                              ║
 * ║   Group Members:                                             ║
 * ║     Jabo Faid Terence    — 222004501                         ║
 * ║     Mutabaruka Martial   — 222003699                         ║
 * ║     Igiraneza Daniel     — 222006806                         ║
 * ║                                                              ║
 * ║   Course : Ubiquitous and Pervasive Computing (UPC) A2       ║
 * ║   Board  : Arduino Mega 2560 (Wokwi simulation)             ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * HOW TO USE THE SIMULATION:
 *   Green button  → Simulate an authorized RFID card scan
 *   Red button    → Simulate an unauthorized RFID card scan
 *   Knob 1 (A0)  → CO2 level (turn right = more CO2 → fan on)
 *   Knob 2 (A1)  → Smoke level (turn right past 65% → FIRE alert)
 *   Knob 3 (A2)  → Flood level (turn right past 65% → FLOOD alert)
 *   PIR sensor   → Click the orange dot to trigger motion
 *
 * WHAT EACH LED MEANS:
 *   Yellow LED  → Classroom lights (ON when room occupied)
 *   Blue LED    → Fan / AC        (ON when hot or CO2 high)
 *   Green LED   → Door lock       (briefly OFF when card accepted)
 *   Red LED     → Emergency alert (FIRE / FLOOD / Unauthorized)
 *   Buzzer      → Audio alerts
 */

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ──────────────────────────────────────────────────────────────────────
//  PIN MAP
// ──────────────────────────────────────────────────────────────────────
#define PIN_DHT         3    // DHT22 data line
#define PIN_PIR         2    // HC-SR501 motion output
#define PIN_CO2         A0   // MQ-135 (potentiometer simulation)
#define PIN_SMOKE       A1   // MQ-2   (potentiometer simulation)
#define PIN_FLOOD       A2   // FC-37  (potentiometer simulation)
#define PIN_BTN_AUTH    22   // Green button — authorized RFID card
#define PIN_BTN_UNAUTH  23   // Red button   — unauthorized RFID card
#define PIN_LIGHT       4    // Relay 1 → classroom lights  (yellow LED)
#define PIN_FAN         5    // Relay 2 → fan / AC          (blue LED)
#define PIN_DOOR        6    // Relay 3 → door lock         (green LED)
#define PIN_ALERT_LED   7    // Alert indicator             (red LED)
#define PIN_BUZZER      8    // Passive buzzer

// ──────────────────────────────────────────────────────────────────────
//  THRESHOLDS  (rule engine parameters)
// ──────────────────────────────────────────────────────────────────────
#define TEMP_FAN_ON       27.0f    // °C  — fan turns ON  above this
#define TEMP_FAN_OFF      25.0f    // °C  — fan turns OFF below this (hysteresis)
#define CO2_WARN_PPM      800      // ppm — warning, force fan ventilation
#define CO2_CRIT_PPM      1200     // ppm — critical, send alert
#define SMOKE_ALERT_PCT   65       // %   — fire alert threshold
#define FLOOD_ALERT_PCT   65       // %   — flood alert threshold
#define INACTIVITY_MS     20000UL  // ms  — 20s sim = 10min real; lights/fan off
#define SENSOR_PERIOD_MS  2000UL   // ms  — how often to read sensors

// ──────────────────────────────────────────────────────────────────────
//  OBJECTS
// ──────────────────────────────────────────────────────────────────────
DHT              dht(PIN_DHT, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ──────────────────────────────────────────────────────────────────────
//  ROOM STATE  (single source of truth for all actuator decisions)
// ──────────────────────────────────────────────────────────────────────
struct RoomState {
  bool     occupied    = false;
  bool     lights      = false;
  bool     fan         = false;
  bool     doorLocked  = true;
  bool     alertActive = false;
  String   alertType   = "";
  unsigned long lastMotion = 0;
  unsigned long lastSensor = 0;
  int      rfidIndex   = 0;
} room;

// ──────────────────────────────────────────────────────────────────────
//  SIMULATED STUDENT REGISTRY  (3 demo RFID cards)
// ──────────────────────────────────────────────────────────────────────
const char* CARD_IDS[]   = { "A1B2C3D4", "E5F6G7H8", "I9J0K1L2" };
const char* CARD_NAMES[] = { "Terence J.", "Martial K.", "Daniel I." };
const int   CARD_COUNT   = 3;


// ══════════════════════════════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  // Configure pins
  pinMode(PIN_PIR,        INPUT);
  pinMode(PIN_BTN_AUTH,   INPUT_PULLUP);
  pinMode(PIN_BTN_UNAUTH, INPUT_PULLUP);
  pinMode(PIN_LIGHT,      OUTPUT);
  pinMode(PIN_FAN,        OUTPUT);
  pinMode(PIN_DOOR,       OUTPUT);
  pinMode(PIN_ALERT_LED,  OUTPUT);
  pinMode(PIN_BUZZER,     OUTPUT);

  // Safe initial state — all off, door locked
  digitalWrite(PIN_LIGHT,     LOW);
  digitalWrite(PIN_FAN,       LOW);
  digitalWrite(PIN_DOOR,      HIGH);   // HIGH = locked (relay NC)
  digitalWrite(PIN_ALERT_LED, LOW);

  // Init sensors & display
  dht.begin();
  lcd.init();
  lcd.backlight();
  showSplash();

  // Broadcast boot event over serial (Raspberry Pi gateway reads this)
  mqttPublish("system/status",
    "{\"room\":\"101\",\"building\":\"CST\",\"node\":\"arduino-mega\","
    "\"status\":\"ONLINE\",\"sensors\":[\"PIR\",\"DHT22\",\"MQ135\",\"MQ2\",\"FC37\",\"RFID\"]}");

  Serial.println(F("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
  Serial.println(F("  SC-IES Serial Bridge — Room 101"));
  Serial.println(F("  Green BTN = RFID Authorized Scan"));
  Serial.println(F("  Red BTN   = RFID Unauthorized Scan"));
  Serial.println(F("  Knob 1    = CO2 Level  (MQ-135)"));
  Serial.println(F("  Knob 2    = Smoke Level(MQ-2)"));
  Serial.println(F("  Knob 3    = Flood Level(FC-37)"));
  Serial.println(F("  PIR dot   = Click to trigger motion"));
  Serial.println(F("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
}


// ══════════════════════════════════════════════════════════════════════
//  MAIN LOOP
// ══════════════════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  checkPIR(now);
  checkRFIDButtons();

  if (now - room.lastSensor >= SENSOR_PERIOD_MS) {
    room.lastSensor = now;
    sensorCycle();
  }
}


// ══════════════════════════════════════════════════════════════════════
//  PIR OCCUPANCY DETECTION
// ══════════════════════════════════════════════════════════════════════
void checkPIR(unsigned long now) {
  bool motion = digitalRead(PIN_PIR);

  if (motion) {
    if (!room.occupied) {
      room.occupied = true;
      mqttPublish("sensor/occupancy",
        "{\"room\":\"101\",\"occupied\":true,\"source\":\"PIR_HC-SR501\"}");
    }
    room.lastMotion = now;
  }

  // Inactivity timeout → room considered empty
  if (room.occupied && (now - room.lastMotion > INACTIVITY_MS)) {
    room.occupied = false;
    mqttPublish("sensor/occupancy",
      "{\"room\":\"101\",\"occupied\":false,\"reason\":\"inactivity_timeout\","
      "\"timeout_s\":20}");
  }
}


// ══════════════════════════════════════════════════════════════════════
//  RFID CARD SIMULATION (buttons replace physical RC522 reader)
// ══════════════════════════════════════════════════════════════════════
void checkRFIDButtons() {
  if (digitalRead(PIN_BTN_AUTH) == LOW) {
    delay(50);
    if (digitalRead(PIN_BTN_AUTH) == LOW) {
      handleRFID(true);
      while (digitalRead(PIN_BTN_AUTH) == LOW);
    }
  }
  if (digitalRead(PIN_BTN_UNAUTH) == LOW) {
    delay(50);
    if (digitalRead(PIN_BTN_UNAUTH) == LOW) {
      handleRFID(false);
      while (digitalRead(PIN_BTN_UNAUTH) == LOW);
    }
  }
}


// ══════════════════════════════════════════════════════════════════════
//  SENSOR READ + RULE ENGINE CYCLE  (every 2 seconds)
// ══════════════════════════════════════════════════════════════════════
void sensorCycle() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println(F("[WARN] DHT22 read failed"));
    return;
  }

  // Map potentiometers to real-world sensor ranges
  int co2ppm   = map(analogRead(PIN_CO2),   0, 1023, 400, 2000); // ppm
  int smokePct = map(analogRead(PIN_SMOKE), 0, 1023, 0,   100);  // %
  int floodPct = map(analogRead(PIN_FLOOD), 0, 1023, 0,   100);  // %

  // Publish environmental reading
  String env =
    "{\"room\":\"101\",\"temp\":"  + String(temp, 1) +
    ",\"hum\":"                    + String(hum,  1) +
    ",\"co2\":"                    + String(co2ppm)  +
    ",\"smoke\":"                  + String(smokePct)+
    ",\"flood\":"                  + String(floodPct)+
    ",\"occupied\":"               + (room.occupied ? "true" : "false") + "}";
  mqttPublish("sensor/environment", env);

  // Run rule engine
  applyRules(temp, co2ppm, smokePct, floodPct);

  // Update LCD (unless an alert message is showing)
  if (!room.alertActive) {
    updateLCD(temp, hum, co2ppm);
  }
}


// ══════════════════════════════════════════════════════════════════════
//  RULE ENGINE  — IF-THEN decision logic
// ══════════════════════════════════════════════════════════════════════
void applyRules(float temp, int co2, int smoke, int flood) {

  // ── PRIORITY 1: Emergency conditions (override everything) ──────────
  if (smoke >= SMOKE_ALERT_PCT) { triggerEmergency("FIRE",  smoke); return; }
  if (flood >= FLOOD_ALERT_PCT) { triggerEmergency("FLOOD", flood); return; }

  // Auto-clear emergency if levels drop back below threshold
  if (room.alertActive && smoke < SMOKE_ALERT_PCT && flood < FLOOD_ALERT_PCT) {
    clearEmergency();
  }
  if (room.alertActive) return;

  // ── RULE 1: Lights follow occupancy ─────────────────────────────────
  // IF occupancy=TRUE THEN lights=ON   |   IF occupancy=FALSE THEN lights=OFF
  setDevice(PIN_LIGHT, room.lights, room.occupied, "lights",
    room.occupied ? "occupancy_detected" : "room_empty");

  // ── RULE 2: Fan = occupied AND (temperature high OR CO2 high) ───────
  // IF occupancy=TRUE AND temp>27°C THEN fan=ON
  // IF occupancy=TRUE AND co2>800ppm THEN fan=ON (forced ventilation)
  // IF occupancy=FALSE OR (temp<25°C AND co2<800) THEN fan=OFF
  bool fanNeeded = room.occupied && (temp >= TEMP_FAN_ON || co2 >= CO2_WARN_PPM);
  bool fanOff    = !room.occupied || (temp < TEMP_FAN_OFF && co2 < CO2_WARN_PPM);

  if      (fanNeeded) setDevice(PIN_FAN, room.fan, true,  "fan",
    co2 >= CO2_WARN_PPM ? "co2_ventilation" : "temperature_control");
  else if (fanOff)    setDevice(PIN_FAN, room.fan, false, "fan", "auto_off");

  // ── RULE 3: CO2 alert levels ─────────────────────────────────────────
  if (co2 >= CO2_CRIT_PPM) {
    mqttPublish("alert/co2",
      "{\"room\":\"101\",\"ppm\":" + String(co2) + ",\"severity\":\"CRITICAL\"}");
    tone(PIN_BUZZER, 800, 200);
  } else if (co2 >= CO2_WARN_PPM) {
    mqttPublish("alert/co2",
      "{\"room\":\"101\",\"ppm\":" + String(co2) + ",\"severity\":\"WARNING\"}");
  }
}

// Helper: only toggle and publish if state actually changes (avoids MQTT spam)
void setDevice(int pin, bool &state, bool target,
               const char* device, const char* reason) {
  if (state == target) return;
  state = target;
  digitalWrite(pin, target ? HIGH : LOW);
  mqttPublish("actuator/control",
    "{\"room\":\"101\",\"device\":\"" + String(device) + "\","
    "\"state\":\""  + String(target ? "ON" : "OFF") + "\","
    "\"reason\":\"" + String(reason) + "\"}");
}


// ══════════════════════════════════════════════════════════════════════
//  EMERGENCY: FIRE / FLOOD
// ══════════════════════════════════════════════════════════════════════
void triggerEmergency(const char* type, int level) {
  if (room.alertActive && room.alertType == String(type)) return;

  room.alertActive = true;
  room.alertType   = String(type);

  digitalWrite(PIN_ALERT_LED, HIGH);
  digitalWrite(PIN_LIGHT,     HIGH);   // Lights ON for safe evacuation

  // Three-pulse alarm pattern
  for (int i = 0; i < 3; i++) {
    tone(PIN_BUZZER, 1000, 250); delay(300);
    tone(PIN_BUZZER, 1500, 150); delay(250);
  }
  noTone(PIN_BUZZER);

  mqttPublish("alert/emergency",
    "{\"room\":\"101\",\"type\":\"" + String(type) + "\","
    "\"level\":" + String(level) + ",\"severity\":\"CRITICAL\","
    "\"action\":\"EVACUATE\",\"sms_target\":\"admin\"}");

  lcdShow(String("!! ") + type + " !!", "EVACUATE NOW!");
}

void clearEmergency() {
  room.alertActive = false;
  room.alertType   = "";
  digitalWrite(PIN_ALERT_LED, LOW);
  noTone(PIN_BUZZER);
  mqttPublish("alert/emergency",
    "{\"room\":\"101\",\"type\":\"CLEAR\",\"status\":\"resolved\"}");
}


// ══════════════════════════════════════════════════════════════════════
//  RFID ATTENDANCE + ACCESS CONTROL
// ══════════════════════════════════════════════════════════════════════
void handleRFID(bool authorized) {
  if (authorized) {
    int    idx    = room.rfidIndex % CARD_COUNT;
    String cardId = String(CARD_IDS[idx]);
    String name   = String(CARD_NAMES[idx]);
    room.rfidIndex++;

    // Unlock door briefly (simulates electric strike releasing)
    room.doorLocked = false;
    digitalWrite(PIN_DOOR, LOW);

    // Double-beep = access granted
    tone(PIN_BUZZER, 1200, 80); delay(110);
    tone(PIN_BUZZER, 1600, 80); delay(110);
    noTone(PIN_BUZZER);

    lcdShow("Welcome!", name);

    // Publish attendance record (< 2s from button press)
    mqttPublish("rfid/attendance",
      "{\"room\":\"101\",\"card\":\"" + cardId + "\","
      "\"student\":\"" + name + "\",\"status\":\"GRANTED\","
      "\"uptime_ms\":" + String(millis()) + "}");

    delay(2000);
    room.doorLocked = true;
    digitalWrite(PIN_DOOR, HIGH);   // Re-lock

  } else {
    // Double long-beep = access denied
    tone(PIN_BUZZER, 400, 500); delay(600);
    tone(PIN_BUZZER, 400, 500); delay(600);
    noTone(PIN_BUZZER);

    digitalWrite(PIN_ALERT_LED, HIGH);
    lcdShow("ACCESS DENIED", "Unknown Card!");

    mqttPublish("alert/security",
      "{\"room\":\"101\",\"type\":\"UNAUTHORIZED_ACCESS\","
      "\"severity\":\"HIGH\",\"action\":\"security_notified\"}");

    delay(2500);
    if (!room.alertActive) digitalWrite(PIN_ALERT_LED, LOW);
  }
}


// ══════════════════════════════════════════════════════════════════════
//  LCD HELPERS
// ══════════════════════════════════════════════════════════════════════
void lcdShow(String top, String bot) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(top.substring(0, 16));
  lcd.setCursor(0, 1); lcd.print(bot.substring(0, 16));
}

void updateLCD(float temp, float hum, int co2) {
  // Row 0:  T:26.4C H:58%
  char tempStr[7];
  dtostrf(temp, 4, 1, tempStr);
  char row0[17];
  snprintf(row0, 17, "T:%sC H:%.0f%%", tempStr, hum);

  // Row 1:  CO2:820  O-FD   (O=occupied, L=lights, F=fan, D/U=door)
  char flags[5];
  snprintf(flags, 5, "%s%s%s%s",
    room.occupied   ? "O" : "-",
    room.lights     ? "L" : "-",
    room.fan        ? "F" : "-",
    room.doorLocked ? "D" : "U");
  char row1[17];
  snprintf(row1, 17, "CO2:%-4d %s", co2, flags);

  lcdShow(String(row0), String(row1));
}

void showSplash() {
  lcdShow("SC-IES  v1.0", "Initializing...");  delay(1200);
  lcdShow("Room 101 Online", "UR-CST Block B"); delay(1200);
  lcdShow("Sensors Ready", "MQTT Bridge ON");   delay(1000);
}


// ══════════════════════════════════════════════════════════════════════
//  MQTT BRIDGE  — Serial output parsed by Raspberry Pi Python gateway
// ══════════════════════════════════════════════════════════════════════
void mqttPublish(const char* topic, String payload) {
  Serial.print(F("MQTT >> campus/CST/room101/"));
  Serial.println(topic);
  Serial.print(F("       "));
  Serial.println(payload);
  Serial.println();
}
