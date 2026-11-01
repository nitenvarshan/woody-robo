/*
 * ============================================================================
 *   WOODY THE ROBOT COMPANION - FRIEND EDITION (SYNCHRONIZED DUAL-ARM) 🤖✨
 * ============================================================================
 *   Hardware:
 *     - Arduino Uno / Nano / Mega
 *     - 2 x SG90 or MG90S Servos (Left Arm on Pin 9, Right Arm on Pin 10)
 *     - 1 x IR Obstacle Sensor (Digital 3-pin: VCC, GND, OUT on Pin 2)
 *     - Built-in LED on Pin 13 (Heartbeat & Mode Indicator)
 *     - NO OLED Screen Required!
 *
 *   Key Features:
 *     - 100% Synchronized Dual-Arm Motion: Both arms move together in locked,
 *       harmonious synchrony (raising, lowering, waving, and celebrating in sync).
 *     - Calibration Mode Built-in:
 *         * Type 'C' or 'CAL' in Serial Monitor to lock both arms at exactly 90°!
 *         * Or set START_IN_CALIBRATION_MODE = true to calibrate via battery.
 *         * Fine-tune angles with 'L+', 'L-', 'R+', 'R-' commands.
 *         * Type 'C' or 'W' to exit calibration and resume the synchronized loop.
 *     - Autonomous Continuous Loop: No commands needed to run! Plug in power
 *       and the robot runs its synchronized choreography automatically.
 *     - Interactive IR Reaction: Wave a hand in front of the IR sensor and both
 *       arms instantly swing UP in sync with rapid celebration LED pulses.
 *     - Direct-Plug IR Support: Digital pins 3 (GND) and 4 (5V) can power the
 *       IR sensor directly if your 5V/GND pins are already taken by servos!
 * ============================================================================
 */

#include <Servo.h>

// ----------------------------------------------------------------------------
// 1. PIN CONFIGURATIONS
// ----------------------------------------------------------------------------
#define LEFT_SERVO_PIN   9    // Signal pin for Left Arm Servo
#define RIGHT_SERVO_PIN  10   // Signal pin for Right Arm Servo
#define LED_PIN          13   // Built-in Arduino LED (Status indicator)

#define IR_SENSOR_PIN    2    // Signal / OUT pin from IR Obstacle Sensor (Active LOW)
#define IR_GND_PIN       3    // Pure 0V Ground pin (Optional direct plug-in)
#define IR_POWER_PIN     4    // Pure 5V VCC power pin (Optional direct plug-in)

// ----------------------------------------------------------------------------
// 2. CALIBRATION & SERVO SETTINGS
// ----------------------------------------------------------------------------
// Set to true if you want the robot to immediately boot into 90° calibration
// lock without needing a computer (great for assembling servo horns on battery!)
const bool START_IN_CALIBRATION_MODE = false;

// Arm Angle Limits (Degrees: 0 to 180)
const int ARM_DOWN_ANGLE = 25;    // Rest position (both arms down)
const int ARM_MID_ANGLE  = 75;    // Mid-level gesture
const int ARM_UP_ANGLE   = 135;   // Fully raised (victory celebration / high-five)

// Fine-tune offsets (Adjust if a horn is a couple degrees off perpendicular)
int leftOffset  = 0;
int rightOffset = 0;

// Facing servos mirror each other:
// When true, right arm calculates (180 - angle) so both arms raise together.
// Can be toggled anytime in Serial Monitor with 'M' command!
bool MIRROR_RIGHT_ARM = true;

// State Flags
bool calibrationMode = START_IN_CALIBRATION_MODE;
bool autoDanceMode   = true;
bool isStopped       = false;

// ----------------------------------------------------------------------------
// 3. SYNCHRONIZED SERVO ENGINE
// ----------------------------------------------------------------------------
Servo leftServo;
Servo rightServo;

// Both arms share the exact same synchronized angle coordinate!
float currentServoAngle = ARM_DOWN_ANGLE;
float targetServoAngle  = ARM_DOWN_ANGLE;
float armSpeed          = 2.4; // Degrees per 20ms frame

unsigned long lastArmUpdateTime = 0;
const unsigned long ARM_UPDATE_INTERVAL_MS = 20; // 50 Hz smooth refresh rate

// ----------------------------------------------------------------------------
// 4. IR SENSOR & CHOREOGRAPHY STATE
// ----------------------------------------------------------------------------
bool isHandDetected = false;
unsigned long irDetectStartTime = 0;
unsigned long irTriggerTime = 0;
const unsigned long IR_DEBOUNCE_MS      = 50;   // Noise filter
const unsigned long IR_HOLD_DURATION_MS = 1800; // Hold celebration after hand leaves

int storyStep = 0;
unsigned long storyStepTimer = 0;
int waveSubStep = 0;
unsigned long waveSubTimer = 0;

// ----------------------------------------------------------------------------
// HELPER: MOVE BOTH ARMS IN LOCKED SYNCHRONY
// ----------------------------------------------------------------------------
void setSyncTarget(float targetAngle, float speed = 2.4) {
  targetServoAngle = constrain(targetAngle, 0, 180);
  armSpeed = speed;
}

// ----------------------------------------------------------------------------
// 5. SYNCHRONIZED SERVO UPDATE (WITH CALIBRATION LOCK)
// ----------------------------------------------------------------------------
void updateArms() {
  // If Calibration Mode is active, lock both arms at exactly 90 degrees!
  if (calibrationMode) {
    int leftCalib  = constrain(90 + leftOffset, 0, 180);
    int rightCalib = constrain(90 + rightOffset, 0, 180);
    currentServoAngle = 90;
    targetServoAngle = 90;
    leftServo.write(leftCalib);
    rightServo.write(rightCalib);
    return;
  }

  if (isStopped) return;

  unsigned long now = millis();
  if (now - lastArmUpdateTime < ARM_UPDATE_INTERVAL_MS) return;
  lastArmUpdateTime = now;

  // Smooth interpolation towards targetServoAngle
  if (abs(currentServoAngle - targetServoAngle) > 0.4) {
    if (currentServoAngle < targetServoAngle) {
      currentServoAngle += armSpeed;
      if (currentServoAngle > targetServoAngle) currentServoAngle = targetServoAngle;
    } else {
      currentServoAngle -= armSpeed;
      if (currentServoAngle < targetServoAngle) currentServoAngle = targetServoAngle;
    }

    // Write synchronized positions to both servos simultaneously
    int leftAngle = constrain((int)currentServoAngle + leftOffset, 0, 180);
    int baseRight = MIRROR_RIGHT_ARM ? (180 - (int)currentServoAngle) : (int)currentServoAngle;
    int rightAngle = constrain(baseRight + rightOffset, 0, 180);

    leftServo.write(leftAngle);
    rightServo.write(rightAngle);
  }
}

// ----------------------------------------------------------------------------
// 6. IR SENSOR CHECK
// ----------------------------------------------------------------------------
void checkIrSensor() {
  if (calibrationMode) return; // Do not trigger IR reactions while calibrating!

  bool rawReading = (digitalRead(IR_SENSOR_PIN) == LOW);
  unsigned long now = millis();

  if (rawReading) {
    if (irDetectStartTime == 0) irDetectStartTime = now;
  } else {
    irDetectStartTime = 0;
  }

  bool verified = (irDetectStartTime > 0 && (now - irDetectStartTime >= IR_DEBOUNCE_MS));

  // Hand just appeared!
  if (verified && !isHandDetected) {
    isHandDetected = true;
    irTriggerTime = now;
    waveSubStep = 0;
    waveSubTimer = now;

    // Both arms shoot UP together in synchrony!
    setSyncTarget(ARM_UP_ANGLE, 4.2);
    Serial.println(F("\n👋 >>> [IR SENSOR]: Hand detected! Both arms swinging UP in sync! <<< 🎉"));
  }
  // Hand removed!
  else if (!verified && isHandDetected) {
    if (now - irTriggerTime >= IR_HOLD_DURATION_MS) {
      isHandDetected = false;
      setSyncTarget(ARM_DOWN_ANGLE, 2.0); // Smooth glide down in sync
      storyStep = 0;
      storyStepTimer = now;
      Serial.println(F("👋 [IR SENSOR]: Hand cleared. Resuming synchronized autonomous loop."));
    }
  }
}

// ----------------------------------------------------------------------------
// 7. SYNCHRONIZED CELEBRATION REACTION (WHEN HAND DETECTED)
// ----------------------------------------------------------------------------
void updateCelebrationAnimation() {
  unsigned long now = millis();

  // Synchronized celebration pulse: both arms bounce between 110° and 135° together!
  if (now - waveSubTimer >= 240) {
    waveSubTimer = now;
    waveSubStep++;

    if (waveSubStep % 2 == 1) {
      setSyncTarget(ARM_UP_ANGLE - 25, 3.8);
    } else {
      setSyncTarget(ARM_UP_ANGLE, 3.8);
    }
  }

  // Fast excitement heartbeat blink on Pin 13 (100ms)
  digitalWrite(LED_PIN, (now / 100) % 2);
}

// ----------------------------------------------------------------------------
// 8. SYNCHRONIZED AUTONOMOUS CHOREOGRAPHY (WHEN IDLE)
// ----------------------------------------------------------------------------
void updateAutonomousChoreography() {
  unsigned long now = millis();

  // Gentle idle heartbeat blink (500ms)
  digitalWrite(LED_PIN, (now / 500) % 2);

  // --------------------------------------------------------------------------
  // STEP 0: Gentle Rest / Breathing (Both arms down, resting ~2.8s)
  // --------------------------------------------------------------------------
  if (storyStep == 0) {
    if (now - storyStepTimer >= 2800) {
      storyStep = 1;
      storyStepTimer = now;
      waveSubStep = 0;
      waveSubTimer = now;
      setSyncTarget(ARM_MID_ANGLE, 2.2); // Glide up to mid-height together
      Serial.println(F("[SYNC LOOP] Step 1: Synchronized Gentle Mid-Wave 👋"));
    }
  }
  // --------------------------------------------------------------------------
  // STEP 1: Synchronized Gentle Mid-Wave (Both arms wave up and down in sync)
  // --------------------------------------------------------------------------
  else if (storyStep == 1) {
    if (now - waveSubTimer >= 400) {
      waveSubTimer = now;
      waveSubStep++;

      if (waveSubStep <= 5) {
        if (waveSubStep % 2 == 1) {
          setSyncTarget(ARM_MID_ANGLE + 25, 2.6);
        } else {
          setSyncTarget(ARM_MID_ANGLE - 20, 2.6);
        }
      } else {
        // Wave complete, proceed to Step 2
        setSyncTarget(ARM_DOWN_ANGLE, 2.0);
        storyStep = 2;
        storyStepTimer = now;
        Serial.println(F("[SYNC LOOP] Step 2: Synchronized High Victory Celebration 🙌"));
      }
    }
  }
  // --------------------------------------------------------------------------
  // STEP 2: Synchronized High Victory Celebration (Both arms swing high up)
  // --------------------------------------------------------------------------
  else if (storyStep == 2) {
    if (now - storyStepTimer >= 1200) {
      // Both arms shoot all the way up together!
      setSyncTarget(ARM_UP_ANGLE, 3.0);

      // Hold at top for 2.4 seconds, then proceed
      if (now - storyStepTimer >= 3600) {
        setSyncTarget(ARM_DOWN_ANGLE, 1.8); // Smooth glide back down
        storyStep = 3;
        storyStepTimer = now;
        waveSubStep = 0;
        waveSubTimer = now;
        Serial.println(F("[SYNC LOOP] Step 3: Synchronized Double Pump Dance 🎶"));
      }
    }
  }
  // --------------------------------------------------------------------------
  // STEP 3: Synchronized Double Pump Dance (Both arms rhythmically pulse in sync)
  // --------------------------------------------------------------------------
  else if (storyStep == 3) {
    if (now - storyStepTimer >= 1000) {
      if (now - waveSubTimer >= 320) {
        waveSubTimer = now;
        waveSubStep++;

        if (waveSubStep <= 6) {
          if (waveSubStep % 2 == 1) {
            setSyncTarget(ARM_MID_ANGLE + 30, 3.4);
          } else {
            setSyncTarget(ARM_DOWN_ANGLE + 15, 3.4);
          }
        } else {
          // Dance complete, smoothly return to resting Step 0
          setSyncTarget(ARM_DOWN_ANGLE, 2.0);
          storyStep = 0;
          storyStepTimer = now;
          Serial.println(F("[SYNC LOOP] Step 0: Gentle Rest / Breathing"));
        }
      }
    }
  }
}

// ----------------------------------------------------------------------------
// 9. SERIAL COMMAND CENTER (CALIBRATION & MANUAL CONTROL)
// ----------------------------------------------------------------------------
void printMenu() {
  Serial.println(F("\n========================================================"));
  Serial.println(F("   🤖 WOODY FRIEND ROBOT - SYNCHRONIZED CONTROLLER 🤖"));
  Serial.println(F("========================================================"));
  Serial.println(F("  [SYNCHRONIZED LOOP RUNNING]"));
  Serial.println(F("    - Both left & right servos move 100% in sync!"));
  Serial.println(F("    - Automatic loop runs continuously hands-free."));
  Serial.println(F(""));
  Serial.println(F("  [SERVO CONTROLS & DIAGNOSTICS]"));
  Serial.println(F("    'U' / 'UP'       -> Move both arms UP in sync 🙌"));
  Serial.println(F("    'D' / 'DOWN'     -> Move both arms DOWN in sync 👇"));
  Serial.println(F("    'C' or 'CAL'     -> Calibration Mode (Locks arms at 90°) 🎯"));
  Serial.println(F("    'TEST' or 'CHECK'-> Step-by-step diagnostic test of each arm"));
  Serial.println(F("    'M' or 'MIRROR'  -> Toggle Right Arm direction (Mirrored vs Direct) 🔄"));
  Serial.println(F("    'L <angle>'      -> Test Left arm only (e.g. 'L 90', 'L 25')"));
  Serial.println(F("    'R <angle>'      -> Test Right arm only (e.g. 'R 90', 'R 25')"));
  Serial.println(F("    '0' - '180'      -> Test any specific angle on both arms (e.g. '90')"));
  Serial.println(F("    'L+' / 'L-'      -> Fine-tune Left servo offset angle"));
  Serial.println(F("    'R+' / 'R-'      -> Fine-tune Right servo offset angle"));
  Serial.println(F("    'W' or 'RESUME'  -> Resume Synchronized Autonomous Loop 👋"));
  Serial.println(F("    'S' or 'STOP'    -> Stop / Freeze arms 🛑"));
  Serial.println(F("    '?' or 'HELP'    -> Show this command menu"));
  Serial.println(F("========================================================\n"));
}

String serialCmdBuffer = "";
unsigned long lastSerialCharTime = 0;

int extractFirstNumber(const String& str) {
  String num = "";
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (isDigit(c)) {
      num += c;
    } else if (num.length() > 0) {
      break;
    }
  }
  if (num.length() > 0) return num.toInt();
  return -1;
}

void processSerialCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  if (cmd.length() == 0) return;

  Serial.print(F("\n>>> [COMMAND RECEIVED]: '"));
  Serial.print(cmd);
  Serial.println(F("'"));

  if (cmd == "?" || cmd == "HELP" || cmd == "MENU") {
    printMenu();
  }
  else if (cmd == "C" || cmd == "CAL" || cmd == "CALIBRATE") {
    calibrationMode = !calibrationMode;
    isStopped = false;
    isHandDetected = false;
    autoDanceMode = false;

    if (calibrationMode) {
      int leftCalib  = constrain(90 + leftOffset, 0, 180);
      int rightCalib = constrain(90 + rightOffset, 0, 180);
      leftServo.write(leftCalib);
      rightServo.write(rightCalib);
      currentServoAngle = 90;
      targetServoAngle = 90;
      Serial.println(F("🎯 >>> [CALIBRATION MODE ACTIVE: Both arms LOCKED at 90°] <<<"));
      Serial.println(F("    1. Keep power on. Both motors are holding at 90°."));
      Serial.println(F("    2. If an arm is crooked or 'slightly turning', unscrew its horn now."));
      Serial.println(F("    3. Re-attach both horns pointing STRAIGHT DOWN (parallel)."));
      Serial.println(F("    4. Fine-tune with 'L+' / 'L-' or 'R+' / 'R-' if needed."));
      Serial.println(F("    5. Type 'C' or 'W' when finished to save and resume auto loop!"));
    } else {
      autoDanceMode = true;
      Serial.println(F("✅ [CALIBRATION COMPLETE] Resumed auto loop."));
      setSyncTarget(ARM_DOWN_ANGLE, 2.0);
      storyStep = 0;
      storyStepTimer = millis();
    }
  }
  else if (cmd == "M" || cmd == "MIRROR") {
    MIRROR_RIGHT_ARM = !MIRROR_RIGHT_ARM;
    Serial.print(F("🔄 [SERVO DIRECTION]: Right arm mirroring is now: "));
    if (MIRROR_RIGHT_ARM) {
      Serial.println(F("MIRRORED (Back-to-back mounting: arms move symmetrically!)"));
    } else {
      Serial.println(F("DIRECT (Side-by-side mounting: arms move in identical internal directions)"));
    }
  }
  else if (cmd == "L+" || cmd == "LEFT+") {
    leftOffset++;
    Serial.print(F("Left Offset: ")); Serial.println(leftOffset);
  }
  else if (cmd == "L-" || cmd == "LEFT-") {
    leftOffset--;
    Serial.print(F("Left Offset: ")); Serial.println(leftOffset);
  }
  else if (cmd == "R+" || cmd == "RIGHT+") {
    rightOffset++;
    Serial.print(F("Right Offset: ")); Serial.println(rightOffset);
  }
  else if (cmd == "R-" || cmd == "RIGHT-") {
    rightOffset--;
    Serial.print(F("Right Offset: ")); Serial.println(rightOffset);
  }
  else if (cmd == "W" || cmd == "RESUME" || cmd == "PLAY") {
    calibrationMode = false;
    isStopped = false;
    isHandDetected = false;
    autoDanceMode = true;
    storyStep = 0;
    storyStepTimer = millis();
    setSyncTarget(ARM_DOWN_ANGLE, 2.0);
    Serial.println(F("👋 [RESUMED] Synchronized Autonomous Loop Active!"));
  }
  else if (cmd == "U" || cmd == "UP") {
    calibrationMode = false;
    isStopped = false;
    isHandDetected = false;
    autoDanceMode = false;
    setSyncTarget(ARM_UP_ANGLE, 3.0);
    Serial.println(F("🙌 [MANUAL] Both arms UP in sync! (Type 'W' to resume auto loop)"));
  }
  else if (cmd == "D" || cmd == "DOWN") {
    calibrationMode = false;
    isStopped = false;
    isHandDetected = false;
    autoDanceMode = false;
    setSyncTarget(ARM_DOWN_ANGLE, 2.5);
    Serial.println(F("👇 [MANUAL] Both arms DOWN in sync! (Type 'W' to resume auto loop)"));
  }
  else if (cmd.startsWith("LEFT") || cmd.startsWith("L")) {
    int angle = extractFirstNumber(cmd);
    if (angle >= 0) {
      angle = constrain(angle, 0, 180);
      calibrationMode = false;
      isStopped = false;
      autoDanceMode = false;
      leftServo.write(constrain(angle + leftOffset, 0, 180));
      Serial.print(F("👈 [LEFT SERVO]: Moved to "));
      Serial.print(angle);
      Serial.println(F("°"));
    } else {
      Serial.println(F("❓ Specify angle for Left arm, e.g. 'L 90', 'L 45'"));
    }
  }
  else if (cmd.startsWith("RIGHT") || cmd.startsWith("R")) {
    int angle = extractFirstNumber(cmd);
    if (angle >= 0) {
      angle = constrain(angle, 0, 180);
      calibrationMode = false;
      isStopped = false;
      autoDanceMode = false;
      rightServo.write(constrain(angle + rightOffset, 0, 180));
      Serial.print(F("👉 [RIGHT SERVO]: Moved to "));
      Serial.print(angle);
      Serial.println(F("°"));
    } else {
      Serial.println(F("❓ Specify angle for Right arm, e.g. 'R 90', 'R 45'"));
    }
  }
  else if (cmd == "TEST" || cmd == "SWEEP" || cmd == "CHECK") {
    calibrationMode = false;
    isStopped = false;
    autoDanceMode = false;
    Serial.println(F("\n🔬 ==============================================="));
    Serial.println(F("   DIAGNOSTIC TEST - TESTING SERVOS INDEPENDENTLY"));
    Serial.println(F("==============================================="));
    Serial.println(F("  [STEP 1]: Moving LEFT arm only to 30°..."));
    leftServo.write(30 + leftOffset);
    delay(450);
    Serial.println(F("  [STEP 2]: Moving LEFT arm only to 130°..."));
    leftServo.write(130 + leftOffset);
    delay(450);
    Serial.println(F("  [STEP 3]: Centering LEFT arm at 90°..."));
    leftServo.write(90 + leftOffset);
    delay(450);

    Serial.println(F("  [STEP 4]: Moving RIGHT arm only to 30°..."));
    rightServo.write(30 + rightOffset);
    delay(450);
    Serial.println(F("  [STEP 5]: Moving RIGHT arm only to 130°..."));
    rightServo.write(130 + rightOffset);
    delay(450);
    Serial.println(F("  [STEP 6]: Centering RIGHT arm at 90°..."));
    rightServo.write(90 + rightOffset);
    delay(450);
    currentServoAngle = 90;
    targetServoAngle = 90;

    Serial.println(F("  ✅ Both arms tested independently!"));
    Serial.println(F("  👉 If an arm only slightly turned:"));
    Serial.println(F("     1. Type 'C' to lock at 90° and re-attach horn pointing down."));
    Serial.println(F("     2. If it buzzes or stutters, check 5V power (avoid weak USB)."));
    Serial.println(F("     3. Type 'M' if right arm moves in the wrong direction."));
    Serial.println(F("  Type 'U' and 'D' to test both together, or 'W' to resume auto loop."));
    Serial.println(F("===============================================\n"));
  }
  else if (cmd == "S" || cmd == "STOP" || cmd == "FREEZE") {
    isStopped = true;
    autoDanceMode = false;
    Serial.println(F("🛑 [STOP] Arms frozen at current position."));
  }
  else if (extractFirstNumber(cmd) >= 0) {
    calibrationMode = false;
    isStopped = false;
    isHandDetected = false;
    autoDanceMode = false;
    int angle = constrain(extractFirstNumber(cmd), 0, 180);
    setSyncTarget(angle, 3.0);
    Serial.print(F("🎯 [MANUAL] Moving both arms in sync to: "));
    Serial.print(angle);
    Serial.println(F("° (Type 'W' to resume auto loop)"));
  }
  else {
    Serial.println(F("❓ [UNKNOWN COMMAND]: Send '?' or 'HELP' for menu."));
  }
}

void checkSerialCommands() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    lastSerialCharTime = millis();
    if (c == '\n' || c == '\r') {
      if (serialCmdBuffer.length() > 0) {
        processSerialCommand(serialCmdBuffer);
        serialCmdBuffer = "";
      }
    } else {
      if (serialCmdBuffer.length() < 30) {
        serialCmdBuffer += c;
      }
    }
  }

  // Support "No line ending" after 60ms pause
  if (serialCmdBuffer.length() > 0 && (millis() - lastSerialCharTime > 60)) {
    processSerialCommand(serialCmdBuffer);
    serialCmdBuffer = "";
  }
}

// ----------------------------------------------------------------------------
// SETUP (RUNS ONCE ON POWER-ON)
// ----------------------------------------------------------------------------
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // 1. Initialize Serial Monitor (9600 baud)
  Serial.begin(9600);
  Serial.setTimeout(50);
  delay(100);

  printMenu();

  // 2. Initialize IR Sensor Pins
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(IR_GND_PIN, OUTPUT);
  digitalWrite(IR_GND_PIN, LOW);    // Pin 3 = Pure 0V Ground for sensor
  pinMode(IR_POWER_PIN, OUTPUT);
  digitalWrite(IR_POWER_PIN, HIGH); // Pin 4 = Pure 5V Power for sensor

  // 3. Attach Servos with standard 544-2400us pulses
  leftServo.attach(LEFT_SERVO_PIN, 544, 2400);
  rightServo.attach(RIGHT_SERVO_PIN, 544, 2400);

  // Check if starting in calibration mode
  if (calibrationMode) {
    int leftCalib  = constrain(90 + leftOffset, 0, 180);
    int rightCalib = constrain(90 + rightOffset, 0, 180);
    leftServo.write(leftCalib);
    rightServo.write(rightCalib);
    currentServoAngle = 90;
    targetServoAngle = 90;
    Serial.println(F("🎯 [BOOT]: Started in CALIBRATION MODE (Holding at 90°)"));
  } else {
    // Initial synchronized arms down position
    int initLeft  = constrain(ARM_DOWN_ANGLE + leftOffset, 0, 180);
    int initRight = constrain((MIRROR_RIGHT_ARM ? (180 - ARM_DOWN_ANGLE) : ARM_DOWN_ANGLE) + rightOffset, 0, 180);
    leftServo.write(initLeft);
    rightServo.write(initRight);
    currentServoAngle = ARM_DOWN_ANGLE;
    targetServoAngle = ARM_DOWN_ANGLE;
    Serial.println(F("🚀 [BOOT]: Synchronized Autonomous Loop Ready!"));
  }

  delay(400);
  digitalWrite(LED_PIN, LOW);

  storyStep = 0;
  storyStepTimer = millis();
  lastArmUpdateTime = millis();
}

// ----------------------------------------------------------------------------
// MAIN LOOP (RUNS CONTINUOUSLY IN C/C++)
// ----------------------------------------------------------------------------
void loop() {
  // 1. Process optional serial commands (e.g. 'C' for calibration)
  checkSerialCommands();

  // 2. If in Calibration Mode: Blink LED steadily and keep arms locked at 90°
  if (calibrationMode) {
    digitalWrite(LED_PIN, (millis() / 500) % 2);
    updateArms(); // Keeps servos locked at 90°
    return;
  }

  // 3. Check IR Sensor (Hand detection)
  checkIrSensor();

  // 4. Update Movement Routine
  if (isHandDetected) {
    updateCelebrationAnimation(); // Synchronized excitement reaction
  } else if (autoDanceMode) {
    updateAutonomousChoreography(); // Synchronized continuous dance loop
  }

  // 5. Smoothly advance both servos in locked synchrony
  updateArms();
}
