# Animated Wooden Robot Companion 🤖✨

## 📦 What We Built
- **Synchronized Dual-Arm Mechanism**: Both arms connected via a lower cross-stick and lifted together by 1 servo motor.
- **Animated OLED Face**: Big cartoon eyes that blink, look left/right, smile, and make surprised faces.
- **Arduino Uno Brain**: Controls both screen and servo smoothly without any freezing.

---

## 🔌 Pin Connections (Cheat Sheet)

### 1. Power Rails (Breadboard)
- **Arduino 5V**  ➡️  Breadboard **Red (+) Rail**
- **Arduino GND** ➡️  Breadboard **Blue (-) Rail**

### 2. OLED Display (4 Pins)
- **VCC** ➡️ Breadboard **Red (+) Rail** (5V)
- **GND** ➡️ Breadboard **Blue (-) Rail** (GND)
- **SDA** ➡️ Arduino Uno **Pin A4** (or SDA pin)
- **SCL** ➡️ Arduino Uno **Pin A5** (or SCL pin)

### 3. Servo Motor (3 Wires)
- **Red wire** (VCC)    ➡️ Breadboard **Red (+) Rail** (5V)
- **Brown/Black wire** (GND) ➡️ Breadboard **Blue (-) Rail** (GND)
- **Yellow/Orange wire** (Signal) ➡️ Arduino Uno **Digital Pin 9**

---

## 🚀 How to Run

### Option 1: Friend Edition (2 Servos + IR Sensor, No OLED)
- Location: [friend_robot_controller.ino](file:///c:/Users/niten_2e/OneDrive/Desktop/Test-1/friend_robot_controller/friend_robot_controller.ino)
- **Zero external libraries needed!** Just open and upload.
- Runs 100% autonomously in a continuous loop with interactive hand-wave IR detection.
- See [Friend Edition Guide](file:///c:/Users/niten_2e/OneDrive/Desktop/Test-1/friend_robot_controller/README.md) for wiring details.

### Option 2: Full Edition (OLED Face + 2 Servos + IR + Serial Commands)
1. Open the Arduino IDE.
2. Go to **Sketch** ➡️ **Include Library** ➡️ **Manage Libraries...**
3. Install:
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`
4. Open [robot_controller.ino](robot_controller/robot_controller.ino).
5. Select **Arduino Uno** and your COM Port, then click **Upload** (➡️ arrow button)!

---

### Option 3: ESP32 DevKit V1 Upgrade (Bluetooth Phone App + Dual-Servo Sync) 🚀📱
Upgrade Woody from the Arduino Uno to the **30-pin ESP32 DevKit V1** for wireless smartphone control and hardware-timed jitter-free servo motion.

- **Location:** [`esp32_robot_controller/esp32_robot_controller.ino`](esp32_robot_controller/esp32_robot_controller.ino)
- **Detailed Guide:** [`esp32_robot_controller/README.md`](esp32_robot_controller/README.md)

#### ✨ What the ESP32 Upgrade Adds:
- **Bluetooth Phone App Control:** Pairs with your phone via Bluetooth (`Woody-Robot`). Keep your mobile data/Wi-Fi active while controlling Woody with instant one-tap buttons (`U`=Arms UP, `D`=Arms DOWN, `W`=Wave, `G`=Giga Chad, `H`=Heart Eyes).
- **Dual-Servo Synchronization & Alignment Fix:**
  - **Back-to-Back Mirroring:** Automatically mirrors the right arm (`180 - angle`) so both arms swing forward and up in unison. Can be toggled on-the-fly by sending `M`.
  - **One-Touch Horn Calibration (`C`):** Locks both motors at $90.0^\circ$ so you can pull the horns off and seat them pointing straight down without jamming.
  - **Smooth Motion Easing:** Moves arms at $2.2^\circ$/frame to eliminate 1.2A+ power spikes and protect wooden joints.
- **Autonomous Alive Loop:** Continuous natural blinking, glancing eyeballs, animated winking, and shoulder IR hand-wave detection.

#### 🔌 ESP32 Wiring Quick Reference:
| Component | ESP32 Pin | Power Rail |
| :--- | :--- | :--- |
| **OLED (I2C)** | `GPIO 21` (SDA) / `GPIO 22` (SCL) | ESP32 `3V3` + GND |
| **Left Arm Servo** | `GPIO 18` | **5.0V External Buck Rail** + GND |
| **Right Arm Servo** | `GPIO 19` | **5.0V External Buck Rail** + GND |
| **Shoulder IR Sensor** | `GPIO 14` | ESP32 `3V3` (⚠️ Do NOT use 5V!) |
| **ESP32 Power** | `VIN` | **5.0V Buck Output** + Common GND |

#### 📊 Live Chip Diagnostics & Web App:
- **Web App Dashboard (`http://192.168.4.1`):** Connect to Wi-Fi `Woody-Robot` (pass: `12345678`) to open the touch dashboard. The bottom navigation bar includes a **📊 Chip & System** page showing real-time **Core Temperature**, **RAM (Heap)** usage bar, **Uptime Running Time**, **CPU specs**, and **Flash Storage**.
- **Bluetooth/Serial Telemetry:** Send **`SYS`** or **`DIAG`** via Bluetooth App or Serial Monitor to print a live chip telemetry status card.



