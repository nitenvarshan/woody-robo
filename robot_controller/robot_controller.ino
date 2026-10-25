/*
 * ============================================================================
 *   CUTE ANIMATED ROBOT COMPANION (DEFAULT: ADORABLE BLINKING EYES!) 🤖✨
 *   Dual-Arm Synchronized Robot + Expressive Cartoon OLED Face
 * ============================================================================
 *   Serial Commands (115200 baud):
 *     - '3' or 'ROBOT'     = Cute Animated Blinking Robot Face! 🤖 (DEFAULT)
 *     - '1' or 'CREEPER'   = Pixel-Accurate Minecraft Creeper! 🟩
 *     - '2' or 'HEROBRINE' = Spooky Glowing Herobrine Face! 👻
 *
 *     - 'S'   = STOP / FREEZE ARMS! 🛑
 *     - 'W'   = RESUME Auto-Dance / Wave! 👋
 *     - 'U'   = Both Arms UP! 🙌
 *     - 'D'   = Both Arms DOWN! 👇
 *     - 'C'   = Calibration Mode (Locks at 90°) 🎯
 *     - 'L+' / 'L-' = Left arm trim
 *     - 'R+' / 'R-' = Right arm trim
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
// 1. FACE MODES (DEFAULT: CUTE ROBOT!)
// ----------------------------------------------------------------------------
enum FaceMode {
  FACE_CUTE_ROBOT,   // Default: Big adorable blinking eyes!
  FACE_CREEPER,      // 1:1 Pixel Minecraft Creeper
  FACE_HEROBRINE     // Glowing Herobrine Face
};

FaceMode currentFace = FACE_CUTE_ROBOT; // <-- DEFAULT FACE!

// Minecraft Creeper Pixel Map
const uint8_t CREEPER_MAP[8] PROGMEM = {
  0b11111111,
  0b10011001,
  0b10011001,
  0b11100111,
  0b11000011,
  0b11000011,
  0b11011011,
  0b11011011
};

// Minecraft Herobrine Pixel Map
const uint8_t HEROBRINE_MAP[8] PROGMEM = {
  0b11111111,
  0b11000011,
  0b01100110,
  0b00011000,
  0b00111100,
  0b01111110,
  0b00111100,
  0b00000000
};

// ----------------------------------------------------------------------------
// 2. CALIBRATION & MOTION SETTINGS
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

// Timers
unsigned long lastArmUpdateTime = 0;
unsigned long lastActionTime = 0;
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkInterval = 3200;
unsigned long lastOledDraw = 0;

int blinkState = 0; // 0 = open, 1 = half, 2 = fully closed
int currentRoutine = 0;
int eyeShiftX = 0;
bool isHappy = false;

// ----------------------------------------------------------------------------
// 3. HARDWARE SH1106 OLED CONTROLLER
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
// 4. THE CUTE BLINKING ROBOT FACE ENGINE
// ----------------------------------------------------------------------------
bool isInsideCuteEye(int x, int y, int centerX, int centerY, int shiftX, int blink, bool happy) {
  int curX = centerX + shiftX;
  int curY = centerY;

  int w = 26; // Eye width
  int h = 34; // Eye height when open

  // If Happy: eyes turn into upward curved crescents!
  if (happy) {
    int dx = x - curX;
    int dy = y - (curY + 2);
    // Outer circle
    if (dx*dx + dy*dy <= 169) { // radius 13
      // Cut out bottom to make a smiling crescent curve
      if (y <= curY + 4 && (dx*dx + (dy-6)*(dy-6) > 49)) {
        return true;
      }
    }
    return false;
  }

  // Normal Eyelid Blinking
  if (blink == 2) {
    // Stage 2: Closed eye slit (cute rounded horizontal line)
    return (x >= curX - 12 && x <= curX + 12 && y >= curY - 2 && y <= curY + 2);
  } else if (blink == 1) {
    // Stage 1: Half-closed drooping eye
    h = 16;
    curY += 6;
  }

  int x0 = curX - w / 2;
  int x1 = curX + w / 2;
  int y0 = curY - h / 2;
  int y1 = curY + h / 2;

  if (x >= x0 && x <= x1 && y >= y0 && y <= y1) {
    // Cut rounded corners
    if ((x == x0 || x == x1) && (y == y0 || y == y1)) return false;
    if ((x == x0+1 || x == x1-1) && (y == y0 || y == y1)) return false;
    if ((x == x0 || x == x1) && (y == y0+1 || y == y1-1)) return false;

    // Shiny Gloss Reflection Dot (top-right sparkle)
    if (blink == 0 && (x >= curX + 4 && x <= curX + 7 && y >= curY - 11 && y <= curY - 8)) {
      return false; // Sparkle cutout!
    }
    return true;
  }
  return false;
}

// Renders the chosen face directly to the 1.3" OLED screen
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
      // FACE 1: CUTE BLINKING ANIMATED ROBOT (DEFAULT!)
      // ======================================================================
      if (currentFace == FACE_CUTE_ROBOT) {
        for (uint8_t bit = 0; bit < 8; bit++) {
          int y = p * 8 + bit;
          bool on = false;

          // Left Eye (Center X=38, Y=28)
          if (isInsideCuteEye(col, y, 38, 28, eyeShiftX, blinkState, isHappy)) {
            on = true;
          }
          // Right Eye (Center X=90, Y=28)
          if (isInsideCuteEye(col, y, 90, 28, eyeShiftX, blinkState, isHappy)) {
            on = true;
          }

          // Cute Smile Mouth (At center X=64, Y=51)
          if (isHappy) {
            // Big happy smile arc
            int dx = col - 64;
            int dy = y - 48;
            if (dx*dx + dy*dy <= 49 && dx*dx + dy*dy >= 25 && y >= 49) {
              on = true;
            }
          } else {
            // Calm little friendly mouth
            if (y == 52 && col >= 60 && col <= 68) {
              on = true;
            } else if (y == 51 && (col == 59 || col == 69)) {
              on = true; // upturned corners!
            }
          }

          // Rosy Cheeks (Cute little blush lines under eyes!)
          if (isHappy && y == 44) {
            if ((col >= 22 && col <= 26) || (col >= 102 && col <= 106)) on = true;
          }

          if (on) pixelByte |= (1 << bit);
        }
      }

      // ======================================================================
      // FACE 2: MINECRAFT CREEPER (1:1 Square)
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

              if (blinkState == 2 && (faceRow == 1 || faceRow == 2) && (faceCol == 1 || faceCol == 2 || faceCol == 5 || faceCol == 6)) {
                if (faceRow == 2 && (y % 6 >= 3)) pixelByte |= (1 << bit);
              } else {
                if (feature) {
                  if ((col + y) % 6 != 0) pixelByte |= (1 << bit); // Pixel texture
                }
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
                if (blinkState != 2) pixelByte |= (1 << bit); // Glowing eyes!
              } else {
                if (feature) pixelByte |= (1 << bit);
              }
            }
          }
        }
      }

      // Send 16-byte burst
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
// 5. UPDATE FUNCTIONS
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

// Fluid 3-stage natural eyelid blinking
void updateBlinking() {
  unsigned long now = millis();

  static unsigned long blinkStageTimer = 0;

  if (blinkState == 0 && (now - lastBlinkTime > nextBlinkInterval)) {
    blinkState = 1; // Stage 1: Half-closed
    blinkStageTimer = now;
  } else if (blinkState == 1 && (now - blinkStageTimer > 50)) {
    blinkState = 2; // Stage 2: Fully closed slit
    blinkStageTimer = now;
  } else if (blinkState == 2 && (now - blinkStageTimer > 90)) {
    blinkState = 0; // Stage 0: Pop wide open!
    lastBlinkTime = now;
    nextBlinkInterval = random(2500, 5500);
  }
}

void updateRobotBehavior() {
  if (!autoDanceMode || calibrationMode || isStopped) return;

  unsigned long now = millis();

  if (now - lastActionTime > 3500) {
    lastActionTime = now;
    currentRoutine = (currentRoutine + 1) % 4;

    switch (currentRoutine) {
      // 0: Arms Resting Down + Look Straight
      case 0:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.5;
        eyeShiftX = 0;
        isHappy = false;
        Serial.println(F("[ROBOT] Resting posture"));
        break;

      // 1: Arms UP in the air! Face smiles happily! 🙌
      case 1:
        targetServoAngle = ARM_UP_ANGLE;
        armSpeed = 2.5;
        eyeShiftX = 0;
        isHappy = true; // Happy smiling eyes + smile mouth!
        Serial.println(F("[ROBOT] Hands UP & Happy Smile! 🙌"));
        break;

      // 2: Glance Left
      case 2:
        targetServoAngle = 80;
        armSpeed = 3.0;
        eyeShiftX = -4; // Look Left
        isHappy = false;
        Serial.println(F("[ROBOT] Glancing Left"));
        break;

      // 3: Glance Right
      case 3:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.8;
        eyeShiftX = 4; // Look Right
        isHappy = false;
        Serial.println(F("[ROBOT] Glancing Right"));
        break;
    }
  }
}

// ----------------------------------------------------------------------------
// 6. SERIAL COMMAND PROCESSOR
// ----------------------------------------------------------------------------

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "3" || cmd == "ROBOT") {
      currentFace = FACE_CUTE_ROBOT;
      Serial.println(F("\n🤖 [FACE]: Cute Blinking Animated Robot Face! (Default)"));
    }
    else if (cmd == "1" || cmd == "CREEPER") {
      currentFace = FACE_CREEPER;
      Serial.println(F("\n🟩 [FACE]: 1:1 Pixel Minecraft Creeper!"));
    }
    else if (cmd == "2" || cmd == "HEROBRINE") {
      currentFace = FACE_HEROBRINE;
      Serial.println(F("\n👻 [FACE]: Spooky Glowing Herobrine Face!"));
    }
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
      isHappy = true;
      Serial.println(F("[COMMAND] Arms UP! 🙌"));
    }
    else if (cmd == "D") {
      isStopped = false;
      calibrationMode = false;
      autoDanceMode = false;
      targetServoAngle = ARM_DOWN_ANGLE;
      isHappy = false;
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
// 7. SETUP & MAIN LOOP
// ----------------------------------------------------------------------------

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.begin(115200);
  Serial.println(F("\n============================================="));
  Serial.println(F("   CUTE ANIMATED ROBOT READY! 🤖✨"));
  Serial.println(F("   Default Face: Cute Blinking Cartoon Eyes"));
  Serial.println(F("   Commands:"));
  Serial.println(F("     '3' = Cute Robot (Default) | '1' = Creeper | '2' = Herobrine"));
  Serial.println(F("     'S' = Stop | 'W' = Dance | 'U' = Up | 'D' = Down"));
  Serial.println(F("     'C' = Calibrate"));
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
  lastActionTime = millis();
}

void loop() {
  // 1. Heartbeat LED
  digitalWrite(LED_PIN, (millis() / 500) % 2);

  // 2. Read Serial Commands
  checkSerialCommands();

  // 3. Move Arms
  updateArms();
  updateBlinking();
  updateRobotBehavior();

  // 4. Render Face at smooth ~20 FPS
  if (millis() - lastOledDraw >= 50) {
    lastOledDraw = millis();
    renderFaceToScreen();
  }
}
