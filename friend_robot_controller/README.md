# Woody The Robot Companion - Friend Edition 🤖✨

This edition is specifically configured for setups with **2 Servos + IR Sensor (No OLED display)**. Both servos move in **100% locked synchrony**, and a dedicated **Calibration Mode** is included for easy alignment!

---

## 🛠️ Hardware Needed
- **Arduino** (Uno, Nano, or Mega)
- **2 x Servos** (SG90 or MG90S) for Left Arm & Right Arm
- **1 x IR Obstacle Sensor** (Digital 3-pin: VCC, GND, OUT)
- **USB Cable or Battery** (5V to 6V power)

---

## 🔌 Pin Connections

### 1. Servo Motors
| Servo | Wire Color | Arduino Pin |
|---|---|---|
| **Left Servo** | Yellow / Orange (Signal) | **Digital Pin 9** |
| | Red (VCC) | **5V** (Breadboard 5V Rail) |
| | Brown / Black (GND) | **GND** (Breadboard GND Rail) |
| **Right Servo** | Yellow / Orange (Signal) | **Digital Pin 10** |
| | Red (VCC) | **5V** (Breadboard 5V Rail) |
| | Brown / Black (GND) | **GND** (Breadboard GND Rail) |

---

### 2. IR Obstacle Sensor (Choose Method A or Method B)

#### Method A: Direct Plug-in (No breadboard power rails needed!)
The code configures Pins 3 & 4 as power pins so you can plug the sensor directly into the Arduino header:
- **OUT / Signal** ➡️ Arduino **Digital Pin 2**
- **GND** ➡️ Arduino **Digital Pin 3** (Software 0V)
- **VCC** ➡️ Arduino **Digital Pin 4** (Software 5V)

#### Method B: Standard Connection
- **OUT / Signal** ➡️ Arduino **Digital Pin 2**
- **GND** ➡️ Arduino **GND**
- **VCC** ➡️ Arduino **5V**

---

## 🎯 How to Use Calibration Mode

Calibration Mode locks both servos at exactly **90 degrees** so you can attach the robot's servo horns perpendicular and tighten the screws accurately!

### Method 1: Via Serial Monitor (Recommended)
1. Open the **Serial Monitor** in Arduino IDE (set to **9600 baud**).
2. Type **`C`** (or **`CAL`**) and press Enter.
3. Both arms will immediately lock at **90 degrees**.
4. Screw on your servo horns so both arms point straight.
5. *(Optional)* Fine-tune alignment:
   - Type **`L+`** or **`L-`** to adjust the left arm angle.
   - Type **`R+`** or **`R-`** to adjust the right arm angle.
6. Type **`C`** or **`W`** to exit calibration and resume the synchronized animation loop!

### Method 2: Offline / Battery Calibration
At the top of [friend_robot_controller.ino](file:///c:/Users/niten_2e/OneDrive/Desktop/Test-1/friend_robot_controller/friend_robot_controller.ino):
```cpp
const bool START_IN_CALIBRATION_MODE = true; // Set to true to lock at 90° on power-on
```
Upload this once, attach your arms, then set it back to `false` and upload again!

---

## 🚀 Synchronized Autonomous Choreography

When not in calibration mode, the robot runs **100% hands-free** with both arms moving in locked harmony:

1. **Step 0**: Synchronized gentle rest / breathing posture (~2.8s).
2. **Step 1**: Synchronized gentle mid-wave (both arms wave up and down together).
3. **Step 2**: Synchronized high victory celebration (both arms swing high up in triumph).
4. **Step 3**: Synchronized double pump dance (both arms rhythmically pulse in sync).
5. **Repeats endlessly in a smooth loop!**

### 👋 Interactive IR Sensor Reaction
- Wave your hand in front of the IR sensor at any time:
  - Both arms **instantly shoot straight UP together** in excitement!
  - Both arms do an excited celebration wave in sync.
  - The built-in LED on Pin 13 flashes rapidly.
  - When the hand is cleared, both arms glide down together and resume the synchronized loop seamlessly.

---

## 💻 Serial Monitor Commands (9600 Baud)
| Command | Action |
|---|---|
| **`C`** or **`CAL`** | Toggle Calibration Mode (Locks both arms at 90.0° for horn installation) 🎯 |
| **`TEST`** or **`CHECK`** | Step-by-step diagnostic test of Left servo then Right servo 🔬 |
| **`M`** or **`MIRROR`** | Toggle Mirror direction (for back-to-back vs side-by-side mounting) 🔄 |
| **`L <angle>`** | Move Left arm only (e.g. `L 90`, `L 25`, `L 130`) 👈 |
| **`R <angle>`** | Move Right arm only (e.g. `R 90`, `R 25`, `R 130`) 👉 |
| **`U`** or **`UP`** | Both arms UP in sync 🙌 |
| **`D`** or **`DOWN`** | Both arms DOWN in sync 👇 |
| **`0` to `180`** | Move both arms in sync to any angle (e.g. `90`) |
| **`L+`** / **`L-`** | Fine-tune Left servo angle offset |
| **`R+`** / **`R-`** | Fine-tune Right servo angle offset |
| **`W`** or **`RESUME`** | Resume Synchronized Autonomous Loop 👋 |
| **`S`** or **`STOP`** | Freeze / Stop arms 🛑 |
| **`?`** or **`HELP`** | Print command menu |
