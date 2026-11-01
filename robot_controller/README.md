# Woody The Robot Controller - User Edition 🤖✨

Full Robot Companion with **SH1106 / SSD1306 Fast OLED Display**, **Dual Synchronized Servos**, **Interactive Hand-Wave IR Detection (Giga Chad Wink & Smirk)**, and **Full Serial Control Console**.

---

## 🔧 Why Was One Servo "Slightly Turning"?

If one servo turned normally while the other only moved a tiny bit, here is exactly why:

1. **Mechanical Limit Stop & Horn Misalignment (Most Common)**:
   - Standard SG90 servos have internal plastic stop pins limiting rotation to 0°–180°.
   - If the horn was pushed onto the gear shaft while the motor was at ~150°, asking it to rotate up to 130° only moves it 20° before it jams against the internal physical gear stop!
2. **Back-to-Back Servo Mounting (Opposite Rotation Direction)**:
   - Your servos are mounted **back-to-back** (as shown in your photo).
   - To make both arms swing **forward and up into the air together**, the right motor must rotate in the **opposite internal direction** (`180 - angle`).
   - If mirroring is disabled, the right motor tries to rotate **backwards** into the robot body/frame and jams against the stop pin after moving only a few millimeters ("slightly turning")!
3. **USB Power Brownout**:
   - Two SG90 servos accelerating simultaneously can pull 1.2A+ peak current.
   - A weak USB port drops voltage down to ~3.8V, starving the second servo of torque.

---

## 🎯 3-Step Fix: Perfect Alignment & Sync

Follow these steps once in the **Arduino Serial Monitor (9600 baud)**:

1. **Step 1: Enter Calibration Mode**
   - Type **`C`** and press Enter.
   - Both servo shafts will immediately rotate and **lock firmly at exactly 90.0°**.
2. **Step 2: Align the Horns**
   - While the Arduino remains powered and both motors are locked at 90°:
   - Unscrew the plastic horn from the misaligned servo.
   - Press both horns back on so they point **STRAIGHT DOWN** (parallel to each other).
   - Now both servos have **full 90° travel in both directions** (from 25° down to 130° up) without ever hitting a stop!
3. **Step 3: Run Diagnostic Test**
   - Type **`TEST`** in Serial Monitor:
     * Moves **Left arm only**: 30° ➡️ 130° ➡️ 90°
     * Moves **Right arm only**: 30° ➡️ 130° ➡️ 90°
   - Type **`U`** (Both arms UP in sync 🙌)
   - Type **`D`** (Both arms DOWN in sync 👇)
   - Type **`W`** (Resumes the cute default story animation loop 👋)

> 💡 **Tip:** If the right arm moves in the opposite direction (e.g., backwards), simply type **`M`** to toggle Mirror Mode!

---

## 💻 Complete Serial Monitor Commands (9600 Baud)

Open the Arduino Serial Monitor at **9600 baud** (works with *No line ending*, *Newline*, or *Both NL & CR*):

### 🦾 Servo Motion & Calibration
| Command | Action |
|---|---|
| **`C`** or **`CAL`** | **Calibration Mode**: Locks both servos at 90.0° for horn installation 🎯 |
| **`TEST`** or **`CHECK`** | **Diagnostic Sweep**: Tests Left arm then Right arm independently 🔬 |
| **`M`** or **`MIRROR`** | **Toggle Mirror Mode**: Switches between Mirrored (back-to-back) and Direct 🔄 |
| **`U`** or **`UP`** | Both arms swing **UP** in sync 🙌 |
| **`D`** or **`DOWN`** | Both arms swing **DOWN** in sync 👇 |
| **`L <angle>`** | Move **Left arm only** (e.g. `L 90`, `L 25`, `L 130`) 👈 |
| **`R <angle>`** | Move **Right arm only** (e.g. `R 90`, `R 25`, `R 130`) 👉 |
| **`0` to `180`** | Move both arms in sync to any angle (e.g. `90`, `120`) |
| **`L+`** / **`L-`** | Fine-tune Left servo offset angle (±1°) |
| **`R+`** / **`R-`** | Fine-tune Right servo offset angle (±1°) |
| **`S`** or **`STOP`** | Emergency STOP / Freeze arms at current position 🛑 |

---

### 👁️ Expressions & Story Animation
| Command | Action |
|---|---|
| **`W`** or **`3`** | **Resume Default Story Animation Loop** (Eyes Look Left ➡️ Right ➡️ Cute Wink ➡️ Glowing Hearts + Arms UP) 👋 |
| **`G`** or **`CHAD`** | Instant **Giga Chad Wink & Smirk** + Arms UP! 😎💪 |
| **`K`** or **`WINK`** | Smooth playful one-eye animated wink 😉 |
| **`H`** or **`HEART`** | HUGE Glowing Heart Eyes in BOTH eyes ❤️❤️ |
| **`O`** or **`WOW`** | Dilated surprised wide eyes 😲 |
| **`1`** or **`CREEPER`**| Pixel-perfect Minecraft Creeper face 🟩 |
| **`2`** or **`HEROBRINE`**| Glowing white Herobrine face 👻 |
| **`T`** or **`INTRO`** | Batch 4 Team Intro text screen 📺 |

---

### 📡 IR Sensor (Hand Detection)
| Command | Action |
|---|---|
| **`IR`** | Toggle IR Sensor ON / OFF (Default is **OFF** so manual testing is never interrupted) |
| **`IR ON`** | Enable IR hand detection (Triggers Giga Chad wink & smirk when object detected) |
| **`IR OFF`** | Disable IR sensor |
