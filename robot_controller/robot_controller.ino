/*
 * ============================================================================
 *   MINECRAFT CREEPER ROBOT (DIRECT SH1106 1.3" DRIVER - ZERO EXTRA LIBS!)
 *   Dual-Arm Synchronized Robot + Crystal-Clear Creeper Face (No Dots!)
 * ============================================================================
 *   Board: Arduino Uno R3 (Direct Pin Connections - No Breadboard Needed!)
 *   Display: 1.3" I2C OLED (128x64 SH1106)
 *   Actuators: 
 *     - Left Arm Servo:  Pin 9
 *     - Right Arm Servo: Pin 10
 * ============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Servo.h>

// ----------------------------------------------------------------------------
// 1. PIN & HARDWARE SETUP
// ----------------------------------------------------------------------------
#define OLED_I2C_ADDR   0x3C
#define LEFT_SERVO_PIN  9
#define RIGHT_SERVO_PIN 10
#define LED_PIN         13

// 128x64 drawing canvas powered by standard Adafruit_GFX
GFXcanvas1 canvas(128, 64);

Servo leftServo;
Servo rightServo;

// ----------------------------------------------------------------------------
// 2. DUAL ARM SWING SETTINGS
// ----------------------------------------------------------------------------
const int ARM_DOWN_ANGLE = 25;   // Rest down position
const int ARM_UP_ANGLE   = 130;  // Raised up in the air!

float currentServoAngle  = ARM_DOWN_ANGLE;
float targetServoAngle   = ARM_DOWN_ANGLE;
float armSpeed           = 1.5;

const bool MIRROR_RIGHT_ARM = true;
bool autoDanceMode = true;

// ----------------------------------------------------------------------------
// 3. EMOTION & BLINK SETTINGS
// ----------------------------------------------------------------------------
enum RobotEmotion {
  EMO_NORMAL,
  EMO_HAPPY,
  EMO_SURPRISED
};

RobotEmotion currentEmotion = EMO_NORMAL;

int gazeOffsetX = 0;
int gazeOffsetY = 0;

bool isBlinking = false;
int blinkProgress = 0;
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkInterval = 3000;

unsigned long lastArmUpdateTime = 0;
unsigned long lastScreenUpdateTime = 0;
unsigned long lastActionTime = 0;
int currentRoutine = 0;

// ----------------------------------------------------------------------------
// 4. BULLETPROOF 1.3" SH1106 I2C CONTROLLER (Direct Hardware Driver)
// ----------------------------------------------------------------------------

void sendSH1106Command(uint8_t cmd) {
  Wire.beginTransmission(OLED_I2C_ADDR);
  Wire.write(0x80); // Command flag
  Wire.write(cmd);
  Wire.endTransmission();
}

void initSH1106() {
  Wire.begin();
  delay(100);

  // Official initialization sequence for 1.3" SH1106 OLED
  static const uint8_t initCmds[] PROGMEM = {
    0xAE,         // Display OFF
    0xD5, 0x80,   // Clock divide ratio
    0xA8, 0x3F,   // Multiplex ratio: 64
    0xD3, 0x00,   // Display offset: 0
    0x40,         // Start line: 0
    0xAD, 0x8B,   // DC-DC charge pump ON
    0xA1,         // Segment re-map (left to right)
    0xC8,         // COM output scan direction (top to bottom)
    0xDA, 0x12,   // COM pins config
    0x81, 0xBF,   // Contrast: bright!
    0xD9, 0x22,   // Pre-charge period
    0xDB, 0x40,   // VCOM deselect
    0xA4,         // Resume to RAM content
    0xA6,         // Normal (non-inverted)
    0xAF          // Display ON!
  };

  for (uint8_t i = 0; i < sizeof(initCmds); i++) {
    sendSH1106Command(pgm_read_byte(&initCmds[i]));
  }

  // Clear all random static noise/dots to pure black
  clearSH1106Hardware();
}

// Clears all 8 internal RAM pages of SH1106 to eliminate static dots
void clearSH1106Hardware() {
  for (uint8_t page = 0; page < 8; page++) {
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 + page); // Page address
    Wire.write(0x02);        // Column offset 2 (SH1106 specific)
    Wire.write(0x10);
    Wire.endTransmission();

    for (uint8_t chunk = 0; chunk < 8; chunk++) {
      Wire.beginTransmission(OLED_I2C_ADDR);
      Wire.write(0x40); // Data mode
      for (uint8_t b = 0; b < 16; b++) {
        Wire.write(0x00); // Write pure black
      }
      Wire.endTransmission();
    }
  }
}

// Sends the 128x64 canvas buffer to the 1.3" screen page-by-page
void pushCanvasToSH1106() {
  uint8_t* buffer = canvas.getBuffer();

  for (uint8_t page = 0; page < 8; page++) {
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 + page);
    Wire.write(0x02); // Column address offset 2
    Wire.write(0x10);
    Wire.endTransmission();

    for (uint8_t chunk = 0; chunk < 8; chunk++) {
      Wire.beginTransmission(OLED_I2C_ADDR);
      Wire.write(0x40); // Data stream
      for (uint8_t col = 0; col < 16; col++) {
        uint8_t x = chunk * 16 + col;
        uint8_t byteVal = 0;
        // Construct vertical 8-pixel byte for this page
        for (uint8_t bit = 0; bit < 8; bit++) {
          uint8_t y = page * 8 + bit;
          // Canvas buffer is 1-bit per pixel stored horizontally
          uint16_t byteIndex = (x + y * 128) / 8;
          uint8_t bitMask = 0x80 >> (x & 7);
          if (buffer[byteIndex] & bitMask) {
            byteVal |= (1 << bit);
          }
        }
        Wire.write(byteVal);
      }
      Wire.endTransmission();
    }
  }
}

// ----------------------------------------------------------------------------
// 5. DRAWING THE MINECRAFT CREEPER FACE
// ----------------------------------------------------------------------------
void drawCreeperFace(int centerX, int centerY, int blockSize, int eyeShiftX, int eyeShiftY) {
  int totalSize = blockSize * 8;
  int startX = centerX - totalSize / 2;
  int startY = centerY - totalSize / 2;

  // 1. Bright glowing Creeper head
  canvas.fillRect(startX, startY, totalSize, totalSize, 1);

  // 2. Pixelated skin texture
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      if ((x + y) % 3 == 0) {
        canvas.drawRect(startX + x * blockSize, startY + y * blockSize, blockSize, blockSize, 0);
      }
    }
  }

  // 3. Eyes (Blink vs Open)
  if (isBlinking) {
    canvas.fillRect(startX + 1 * blockSize, startY + 1 * blockSize + blockSize / 2, 2 * blockSize, 3, 0);
    canvas.fillRect(startX + 5 * blockSize, startY + 1 * blockSize + blockSize / 2, 2 * blockSize, 3, 0);
  } else {
    // Left Eye (2x2 blocks)
    canvas.fillRect(startX + (1 * blockSize) + eyeShiftX, startY + (1 * blockSize) + eyeShiftY, 2 * blockSize, 2 * blockSize, 0);
    // Right Eye (2x2 blocks)
    canvas.fillRect(startX + (5 * blockSize) + eyeShiftX, startY + (1 * blockSize) + eyeShiftY, 2 * blockSize, 2 * blockSize, 0);
  }

  // 4. Nose Bridge (2x1 blocks)
  canvas.fillRect(startX + 3 * blockSize, startY + 3 * blockSize, 2 * blockSize, 1 * blockSize, 0);

  // 5. Upper Mouth (4x2 blocks)
  canvas.fillRect(startX + 2 * blockSize, startY + 4 * blockSize, 4 * blockSize, 2 * blockSize, 0);

  // 6. Left Mouth Drop (1x2 blocks)
  canvas.fillRect(startX + 2 * blockSize, startY + 6 * blockSize, 1 * blockSize, 2 * blockSize, 0);

  // 7. Right Mouth Drop (1x2 blocks)
  canvas.fillRect(startX + 5 * blockSize, startY + 6 * blockSize, 1 * blockSize, 2 * blockSize, 0);
}

void renderScreen() {
  canvas.fillScreen(0); // Clear canvas

  // Draw authentic 56x56 Creeper Face in the center!
  drawCreeperFace(64, 32, 7, gazeOffsetX, gazeOffsetY);

  // Side Text
  canvas.setTextColor(1);
  canvas.setTextSize(1);

  if (currentEmotion == EMO_HAPPY) {
    canvas.setCursor(4, 22);
    canvas.println(F("HI!"));
    canvas.setCursor(4, 34);
    canvas.println(F("FRIEND"));

    canvas.setCursor(96, 22);
    canvas.println(F("ARMS"));
    canvas.setCursor(96, 34);
    canvas.println(F("UP! ^_^"));
  } else if (currentEmotion == EMO_SURPRISED) {
    canvas.setCursor(4, 28);
    canvas.println(F("WHAT?"));

    canvas.setCursor(96, 28);
    canvas.println(F("BOOM!"));
  } else {
    canvas.setCursor(4, 28);
    canvas.println(F("CREEPER"));

    canvas.setCursor(98, 28);
    canvas.println(F("BOT 🤖"));
  }

  pushCanvasToSH1106();
}

// ----------------------------------------------------------------------------
// 6. UPDATE & CONTROL FUNCTIONS
// ----------------------------------------------------------------------------

void updateArms() {
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

      int leftAngle = (int)currentServoAngle;
      int rightAngle = MIRROR_RIGHT_ARM ? (180 - leftAngle) : leftAngle;

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
  if (!autoDanceMode) return;

  unsigned long now = millis();

  if (now - lastActionTime > 4000) {
    lastActionTime = now;
    currentRoutine = (currentRoutine + 1) % 4;

    switch (currentRoutine) {
      // 0: Arms Resting Down
      case 0:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.2;
        currentEmotion = EMO_NORMAL;
        gazeOffsetX = 0;
        gazeOffsetY = 0;
        break;

      // 1: Both arms swing UP together! 🙌
      case 1:
        targetServoAngle = ARM_UP_ANGLE;
        armSpeed = 2.0;
        currentEmotion = EMO_HAPPY;
        gazeOffsetX = 0;
        gazeOffsetY = -1;
        break;

      // 2: Arms hold halfway, Creeper looks to the LEFT! 😲
      case 2:
        targetServoAngle = 75;
        armSpeed = 2.8;
        currentEmotion = EMO_SURPRISED;
        gazeOffsetX = -2;
        gazeOffsetY = 0;
        break;

      // 3: Creeper looks to the RIGHT & lowers arms gently
      case 3:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.5;
        currentEmotion = EMO_NORMAL;
        gazeOffsetX = 2;
        gazeOffsetY = 1;
        break;
    }
  }
}

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "U") {
      autoDanceMode = false;
      targetServoAngle = ARM_UP_ANGLE;
      currentEmotion = EMO_HAPPY;
      Serial.println(F("[COMMAND] Arms UP! 🙌"));
    } else if (cmd == "D") {
      autoDanceMode = false;
      targetServoAngle = ARM_DOWN_ANGLE;
      currentEmotion = EMO_NORMAL;
      Serial.println(F("[COMMAND] Arms DOWN! 👇"));
    } else if (cmd == "W") {
      autoDanceMode = true;
      Serial.println(F("[COMMAND] Auto Dance / Wave Mode ON! 👋"));
    } else if (cmd.toInt() > 0 || cmd == "0") {
      autoDanceMode = false;
      int angle = constrain(cmd.toInt(), 0, 180);
      targetServoAngle = angle;
      Serial.print(F("[COMMAND] Set custom angle to: "));
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
  Serial.println(F("\n========================================"));
  Serial.println(F("   MINECRAFT CREEPER ROBOT READY! 🤖"));
  Serial.println(F("   Commands in Serial Monitor:"));
  Serial.println(F("   'U' = Arms Up | 'D' = Arms Down"));
  Serial.println(F("   'W' = Auto Dance Mode | or type angle (0-180)"));
  Serial.println(F("========================================\n"));

  // 1. Start Servos Immediately!
  leftServo.attach(LEFT_SERVO_PIN);
  rightServo.attach(RIGHT_SERVO_PIN);

  // Move them gently to resting angle
  leftServo.write(ARM_DOWN_ANGLE);
  rightServo.write(MIRROR_RIGHT_ARM ? (180 - ARM_DOWN_ANGLE) : ARM_DOWN_ANGLE);
  delay(300);

  // 2. Initialize 1.3" SH1106 Display & Wipe Dots Clean!
  initSH1106();

  // 3. Show Startup Welcome
  canvas.fillScreen(0);
  canvas.drawRoundRect(2, 2, 124, 60, 4, 1);
  drawCreeperFace(40, 32, 6, 0, 0);

  canvas.setTextColor(1);
  canvas.setTextSize(2);
  canvas.setCursor(72, 16);
  canvas.println(F("HI!"));

  canvas.setTextSize(1);
  canvas.setCursor(72, 38);
  canvas.println(F("WELCOME"));

  pushCanvasToSH1106();
  delay(2000);

  lastBlinkTime = millis();
  lastActionTime = millis();
}

void loop() {
  // 1. Heartbeat LED: Blinks every 1 second
  digitalWrite(LED_PIN, (millis() / 500) % 2);

  // 2. Listen to Serial Monitor commands (U, D, W, angles)
  checkSerialCommands();

  // 3. Update Robot Physical Motion
  updateArms();
  updateBlinking();
  updateRobotBehavior();

  // 4. Render to 1.3" OLED Screen
  if (millis() - lastScreenUpdateTime >= 40) { // ~25 FPS crisp refresh
    lastScreenUpdateTime = millis();
    renderScreen();
  }
}
