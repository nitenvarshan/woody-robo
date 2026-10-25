/*
 * ============================================================================
 *   BUTTERY-SMOOTH ANIMATED ROBOT (WITH FLUID ANIMATED WINK!) 🤖😉✨
 * ============================================================================
 *   Interactive Serial Commands (115200 baud):
 *     - 'K' or 'WINK'      = Smooth Animated One-Eye Wink! 😉
 *     - 'H' or 'HEART'     = Glowing Heart Eyes! ❤️
 *     - 'O' or 'SURPRISED' = Surprised Wide Eyes! 😲
 *     - '3' or 'ROBOT'     = Cute Cartoon Robot Eyes! 🤖 (Default)
 *     - '1' or 'CREEPER'   = 1:1 Pixel Minecraft Creeper! 🟩
 *     - '2' or 'HEROBRINE' = Glowing Herobrine Face! 👻
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
// 1. MOODS & MODES
// ----------------------------------------------------------------------------
enum FaceMode {
  FACE_COOL_ROBOT,
  FACE_CREEPER,
  FACE_HEROBRINE
};

enum RobotMood {
  MOOD_NORMAL,
  MOOD_HAPPY,
  MOOD_WINK,
  MOOD_HEART,
  MOOD_SURPRISED
};

FaceMode currentFace = FACE_COOL_ROBOT;
RobotMood currentMood = MOOD_NORMAL;

// Minecraft Bitmaps
const uint8_t CREEPER_MAP[8] PROGMEM = {
  0b11111111, 0b10011001, 0b10011001, 0b11100111,
  0b11000011, 0b11000011, 0b11011011, 0b11011011
};

const uint8_t HEROBRINE_MAP[8] PROGMEM = {
  0b11111111, 0b11000011, 0b01100110, 0b00011000,
  0b00111100, 0b01111110, 0b00111100, 0b00000000
};

// ----------------------------------------------------------------------------
// 2. SERVO MOTION & CALIBRATION
// ----------------------------------------------------------------------------
int leftOffset  = 0;
int rightOffset = 0;

const int ARM_DOWN_ANGLE = 25;
const int ARM_UP_ANGLE   = 130;

float currentServoAngle  = ARM_DOWN_ANGLE;
float targetServoAngle   = ARM_DOWN_ANGLE;
float armSpeed           = 2.2;

const bool MIRROR_RIGHT_ARM = true;
bool autoDanceMode = true;
bool calibrationMode = false;
bool isStopped = false;

// ----------------------------------------------------------------------------
// 3. SILK-SMOOTH EYE PHYSICS & ANIMATION
// ----------------------------------------------------------------------------
float eyeShiftX = 0.0;
float eyeTargetShiftX = 0.0;

// Symmetrical double-eye blink
float blinkProgress = 0.0;
int blinkDirection = 0; // 0 = idle, 1 = closing, -1 = opening
bool doubleBlinkQueued = false;
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkInterval = 3000;

// Independent Animated Wink Engine (Right Eye)
float winkProgress = 0.0;       // 0 = open, 100 = full cheeky wink
int winkDirection = 0;          // 0 = idle, 1 = closing, 2 = holding, -1 = opening
unsigned long winkHoldTimer = 0;

// Timers
unsigned long lastArmUpdateTime = 0;
unsigned long lastBehaviorTime = 0;
unsigned long lastOledDraw = 0;
int currentRoutine = 0;

// ----------------------------------------------------------------------------
// 4. HIGH-SPEED SH1106 CONTROLLER (400 kHz Fast-Mode!)
// ----------------------------------------------------------------------------

void sendSH1106Command(uint8_t cmd) {
  Wire.beginTransmission(OLED_I2C_ADDR);
  Wire.write(0x80);
  Wire.write(cmd);
  Wire.endTransmission();
}

void initSH1106() {
  Wire.begin();
  Wire.setClock(400000); // 400 kHz Fast I2C Bus!
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
// 5. ULTRA-OPTIMIZED RENDERER WITH SMOOTH WINKING
// ----------------------------------------------------------------------------

const uint16_t HEART_BITMAP[14] PROGMEM = {
  0b00110000110000,
  0b01111001111000,
  0b11111111111100,
  0b11111111111100,
  0b11111111111100,
  0b11111111111100,
  0b01111111111000,
  0b00111111110000,
  0b00011111100000,
  0b00001111000000,
  0b00000110000000,
  0b00000000000000,
  0b00000000000000,
  0b00000000000000
};

void triggerSmoothWink() {
  currentFace = FACE_COOL_ROBOT;
  currentMood = MOOD_WINK;
  winkProgress = 0.0;
  winkDirection = 1; // Start smoothly squeezing shut!
}

void renderFastScreen() {
  int shiftInt = (int)round(eyeShiftX);
  int curBlink = (int)round(blinkProgress);
  int curWink  = (int)round(winkProgress);

  int leftEyeCenterX = 38 + shiftInt;
  int rightEyeCenterX = 90 + shiftInt;
  int eyeCenterY = 28;

  int eyeHalfW = (currentMood == MOOD_SURPRISED) ? 15 : 13;
  int eyeFullH = (currentMood == MOOD_SURPRISED) ? 38 : 34;

  // Left Eye Height (controlled by normal blink)
  int leftHalfH = map(curBlink, 0, 100, eyeFullH / 2, 2);
  if (leftHalfH < 2) leftHalfH = 2;

  // Right Eye Height (affected by normal blink OR smooth wink!)
  int effectiveRightBlink = max(curBlink, curWink);
  int rightHalfH = map(effectiveRightBlink, 0, 100, eyeFullH / 2, 2);
  if (rightHalfH < 2) rightHalfH = 2;

  for (uint8_t p = 0; p < 8; p++) {
    int pageTopY = p * 8;
    int pageBotY = pageTopY + 7;

    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 + p);
    Wire.write(0x02);
    Wire.write(0x10);
    Wire.endTransmission();

    for (uint8_t col = 0; col < 128; col++) {
      uint8_t pixelByte = 0x00;

      // ======================================================================
      // 1. CUTE ROBOT EYES (40 FPS Optimized!)
      // ======================================================================
      if (currentFace == FACE_COOL_ROBOT) {

        // --- HEART EYES MOOD ---
        if (currentMood == MOOD_HEART) {
          int relLX = col - (leftEyeCenterX - 7);
          int relRX = col - (rightEyeCenterX - 7);
          int heartX = -1;
          if (relLX >= 0 && relLX < 14) heartX = relLX;
          else if (relRX >= 0 && relRX < 14) heartX = relRX;

          if (heartX >= 0) {
            for (uint8_t bit = 0; bit < 8; bit++) {
              int y = pageTopY + bit;
              int relY = y - (eyeCenterY - 7);
              if (relY >= 0 && relY < 14) {
                uint16_t row = pgm_read_word(&HEART_BITMAP[relY]);
                if (row & (1 << (13 - heartX))) {
                  pixelByte |= (1 << bit);
                }
              }
            }
          }
        }
        // --- NORMAL / HAPPY / WINK / SURPRISED EYES ---
        else {
          bool inLeftX = (col >= leftEyeCenterX - eyeHalfW && col <= leftEyeCenterX + eyeHalfW);
          bool inRightX = (col >= rightEyeCenterX - eyeHalfW && col <= rightEyeCenterX + eyeHalfW);

          if (inLeftX || inRightX) {
            bool isRightEye = inRightX;
            int curCenterX = isRightEye ? rightEyeCenterX : leftEyeCenterX;
            int curHalfH   = isRightEye ? rightHalfH : leftHalfH;
            int curTopY    = eyeCenterY - curHalfH;
            int curBotY    = eyeCenterY + curHalfH;
            int curEyeBlink = isRightEye ? effectiveRightBlink : curBlink;

            // SMOOTH WINK: When right eye is fully or mostly squeezed, draw a thick cute smile curve!
            if (isRightEye && curWink >= 75) {
              int dx = col - curCenterX;
              int targetY = eyeCenterY + 2 + (dx * dx) / 26; // Smooth arc curve
              for (uint8_t bit = 0; bit < 8; bit++) {
                int y = pageTopY + bit;
                // 3-pixel thick soft cartoon line
                if (abs(y - targetY) <= 1) pixelByte |= (1 << bit);
              }
            }
            // HAPPY CRESCENTS: Upward curved squint
            else if (currentMood == MOOD_HAPPY) {
              int dx = col - curCenterX;
              for (uint8_t bit = 0; bit < 8; bit++) {
                int y = pageTopY + bit;
                int dy = y - (eyeCenterY + 3);
                int r2 = dx*dx + dy*dy;
                if (r2 <= 196 && y <= eyeCenterY + 4) {
                  int innerDy = dy - 7;
                  if (dx*dx + innerDy*innerDy > 64) pixelByte |= (1 << bit);
                }
              }
            }
            // NORMAL & SURPRISED & GLIDING WINK: Fluid easing capsule
            else {
              int dx = abs(col - curCenterX);
              int distFromEdgeX = eyeHalfW - dx;

              for (uint8_t bit = 0; bit < 8; bit++) {
                int y = pageTopY + bit;

                if (y >= curTopY && y <= curBotY) {
                  bool cornerCut = false;
                  // Rounded corners
                  if (curEyeBlink < 80) {
                    int distFromEdgeY = min(y - curTopY, curBotY - y);
                    if (distFromEdgeX < 4 && distFromEdgeY < 4) {
                      int cx = 4 - distFromEdgeX;
                      int cy = 4 - distFromEdgeY;
                      if (cx*cx + cy*cy > 16) cornerCut = true;
                    }
                  }

                  if (!cornerCut) {
                    bool isSparkle = false;
                    // Dual Sparkles (Disney gloss)
                    if (curEyeBlink < 25) {
                      // Big sparkle
                      if (col >= curCenterX + 3 && col <= curCenterX + 6 && y >= eyeCenterY - 11 && y <= eyeCenterY - 8) {
                        isSparkle = true;
                      }
                      // Little twinkle
                      if (col >= curCenterX - 7 && col <= curCenterX - 5 && y >= eyeCenterY + 5 && y <= eyeCenterY + 7) {
                        isSparkle = true;
                      }
                    }
                    if (!isSparkle) pixelByte |= (1 << bit);
                  }
                }
              }
            }
          }
        }

        // --- CUTE MOUTHS & BLUSH ---
        for (uint8_t bit = 0; bit < 8; bit++) {
          int y = pageTopY + bit;

          if (currentMood == MOOD_HAPPY || currentMood == MOOD_HEART) {
            // Smile Arc
            int dx = col - 64; int dy = y - 48;
            int r2 = dx*dx + dy*dy;
            if (r2 <= 49 && r2 >= 25 && y >= 49) pixelByte |= (1 << bit);
            // Rosy Blushing Cheeks
            if (y == 44 && ((col >= 22 && col <= 26) || (col >= 102 && col <= 106))) pixelByte |= (1 << bit);
          } else if (currentMood == MOOD_WINK || winkProgress > 20) {
            // Cheeky Smirk for the Wink!
            if (y == 51 && col >= 62 && col <= 72) pixelByte |= (1 << bit);
            if (y == 50 && col == 73) pixelByte |= (1 << bit);
            if (y == 44 && col >= 102 && col <= 106) pixelByte |= (1 << bit); // Right cheek blush!
          } else if (currentMood == MOOD_SURPRISED) {
            int dx = col - 64; int dy = y - 51;
            int r2 = dx*dx + dy*dy;
            if (r2 <= 20 && r2 >= 8) pixelByte |= (1 << bit);
          } else {
            // Calm neutral mouth
            if (y == 52 && col >= 60 && col <= 68) pixelByte |= (1 << bit);
            if (y == 51 && (col == 59 || col == 69)) pixelByte |= (1 << bit);
          }
        }
      }

      // ======================================================================
      // 2. MINECRAFT CREEPER
      // ======================================================================
      else if (currentFace == FACE_CREEPER) {
        if (col >= 40 && col < 88) {
          int faceCol = (col - 40) / 6;
          for (uint8_t bit = 0; bit < 8; bit++) {
            int y = pageTopY + bit;
            if (y >= 8 && y < 56) {
              int faceRow = (y - 8) / 6;
              uint8_t rowData = pgm_read_byte(&CREEPER_MAP[faceRow]);
              bool feature = (rowData & (1 << (7 - faceCol))) != 0;

              if (curBlink >= 80 && (faceRow == 1 || faceRow == 2) && (faceCol == 1 || faceCol == 2 || faceCol == 5 || faceCol == 6)) {
                if (faceRow == 2 && (y % 6 >= 3)) pixelByte |= (1 << bit);
              } else {
                if (feature && ((col + y) % 6 != 0)) pixelByte |= (1 << bit);
              }
            }
          }
        }
      }

      // ======================================================================
      // 3. GLOWING HEROBRINE
      // ======================================================================
      else if (currentFace == FACE_HEROBRINE) {
        if (col >= 40 && col < 88) {
          int faceCol = (col - 40) / 6;
          for (uint8_t bit = 0; bit < 8; bit++) {
            int y = pageTopY + bit;
            if (y >= 8 && y < 56) {
              int faceRow = (y - 8) / 6;
              uint8_t rowData = pgm_read_byte(&HEROBRINE_MAP[faceRow]);
              bool feature = (rowData & (1 << (7 - faceCol))) != 0;

              if (faceRow == 2 && (faceCol == 1 || faceCol == 2 || faceCol == 5 || faceCol == 6)) {
                if (curBlink < 80) pixelByte |= (1 << bit);
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
// 6. CONTINUOUS INTERPOLATED BLINKING & WINK PHYSICS
// ----------------------------------------------------------------------------

void updateEyePhysics() {
  unsigned long now = millis();

  // 1. Smooth Spring Easing for Glancing Left/Right
  eyeShiftX += (eyeTargetShiftX - eyeShiftX) * 0.25;

  // 2. Continuous Fluid Wink Physics (Right Eye)
  if (winkDirection == 1) {
    // Squeezing closed smoothly!
    winkProgress += 14.0;
    if (winkProgress >= 100.0) {
      winkProgress = 100.0;
      winkDirection = 2; // Hold the wink for a playful moment!
      winkHoldTimer = now;
    }
  } else if (winkDirection == 2) {
    // Hold wink for 450 milliseconds
    if (now - winkHoldTimer >= 450) {
      winkDirection = -1; // Smoothly open back up!
    }
  } else if (winkDirection == -1) {
    // Opening back up!
    winkProgress -= 16.0;
    if (winkProgress <= 0.0) {
      winkProgress = 0.0;
      winkDirection = 0; // Done winking!
      currentMood = MOOD_NORMAL;
    }
  }

  // 3. Normal Symmetrical Blinking Machine
  if (blinkDirection == 0 && (now - lastBlinkTime > nextBlinkInterval)) {
    // Don't blink if currently winking
    if (winkDirection == 0) {
      blinkDirection = 1;
      doubleBlinkQueued = (random(0, 10) < 3);
    }
  }

  if (blinkDirection == 1) {
    blinkProgress += 18.0;
    if (blinkProgress >= 100.0) {
      blinkProgress = 100.0;
      blinkDirection = -1;
    }
  } else if (blinkDirection == -1) {
    blinkProgress -= 16.0;
    if (blinkProgress <= 0.0) {
      blinkProgress = 0.0;
      blinkDirection = 0;
      lastBlinkTime = now;

      if (doubleBlinkQueued) {
        doubleBlinkQueued = false;
        nextBlinkInterval = 100;
      } else {
        nextBlinkInterval = random(2500, 5000);
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
      case 0:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.5;
        eyeTargetShiftX = 0;
        currentMood = MOOD_NORMAL;
        Serial.println(F("[ROBOT] Resting posture"));
        break;

      case 1:
        targetServoAngle = ARM_UP_ANGLE;
        armSpeed = 2.5;
        eyeTargetShiftX = 0;
        currentMood = MOOD_HAPPY;
        Serial.println(F("[ROBOT] Arms UP & Happy! 🙌"));
        break;

      case 2:
        targetServoAngle = 80;
        armSpeed = 3.0;
        eyeTargetShiftX = -6; // Glance Left
        currentMood = MOOD_NORMAL;
        Serial.println(F("[ROBOT] Glancing Left"));
        break;

      case 3:
        targetServoAngle = 100;
        armSpeed = 2.0;
        eyeTargetShiftX = 3;
        triggerSmoothWink(); // Smooth Animated Wink! 😉
        Serial.println(F("[ROBOT] Playful Animated Wink! 😉"));
        break;

      case 4:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.8;
        eyeTargetShiftX = 6; // Glance Right
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
    if (cmd == "K" || cmd == "WINK") {
      triggerSmoothWink();
      Serial.println(F("\n😉 [EXPRESSION]: Smooth Animated Wink!"));
    }
    else if (cmd == "H" || cmd == "HEART") {
      currentFace = FACE_COOL_ROBOT;
      currentMood = MOOD_HEART;
      Serial.println(F("\n💖 [EXPRESSION]: Glowing Heart Eyes! ❤️"));
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
  Serial.println(F("   BUTTERY-SMOOTH ANIMATED ROBOT READY! 🤖✨"));
  Serial.println(F("   Type 'K' or 'WINK' for Smooth Wink! 😉"));
  Serial.println(F("=============================================\n"));

  // 1. Initialize Servos
  leftServo.attach(LEFT_SERVO_PIN);
  rightServo.attach(RIGHT_SERVO_PIN);

  leftServo.write(ARM_DOWN_ANGLE);
  rightServo.write(MIRROR_RIGHT_ARM ? (180 - ARM_DOWN_ANGLE) : ARM_DOWN_ANGLE);
  delay(300);

  // 2. Initialize OLED Display at 400kHz Fast Speed
  initSH1106();
  renderFastScreen();

  lastBlinkTime = millis();
  lastBehaviorTime = millis();
}

void loop() {
  // 1. Heartbeat LED
  digitalWrite(LED_PIN, (millis() / 500) % 2);

  // 2. Process Commands
  checkSerialCommands();

  // 3. Fluid Eye Interpolation & Wink Physics
  updateEyePhysics();

  // 4. Update Arm Positions
  updateArms();
  updateRobotBehavior();

  // 5. Render Screen at 35+ FPS
  if (millis() - lastOledDraw >= 28) {
    lastOledDraw = millis();
    renderFastScreen();
  }
}
