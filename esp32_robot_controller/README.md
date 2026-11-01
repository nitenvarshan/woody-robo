# 🤖 Woody Robot Companion — ESP32 DevKit V1 Upgrade

Complete firmware guide for Woody running on the **30-pin ESP32 DevKit V1** with **Dual-Servo Synchronization**, **Animated OLED Vector Face**, **Shoulder IR Obstacle Sensor**, and **Bluetooth Phone App Control**.

---

## ⚡ 1. Why Was Servo Sync a Problem on Arduino Uno?

If one servo moved while the other "barely turned" or hummed, here is the exact reason:
1. **Back-to-Back Mounting Geometry**:
   - Because the two servos are mounted on opposite sides of Woody's wooden torso, the left motor and right motor rotate in physically opposite directions.
   - Without software mirroring, commanding both motors to rotate to $130^\circ$ causes the left arm to swing forward and up, while the right arm tries to rotate **backwards into the torso**!
   - In this new ESP32 firmware, **Mirror Mode is enabled by default** (`MIRROR_RIGHT_ARM = true`) and can be toggled on-the-fly simply by sending `M`.
2. **Plastic Horn Misalignment**:
   - SG90 micro-servos have an internal plastic stop pin limiting motion to $0^\circ \dots 180^\circ$.
   - If the plastic horn was pressed onto the gear teeth when the internal shaft was at $150^\circ$, asking it to go up to $135^\circ$ only gives $15^\circ$ of travel before the gear jams against the internal stop pin.
   - **The 30-Second Fix**: Use **Calibration Mode (`C`)** below!

---

## 🎯 2. The 3-Step Servo Calibration Fix (Do This Once)

1. **Step 1: Send `C` (Calibration Mode)**
   - Open the Arduino Serial Monitor (115200 baud) or the Bluetooth App.
   - Type **`C`** and hit Enter.
   - Both servo motors will immediately rotate and **lock firmly at exactly 90.0°**.
2. **Step 2: Align the Horns**
   - While both motors are powered and locked at $90^\circ$:
   - Unscrew and pull the plastic horns off the splines.
   - Press both horns back on so they point **STRAIGHT DOWN** (parallel to Woody's wooden body).
   - Screw them in. Now both arms have full, unobstructed $90^\circ$ travel in both directions (from $25^\circ$ resting down to $135^\circ$ victory salute)!
3. **Step 3: Run Diagnostic Test**
   - Type **`TEST`**:
     * Moves **Left arm only**: $30^\circ \rightarrow 130^\circ \rightarrow 90^\circ$
     * Moves **Right arm only**: $30^\circ \rightarrow 130^\circ \rightarrow 90^\circ$
   - Type **`U`** (Both arms swing UP in unison 🙌)
   - Type **`D`** (Both arms swing DOWN in unison 👇)
   - If the right arm moves in the wrong direction, type **`M`** to toggle mirror mode!

---

## 🔌 3. Complete Wiring Diagram (30-Pin ESP32 DevKit V1)

```
                       ┌───────────────────────┐
                       │   ESP32 DevKit V1     │
                       │       (30-Pin)        │
                       │                       │
      [5V Buck Out] ───┤ VIN               GND ├── [Common Ground]
     [OLED & IR 3V3] ──┤ 3V3              GPIO1── TX0 (Debug)
      [Common Ground] ─┤ GND              GPIO3── RX0 (Debug)
                       │ GPIO13          GPIO21├── [OLED SDA]
   [Shoulder IR OUT] ──┤ GPIO14          GPIO22├── [OLED SCL]
                       │ GPIO27          GPIO19├── [RIGHT SERVO PWM]
                       │ GPIO26          GPIO18├── [LEFT SERVO PWM]
                       └───────────────────────┘
```

### Pin Cheat Sheet

| Component | ESP32 Pin | Wire Color / Pin | Power Supply Rail |
| :--- | :--- | :--- | :--- |
| **OLED Display (0.96" I2C)** | `GPIO 21` (SDA)<br>`GPIO 22` (SCL) | Data & Clock | **ESP32 3V3 Rail** + Common GND |
| **Left Arm Servo (SG90)** | `GPIO 18` | Yellow / Orange (Signal) | **5.0V Buck Converter Rail** + GND |
| **Right Arm Servo (SG90)**| `GPIO 19` | Yellow / Orange (Signal) | **5.0V Buck Converter Rail** + GND |
| **Shoulder IR Sensor** | `GPIO 14` | Digital OUT Pin | **ESP32 3V3 Rail** (⚠️ Do NOT power with 5V!) |
| **ESP32 Power Input** | `VIN` | 5.0V Regulated | **5.0V Buck Converter Rail** + GND |

> [!CAUTION]
> 1. **DO NOT power servos from the ESP32 3.3V or 5V pin!** Servos accelerating together draw up to 1.2A, which will reboot the ESP32. Power servo red wires from the 5V buck converter.
> 2. **Connect all grounds together!** Battery negative, buck converter ground, servo brown wires, and ESP32 GND must all meet at the breadboard blue rail.
> 3. **Power the IR sensor with 3.3V!** ESP32 GPIOs are not 5V tolerant.

---

## 📱 4. Bluetooth Phone App Setup (Zero Lag)

Because the ESP32 has onboard Bluetooth, your phone **does NOT have to disconnect from Wi-Fi or mobile data**.

### Recommended App: **Serial Bluetooth Terminal** (by Kai Morich on Google Play)
*(Or any standard Bluetooth Serial app on iOS/Android)*

1. **Turn on Bluetooth** on your smartphone.
2. In your phone's Bluetooth settings, scan and pair with **`Woody-Robot`** (Pairing code is standard `1234` or `0000` if prompted, usually none needed).
3. Open **Serial Bluetooth Terminal**:
   - Go to **Menu ☰ ➡️ Devices** ➡️ Select **Woody-Robot**.
   - Tap the **Connect icon** at the top right. You will see:
     ```
     Connected to Woody-Robot
     ========================================================
            WOODY ROBOT — ESP32 COMMAND CONSOLE 🤖           
     ========================================================
     ```
4. **Set Up Custom One-Tap Quick Buttons in the App**:
   - Long-press any button bar at the bottom (`M1`, `M2`, `M3`, etc.) and set:
     - Button 1: Name: **🙌 UP** | Value: `U`
     - Button 2: Name: **👇 DOWN** | Value: `D`
     - Button 3: Name: **👋 WAVE** | Value: `W`
     - Button 4: Name: **😎 CHAD** | Value: `G`
     - Button 5: Name: **❤️❤️ HEART** | Value: `H`
     - Button 6: Name: **🎯 CALIBRATE** | Value: `C`
     - Button 7: Name: **🔄 MIRROR** | Value: `M`
     - Button 8: Name: **🟩 CREEPER** | Value: `1`

Now you can control Woody with instant, tactile one-tap buttons right from your phone!

---

## 🚀 5. How to Compile & Upload in Arduino IDE

1. **Install ESP32 Board Support** (if not already installed):
   - Open Arduino IDE ➡️ **File ➡️ Preferences**.
   - Additional Board Manager URLs:
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Go to **Tools ➡️ Board ➡️ Boards Manager**, search **ESP32**, and install `esp32 by Espressif Systems`.
2. **Install Required Library**:
   - Go to **Sketch ➡️ Include Library ➡️ Manage Libraries...**
   - Search for **`ESP32Servo`** (by Kevin Harrington).
   - Click **Install**.
3. **Upload Settings**:
   - Open `esp32_robot_controller.ino`.
   - **Board**: `ESP32 Dev Module` (or `DOIT ESP32 DEVKIT V1`).
   - **Port**: Select your ESP32 COM port.
   - **Upload Speed**: `921600` (or `115200`).
   - Click **Upload (➡️)**!

---

## 📊 6. Live Chip Condition, Telemetry & Web App

You can analyze the ESP32's hardware health, temperature, RAM, flash storage, and running time using either the **Web App** or the **Bluetooth App**:

### A. Web App Dashboard (With Bottom Tab Bar)
1. On your phone, connect to Woody's Wi-Fi network:
   - **Wi-Fi SSID:** `Woody-Robot`
   - **Password:** `12345678`
2. Open any browser (Chrome, Safari) and go to:
   👉 **`http://192.168.4.1`**
3. Tap the **📊 Chip & System** button on the bottom navigation bar to see:
   - 🌡️ **Core Chip Temperature:** Real-time °C and °F readout with health color badge (Green = Normal, Orange = Warm, Red = Hot).
   - ⏱️ **Running Time (Uptime):** Total time since boot (`DDd HHh MMm SSs`).
   - 🧠 **RAM (Heap) Memory:** Free vs Total RAM with live visual percentage bar and minimum heap watermark.
   - ⚡ **Processor Specs:** CPU frequency (240 MHz), Core count, and Silicon Model/Revision.
   - 💾 **Flash Storage:** Sketch usage vs Total Flash chip size.
   - 🦾 **Servo Coordinates & Mirror Status:** Live left and right arm angles.
   - 📡 **IR Sensor Status:** Live obstacle state and trigger counter.

### B. Bluetooth / Serial Console Command
In **Serial Bluetooth Terminal** or the Arduino Serial Monitor, type:
- **`SYS`** or **`DIAG`** (or **`STATUS`**)
Prints a clean, formatted ASCII telemetry card with all real-time chip metrics!


