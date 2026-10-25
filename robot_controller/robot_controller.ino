/*
 * ============================================================================
 *   MINECRAFT CREEPER ROBOT (WITH LIVE DUAL-ARM CALIBRATION!)
 * ============================================================================
 *   Interactive Commands in Serial Monitor (115200 baud):
 *     - 'C'   = CALIBRATION MODE (Locks both arms at 90° to align horns!)
 *     - 'U'   = Both Arms UP! 🙌
 *     - 'D'   = Both Arms DOWN! 👇
 *     - 'W'   = Auto Dance / Wave Mode! 👋
 *     - 'L+' / 'L-' = Fine-tune Left arm by 1 degree
 *     - 'R+' / 'R-' = Fine-tune Right arm by 1 degree
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
// 1. CALIBRATION OFFSETS & ANGLES
// ----------------------------------------------------------------------------
// Fine-tune offsets (adjust if one arm is slightly higher than the other!)
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

// Timing clocks
unsigned long lastArmUpdateTime = 0;
unsigned long lastActionTime = 0;
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkInterval = 3000;

bool isBlinking = false;
int blinkProgress = 0;
int currentRoutine = 0;

int eyeShiftX = 0;
int eyeShiftY = 0;

// ----------------------------------------------------------------------------
// 2. HARDWARE SH1106 OLED CONTROLLER
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

void clearOLED() {
  for (uint8_t p = 0; p < 8; p++) {
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 + p);
    Wire.write(0x02);
    Wire.write(0x10);
    Wire.endTransmission();

    for (uint8_t chunk = 0; chunk < 8; chunk++) {
      Wire.beginTransmission(OLED_I2C_ADDR);
      Wire.write(0x40);
      for (uint8_t i = 0; i < 16; i++) {
        Wire.write(0x00);
      }
      Wire.endTransmission();
    }
  }
}

void drawCreeperFace(int shiftX, int shiftY, bool blinking) {
  for (uint8_t p = 0; p < 8; p++) {
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 + p);
    Wire.write(0x02);
    Wire.write(0x10);
    Wire.endTransmission();

    for (uint8_t col = 0; col < 128; col++) {
      uint8_t pixelByte = 0x00;

      // Creeper Head block in center
      if (col >= 36 && col < 92 && p >= 1 && p <= 6) {
        int headCol = col - 36;
        pixelByte = 0xFF;

        if ((col + p) % 5 == 0) {
          pixelByte = 0xEE;
        }

        // Eyes
        if (p == 2 || p == 3) {
          if (blinking) {
            if (p == 3) pixelByte = 0x00;
          } else {
            if (headCol >= (8 + shiftX) && headCol < (22 + shiftX)) pixelByte = 0x00;
            if (headCol >= (34 + shiftX) && headCol < (48 + shiftX)) pixelByte = 0x00;
          }
        }

        // Nose
        if (p == 3 && headCol >= 22 && headCol < 34) pixelByte = 0x00;

        // Mouth Top
        if ((p == 4 || p == 5) && headCol >= 15 && headCol < 41) pixelByte = 0x00;

        // Mouth Fangs
        if (p == 6 && ((headCol >= 15 && headCol < 22) || (headCol >= 34 && headCol < 41))) pixelByte = 0x00;
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
// 3. SYNCHRONIZED ARMS WITH OFFSETS
// ----------------------------------------------------------------------------

void updateArms() {
  if (calibrationMode) {
    // In Calibration mode, hold solid 90 degrees + offsets!
    int leftCalib  = constrain(90 + leftOffset, 0, 180);
    int rightCalib = constrain((MIRROR_RIGHT_ARM ? (180 - 90) : 90) + rightOffset, 0, 180);
    leftServo.write(leftCalib);
    rightServo.write(rightCalib);
    return;
  }

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
    blinkProgress += 25;
    if (blinkProgress >= 100) {
      isBlinking = false;
      lastBlinkTime = now;
      nextBlinkInterval = random(2500, 5000);
    }
  }
}

void updateRobotBehavior() {
  if (!autoDanceMode || calibrationMode) return;

  unsigned long now = millis();

  if (now - lastActionTime > 3500) {
    lastActionTime = now;
    currentRoutine = (currentRoutine + 1) % 4;

    switch (currentRoutine) {
      case 0:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.5;
        eyeShiftX = 0;
        Serial.println(F("[ROBOT] Arms Resting Down"));
        break;

      case 1:
        targetServoAngle = ARM_UP_ANGLE;
        armSpeed = 2.5;
        eyeShiftX = 0;
        Serial.println(F("[ROBOT] Arms UP In The Air! 🙌"));
        break;

      case 2:
        targetServoAngle = 80;
        armSpeed = 3.0;
        eyeShiftX = -3;
        Serial.println(F("[ROBOT] Looking Left"));
        break;

      case 3:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.8;
        eyeShiftX = 3;
        Serial.println(F("[ROBOT] Looking Right"));
        break;
    }
  }
}

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    // 1. Calibration Mode (Lock at 90°)
    if (cmd == "C") {
      calibrationMode = true;
      autoDanceMode = false;
      Serial.println(F("\n>>> [CALIBRATION MODE ACTIVATED] <<<"));
      Serial.println(F("Both servos are now holding at exactly 90 degrees (Horizontal)."));
      Serial.println(F("Attach your wooden arms/horns so they are perfectly aligned!"));
      Serial.println(F("Type 'L+' or 'L-' to trim Left arm by 1 deg."));
      Serial.println(F("Type 'R+' or 'R-' to trim Right arm by 1 deg."));
      Serial.println(F("Type 'W' to exit calibration and dance!\n"));
    }
    // 2. Left Arm Fine Trim
    else if (cmd == "L+") {
      leftOffset++;
      Serial.print(F("Left Offset: ")); Serial.println(leftOffset);
    } else if (cmd == "L-") {
      leftOffset--;
      Serial.print(F("Left Offset: ")); Serial.println(leftOffset);
    }
    // 3. Right Arm Fine Trim
    else if (cmd == "R+") {
      rightOffset++;
      Serial.print(F("Right Offset: ")); Serial.println(rightOffset);
    } else if (cmd == "R-") {
      rightOffset--;
      Serial.print(F("Right Offset: ")); Serial.println(rightOffset);
    }
    // 4. Arms UP
    else if (cmd == "U") {
      calibrationMode = false;
      autoDanceMode = false;
      targetServoAngle = ARM_UP_ANGLE;
      Serial.println(F("[COMMAND] Arms UP! 🙌"));
    }
    // 5. Arms DOWN
    else if (cmd == "D") {
      calibrationMode = false;
      autoDanceMode = false;
      targetServoAngle = ARM_DOWN_ANGLE;
      Serial.println(F("[COMMAND] Arms DOWN! 👇"));
    }
    // 6. Auto Wave
    else if (cmd == "W") {
      calibrationMode = false;
      autoDanceMode = true;
      Serial.println(F("[COMMAND] Auto Dance / Wave Mode ON! 👋"));
    }
    // 7. Custom Degree
    else if (cmd.toInt() > 0 || cmd == "0") {
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
// 4. SETUP & MAIN LOOP
// ----------------------------------------------------------------------------

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.begin(115200);
  Serial.println(F("\n============================================="));
  Serial.println(F("   MINECRAFT CREEPER ROBOT (CALIBRATION READY)"));
  Serial.println(F("   Type 'C' in Serial Monitor to Calibrate!"));
  Serial.println(F("   Type 'U' = Up, 'D' = Down, 'W' = Wave"));
  Serial.println(F("=============================================\n"));

  // 1. Initialize Servos
  leftServo.attach(LEFT_SERVO_PIN);
  rightServo.attach(RIGHT_SERVO_PIN);

  leftServo.write(ARM_DOWN_ANGLE);
  rightServo.write(MIRROR_RIGHT_ARM ? (180 - ARM_DOWN_ANGLE) : ARM_DOWN_ANGLE);
  delay(300);

  // 2. Initialize OLED Display
  initSH1106();
  clearOLED();
  drawCreeperFace(0, 0, false);

  lastBlinkTime = millis();
  lastActionTime = millis();
}

void loop() {
  // 1. Heartbeat LED: Blinks every 500ms
  digitalWrite(LED_PIN, (millis() / 500) % 2);

  // 2. Check Keyboard / Serial commands
  checkSerialCommands();

  // 3. Move Arms
  updateArms();
  updateBlinking();
  updateRobotBehavior();

  // 4. Update Creeper Face on OLED
  static unsigned long lastOledDraw = 0;
  if (millis() - lastOledDraw >= 100) {
    lastOledDraw = millis();
    drawCreeperFace(eyeShiftX, eyeShiftY, isBlinking);
  }
}
