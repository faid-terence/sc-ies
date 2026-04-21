# SC-IES Demo Guide — Full Presentation Script

---

## Before You Start (Setup)

**Open two windows side by side on the projector:**
- **Left:** VS Code with Wokwi simulation running
- **Right:** Browser at `http://localhost:5000` (the dashboard)

**Terminal — start the dashboard:**
```bash
cd /home/terence-faid/SC-IES-Wokwi/dashboard && python3 app.py
```

**VS Code — start Wokwi:**
```
F1 → Wokwi: Start Simulator → press ▶ Play
```

---

## Opening Statement (30 seconds)

> *"Good morning. Our project is the Smart Campus Intelligent Environment System — SC-IES. It addresses four real problems at UR-CST: classroom lights and fans left on in empty rooms wasting energy, manual paper-based attendance, no automated environmental monitoring, and no real-time security alerting. We built a fully simulated prototype using an Arduino Mega as the sensor node, connected through MQTT to a live web dashboard. Everything you are about to see is running in real time."*

**Point to the screen:**
- Left = the hardware layer (Arduino circuit in Wokwi)
- Right = the software layer (dashboard receiving data)

---

## Scenario 1 — Room Occupancy & Automatic Lighting (2 minutes)

**What to say:**
> *"The first feature is occupancy-based automation. Right now the room is empty — notice the lights and fan are both OFF on the dashboard, and the Occupancy card says EMPTY. Watch what happens when someone enters the room."*

**What to click:**
1. On the **dashboard** → press the green **"🚶 Toggle PIR Motion"** button
2. On **Wokwi** → click the **orange dot** on the PIR sensor

**What to point out:**
- Occupancy card turns **green → OCCUPIED**
- **💡 Lights badge** flips to **ON**
- MQTT log shows: `sensor/occupancy {"occupied":true}` then `actuator/control {"device":"lights","state":"ON"}`

**What to say:**
> *"The PIR sensor detected motion. The rule engine immediately fired: IF occupancy = TRUE THEN lights = ON. This is Layer 4 — the decision layer — processing the event from Layer 1 in real time. No human touched a switch."*

**Then say:**
> *"Now watch what happens when the room goes empty. I'll toggle the PIR off and we wait 20 seconds — in our simulation that represents the 10-minute inactivity timeout from the proposal."*

**What to click:**
- Press **"🚶 Toggle PIR Motion"** again to stop motion

**Wait 20 seconds, then point out:**
- Occupancy → **EMPTY**
- Lights → **OFF**
- Fan → **OFF**
- MQTT log shows: `sensor/occupancy {"occupied":false,"reason":"inactivity_timeout"}`

**What to say:**
> *"Lights and fan shut off automatically. No human intervention. This is the energy saving we measured — look at the energy chart on the right."*

---

## Scenario 2 — Temperature & CO₂ Ventilation (2 minutes)

**What to say:**
> *"SC-IES also monitors the environment continuously. Every 2 seconds the DHT22 sends temperature and humidity readings. The fan doesn't just respond to occupancy — it also responds to air quality."*

**First — trigger occupancy so the fan can activate:**
- Press **"🚶 Toggle PIR Motion"** to make the room occupied

**What to say:**
> *"The room is now occupied. Notice the CO₂ is around 400–500 ppm — normal fresh air. Watch what happens as people stay in the room. CO₂ naturally rises."*

**What to point at:**
- The CO₂ card drifting upward every 2 seconds
- When it hits **800 ppm** → fan turns ON, CO₂ card turns **yellow**
- MQTT log shows: `actuator/control {"device":"fan","state":"ON","reason":"co2_ventilation"}`

**What to say:**
> *"At 800 ppm the rule engine forces the fan on for ventilation, even if the temperature is comfortable. At 1200 ppm it escalates to a critical alert. This protects student health — high CO₂ causes drowsiness and poor concentration, which is a real problem in closed classrooms."*

---

## Scenario 3 — RFID Smart Attendance (2 minutes)

**What to say:**
> *"Traditional attendance at UR-CST is paper-based. A lecturer calls names, students sign sheets — it takes 10 minutes and the records can be lost. Our RFID system logs attendance in under 2 seconds."*

**What to click:**
1. Press **"🪪 RFID — Authorized"** button three times slowly

**What to point out after each click:**
- The **Attendance Log** table — a new row slides in each time
- Shows: time, student name, card ID, **✓ GRANTED** in green
- **Green LED** in Wokwi briefly turns off (door unlocks) then back on
- MQTT log: `rfid/attendance {"student":"Terence J.","card":"A1B2C3D4","status":"GRANTED"}`

**What to say:**
> *"Each scan is logged instantly with timestamp, student name, and card ID. This goes into the MySQL database in the full system. Lecturers can pull an attendance report for any date and course from the dashboard — no paper needed."*

**Now demonstrate unauthorized access:**
1. Press **"🚫 RFID — Unauthorized"** button

**What to point out:**
- Alert panel turns **yellow → UNAUTHORIZED ACCESS**
- Red LED lights up in Wokwi
- MQTT log: `alert/security {"type":"UNAUTHORIZED_ACCESS","severity":"HIGH"}`

**What to say:**
> *"An unknown card was scanned. The system immediately raises a security alert and notifies the security team. In the full deployment this sends an SMS via Twilio API to the administrator."*

---

## Scenario 4 — Fire Emergency (2 minutes)

**What to say:**
> *"Now the most critical feature — emergency detection. The MQ-2 smoke sensor monitors air quality continuously. I'm going to simulate a fire by raising the smoke level."*

**What to click:**
1. Slowly drag the **🔥 Smoke slider** to the right
2. Stop at around **70–80%**

**What to point out immediately:**
- Alert panel **flashes red** with 🔥 **FIRE ALERT — EVACUATE NOW**
- Lights turn **ON** (for safe evacuation)
- Buzzer sounds in Wokwi
- Red LED activates in Wokwi
- MQTT log: `alert/emergency {"type":"FIRE","level":75,"severity":"CRITICAL","action":"EVACUATE"}`

**What to say:**
> *"Within 2 seconds of the smoke threshold being breached, the system has: triggered a visual alert on every dashboard screen, turned on the lights so people can evacuate safely, activated the buzzer on the Arduino for local audio warning, and published a CRITICAL MQTT message that would dispatch an SMS to the administrator. Our target was 5 seconds — we achieved under 2."*

**Then clear it:**
1. Drag the smoke slider back to 0
2. Press **"✅ Clear Alert"**

**What to say:**
> *"When the hazard clears, the system resolves the alert automatically."*

---

## Scenario 5 — Flood Emergency (1 minute)

**What to say:**
> *"The same system handles flooding — important for ground-floor labs. The FC-37 sensor detects water presence."*

**What to click:**
1. Drag the **💧 Flood slider** to **75%**

**What to point out:**
- Alert panel turns **blue → 💧 FLOOD ALERT**
- Same emergency chain fires

**What to say:**
> *"Same response chain — alert, lights on, buzzer, MQTT critical event, SMS to admin."*

**Clear it:**
- Drag flood slider to 0 → press **"✅ Clear Alert"**

---

## Scenario 6 — Energy Savings Comparison (1 minute)

**What to say:**
> *"One of our secondary objectives was to quantify energy savings. Look at the energy chart."*

**What to point at:**
- **Red bar** = Always-On baseline (lights + fan running 100% of the time)
- **Green bar** = SC-IES (only consuming when the room is actually occupied)
- **Blue savings %** — growing over time

**What to say:**
> *"In a typical university day, classrooms are empty roughly 60% of the time. SC-IES saves all that wasted energy automatically. The longer the simulation runs, the bigger the gap. In a full 8-hour day simulation this reaches 55–65% energy savings — directly measurable on this chart."*

---

## Scenario 7 — MQTT Architecture Proof (30 seconds)

**What to say:**
> *"Finally — the MQTT log at the bottom of the dashboard shows every message flowing through the system. Each message follows the topic hierarchy from our proposal: campus/CST/room101/sensor, actuator, alert, rfid. In the full deployment the Raspberry Pi gateway subscribes to all these topics and routes them to InfluxDB for time-series storage and MySQL for attendance records. The Arduino serial output you see in Wokwi is exactly what the Raspberry Pi would read over USB."*

**What to point at:**
- The MQTT log scrolling with real JSON payloads
- Wokwi serial monitor showing the same messages

---

## Closing Statement (30 seconds)

> *"To summarize — SC-IES demonstrates all five layers of a ubiquitous computing system working together: physical sensors detecting the environment, a device layer making real-time decisions, MQTT communication between nodes, an application layer applying configurable rules, and a user interface showing everything live. It addresses real problems at UR-CST using open-source, low-cost hardware and software. Thank you."*

---

## Quick Reference Card

> Keep this open on your phone during the presentation.

| Button | What it demos |
|---|---|
| 🚶 Toggle PIR | Occupancy → lights/fan auto on/off |
| 🪪 RFID Authorized | Attendance logging, door unlock |
| 🚫 RFID Unauthorized | Security alert |
| 🔥 Smoke slider → 70% | Fire emergency chain |
| 💧 Flood slider → 70% | Flood emergency chain |
| ✅ Clear Alert | Hazard resolved |
| Watch CO₂ drift up | Ventilation rule fires at 800 ppm |
| Energy chart | Savings growing over time |

**Total demo time: ~10 minutes**
