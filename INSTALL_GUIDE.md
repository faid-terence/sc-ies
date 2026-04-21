# Installation Guide — VS Code, PlatformIO & Wokwi

Complete setup guide for running the SC-IES simulation from scratch.

- [Linux (Ubuntu / Debian)](#linux-ubuntu--debian)
- [Windows](#windows)

---

# Linux (Ubuntu / Debian)

## Step 1 — Install VS Code

1. Go to **https://code.visualstudio.com**
2. Click **Download for Linux** → download the `.deb` file
3. Open a terminal and run:

```bash
cd ~/Downloads
sudo dpkg -i code_*.deb
sudo apt install -f
```

Verify:
```bash
code --version
```

---

## Step 2 — Install the 3 Required VS Code Extensions

Open VS Code → press `Ctrl+Shift+X`.

Search and install each one:

| Extension | Publisher |
|---|---|
| `PlatformIO IDE` | PlatformIO |
| `Wokwi Simulator` | Wokwi |
| `C/C++` | Microsoft |

After installing all three, **restart VS Code**.

---

## Step 3 — Install PlatformIO Core (CLI)

```bash
sudo apt install pipx -y
pipx install platformio
pipx ensurepath
source ~/.bashrc
```

Verify:
```bash
pio --version
# PlatformIO Core, version 6.x.x
```

---

## Step 4 — Get a Free Wokwi License

1. Open VS Code
2. Press `F1` → type `Wokwi: Request a Free License` → Enter
3. Sign in with GitHub or Google in the browser
4. Click **Activate License**

> Free, takes under 1 minute.

---

## Step 5 — Open the Project

```bash
code /home/terence-faid/SC-IES-Wokwi
```

Or: **File → Open Folder → SC-IES-Wokwi**

---

## Step 6 — Build the Firmware

```bash
cd /home/terence-faid/SC-IES-Wokwi
pio run
```

Wait for `[SUCCESS]`. This compiles the firmware Wokwi needs to simulate.

---

## Step 7 — Start the Wokwi Simulation

```
F1 → Wokwi: Start Simulator → Enter → ▶ Play
```

---

## Step 8 — Install Python Dependencies

```bash
cd /home/terence-faid/SC-IES-Wokwi/dashboard
pip install -r requirements.txt --break-system-packages
```

---

## Step 9 — Run the Full System

Open **3 terminals**:

```bash
# Terminal 1 — Dashboard
cd SC-IES-Wokwi/dashboard && python3 app.py

# Terminal 2 — Wokwi Bridge
cd SC-IES-Wokwi/dashboard && python3 bridge.py

# Terminal 3 — Open browser
xdg-open http://localhost:5000
```

---

## Linux Troubleshooting

| Problem | Fix |
|---|---|
| `pio` not found | `pipx ensurepath && source ~/.bashrc` |
| PlatformIO stuck installing | `rm -rf ~/.platformio && pipx install platformio` |
| Firmware not found | Run `pio run` first |
| Bridge "Connection refused" | Start Wokwi simulation first |
| Port 5000 in use | `fuser -k 5000/tcp` |
| `python` not found | Use `python3` instead |

---
---

# Windows

## Step 1 — Install VS Code

1. Go to **https://code.visualstudio.com**
2. Click **Download for Windows**
3. Run the installer (`VSCodeSetup-x64.exe`)
4. During install — check both options:
   - ✅ Add to PATH
   - ✅ Register Code as editor for supported file types
5. Click **Install** → **Finish**

Verify — open **Command Prompt** and run:
```cmd
code --version
```

---

## Step 2 — Install the 3 Required VS Code Extensions

Open VS Code → press `Ctrl+Shift+X`.

Search and install each one:

| Extension | Publisher |
|---|---|
| `PlatformIO IDE` | PlatformIO |
| `Wokwi Simulator` | Wokwi |
| `C/C++` | Microsoft |

After installing all three, **restart VS Code**.

---

## Step 3 — Install Python

1. Go to **https://www.python.org/downloads/**
2. Click **Download Python 3.x.x**
3. Run the installer
4. On the first screen — check:
   - ✅ **Add Python to PATH** *(important — do this before clicking Install)*
5. Click **Install Now**

Verify — open **Command Prompt**:
```cmd
python --version
pip --version
```

---

## Step 4 — Install PlatformIO Core (CLI)

Open **Command Prompt** and run:

```cmd
pip install platformio
```

Verify:
```cmd
pio --version
```

You should see:
```
PlatformIO Core, version 6.x.x
```

> If `pio` is not found after install, close and reopen Command Prompt.

---

## Step 5 — Get a Free Wokwi License

1. Open VS Code
2. Press `F1` → type `Wokwi: Request a Free License` → Enter
3. Sign in with GitHub or Google in the browser
4. Click **Activate License**

> Free, takes under 1 minute.

---

## Step 6 — Open the Project

Option A — from Command Prompt:
```cmd
code C:\path\to\SC-IES-Wokwi
```

Option B — in VS Code:
**File → Open Folder → select SC-IES-Wokwi folder → Select Folder**

---

## Step 7 — Build the Firmware

Open the VS Code **Terminal** (`Ctrl+`` ` ``) and run:

```cmd
pio run
```

Wait for:
```
[SUCCESS] Took X.XX seconds
```

This creates the compiled firmware at:
```
.pio\build\megaatmega2560\firmware.hex
```

> You must rebuild every time you change `src/main.cpp`

---

## Step 8 — Start the Wokwi Simulation

```
F1 → Wokwi: Start Simulator → Enter
```

Press the **▶ Play** button in the Wokwi panel.

You will see the circuit come alive and the terminal start printing MQTT messages.

---

## Step 9 — Install Python Dashboard Dependencies

In VS Code terminal:

```cmd
cd dashboard
pip install -r requirements.txt
```

---

## Step 10 — Run the Full System

Open **3 separate terminals** in VS Code (`+` button in the terminal panel):

**Terminal 1 — Dashboard:**
```cmd
cd dashboard
python app.py
```

You should see:
```
SC-IES Dashboard
http://localhost:5000
```

**Terminal 2 — Serial Bridge:**
```cmd
cd dashboard
python bridge.py
```

You should see:
```
✓ Connected — bridging Wokwi → Dashboard
```

**Terminal 3 — Open browser:**
```cmd
start http://localhost:5000
```

---

## How to Know Everything is Working

| What you see | What it means |
|---|---|
| Wokwi circuit animating | Firmware running correctly |
| Terminal printing MQTT JSON | Arduino serial output flowing |
| Bridge terminal printing lines | Serial bridge connected |
| Dashboard loading at localhost:5000 | Flask server running |
| **⚡ Wokwi Connected** badge in header | Full integration working |
| Sensor cards updating every 2 seconds | Live data from Wokwi |

---

## Windows Troubleshooting

### `pio` not found after install
Close and reopen Command Prompt or PowerShell, then try again.
If still not found:
```cmd
python -m platformio --version
```
Use `python -m platformio run` instead of `pio run`.

### PlatformIO stuck installing in VS Code
1. Close VS Code
2. Delete the PlatformIO extension data:
   - Open File Explorer → `C:\Users\YourName\.platformio` → delete this folder
   - Open File Explorer → `C:\Users\YourName\.vscode\extensions` → delete the `platformio.*` folder
3. Reopen VS Code — it will reinstall cleanly

### Python not recognized
You forgot to check **Add Python to PATH** during install.
Fix:
1. Search **Environment Variables** in Windows search
2. Click **Edit the system environment variables**
3. Click **Environment Variables**
4. Under **System variables** → find `Path` → Edit
5. Add: `C:\Users\YourName\AppData\Local\Programs\Python\Python3x\`
6. Add: `C:\Users\YourName\AppData\Local\Programs\Python\Python3x\Scripts\`
7. Click OK → restart Command Prompt

### Firmware not found
Run `pio run` in the project folder first.

### Bridge shows "Connection refused"
Wokwi simulation is not running. Start it first:
```
F1 → Wokwi: Start Simulator → ▶ Play
```

### Port 5000 already in use
```cmd
netstat -ano | findstr :5000
taskkill /PID <PID_NUMBER> /F
```

---

## Windows Quick Start (after everything is installed)

```cmd
# Terminal 1 — Build + Dashboard
cd SC-IES-Wokwi
pio run
cd dashboard && python app.py

# Terminal 2 — Bridge
cd SC-IES-Wokwi\dashboard && python bridge.py
```

In VS Code:
```
F1 → Wokwi: Start Simulator → ▶ Play
```

Open browser: **http://localhost:5000**
