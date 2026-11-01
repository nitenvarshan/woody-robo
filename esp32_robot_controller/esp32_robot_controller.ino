/*
 * ============================================================================
 *   WOODY THE ROBOT COMPANION — ESP32 DEVKIT V1 EDITION 🤖📱✨
 *   Dual-Servo Synchronization + Animated OLED Face + Bluetooth App Control
 * ============================================================================
 *   Hardware:
 *     - MCU: ESP32 DevKit V1 (30-Pin)
 *     - OLED: 0.96" I2C SSD1306 / SH1106 (GPIO 21 SDA, GPIO 22 SCL) @ 3.3V
 *     - Servos: 2x SG90 Micro-Servos (Left: GPIO 18, Right: GPIO 19) @ 5V Buck Rail
 *     - Sensor: Shoulder IR Obstacle Sensor (GPIO 14) @ 3.3V
 *     - Bluetooth: Built-in Bluetooth Serial ("Woody-Robot")
 *
 *   Bluetooth & Serial Commands (Works via Serial Monitor OR Phone App):
 *     - 'C' or 'CAL'     -> CALIBRATION MODE (Locks both at 90.0° for horn alignment) 🎯
 *     - 'M' or 'MIRROR'  -> Toggle Mirror Mode (for back-to-back vs matching servos) 🔄
 *     - 'TEST'           -> Diagnostic Sweep: Tests Left arm then Right arm 🔬
 *     - 'U' or 'UP'      -> Both arms swing UP in sync 🙌
 *     - 'D' or 'DOWN'    -> Both arms swing DOWN in sync 👇
 *     - 'W' or 'WAVE'    -> Both arms perform cute synchronized wave 👋
 *     - 'H' or 'HEART'   -> HUGE Glowing Heart Eyes ❤️❤️ + Arms UP!
 *     - 'G' or 'CHAD'    -> Giga Chad Wink & Smirk 😎💪 + Victory Arms
 *     - 'K' or 'WINK'    -> Playful animated one-eye wink 😉
 *     - '1' or 'CREEPER' -> 1:1 Pixel Minecraft Creeper 🟩
 *     - '2' or 'HERO'    -> Glowing Herobrine Face 👻
 *     - '3' or 'STORY'   -> Resume Default Autonomous Story Animation Loop
 *     - 'L <angle>'      -> Move Left arm only (e.g. 'L 90', 'L 45', 'L 135')
 *     - 'R <angle>'      -> Move Right arm only (e.g. 'R 90', 'R 45', 'R 135')
 *     - '0' to '180'     -> Set both arms to exact angle
 *     - 'S' or 'STOP'    -> Emergency stop / Freeze arms 🛑
 *     - '?' or 'HELP'    -> Print command menu
 * ============================================================================
 */

#include <Wire.h>
#include <ESP32Servo.h>
#include "BluetoothSerial.h"

// ----------------------------------------------------------------------------
// 1. PIN DEFINITIONS & CONSTANTS (ESP32 DevKit V1)
// ----------------------------------------------------------------------------
#define OLED_I2C_ADDR   0x3C
#define OLED_SDA_PIN    21
#define OLED_SCL_PIN    22

#define LEFT_SERVO_PIN  18
#define RIGHT_SERVO_PIN 19
#define IR_SENSOR_PIN   14    // Must be powered at 3.3V!

// Check Bluetooth availability in ESP32 core
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled in this ESP32 core!
#endif

BluetoothSerial SerialBT;

Servo leftServo;
Servo rightServo;

// ----------------------------------------------------------------------------
// 2. ROBOT MOODS & FACES
// ----------------------------------------------------------------------------
enum FaceMode {
  FACE_ORGANIC_ROBOT, // Default: Moving eyes, winks & glowing hearts!
  FACE_TEXT_INTRO,    // "HELLO! I AM WOODY - BATCH 4 TEAM"
  FACE_CREEPER,       // Minecraft Creeper
  FACE_HEROBRINE      // Glowing Herobrine
};

enum RobotMood {
  MOOD_NORMAL,        // Natural glancing eyeballs
  MOOD_HAPPY,         // Happy crescent eyes + smile + blush
  MOOD_WINK,          // Smooth animated wink 😉
  MOOD_HEART,         // HUGE glowing heart eyes in BOTH sockets ❤️❤️
  MOOD_SURPRISED,     // Wide alert dilated eyes 😲
  MOOD_GIGACHAD       // 😎 Ultra-confident Giga Chad Wink & Smirk!
};

FaceMode  currentFace = FACE_ORGANIC_ROBOT;
RobotMood currentMood = MOOD_NORMAL;

// ----------------------------------------------------------------------------
// 3. SYNCHRONIZED SERVO MOTION ENGINE (FOOLPROOF CALIBRATION & MIRRORING)
// ----------------------------------------------------------------------------
int leftOffset  = 0;
int rightOffset = 0;

// Arm Angle Limits (Degrees: 0 to 180)
const int ARM_DOWN_ANGLE = 25;    // Rest position (both arms down at sides)
const int ARM_MID_ANGLE  = 75;    // Mid-level gesture
const int ARM_UP_ANGLE   = 135;   // Fully raised (victory celebration / high-five)

// Both arms share the exact same synchronized master angle coordinate!
float currentServoAngle  = ARM_DOWN_ANGLE;
float targetServoAngle   = ARM_DOWN_ANGLE;
float armSpeed           = 2.2;   // Smooth 2.2° per frame (human-like, zero jerk)

// BACK-TO-BACK SERVO MIRRORING:
// When servos are mounted back-to-back, one faces left and one faces right.
// If MIRROR_RIGHT_ARM is true: rightAngle = 180 - targetAngle.
// This ensures BOTH arms swing forward and up together! Can toggle anytime via 'M'.
bool MIRROR_RIGHT_ARM    = true;
bool faceStoryActive     = true;  // Continuous story animation loop active
bool calibrationMode     = false; // Locks both motors at 90.0° for horn installation
bool isStopped           = false;

void setSyncTarget(float targetAngle, float speed = 2.2) {
  targetServoAngle = constrain(targetAngle, 0, 180);
  armSpeed = speed;
  isStopped = false;
  calibrationMode = false;
}

// ----------------------------------------------------------------------------
// 4. ORGANIC EYEBALL PHYSICS, BLINKING & WINKING
// ----------------------------------------------------------------------------
float pupilX = 0.0;
float pupilY = 0.0;
float targetPupilX = 0.0;
float targetPupilY = 0.0;

// Continuous Eyelid Blink Engine
float blinkProgress = 0.0;
int blinkDirection = 0;
bool doubleBlinkQueued = false;
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkInterval = 2800;

// Smooth Animated Wink Engine (Right Eye)
float winkProgress = 0.0;
int winkDirection = 0;
unsigned long winkHoldTimer = 0;

// Story Animation Choreography Timers
int storyStep = 0;
unsigned long storyStepTimer = 0;
unsigned long introStartTime = 0;

// Timing tickers
unsigned long lastArmUpdateTime = 0;
unsigned long lastOledDraw = 0;

// IR Sensor State
bool irSensorEnabled = true;
bool lastIrDetected = false;
unsigned long irTriggerTime = 0;
unsigned long irDetectStartTime = 0;

// ----------------------------------------------------------------------------
// 5. 5x7 FONT & MINECRAFT BITMAPS IN PROGMEM
// ----------------------------------------------------------------------------
const uint8_t FONT5x7[][5] PROGMEM = {
  {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
  {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 '!'
  {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 '"'
  {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 '#'
  {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 '$'
  {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 '%'
  {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 '&'
  {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '''
  {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 '('
  {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 ')'
  {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // 42 '*'
  {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 '+'
  {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ','
  {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 '-'
  {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 '.'
  {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 '/'
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 '0'
  {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 '1'
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 '2'
  {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 '3'
  {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 '4'
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 '5'
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 '6'
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 '7'
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 '8'
  {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 '9'
  {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 ':'
  {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ';'
  {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 '<'
  {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 '='
  {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 '>'
  {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 '?'
  {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 '@'
  {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 'A'
  {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 'B'
  {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 'C'
  {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 'D'
  {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 'E'
  {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 'F'
  {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 'G'
  {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 'H'
  {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 'I'
  {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 'J'
  {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 'K'
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 'L'
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 'M'
  {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 'N'
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 'O'
  {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 'P'
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 'Q'
  {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 'R'
  {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 'S'
  {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 'T'
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 'U'
  {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 'V'
  {0x7F, 0x20, 0x18, 0x20, 0x7F}, // 87 'W'
  {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 'X'
  {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 'Y'
  {0x61, 0x51, 0x49, 0x45, 0x43}  // 90 'Z'
};

const uint8_t CREEPER_MAP[8] PROGMEM = {
  0b11111111, 0b10011001, 0b10011001, 0b11100111,
  0b11000011, 0b11000011, 0b11011011, 0b11011011
};

const uint8_t HEROBRINE_MAP[8] PROGMEM = {
  0b11111111, 0b11000011, 0b01100110, 0b00011000,
  0b00111100, 0b01111110, 0b00111100, 0b00000000
};

// 40-Row Symmetric Plump Cartoon Heart Table (Half-Spans in PROGMEM)
const uint8_t BIG_HEART_IN[40] PROGMEM = {
  9, 4, 3, 2, 1, 1, 1, 1, 1, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const uint8_t BIG_HEART_OUT[40] PROGMEM = {
  10, 14, 15, 16, 17, 18, 18, 18, 19, 19,
  19, 19, 18, 18, 17, 17, 16, 16, 15, 15,
  14, 14, 13, 13, 12, 11, 11, 10,  9,  9,
   8,  7,  6,  6,  5,  4,  3,  2,  1,  0
};

// ----------------------------------------------------------------------------
// 6. HIGH-SPEED SH1106 / SSD1306 DISPLAY DRIVER (400 kHz Fast I2C)
// ----------------------------------------------------------------------------
void sendOledCmd(uint8_t cmd) {
  Wire.beginTransmission(OLED_I2C_ADDR);
  Wire.write(0x80);
  Wire.write(cmd);
  Wire.endTransmission();
}

void initOled() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(400000); // ⚡ 400 kHz Fast I2C Bus!
  delay(50);

  // Universal Initialization Sequence (Compatible with SH1106 and SSD1306)
  static const uint8_t initSeq[] PROGMEM = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0xAD, 0x8B, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xBF,
    0xD9, 0x22, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
  };

  for (uint8_t i = 0; i < sizeof(initSeq); i++) {
    sendOledCmd(pgm_read_byte(&initSeq[i]));
  }
}

void drawIntroTextScreen() {
  const char line1[] = "HELLO!";
  const char line2[] = "I AM WOODY";
  const char line3[] = "MADE BY";
  const char line4[] = "BATCH 4 TEAM";

  for (uint8_t p = 0; p < 8; p++) {
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 + p);
    Wire.write(0x02);
    Wire.write(0x10);
    Wire.endTransmission();

    for (uint8_t col = 0; col < 128; col++) {
      uint8_t b = 0x00;

      if (p == 0) {
        if (col >= 4 && col <= 123) b = 0x01;
        if (col == 4 || col == 123) b = 0xFF;
      } else if (p == 7) {
        if (col >= 4 && col <= 123) b = 0x80;
        if (col == 4 || col == 123) b = 0xFF;
      } else if (col == 4 || col == 123) {
        b = 0xFF;
      }

      const char* curText = NULL;
      int textLen = 0;

      if (p == 1) { curText = line1; textLen = sizeof(line1) - 1; }
      else if (p == 3) { curText = line2; textLen = sizeof(line2) - 1; }
      else if (p == 5) { curText = line3; textLen = sizeof(line3) - 1; }
      else if (p == 6) { curText = line4; textLen = sizeof(line4) - 1; }

      if (curText != NULL) {
        int totalWidth = textLen * 6;
        int startX = (128 - totalWidth) / 2;

        if (col >= startX && col < startX + totalWidth) {
          int relX = col - startX;
          int charIdx = relX / 6;
          int fontCol = relX % 6;

          if (fontCol < 5) {
            char c = curText[charIdx];
            if (c >= ' ' && c <= 'Z') {
              uint8_t fontData = pgm_read_byte(&FONT5x7[c - ' '][fontCol]);
              b |= fontData;
            }
          }
        }
      }

      if (col % 16 == 0) {
        Wire.endTransmission();
        Wire.beginTransmission(OLED_I2C_ADDR);
        Wire.write(0x40);
      }
      Wire.write(b);
    }
    Wire.endTransmission();
  }
}

void triggerSmoothWink() {
  currentFace = FACE_ORGANIC_ROBOT;
  currentMood = MOOD_WINK;
  winkProgress = 0.0;
  winkDirection = 1;
}

void triggerGigaChadWink() {
  currentFace = FACE_ORGANIC_ROBOT;
  currentMood = MOOD_GIGACHAD;
  winkProgress = 100.0;
  winkDirection = 0;
  targetPupilX = -1;
  targetPupilY = -1;
}

void renderFastScreen() {
  int pX = (int)round(pupilX);
  int pY = (int)round(pupilY);
  int curBlink = (int)round(blinkProgress);
  int curWink  = (int)round(winkProgress);
  int giggleY  = (currentMood == MOOD_HEART) ? (int)round(sin(millis() * 0.022) * 3.5) : 0;

  int leftCenterX  = 38;
  int rightCenterX = 90;

  for (uint8_t page = 0; page < 8; page++) {
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 + page);
    Wire.write(0x02);
    Wire.write(0x10);
    Wire.endTransmission();

    uint8_t pageTopY = page * 8;

    for (uint8_t col = 0; col < 128; col++) {
      uint8_t pixelByte = 0x00;

      if (currentFace == FACE_ORGANIC_ROBOT) {
        // --- BLUSH & MOUTHS ---
        for (uint8_t bit = 0; bit < 8; bit++) {
          int y = pageTopY + bit;

          if (currentMood == MOOD_GIGACHAD) {
            if (col >= 55 && col <= 63 && (y == 52 || y == 53)) pixelByte |= (1 << bit);
            if (col >= 64 && col <= 67 && (y == 51 || y == 52)) pixelByte |= (1 << bit);
            if (col >= 68 && col <= 71 && (y == 49 || y == 50)) pixelByte |= (1 << bit);
            if (col >= 72 && col <= 74 && (y == 47 || y == 48)) pixelByte |= (1 << bit);
            if (col == 75 && (y >= 45 && y <= 47)) pixelByte |= (1 << bit);
            if (col >= 62 && col <= 66 && y == 57) pixelByte |= (1 << bit);
          } else if (currentMood == MOOD_HAPPY || currentMood == MOOD_HEART) {
            int mouthYOffset = (currentMood == MOOD_HEART) ? (giggleY / 2) : 0;
            int dx = col - 64; int dy = y - (48 + mouthYOffset);
            int r2 = dx*dx + dy*dy;
            if (r2 <= 49 && r2 >= 25 && y >= 49 + mouthYOffset) pixelByte |= (1 << bit);
            int blushY = 44 + mouthYOffset;
            if (y == blushY && ((col >= 22 && col <= 26) || (col >= 102 && col <= 106))) pixelByte |= (1 << bit);
          } else if (currentMood == MOOD_WINK || winkProgress > 20) {
            if (y == 51 && col >= 62 && col <= 72) pixelByte |= (1 << bit);
            if (y == 50 && col == 73) pixelByte |= (1 << bit);
            if (y == 44 && col >= 102 && col <= 106) pixelByte |= (1 << bit);
          } else if (currentMood == MOOD_SURPRISED) {
            int dx = col - 64; int dy = y - 51;
            int r2 = dx*dx + dy*dy;
            if (r2 <= 20 && r2 >= 8) pixelByte |= (1 << bit);
          } else {
            if (y == 52 && col >= 60 && col <= 68) pixelByte |= (1 << bit);
            if (y == 51 && (col == 59 || col == 69)) pixelByte |= (1 << bit);
          }
        }

        // --- GIGA CHAD EYEBROWS ---
        if (currentMood == MOOD_GIGACHAD) {
          for (uint8_t bit = 0; bit < 8; bit++) {
            int y = pageTopY + bit;
            if (col >= 24 && col <= 50) {
              int browY = 11 + ((col - 24) * 4) / 26;
              if (y == browY || y == browY - 1) pixelByte |= (1 << bit);
            }
            if (col >= 76 && col <= 104) {
              int rdx = col - 90;
              int browY = 8 + (rdx * rdx) / 36;
              if (y == browY || y == browY - 1) pixelByte |= (1 << bit);
            }
          }
        }

        // --- EYEBALLS / HEARTS RENDERING ---
        for (int eye = 0; eye < 2; eye++) {
          int cX = (eye == 0) ? leftCenterX : rightCenterX;
          int dx = col - cX;

          if (currentMood == MOOD_HEART) {
            if (abs(dx) <= 20) {
              for (uint8_t bit = 0; bit < 8; bit++) {
                int y = pageTopY + bit;
                int row = y - (8 + giggleY);
                if (row >= 0 && row < 40) {
                  uint8_t inSpan  = pgm_read_byte(&BIG_HEART_IN[row]);
                  uint8_t outSpan = pgm_read_byte(&BIG_HEART_OUT[row]);
                  if (abs(dx) >= inSpan && abs(dx) <= outSpan) {
                    pixelByte |= (1 << bit);
                  }
                }
              }
            }
          } else {
            // Eyelid blink bounds
            int topLidY    = 8 + (curBlink * 18) / 100;
            int bottomLidY = 44 - (curBlink * 18) / 100;

            if (eye == 1 && (currentMood == MOOD_WINK || currentMood == MOOD_GIGACHAD)) {
              topLidY    = 8 + (curWink * 18) / 100;
              bottomLidY = 44 - (curWink * 18) / 100;
            }

            if (abs(dx) <= 18) {
              for (uint8_t bit = 0; bit < 8; bit++) {
                int y = pageTopY + bit;
                if (y < topLidY || y > bottomLidY) continue;

                int dy = y - 26;
                // Rounded rectangular eyeball socket
                int clampedDx = constrain(dx, -10, 10);
                int clampedDy = constrain(dy, -10, 10);
                int cornerDx = dx - clampedDx;
                int cornerDy = dy - clampedDy;

                if ((cornerDx * cornerDx + cornerDy * cornerDy) <= 64) {
                  int pdx = dx - pX;
                  int pdy = dy - pY;
                  bool isPupil = (pdx * pdx + pdy * pdy) <= 36;
                  bool isSparkle = (pdx >= -3 && pdx <= -1 && pdy >= -3 && pdy <= -1);

                  if (isSparkle) {
                    // White highlight sparkle
                  } else if (isPupil) {
                    continue; // Dark pupil center
                  }
                  pixelByte |= (1 << bit);
                }
              }
            }
          }
        }
      }
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
// 7. SERVO UPDATE LOOP (INTERPOLATION & MIRRORING)
// ----------------------------------------------------------------------------
void updateServos() {
  if (isStopped) return;

  if (calibrationMode) {
    // LOCK BOTH FIRMLY AT 90.0°
    leftServo.write(90);
    rightServo.write(90);
    return;
  }

  unsigned long now = millis();
  if (now - lastArmUpdateTime < 20) return;
  lastArmUpdateTime = now;

  if (abs(currentServoAngle - targetServoAngle) > 0.4) {
    if (currentServoAngle < targetServoAngle) {
      currentServoAngle += armSpeed;
      if (currentServoAngle > targetServoAngle) currentServoAngle = targetServoAngle;
    } else {
      currentServoAngle -= armSpeed;
      if (currentServoAngle < targetServoAngle) currentServoAngle = targetServoAngle;
    }

    int leftAngle  = constrain((int)round(currentServoAngle) + leftOffset, 0, 180);
    int baseRight  = MIRROR_RIGHT_ARM ? (180 - (int)round(currentServoAngle)) : (int)round(currentServoAngle);
    int rightAngle = constrain(baseRight + rightOffset, 0, 180);

    leftServo.write(leftAngle);
    rightServo.write(rightAngle);
  }
}

// ----------------------------------------------------------------------------
// 8. EYE PHYSICS & STORY ANIMATION CHOREOGRAPHY
// ----------------------------------------------------------------------------
void updateEyePhysics() {
  unsigned long now = millis();

  pupilX += (targetPupilX - pupilX) * 0.22;
  pupilY += (targetPupilY - pupilY) * 0.22;

  // Smooth Animated Wink
  if (winkDirection == 1) {
    winkProgress += 10.0;
    if (winkProgress >= 100.0) {
      winkProgress = 100.0;
      winkDirection = 2;
      winkHoldTimer = now;
    }
  } else if (winkDirection == 2) {
    if (now - winkHoldTimer >= 1100) {
      winkDirection = -1;
    }
  } else if (winkDirection == -1) {
    winkProgress -= 12.0;
    if (winkProgress <= 0.0) {
      winkProgress = 0.0;
      winkDirection = 0;
      currentMood = MOOD_NORMAL;
    }
  }

  // Natural Blinking
  if (blinkDirection == 1) {
    blinkProgress += 20.0;
    if (blinkProgress >= 100.0) {
      blinkProgress = 100.0;
      blinkDirection = -1;
    }
  } else if (blinkDirection == -1) {
    blinkProgress -= 20.0;
    if (blinkProgress <= 0.0) {
      blinkProgress = 0.0;
      blinkDirection = 0;
      lastBlinkTime = now;
      if (doubleBlinkQueued) {
        doubleBlinkQueued = false;
        nextBlinkInterval = 180;
      } else {
        nextBlinkInterval = random(2500, 5000);
      }
    }
  } else {
    if (now - lastBlinkTime >= nextBlinkInterval) {
      blinkDirection = 1;
      doubleBlinkQueued = (random(0, 100) < 30);
    }
  }
}

void setStoryStep(int newStep, unsigned long now) {
  storyStep = newStep;
  storyStepTimer = now;

  switch (storyStep) {
    case 0: // Glance Left
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
      targetPupilX = -6; targetPupilY = 0;
      setSyncTarget(ARM_DOWN_ANGLE, 2.0);
      break;
    case 1: // Glance Right
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
      targetPupilX = 6; targetPupilY = 0;
      break;
    case 2: // Look Center
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
      targetPupilX = 0; targetPupilY = 0;
      break;
    case 3: // Smooth Wink 😉
      targetPupilX = 0; targetPupilY = 0;
      triggerSmoothWink();
      break;
    case 4: // Confident Giga Chad Smirk 😎
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_GIGACHAD;
      winkProgress = 0.0; winkDirection = 0;
      targetPupilX = 0; targetPupilY = 0;
      break;
    case 5: // HUGE Glowing Heart Eyes ❤️❤️ + Arms UP! 🙌
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_HEART;
      winkProgress = 0.0; winkDirection = 0;
      targetPupilX = 0; targetPupilY = 0;
      setSyncTarget(ARM_UP_ANGLE, 2.4);
      break;
  }
}

void updateStoryChoreography() {
  unsigned long now = millis();

  if (currentFace == FACE_TEXT_INTRO) {
    if (now - introStartTime > 3500) {
      faceStoryActive = true;
      setStoryStep(0, now);
    }
    return;
  }

  if (!faceStoryActive) return;
  if (lastIrDetected) return; // Hold reaction while hand is near IR sensor

  static const unsigned long stepDurations[] = { 1800, 1800, 1200, 2200, 2500, 3200 };
  const int totalSteps = sizeof(stepDurations) / sizeof(stepDurations[0]);

  if (now - storyStepTimer >= stepDurations[storyStep]) {
    int nextStep = (storyStep + 1) % totalSteps;
    setStoryStep(nextStep, now);
  }
}

// ----------------------------------------------------------------------------
// 9. IR PROXIMITY SENSOR (INTERACTIVE HAND DETECTION)
// ----------------------------------------------------------------------------
void updateIrSensor() {
  if (!irSensorEnabled) return;

  // Active LOW obstacle sensor: LOW = Object/Hand detected!
  bool isNear = (digitalRead(IR_SENSOR_PIN) == LOW);
  unsigned long now = millis();

  if (isNear && !lastIrDetected) {
    lastIrDetected = true;
    irDetectStartTime = now;

    // Trigger instant celebratory reaction!
    currentFace = FACE_ORGANIC_ROBOT;
    currentMood = MOOD_HEART;
    winkProgress = 0.0;
    winkDirection = 0;
    targetPupilX = 0;
    targetPupilY = 0;
    setSyncTarget(ARM_UP_ANGLE, 3.2);

    Serial.println(F("[IR] 🖐️ Hand Wave Detected! Heart Eyes + Arms UP! ❤️🙌"));
    SerialBT.println(F("[IR] 🖐️ Hand Wave Detected! Heart Eyes + Arms UP! ❤️🙌"));
  } else if (!isNear && lastIrDetected) {
    // Hand removed: return to resting story
    if (now - irDetectStartTime > 1500) {
      lastIrDetected = false;
      currentMood = MOOD_NORMAL;
      setSyncTarget(ARM_DOWN_ANGLE, 2.0);
      setStoryStep(0, now);
    }
  }
}

// ----------------------------------------------------------------------------
// 10. UNIFIED COMMAND PARSER (DISPATCHES SERIAL & BLUETOOTH APP INPUTS)
// ----------------------------------------------------------------------------
void printHelp(Stream& out) {
  out.println(F("\n========================================================"));
  out.println(F("       WOODY ROBOT — ESP32 COMMAND CONSOLE 🤖           "));
  out.println(F("========================================================"));
  out.println(F("  C / CAL       -> CALIBRATION: Locks both at 90° for horns 🎯"));
  out.println(F("  M / MIRROR    -> Toggle Mirror Mode (back-to-back arms) 🔄"));
  out.println(F("  TEST          -> Step-by-step diagnostic sweep 🔬"));
  out.println(F("  U / UP        -> Both arms swing UP in sync 🙌"));
  out.println(F("  D / DOWN      -> Both arms swing DOWN in sync 👇"));
  out.println(F("  W / WAVE      -> Synchronized celebratory wave 👋"));
  out.println(F("  G / CHAD      -> Giga Chad Wink & Smirk 😎💪"));
  out.println(F("  H / HEART     -> HUGE Glowing Heart Eyes ❤️❤️"));
  out.println(F("  K / WINK      -> Animated one-eye wink 😉"));
  out.println(F("  1 / CREEPER   -> Minecraft Creeper face 🟩"));
  out.println(F("  2 / HERO      -> Glowing Herobrine face 👻"));
  out.println(F("  3 / STORY     -> Resume default story animation"));
  out.println(F("  L <angle>     -> Move Left arm only (e.g. 'L 90')"));
  out.println(F("  R <angle>     -> Move Right arm only (e.g. 'R 90')"));
  out.println(F("  0 to 180      -> Set both arms to angle (e.g. '90')"));
  out.println(F("  S / STOP      -> Emergency freeze arms 🛑"));
  out.println(F("========================================================\n"));
}

void processCommand(String cmd, Stream& out) {
  cmd.trim();
  if (cmd.length() == 0) return;

  cmd.toUpperCase();

  // Calibration Mode
  if (cmd == "C" || cmd == "CAL") {
    calibrationMode = true;
    isStopped = false;
    currentServoAngle = 90.0;
    targetServoAngle = 90.0;
    leftServo.write(90);
    rightServo.write(90);
    out.println(F("🎯 CALIBRATION MODE ACTIVE!"));
    out.println(F("   Both servos locked at exactly 90.0°."));
    out.println(F("   👉 Pull plastic horns OFF and press them back on pointing STRAIGHT DOWN!"));
    out.println(F("   Type 'U' or 'D' to exit calibration mode."));
    return;
  }

  // Toggle Mirror Mode
  if (cmd == "M" || cmd == "MIRROR") {
    MIRROR_RIGHT_ARM = !MIRROR_RIGHT_ARM;
    out.print(F("🔄 Mirror Mode: "));
    out.println(MIRROR_RIGHT_ARM ? F("ENABLED (Back-to-back mounting)") : F("DISABLED (Standard)"));
    return;
  }

  // Diagnostic Test
  if (cmd == "TEST" || cmd == "CHECK") {
    out.println(F("🔬 DIAGNOSTIC TEST: Testing arms sequentially..."));
    calibrationMode = false;
    isStopped = false;

    out.println(F("  -> Left Arm Sweep (30° -> 130° -> 90°)..."));
    leftServo.write(30); delay(600);
    leftServo.write(130); delay(600);
    leftServo.write(90); delay(400);

    out.println(F("  -> Right Arm Sweep (30° -> 130° -> 90°)..."));
    rightServo.write(30); delay(600);
    rightServo.write(130); delay(600);
    rightServo.write(90); delay(400);

    out.println(F("✅ Test sweep complete! Both returned to 90°."));
    return;
  }

  // Arms UP
  if (cmd == "U" || cmd == "UP") {
    out.println(F("🙌 Both Arms UP!"));
    setSyncTarget(ARM_UP_ANGLE, 2.8);
    return;
  }

  // Arms DOWN
  if (cmd == "D" || cmd == "DOWN") {
    out.println(F("👇 Both Arms DOWN!"));
    setSyncTarget(ARM_DOWN_ANGLE, 2.4);
    return;
  }

  // Wave Gesture
  if (cmd == "W" || cmd == "WAVE") {
    out.println(F("👋 Playful Wave Gesture!"));
    setSyncTarget(ARM_UP_ANGLE, 3.0);
    currentFace = FACE_ORGANIC_ROBOT;
    currentMood = MOOD_HAPPY;
    return;
  }

  // Giga Chad
  if (cmd == "G" || cmd == "CHAD") {
    out.println(F("😎 GIGA CHAD ACTIVE!"));
    faceStoryActive = false;
    triggerGigaChadWink();
    setSyncTarget(ARM_UP_ANGLE, 3.0);
    return;
  }

  // Heart Eyes
  if (cmd == "H" || cmd == "HEART") {
    out.println(F("❤️❤️ HUGE GLOWING HEARTS!"));
    faceStoryActive = false;
    currentFace = FACE_ORGANIC_ROBOT;
    currentMood = MOOD_HEART;
    setSyncTarget(ARM_UP_ANGLE, 2.6);
    return;
  }

  // Wink
  if (cmd == "K" || cmd == "WINK") {
    out.println(F("😉 Animated Wink!"));
    triggerSmoothWink();
    return;
  }

  // Minecraft Creeper
  if (cmd == "1" || cmd == "CREEPER") {
    out.println(F("🟩 Creeper Face!"));
    faceStoryActive = false;
    currentFace = FACE_CREEPER;
    return;
  }

  // Minecraft Herobrine
  if (cmd == "2" || cmd == "HERO" || cmd == "HEROBRINE") {
    out.println(F("👻 Herobrine Face!"));
    faceStoryActive = false;
    currentFace = FACE_HEROBRINE;
    return;
  }

  // Resume Story Animation Loop
  if (cmd == "3" || cmd == "STORY" || cmd == "ROBOT") {
    out.println(F("🎬 Resuming Story Animation Loop!"));
    faceStoryActive = true;
    currentFace = FACE_ORGANIC_ROBOT;
    currentMood = MOOD_NORMAL;
    setStoryStep(0, millis());
    return;
  }

  // Team Intro
  if (cmd == "T" || cmd == "INTRO") {
    out.println(F("📺 Showing Team Intro Screen!"));
    currentFace = FACE_TEXT_INTRO;
    introStartTime = millis();
    drawIntroTextScreen();
    return;
  }

  // Stop / Freeze
  if (cmd == "S" || cmd == "STOP") {
    isStopped = true;
    out.println(F("🛑 Emergency Stop! Arms frozen."));
    return;
  }

  // Help
  if (cmd == "?" || cmd == "HELP") {
    printHelp(out);
    return;
  }

  // Left Arm Manual Angle ('L <angle>')
  if (cmd.startsWith("L ")) {
    int angle = cmd.substring(2).toInt();
    angle = constrain(angle, 0, 180);
    calibrationMode = false;
    leftServo.write(angle);
    out.printf("👈 Left Arm set to %d°\n", angle);
    return;
  }

  // Right Arm Manual Angle ('R <angle>')
  if (cmd.startsWith("R ")) {
    int angle = cmd.substring(2).toInt();
    angle = constrain(angle, 0, 180);
    calibrationMode = false;
    rightServo.write(angle);
    out.printf("👉 Right Arm set to %d°\n", angle);
    return;
  }

  // Number only: Both arms to angle
  if (cmd.charAt(0) >= '0' && cmd.charAt(0) <= '9') {
    int angle = cmd.toInt();
    angle = constrain(angle, 0, 180);
    out.printf("🎯 Both arms target: %d°\n", angle);
    setSyncTarget(angle, 2.4);
    return;
  }

  out.print(F("❓ Unknown command: '"));
  out.print(cmd);
  out.println(F("'. Type '?' or 'HELP' for menu."));
}

// ----------------------------------------------------------------------------
// 11. SETUP
// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println(F("\n╔══════════════════════════════════════════════════╗"));
  Serial.println(F("║   WOODY ROBOT — ESP32 DEVKIT V1 BOOTING... 🤖    ║"));
  Serial.println(F("╚══════════════════════════════════════════════════╝"));

  // 1. OLED Display Bring-Up
  Serial.print(F("[BOOT] OLED Display (I2C on GPIO 21/22)... "));
  initOled();
  drawIntroTextScreen();
  introStartTime = millis();
  Serial.println(F("OK"));

  // 2. Hardware Timer Allocation for ESP32 Servos
  Serial.print(F("[BOOT] Hardware LEDC PWM Servos... "));
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  leftServo.setPeriodHertz(50);   // Standard 50Hz servo refresh rate
  rightServo.setPeriodHertz(50);

  leftServo.attach(LEFT_SERVO_PIN, 500, 2400);   // SG90 pulse widths (500us - 2400us)
  rightServo.attach(RIGHT_SERVO_PIN, 500, 2400);

  // Initial rest position
  int initLeft  = constrain(ARM_DOWN_ANGLE + leftOffset, 0, 180);
  int initRight = constrain((MIRROR_RIGHT_ARM ? (180 - ARM_DOWN_ANGLE) : ARM_DOWN_ANGLE) + rightOffset, 0, 180);
  leftServo.write(initLeft);
  rightServo.write(initRight);
  Serial.println(F("OK"));

  // 3. IR Sensor Bring-Up
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  Serial.println(F("[BOOT] Shoulder IR Sensor (GPIO 14)... OK"));

  // 4. Bluetooth Serial Bring-Up
  Serial.print(F("[BOOT] Starting Bluetooth Serial ('Woody-Robot')... "));
  SerialBT.begin("Woody-Robot"); // Bluetooth device name
  Serial.println(F("READY!"));
  Serial.println(F("       📱 Open your phone's Bluetooth settings, pair with 'Woody-Robot'"));
  Serial.println(F("       📱 Then open Serial Bluetooth Terminal / Dabble App!"));

  printHelp(Serial);
  lastBlinkTime = millis();
  storyStepTimer = millis();
}

// ----------------------------------------------------------------------------
// 12. MAIN LOOP (NON-BLOCKING CONCURRENT EXECUTION)
// ----------------------------------------------------------------------------
void loop() {
  unsigned long now = millis();

  // 1. Check Serial Monitor Input (USB)
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    processCommand(cmd, Serial);
  }

  // 2. Check Bluetooth App Input (Phone)
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    // Echo back to phone and debug serial
    Serial.print(F("[BT RECV] "));
    Serial.println(cmd);
    processCommand(cmd, SerialBT);
  }

  // 3. Update Hardware Subsystems
  updateServos();
  updateIrSensor();
  updateEyePhysics();
  updateStoryChoreography();

  // 4. Render OLED Face at ~30 FPS
  if (now - lastOledDraw >= 33 && currentFace != FACE_TEXT_INTRO) {
    lastOledDraw = now;
    renderFastScreen();
  }
}

