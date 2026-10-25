/*
 * ============================================================================
 *   MINECRAFT ROBOT COMPANION: CREEPER & HEROBRINE FACES! 🤖🟩👻
 * ============================================================================
 *   Interactive Serial Commands (115200 baud):
 *     --- FACE MODES ---
 *     - '1' or 'CREEPER'   = Perfect 1:1 Pixel-Accurate Creeper Face! 🟩
 *     - '2' or 'HEROBRINE' = Spooky Glowing Herobrine Face! 👻
 *     - '3' or 'ROBOT'     = Cute Animated Robot Eyes! 🤖
 *
 *     --- MOTION CONTROLS ---
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
// 1. FACE SELECTION & PIXEL MAPS (Mathematically 1:1 Perfect Squares!)
// ----------------------------------------------------------------------------
enum FaceMode {
  FACE_CREEPER,
  FACE_HEROBRINE,
  FACE_CUTE_ROBOT
};

FaceMode currentFace = FACE_CREEPER;

// 1:1 Pixel-Accurate Canonical 8x8 Minecraft Creeper Face
const uint8_t CREEPER_MAP[8] PROGMEM = {
  0b11111111, // Row 0: Solid forehead
  0b10011001, // Row 1: Black square eyes
  0b10011001, // Row 2: Black square eyes
  0b11100111, // Row 3: Center nose bridge
  0b11000011, // Row 4: Wide mouth center
  0b11000011, // Row 5: Wide mouth center
  0b11011011, // Row 6: Frown legs / corners
  0b11011011  // Row 7: Frown legs / corners
};

// Canonical 8x8 Minecraft Herobrine Face (Steve with Glowing White Eyes!)
const uint8_t HEROBRINE_MAP[8] PROGMEM = {
  0b11111111, // Row 0: Brown hair
  0b11000011, // Row 1: Hair fringe
  0b01100110, // Row 2: GLOWING PURE WHITE EYES!
  0b00011000, // Row 3: Nose
  0b00111100, // Row 4: Mouth
  0b01111110, // Row 5: Beard
  0b00111100, // Row 6: Chin
  0b00000000  // Row 7: Neck
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
unsigned long nextBlinkInterval = 3000;
unsigned long lastOledDraw = 0;

bool isBlinking = false;
int blinkProgress = 0;
int currentRoutine = 0;

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

// Draws any 8x8 face perfectly scaled to 48x48 pixels (1:1 aspect ratio!)
void renderFaceToScreen() {
  for (uint8_t p = 0; p < 8; p++) {
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 + p);
    Wire.write(0x02); // SH1106 Column offset 2
    Wire.write(0x10);
    Wire.endTransmission();

    for (uint8_t col = 0; col < 128; col++) {
      uint8_t pixelByte = 0x00;

      // 48x48 Face is centered between columns 40 and 87 (40 + 48 = 88)
      if (col >= 40 && col < 88) {
        int faceCol = (col - 40) / 6; // 0 to 7

        // Construct 8 vertical pixels for this page
        for (uint8_t bit = 0; bit < 8; bit++) {
          int y = p * 8 + bit;

          // Face is centered vertically between Y=8 and Y=55 (8 + 48 = 56)
          if (y >= 8 && y < 56) {
            int faceRow = (y - 8) / 6; // 0 to 7
            bool pixelOn = false;

            if (currentFace == FACE_CREEPER) {
              uint8_t rowData = pgm_read_byte(&CREEPER_MAP[faceRow]);
              // Bit 7 is leftmost, Bit 0 is rightmost
              bool feature = (rowData & (1 << (7 - faceCol))) != 0;

              if (isBlinking && (faceRow == 1 || faceRow == 2) && (faceCol == 1 || faceCol == 2 || faceCol == 5 || faceCol == 6)) {
                // If blinking, eyes become narrow slit on row 2
                pixelOn = (faceRow == 2 && (y % 6 >= 3));
              } else {
                pixelOn = feature;
              }

              // Subtle pixel checkerboard texture
              if (pixelOn && ((col + y) % 6 == 0)) {
                pixelOn = false;
              }
            }
            else if (currentFace == FACE_HEROBRINE) {
              uint8_t rowData = pgm_read_byte(&HEROBRINE_MAP[faceRow]);
              bool feature = (rowData & (1 << (7 - faceCol))) != 0;

              // Eyes on Row 2: GLOWING BRIGHT WHITE!
              if (faceRow == 2 && (faceCol == 1 || faceCol == 2 || faceCol == 5 || faceCol == 6)) {
                pixelOn = !isBlinking; // Eyes glow unless blinking!
              } else {
                // Hair and face outline
                pixelOn = feature;
              }
            }
            else { // FACE_CUTE_ROBOT
              // Big round expressive eyes
              int relX = col - 40;
              int relY = y - 8;
              // Left Eye (center 14, 24) | Right Eye (center 34, 24)
              int dL = (relX - 14)*(relX - 14) + (relY - 24)*(relY - 24);
              int dR = (relX - 34)*(relX - 34) + (relY - 24)*(relY - 24);
              if (dL < 100 || dR < 100) {
                if (isBlinking && (relY < 20 || relY > 26)) {
                  pixelOn = false;
                } else {
                  pixelOn = true;
                }
              }
              // Cute smile
              if (relY >= 38 && relY <= 42 && relX >= 18 && relX <= 30) {
                pixelOn = true;
              }
            }

            if (pixelOn) {
              pixelByte |= (1 << bit);
            }
          }
        }
      }
      // Side accent bars / glow brackets
      else if (col == 4 || col == 123) {
        if (p >= 1 && p <= 6) pixelByte = 0xFF;
      } else if ((col == 5 || col == 122) && (p == 1 || p == 6)) {
        pixelByte = 0xFF;
      }

      // Send 16-byte burst to I2C
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
// 4. SYNCHRONIZED ARMS
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

void updateBlinking() {
  unsigned long now = millis();

  if (!isBlinking && (now - lastBlinkTime > nextBlinkInterval)) {
    isBlinking = true;
    blinkProgress = 0;
  }

  if (isBlinking) {
    blinkProgress += 30;
    if (blinkProgress >= 100) {
      isBlinking = false;
      lastBlinkTime = now;
      nextBlinkInterval = random(2500, 5000);
    }
  }
}

void updateRobotBehavior() {
  if (!autoDanceMode || calibrationMode || isStopped) return;

  unsigned long now = millis();

  if (now - lastActionTime > 3500) {
    lastActionTime = now;
    currentRoutine = (currentRoutine + 1) % 4;

    switch (currentRoutine) {
      case 0:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.5;
        Serial.println(F("[ROBOT] Arms Resting Down"));
        break;

      case 1:
        targetServoAngle = ARM_UP_ANGLE;
        armSpeed = 2.5;
        Serial.println(F("[ROBOT] Arms UP In The Air! 🙌"));
        break;

      case 2:
        targetServoAngle = 80;
        armSpeed = 3.0;
        Serial.println(F("[ROBOT] Arms at 80 deg"));
        break;

      case 3:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.8;
        Serial.println(F("[ROBOT] Arms Lowering"));
        break;
    }
  }
}

// ----------------------------------------------------------------------------
// 5. SERIAL COMMAND PROCESSOR
// ----------------------------------------------------------------------------

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    // --- FACE SWITCHING COMMANDS ---
    if (cmd == "1" || cmd == "CREEPER") {
      currentFace = FACE_CREEPER;
      Serial.println(F("\n🟩 [FACE CHANGED]: Perfect 1:1 Pixel Minecraft Creeper!"));
    }
    else if (cmd == "2" || cmd == "HEROBRINE") {
      currentFace = FACE_HEROBRINE;
      Serial.println(F("\n👻 [FACE CHANGED]: Spooky Glowing Herobrine Face!"));
    }
    else if (cmd == "3" || cmd == "ROBOT") {
      currentFace = FACE_CUTE_ROBOT;
      Serial.println(F("\n🤖 [FACE CHANGED]: Cute Animated Robot Eyes!"));
    }

    // --- MOTION COMMANDS ---
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
      Serial.println(F("[COMMAND] Arms UP! 🙌"));
    }
    else if (cmd == "D") {
      isStopped = false;
      calibrationMode = false;
      autoDanceMode = false;
      targetServoAngle = ARM_DOWN_ANGLE;
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
// 6. SETUP & MAIN LOOP
// ----------------------------------------------------------------------------

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.begin(115200);
  Serial.println(F("\n============================================="));
  Serial.println(F("   MINECRAFT ROBOT COMPANION READY! 🤖"));
  Serial.println(F("   SWITCH FACES:"));
  Serial.println(F("     '1' = Perfect Pixel Creeper 🟩"));
  Serial.println(F("     '2' = Glowing Herobrine Face 👻"));
  Serial.println(F("     '3' = Cute Robot Eyes 🤖"));
  Serial.println(F("   ARM MOTIONS:"));
  Serial.println(F("     'S' = Stop/Freeze | 'W' = Resume Dance"));
  Serial.println(F("     'U' = Arms Up     | 'D' = Arms Down"));
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
  lastActionTime = millis();
}

void loop() {
  // 1. Heartbeat LED
  digitalWrite(LED_PIN, (millis() / 500) % 2);

  // 2. Process Commands
  checkSerialCommands();

  // 3. Move Arms
  updateArms();
  updateBlinking();
  updateRobotBehavior();

  // 4. Render Screen at smooth 15 FPS
  if (millis() - lastOledDraw >= 65) {
    lastOledDraw = millis();
    renderFaceToScreen();
  }
}
