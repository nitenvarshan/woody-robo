/*
 * ============================================================================
 *   ULTRA-COOL ANIMATED ROBOT COMPANION (DISNEY/PIXAR STYLE!) 🤖✨💖
 * ============================================================================
 *   Features:
 *     - Natural Double-Blinks & Micro-Breathing Eye Physics
 *     - Dual-Glossy Reflection Twinkle Sparkles
 *     - Playful Expressions:
 *         * Normal Curious Eyes (Glancing & Micro-Blinks)
 *         * Playful Wink 😉 ('K')
 *         * Glowing Heart Eyes ❤️ ('H')
 *         * Happy Squint Smile 🙌 ('U')
 *         * Surprised Wide Eyes 😲 ('O')
 *         * 1:1 Pixel Creeper 🟩 ('1')
 *         * Glowing Herobrine 👻 ('2')
 *     - Full Synchronized Servo Arm Motion & Stop Controls
 * ============================================================================
 */

#include <Wire.h>
#include <Servo.h>

#define OLED_I2C_ADDR   0x3C
#define LEFT_SERVO_PIN  9
#define RIGHT_SERVO_PIN 10
#define LED_PIN         13

Servo leftServo;
Servo rightServo;

// ----------------------------------------------------------------------------
// 1. ROBOT MOODS & FACES
// ----------------------------------------------------------------------------
enum FaceMode {
  FACE_COOL_ROBOT,   // Ultra-cool cartoon robot eyes (Default)
  FACE_CREEPER,      // Minecraft Creeper
  FACE_HEROBRINE     // Minecraft Herobrine
};

enum RobotMood {
  MOOD_NORMAL,       // Glossy curious eyes glancing around
  MOOD_HAPPY,        // Upward crescent smile eyes + smile mouth + blush
  MOOD_WINK,         // Cheeky playful wink 😉
  MOOD_HEART,        // Adorable glowing pixel hearts ❤️
  MOOD_SURPRISED     // Wide expanded pupils 😲
};

FaceMode currentFace = FACE_COOL_ROBOT; // Default!
RobotMood currentMood = MOOD_NORMAL;

// Minecraft Pixel Maps (Available anytime!)
const uint8_t CREEPER_MAP[8] PROGMEM = {
  0b11111111, 0b10011001, 0b10011001, 0b11100111,
  0b11000011, 0b11000011, 0b11011011, 0b11011011
};

const uint8_t HEROBRINE_MAP[8] PROGMEM = {
  0b11111111, 0b11000011, 0b01100110, 0b00011000,
  0b00111100, 0b01111110, 0b00111100, 0b00000000
};

// ----------------------------------------------------------------------------
// 2. SERVO MOTION & CALIBRATION SETTINGS
// ----------------------------------------------------------------------------
int leftOffset  = 0;
int rightOffset = 0;

const int ARM_DOWN_ANGLE = 25;
const int ARM_UP_ANGLE   = 130;

float currentServoAngle  = ARM_DOWN_ANGLE;
float targetServoAngle   = ARM_DOWN_ANGLE;
float armSpeed           = 2.0;

const bool MIRROR_RIGHT_ARM = true;
bool autoDanceMode = true;
bool calibrationMode = false;
bool isStopped = false;

// ----------------------------------------------------------------------------
// 3. EYE ANIMATION PHYSICS (Saccades, Double Blinks, Breathing)
// ----------------------------------------------------------------------------
int eyeShiftX = 0;
int eyeShiftY = 0;
int eyeTargetShiftX = 0;

// Eyelid blink state
int blinkProgress = 0; // 0 = wide open, 100 = closed slit
bool isBlinking = false;
bool doubleBlinkQueued = false;
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkInterval = 3000;
unsigned long blinkStepTimer = 0;

// Timers
unsigned long lastArmUpdateTime = 0;
unsigned long lastBehaviorTime = 0;
unsigned long lastOledDraw = 0;
int currentRoutine = 0;

// ----------------------------------------------------------------------------
// 4. HARDWARE SH1106 OLED CONTROLLER
// ----------------------------------------------------------------------------

void sendSH1106Command(uint8_t cmd) {
  Wire.beginTransmission(OLED_I2C_ADDR);
  Wire.write(0x80);
  Wire.write(cmd);
  Wire.endTransmission();
}

void initSH1106() {
  Wire.begin();
  #if defined(WIRE_HAS_TIMEOUT)
    Wire.setWireTimeout(25000, true);
  #endif
  delay(50);

  static const uint8_t initSeq[] PROGMEM = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0xAD, 0x8B, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xBF,
    0xD9, 0x22, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
  };

  for (uint8_t i = 0; i < sizeof(initSeq); i++) {
    sendSH1106Command(pgm_read_byte(&initSeq[i]));
  }
}

// ----------------------------------------------------------------------------
// 5. ULTRA-COOL ROBOT EYE RENDERING ENGINE
// ----------------------------------------------------------------------------

// Check if pixel (x, y) falls inside a glowing Heart shape
bool isInsideHeart(int x, int y, int centerX, int centerY) {
  int relX = x - centerX;
  int relY = y - (centerY - 1);
  // Scale down to 14x14 heart math
  float hx = (float)relX / 9.0;
  float hy = (float)(-relY) / 9.0;
  // Heart formula: (x^2 + y^2 - 1)^3 - x^2 * y^3 <= 0
  float a = (hx*hx + hy*hy - 1.0);
  return ((a*a*a - hx*hx*hy*hy*hy) <= 0.0);
}

// Check if pixel (x, y) falls inside an eye
bool isInsideEyePixel(int x, int y, int centerX, int centerY, bool isRightEye) {
  // If HEART MODE: Draw cute glowing hearts!
  if (currentMood == MOOD_HEART) {
    return isInsideHeart(x, y, centerX + eyeShiftX, centerY);
  }

  // If WINK MODE: Right eye winks shut in a cute curved arc, Left eye stays wide open!
  if (currentMood == MOOD_WINK && isRightEye) {
    int dx = x - (centerX + eyeShiftX);
    int dy = y - (centerY + 2);
    // Cute curved wink line
    return (dx >= -12 && dx <= 12 && abs(dy - (dx*dx)/28) <= 2);
  }

  // If HAPPY SQUINT: Inverted upward smiling crescents
  if (currentMood == MOOD_HAPPY) {
    int dx = x - (centerX + eyeShiftX);
    int dy = y - (centerY + 3);
    if (dx*dx + dy*dy <= 196) { // radius 14
      if (y <= centerY + 4 && (dx*dx + (dy - 7)*(dy - 7) > 64)) {
        return true;
      }
    }
    return false;
  }

  // NORMAL / SURPRISED EYES with dynamic eyelid physics
  int curX = centerX + eyeShiftX;
  int curY = centerY;

  int w = (currentMood == MOOD_SURPRISED) ? 30 : 26;
  int fullH = (currentMood == MOOD_SURPRISED) ? 38 : 34;

  // Calculate current eye height based on blink progress
  int h = map(blinkProgress, 0, 100, fullH, 3);
  if (h < 3) h = 3;

  int x0 = curX - w / 2;
  int x1 = curX + w / 2;
  int y0 = curY - h / 2;
  int y1 = curY + h / 2;

  if (x >= x0 && x <= x1 && y >= y0 && y <= y1) {
    // Eyelid slit when fully shut
    if (blinkProgress >= 85) return true;

    // Smooth rounded corners
    int rad = 6;
    if (x < x0 + rad && y < y0 + rad) {
      int dx = x - (x0 + rad); int dy = y - (y0 + rad);
      if (dx*dx + dy*dy > rad*rad) return false;
    }
    if (x > x1 - rad && y < y0 + rad) {
      int dx = x - (x1 - rad); int dy = y - (y0 + rad);
      if (dx*dx + dy*dy > rad*rad) return false;
    }
    if (x < x0 + rad && y > y1 - rad) {
      int dx = x - (x0 + rad); int dy = y - (y1 - rad);
      if (dx*dx + dy*dy > rad*rad) return false;
    }
    if (x > x1 - rad && y > y1 - rad) {
      int dx = x - (x1 - rad); int dy = y - (y1 - rad);
      if (dx*dx + dy*dy > rad*rad) return false;
    }

    // --- DUAL GLOSSY SPARKLE CUTOUTS (Disney/WALL-E style!) ---
    if (blinkProgress < 20) {
      // 1. Primary Highlight (Big 4x4 glossy reflection)
      if (x >= curX + 3 && x <= curX + 7 && y >= curY - 12 && y <= curY - 8) {
        return false;
      }
      // 2. Secondary Highlight (Cute 2x2 baby twinkle dot)
      if (x >= curX - 7 && x <= curX - 5 && y >= curY + 6 && y <= curY + 8) {
        return false;
      }
    }

    return true;
  }

  return false;
}

// Renders the full OLED screen
void renderFaceToScreen() {
  for (uint8_t p = 0; p < 8; p++) {
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 + p);
    Wire.write(0x02);
    Wire.write(0x10);
    Wire.endTransmission();

    for (uint8_t col = 0; col < 128; col++) {
      uint8_t pixelByte = 0x00;

      // ======================================================================
      // FACE 1: ULTRA-COOL ROBOT FACE (DEFAULT!)
      // ======================================================================
      if (currentFace == FACE_COOL_ROBOT) {
        for (uint8_t bit = 0; bit < 8; bit++) {
          int y = p * 8 + bit;
          bool on = false;

          // Left Eye (Center X=38, Y=28)
          if (isInsideEyePixel(col, y, 38, 28, false)) on = true;

          // Right Eye (Center X=90, Y=28)
          if (isInsideEyePixel(col, y, 90, 28, true)) on = true;

          // --- MOUTH & ACCENTS ---
          if (currentMood == MOOD_HAPPY || currentMood == MOOD_HEART) {
            // Cheerful Smile Arc at center X=64, Y=50
            int dx = col - 64;
            int dy = y - 48;
            if (dx*dx + dy*dy <= 49 && dx*dx + dy*dy >= 25 && y >= 49) on = true;

            // Rosy Cute Blush lines under eyes
            if (y == 44 && ((col >= 22 && col <= 26) || (col >= 102 && col <= 106))) on = true;
          } else if (currentMood == MOOD_WINK) {
            // Cheeky Smirk mouth
            if (y == 51 && col >= 62 && col <= 72) on = true;
            if (y == 50 && col == 73) on = true;
          } else if (currentMood == MOOD_SURPRISED) {
            // Cute "O" Mouth
            int dx = col - 64;
            int dy = y - 51;
            if (dx*dx + dy*dy <= 20 && dx*dx + dy*dy >= 8) on = true;
          } else {
            // Calm cute robot mouth
            if (y == 52 && col >= 60 && col <= 68) on = true;
            if (y == 51 && (col == 59 || col == 69)) on = true;
          }

          if (on) pixelByte |= (1 << bit);
        }
      }

      // ======================================================================
      // FACE 2: MINECRAFT CREEPER
      // ======================================================================
      else if (currentFace == FACE_CREEPER) {
        if (col >= 40 && col < 88) {
          int faceCol = (col - 40) / 6;
          for (uint8_t bit = 0; bit < 8; bit++) {
            int y = p * 8 + bit;
            if (y >= 8 && y < 56) {
              int faceRow = (y - 8) / 6;
              uint8_t rowData = pgm_read_byte(&CREEPER_MAP[faceRow]);
              bool feature = (rowData & (1 << (7 - faceCol))) != 0;

              if (blinkProgress >= 80 && (faceRow == 1 || faceRow == 2) && (faceCol == 1 || faceCol == 2 || faceCol == 5 || faceCol == 6)) {
                if (faceRow == 2 && (y % 6 >= 3)) pixelByte |= (1 << bit);
              } else {
                if (feature && ((col + y) % 6 != 0)) pixelByte |= (1 << bit);
              }
            }
          }
        }
      }

      // ======================================================================
      // FACE 3: GLOWING HEROBRINE
      // ======================================================================
      else if (currentFace == FACE_HEROBRINE) {
        if (col >= 40 && col < 88) {
          int faceCol = (col - 40) / 6;
          for (uint8_t bit = 0; bit < 8; bit++) {
            int y = p * 8 + bit;
            if (y >= 8 && y < 56) {
              int faceRow = (y - 8) / 6;
              uint8_t rowData = pgm_read_byte(&HEROBRINE_MAP[faceRow]);
              bool feature = (rowData & (1 << (7 - faceCol))) != 0;

              if (faceRow == 2 && (faceCol == 1 || faceCol == 2 || faceCol == 5 || faceCol == 6)) {
                if (blinkProgress < 80) pixelByte |= (1 << bit);
              } else {
                if (feature) pixelByte |= (1 << bit);
              }
            }
          }
        }
      }

      if (col % 16 == 0) {
        Wire.endTransmission();
        Wire.beginTransmission(OLED_I2C_ADDR);
        Wire.write(0x40);
      }
      Wire.write(pixelByte);
    }
    Wire.endTransmission();
  }
}

// ----------------------------------------------------------------------------
// 6. LIFE-LIKE ANIMATION PHYSICS & DOUBLE-BLINKS
// ----------------------------------------------------------------------------

void updateBlinkingAndEyes() {
  unsigned long now = millis();

  // Smooth Saccadic Eye Glancing (eased movement)
  if (eyeShiftX != eyeTargetShiftX) {
    if (eyeShiftX < eyeTargetShiftX) eyeShiftX++;
    else eyeShiftX--;
  }

  // Natural Blinking Machine
  if (!isBlinking && (now - lastBlinkTime > nextBlinkInterval)) {
    isBlinking = true;
    blinkProgress = 0;
    blinkStepTimer = now;
    // 30% chance to do a playful double-blink!
    doubleBlinkQueued = (random(0, 10) < 3);
  }

  if (isBlinking) {
    if (now - blinkStepTimer >= 18) { // 18ms per frame step
      blinkStepTimer = now;
      if (blinkProgress < 100) {
        blinkProgress += 25; // Quick closing
      } else {
        // Pop back open!
        blinkProgress = 0;
        isBlinking = false;
        lastBlinkTime = now;

        if (doubleBlinkQueued) {
          doubleBlinkQueued = false;
          nextBlinkInterval = 120; // Immediate second blink!
        } else {
          nextBlinkInterval = random(2400, 5000);
        }
      }
    }
  }
}

// ----------------------------------------------------------------------------
// 7. SYNCHRONIZED ARMS & BEHAVIOR
// ----------------------------------------------------------------------------

void updateArms() {
  if (calibrationMode) {
    int leftCalib  = constrain(90 + leftOffset, 0, 180);
    int rightCalib = constrain((MIRROR_RIGHT_ARM ? (180 - 90) : 90) + rightOffset, 0, 180);
    leftServo.write(leftCalib);
    rightServo.write(rightCalib);
    return;
  }

  if (isStopped) return;

  if (millis() - lastArmUpdateTime >= 20) {
    lastArmUpdateTime = millis();

    if (abs(currentServoAngle - targetServoAngle) > 0.5) {
      if (currentServoAngle < targetServoAngle) {
        currentServoAngle += armSpeed;
        if (currentServoAngle > targetServoAngle) currentServoAngle = targetServoAngle;
      } else {
        currentServoAngle -= armSpeed;
        if (currentServoAngle < targetServoAngle) currentServoAngle = targetServoAngle;
      }

      int leftAngle = constrain((int)currentServoAngle + leftOffset, 0, 180);
      int baseRight = MIRROR_RIGHT_ARM ? (180 - (int)currentServoAngle) : (int)currentServoAngle;
      int rightAngle = constrain(baseRight + rightOffset, 0, 180);

      leftServo.write(leftAngle);
      rightServo.write(rightAngle);
    }
  }
}

void updateRobotBehavior() {
  if (!autoDanceMode || calibrationMode || isStopped) return;

  unsigned long now = millis();

  if (now - lastBehaviorTime > 3600) {
    lastBehaviorTime = now;
    currentRoutine = (currentRoutine + 1) % 5;

    switch (currentRoutine) {
      // 0: Relaxed posture + Normal Curious Eyes
      case 0:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.5;
        eyeTargetShiftX = 0;
        currentMood = MOOD_NORMAL;
        Serial.println(F("[ROBOT] Resting posture"));
        break;

      // 1: Arms UP! Super Happy Squint + Rosy Cheeks! 🙌
      case 1:
        targetServoAngle = ARM_UP_ANGLE;
        armSpeed = 2.5;
        eyeTargetShiftX = 0;
        currentMood = MOOD_HAPPY;
        Serial.println(F("[ROBOT] Arms UP & Super Happy! 🙌"));
        break;

      // 2: Glance Left with Curious Eyes
      case 2:
        targetServoAngle = 80;
        armSpeed = 3.0;
        eyeTargetShiftX = -5; // Glance Left
        currentMood = MOOD_NORMAL;
        Serial.println(F("[ROBOT] Glancing Left"));
        break;

      // 3: Playful Wink 😉
      case 3:
        targetServoAngle = 100;
        armSpeed = 2.0;
        eyeTargetShiftX = 3;
        currentMood = MOOD_WINK;
        Serial.println(F("[ROBOT] Playful Wink! 😉"));
        break;

      // 4: Arms Down + Glance Right
      case 4:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.8;
        eyeTargetShiftX = 5; // Glance Right
        currentMood = MOOD_NORMAL;
        Serial.println(F("[ROBOT] Glancing Right"));
        break;
    }
  }
}

// ----------------------------------------------------------------------------
// 8. SERIAL COMMAND PROCESSOR
// ----------------------------------------------------------------------------

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    // --- INSTANT EXPRESSION COMMANDS ---
    if (cmd == "H" || cmd == "HEART") {
      currentFace = FACE_COOL_ROBOT;
      currentMood = MOOD_HEART;
      Serial.println(F("\n💖 [EXPRESSION]: Glowing Heart Eyes! ❤️"));
    }
    else if (cmd == "K" || cmd == "WINK") {
      currentFace = FACE_COOL_ROBOT;
      currentMood = MOOD_WINK;
      Serial.println(F("\n😉 [EXPRESSION]: Playful Cheeky Wink!"));
    }
    else if (cmd == "O" || cmd == "SURPRISED") {
      currentFace = FACE_COOL_ROBOT;
      currentMood = MOOD_SURPRISED;
      Serial.println(F("\n😲 [EXPRESSION]: Surprised Wide Eyes!"));
    }
    else if (cmd == "3" || cmd == "ROBOT") {
      currentFace = FACE_COOL_ROBOT;
      currentMood = MOOD_NORMAL;
      Serial.println(F("\n🤖 [EXPRESSION]: Cute Cartoon Robot Eyes (Default)"));
    }
    else if (cmd == "1" || cmd == "CREEPER") {
      currentFace = FACE_CREEPER;
      Serial.println(F("\n🟩 [FACE]: 1:1 Pixel Minecraft Creeper!"));
    }
    else if (cmd == "2" || cmd == "HEROBRINE") {
      currentFace = FACE_HEROBRINE;
      Serial.println(F("\n👻 [FACE]: Spooky Glowing Herobrine Face!"));
    }

    // --- ARM MOTION COMMANDS ---
    else if (cmd == "S" || cmd == "STOP") {
      isStopped = true;
      autoDanceMode = false;
      calibrationMode = false;
      targetServoAngle = currentServoAngle;
      Serial.println(F("\n🛑 [MOTION STOPPED / FROZEN] 🛑"));
    }
    else if (cmd == "C") {
      isStopped = false;
      calibrationMode = true;
      autoDanceMode = false;
      Serial.println(F("\n>>> [CALIBRATION MODE: Holding at 90 deg] <<<"));
    }
    else if (cmd == "L+") {
      leftOffset++;
      Serial.print(F("Left Offset: ")); Serial.println(leftOffset);
    } else if (cmd == "L-") {
      leftOffset--;
      Serial.print(F("Left Offset: ")); Serial.println(leftOffset);
    } else if (cmd == "R+") {
      rightOffset++;
      Serial.print(F("Right Offset: ")); Serial.println(rightOffset);
    } else if (cmd == "R-") {
      rightOffset--;
      Serial.print(F("Right Offset: ")); Serial.println(rightOffset);
    }
    else if (cmd == "U") {
      isStopped = false;
      calibrationMode = false;
      autoDanceMode = false;
      targetServoAngle = ARM_UP_ANGLE;
      currentMood = MOOD_HAPPY;
      Serial.println(F("[COMMAND] Arms UP & Happy! 🙌"));
    }
    else if (cmd == "D") {
      isStopped = false;
      calibrationMode = false;
      autoDanceMode = false;
      targetServoAngle = ARM_DOWN_ANGLE;
      currentMood = MOOD_NORMAL;
      Serial.println(F("[COMMAND] Arms DOWN! 👇"));
    }
    else if (cmd == "W") {
      isStopped = false;
      calibrationMode = false;
      autoDanceMode = true;
      Serial.println(F("[COMMAND] Auto Dance / Wave Mode ON! 👋"));
    }
    else if (cmd.toInt() > 0 || cmd == "0") {
      isStopped = false;
      calibrationMode = false;
      autoDanceMode = false;
      int angle = constrain(cmd.toInt(), 0, 180);
      targetServoAngle = angle;
      Serial.print(F("[COMMAND] Moving to angle: "));
      Serial.println(angle);
    }
  }
}

// ----------------------------------------------------------------------------
// 9. SETUP & MAIN LOOP
// ----------------------------------------------------------------------------

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.begin(115200);
  Serial.println(F("\n============================================="));
  Serial.println(F("   ULTRA-COOL ANIMATED ROBOT READY! 🤖✨💖"));
  Serial.println(F("   COOL EXPRESSIONS:"));
  Serial.println(F("     'H' = Heart Eyes ❤️  |  'K' = Playful Wink 😉"));
  Serial.println(F("     'O' = Surprised 😲   |  '3' = Cartoon Eyes"));
  Serial.println(F("     '1' = Creeper 🟩     |  '2' = Herobrine 👻"));
  Serial.println(F("   ARM CONTROLS:"));
  Serial.println(F("     'S' = Stop/Freeze    |  'W' = Dance"));
  Serial.println(F("     'U' = Arms Up        |  'D' = Arms Down"));
  Serial.println(F("     'C' = Calibrate 90 deg"));
  Serial.println(F("=============================================\n"));

  // 1. Initialize Servos
  leftServo.attach(LEFT_SERVO_PIN);
  rightServo.attach(RIGHT_SERVO_PIN);

  leftServo.write(ARM_DOWN_ANGLE);
  rightServo.write(MIRROR_RIGHT_ARM ? (180 - ARM_DOWN_ANGLE) : ARM_DOWN_ANGLE);
  delay(300);

  // 2. Initialize OLED Display
  initSH1106();
  renderFaceToScreen();

  lastBlinkTime = millis();
  lastBehaviorTime = millis();
}

void loop() {
  // 1. Heartbeat LED
  digitalWrite(LED_PIN, (millis() / 500) % 2);

  // 2. Check Serial Commands
  checkSerialCommands();

  // 3. Update Eyelid Physics & Eye Glancing
  updateBlinkingAndEyes();

  // 4. Update Arm Positions & Routines
  updateArms();
  updateRobotBehavior();

  // 5. Render Screen at smooth ~25 FPS
  if (millis() - lastOledDraw >= 40) {
    lastOledDraw = millis();
    renderFaceToScreen();
  }
}
