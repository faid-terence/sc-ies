"""
SC-IES Dashboard — Flask + SocketIO backend
Simulates sensor data and broadcasts real-time updates to the browser.
"""

from flask import Flask, render_template
from flask_socketio import SocketIO, emit
from datetime import datetime
import threading
import random
import time

app = Flask(__name__)
app.config["SECRET_KEY"] = "sc-ies-ur-cst"
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")

# ─── Room State ───────────────────────────────────────────────────────
state = {
    "occupied":         False,
    "temp":             24.5,
    "hum":              55.0,
    "co2":              480,
    "smoke":            4,
    "flood":            2,
    "lights":           False,
    "fan":              False,
    "door_locked":      True,
    "alert":            None,
    "attendance":       [],
    "mqtt_log":         [],
    "energy_always_on": 0.0,
    "energy_sc_ies":    0.0,
    "uptime":           0,
}

STUDENTS = [
    {"card": "A1B2C3D4", "name": "Terence J."},
    {"card": "E5F6G7H8", "name": "Martial K."},
    {"card": "I9J0K1L2", "name": "Daniel I."},
]
rfid_index = 0
lock = threading.Lock()


# ─── Helpers ──────────────────────────────────────────────────────────
def log_mqtt(topic, payload):
    state["mqtt_log"].insert(0, {
        "time":    datetime.now().strftime("%H:%M:%S"),
        "topic":   f"campus/CST/room101/{topic}",
        "payload": payload,
    })
    state["mqtt_log"] = state["mqtt_log"][:10]


def apply_rules():
    prev_lights = state["lights"]
    prev_fan    = state["fan"]

    # RULE 1 — lights follow occupancy
    state["lights"] = state["occupied"]

    # RULE 2 — fan on when hot or CO2 high
    if state["occupied"] and (state["temp"] > 27.0 or state["co2"] > 800):
        state["fan"] = True
    elif not state["occupied"] or (state["temp"] < 25.0 and state["co2"] < 800):
        state["fan"] = False

    if state["lights"] != prev_lights:
        log_mqtt("actuator/control",
            f'{{"device":"lights","state":"{"ON" if state["lights"] else "OFF"}","reason":"occupancy"}}')
    if state["fan"] != prev_fan:
        reason = "co2_ventilation" if state["co2"] > 800 else (
                 "temperature_control" if state["temp"] > 27 else "auto_off")
        log_mqtt("actuator/control",
            f'{{"device":"fan","state":"{"ON" if state["fan"] else "OFF"}","reason":"{reason}"}}')

    # RULE 3 — emergencies
    if state["smoke"] > 65:
        if not state["alert"] or state["alert"]["type"] != "FIRE":
            state["alert"] = {"type": "FIRE", "severity": "CRITICAL", "level": state["smoke"]}
            log_mqtt("alert/emergency",
                f'{{"type":"FIRE","level":{state["smoke"]},"severity":"CRITICAL","action":"EVACUATE"}}')
    elif state["flood"] > 65:
        if not state["alert"] or state["alert"]["type"] != "FLOOD":
            state["alert"] = {"type": "FLOOD", "severity": "CRITICAL", "level": state["flood"]}
            log_mqtt("alert/emergency",
                f'{{"type":"FLOOD","level":{state["flood"]},"severity":"CRITICAL","action":"EVACUATE"}}')
    elif state["alert"] and state["alert"]["type"] in ("FIRE", "FLOOD"):
        if state["smoke"] < 65 and state["flood"] < 65:
            state["alert"] = None
            log_mqtt("alert/emergency", '{"type":"CLEAR","status":"resolved"}')


# ─── Sensor simulation loop (background thread) ───────────────────────
def sensor_loop():
    while True:
        with lock:
            state["uptime"] += 2

            # Drift environmental readings
            state["temp"] = round(
                max(18.0, min(40.0, state["temp"] + random.uniform(-0.3, 0.3))), 1)
            state["hum"] = round(
                max(30.0, min(90.0, state["hum"]  + random.uniform(-0.5, 0.5))), 1)

            # CO2 climbs when occupied, drops when empty
            delta = random.randint(5, 20) if state["occupied"] else random.randint(-15, -2)
            state["co2"] = max(400, min(2000, state["co2"] + delta))

            # Energy accumulation (Wh)
            # Always-on baseline = 50 W (lights + fan running non-stop)
            state["energy_always_on"] = round(state["energy_always_on"] + 50 * (2/3600), 3)
            # SC-IES only consumes when devices are actually on
            if state["lights"]:
                state["energy_sc_ies"] = round(state["energy_sc_ies"] + 30 * (2/3600), 3)
            if state["fan"]:
                state["energy_sc_ies"] = round(state["energy_sc_ies"] + 20 * (2/3600), 3)

            apply_rules()

            log_mqtt("sensor/environment",
                f'{{"temp":{state["temp"]},"hum":{state["hum"]},'
                f'"co2":{state["co2"]},"smoke":{state["smoke"]},'
                f'"flood":{state["flood"]},"occupied":{str(state["occupied"]).lower()}}}')

            socketio.emit("state_update", state)

        time.sleep(2)


# ─── Routes ───────────────────────────────────────────────────────────
@app.route("/")
def index():
    return render_template("index.html")


# ─── Socket events (demo controls) ────────────────────────────────────
@socketio.on("connect")
def on_connect():
    emit("state_update", state)


@socketio.on("toggle_occupancy")
def toggle_occupancy():
    with lock:
        state["occupied"] = not state["occupied"]
        apply_rules()
        log_mqtt("sensor/occupancy",
            f'{{"occupied":{str(state["occupied"]).lower()},"source":"PIR_HC-SR501"}}')
        socketio.emit("state_update", state)


@socketio.on("rfid_scan")
def rfid_scan(data):
    global rfid_index
    with lock:
        authorized = data.get("authorized", True)
        if authorized:
            student = STUDENTS[rfid_index % len(STUDENTS)]
            rfid_index += 1
            entry = {
                "time":   datetime.now().strftime("%H:%M:%S"),
                "card":   student["card"],
                "name":   student["name"],
                "status": "GRANTED",
            }
            log_mqtt("rfid/attendance",
                f'{{"student":"{student["name"]}","card":"{student["card"]}","status":"GRANTED"}}')

            # Unlock door and emit immediately so dashboard shows UNLOCKED
            state["door_locked"] = False
            socketio.emit("state_update", state)

            # Re-lock after 5 seconds in a background thread (non-blocking)
            def relock():
                time.sleep(5)
                with lock:
                    state["door_locked"] = True
                    log_mqtt("actuator/control",
                        '{"device":"door","state":"LOCKED","reason":"auto_relock"}')
                    socketio.emit("state_update", state)
            threading.Thread(target=relock, daemon=True).start()

        else:
            entry = {
                "time":   datetime.now().strftime("%H:%M:%S"),
                "card":   "UNKNOWN",
                "name":   "Unknown",
                "status": "DENIED",
            }
            state["alert"] = {"type": "UNAUTHORIZED_ACCESS", "severity": "HIGH", "level": 0}
            log_mqtt("alert/security",
                '{"type":"UNAUTHORIZED_ACCESS","severity":"HIGH","action":"security_notified"}')

        state["attendance"].insert(0, entry)
        state["attendance"] = state["attendance"][:8]
        socketio.emit("state_update", state)


@socketio.on("set_smoke")
def set_smoke(data):
    with lock:
        state["smoke"] = int(data.get("value", 5))
        apply_rules()
        socketio.emit("state_update", state)


@socketio.on("set_flood")
def set_flood(data):
    with lock:
        state["flood"] = int(data.get("value", 3))
        apply_rules()
        socketio.emit("state_update", state)


@socketio.on("clear_alert")
def clear_alert():
    with lock:
        state["alert"] = None
        state["smoke"] = 4
        state["flood"] = 2
        log_mqtt("alert/emergency", '{"type":"CLEAR","status":"resolved"}')
        socketio.emit("state_update", state)


# ─── Entry point ──────────────────────────────────────────────────────
if __name__ == "__main__":
    t = threading.Thread(target=sensor_loop, daemon=True)
    t.start()
    print("\n  ┌──────────────────────────────────────────┐")
    print("  │  SC-IES Dashboard                        │")
    print("  │  http://localhost:5000                   │")
    print("  └──────────────────────────────────────────┘\n")
    socketio.run(app, host="0.0.0.0", port=5000,
                 debug=False, allow_unsafe_werkzeug=True)
