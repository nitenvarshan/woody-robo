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
1. Open the Arduino IDE.
2. Go to **Sketch** ➡️ **Include Library** ➡️ **Manage Libraries...**
3. Install:
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`
4. Open [robot_controller.ino](robot_controller/robot_controller.ino).
5. Select **Arduino Uno** and your COM Port, then click **Upload** (➡️ arrow button)!
