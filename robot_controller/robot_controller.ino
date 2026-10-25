/*
 * ============================================================================
 *   WOODY THE ROBOT COMPANION - MADE BY BATCH 4 TEAM 🤖🎓✨
 *   Continuous Choreographed Story Animation (WITH BIG GLOWING HEARTS! ❤️❤️)
 * ============================================================================
 *   Continuous Animation Loop:
 *     1. Organic Eyeballs Glance Left & Right while Blinking naturally
 *     2. Smooth Cheeky One-Eye Wink 😉
 *     3. BOTH Eyes Turn into HUGE GLOWING HEARTS ❤️❤️ + Arms Swing UP! 🙌
 *     4. Repeats seamlessly forever!
 *
 *   Interactive Serial Commands (115200 baud):
 *     - 'T' or 'INTRO'   -> Show Batch 4 Team Intro Screen
 *     - '3' or 'ROBOT'   -> Resume Default Story Animation
 *     - '1' or 'CREEPER' -> 1:1 Pixel Minecraft Creeper 🟩
 *     - '2' or 'HEROBRINE'-> Glowing Herobrine Face 👻
 *     - 'S'              -> Emergency STOP / Freeze Arms 🛑
 *     - 'W'              -> Resume Animation Loop 👋
 *     - 'U' / 'D'        -> Arms UP / DOWN
 *     - 'C'              -> Calibration Mode (Locks at 90°) 🎯
 *     - '?' or 'HELP'    -> Show full command menu
 * ============================================================================
 */

#include <Wire.h>
#include <Servo.h>

#define OLED_I2C_ADDR   0x3C
#define LEFT_SERVO_PIN  9
#define RIGHT_SERVO_PIN 10
#define LED_PIN         13
#define IR_SENSOR_PIN   2   // Signal OUT pin from IR Obstacle Sensor
#define IR_GND_PIN      3   // Pure 0V Ground pin for IR sensor (so you don't need to share GND!)
#define IR_POWER_PIN    4   // Pure 5V VCC power pin for IR sensor (if 5V headers are full)

Servo leftServo;
Servo rightServo;

// ----------------------------------------------------------------------------
// 1. MODES & MOODS
// ----------------------------------------------------------------------------
enum FaceMode {
  FACE_ORGANIC_ROBOT, // Default: Lifelike moving eyes, wink & hearts!
  FACE_TEXT_INTRO,    // "HELLO, I AM WOODY - BATCH 4 TEAM"
  FACE_HAIII,         // "HAIII! I SEE YOU!" (Triggered by IR sensor!)
  FACE_CREEPER,       // Minecraft Creeper
  FACE_HEROBRINE      // Glowing Herobrine
};

enum RobotMood {
  MOOD_NORMAL,        // Natural glancing eyeballs
  MOOD_HAPPY,         // Happy crescent eyes + smile + blush
  MOOD_WINK,          // Smooth animated wink 😉
  MOOD_HEART,         // HUGE glowing heart eyes in BOTH eyes ❤️❤️
  MOOD_SURPRISED      // Wide alert dilated eyes 😲
};

FaceMode currentFace = FACE_TEXT_INTRO; // Shows Batch 4 Intro for 3.5s, then starts story loop!
RobotMood currentMood = MOOD_NORMAL;

// ----------------------------------------------------------------------------
// 2. 5x7 FONT TABLE IN PROGMEM (For Crisp Batch 4 Text)
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
// 3. MOTION & CALIBRATION SETTINGS
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
// 4. ORGANIC EYEBALL PHYSICS & BLINKING
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

// Timers
unsigned long lastArmUpdateTime = 0;
unsigned long lastOledDraw = 0;

// IR Sensor State
bool lastIrDetected = false;
unsigned long irTriggerTime = 0;
unsigned long irDetectStartTime = 0;

// ----------------------------------------------------------------------------
// 5. HIGH-SPEED SH1106 CONTROLLER (400 kHz Fast-Mode!)
// ----------------------------------------------------------------------------

void sendSH1106Command(uint8_t cmd) {
  Wire.beginTransmission(OLED_I2C_ADDR);
  Wire.write(0x80);
  Wire.write(cmd);
  Wire.endTransmission();
}

void initSH1106() {
  Wire.begin();
  Wire.setClock(400000); // ⚡ 400 kHz Fast I2C Bus!
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
// 6. SCREEN: "HELLO, I AM WOODY - BATCH 4 TEAM"
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// 6B. SCREEN: "HAIII! I SEE YOU!" (Triggered by IR Sensor Hand Detection!)
// ----------------------------------------------------------------------------

void drawHaiiiScreen() {
  const char line1[] = "HAIII!";
  const char line2[] = "I SEE YOU!";
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

      // Sparkling border
      uint8_t sparkle = (millis() / 180) % 2;
      if (p == 0) {
        if (col >= 4 && col <= 123) b = (col % 2 == sparkle) ? 0x03 : 0x00;
        if (col == 4 || col == 123) b = 0xFF;
      } else if (p == 7) {
        if (col >= 4 && col <= 123) b = (col % 2 == sparkle) ? 0xC0 : 0x00;
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

// ----------------------------------------------------------------------------
// 7. HUGE 39x40 PIXEL GLOWING CARTOON HEARTS (Full Face Sockets!)
// ----------------------------------------------------------------------------

void triggerSmoothWink() {
  currentFace = FACE_ORGANIC_ROBOT;
  currentMood = MOOD_WINK;
  winkProgress = 0.0;
  winkDirection = 1;
}

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

void renderFastScreen() {
  int pX = (int)round(pupilX);
  int pY = (int)round(pupilY);
  int curBlink = (int)round(blinkProgress);
  int curWink  = (int)round(winkProgress);

  int leftCenterX  = 38;
  int rightCenterX = 90;
  int eyeCenterY   = 28;

  int eyeHalfW = 14; // Sclera width = 28px
  int eyeHalfH = (currentMood == MOOD_SURPRISED) ? 19 : 17; // Sclera height = 34px

  int leftLidHalfH  = map(curBlink, 0, 100, eyeHalfH, 2);
  int rightLidHalfH = map(max(curBlink, curWink), 0, 100, eyeHalfH, 2);
  if (leftLidHalfH < 2) leftLidHalfH = 2;
  if (rightLidHalfH < 2) rightLidHalfH = 2;

  for (uint8_t p = 0; p < 8; p++) {
    int pageTopY = p * 8;

    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 + p);
    Wire.write(0x02);
    Wire.write(0x10);
    Wire.endTransmission();

    for (uint8_t col = 0; col < 128; col++) {
      uint8_t pixelByte = 0x00;

      // ======================================================================
      // 1. ORGANIC EYEBALLS WITH HUGE GLOWING HEARTS
      // ======================================================================
      if (currentFace == FACE_ORGANIC_ROBOT) {

        // --- HUGE 39x40 GLOWING CARTOON HEARTS IN BOTH EYES! ❤️❤️ ---
        if (currentMood == MOOD_HEART) {
          // Heartbeat pulse rhythm: thump-thump ... thump-thump
          unsigned long pulseTime = millis() % 850;
          bool isPulsing = (pulseTime < 140) || (pulseTime > 240 && pulseTime < 380);
          int heartTopY = isPulsing ? 5 : 6;

          // Check Left Eye (center col 36) and Right Eye (center col 92)
          int dxL = abs(col - 36);
          int dxR = abs(col - 92);
          int dx = -1;
          int curHeartCenterX = 0;

          if (dxL <= 19) {
            dx = dxL;
            curHeartCenterX = 36;
          } else if (dxR <= 19) {
            dx = dxR;
            curHeartCenterX = 92;
          }

          if (dx >= 0) {
            for (uint8_t bit = 0; bit < 8; bit++) {
              int y = pageTopY + bit;
              int relY = y - heartTopY;

              if (relY >= 0 && relY < 40) {
                uint8_t xIn  = pgm_read_byte(&BIG_HEART_IN[relY]);
                uint8_t xOut = pgm_read_byte(&BIG_HEART_OUT[relY]);

                // Expand by 1px during rhythmic heartbeat pulse!
                if (isPulsing && relY > 0) {
                  xOut = min(20, xOut + 1);
                  if (xIn > 0) xIn--;
                }

                if (dx >= xIn && dx <= xOut) {
                  // Cute glossy shine specular reflection in top-left lobe of both hearts!
                  int relXFromCenter = col - curHeartCenterX;
                  bool isShine = (relXFromCenter >= -12 && relXFromCenter <= -8 && relY >= 5 && relY <= 8);

                  if (!isShine) {
                    pixelByte |= (1 << bit);
                  }
                }
              }
            }
          }
        }
        // --- REALISTIC MOVING EYEBALLS ---
        else {
          bool inLeftX  = (col >= leftCenterX - eyeHalfW && col <= leftCenterX + eyeHalfW);
          bool inRightX = (col >= rightCenterX - eyeHalfW && col <= rightCenterX + eyeHalfW);

          if (inLeftX || inRightX) {
            bool isRight = inRightX;
            int curCenterX = isRight ? rightCenterX : leftCenterX;
            int curLidH    = isRight ? rightLidHalfH : leftLidHalfH;
            int curEyeBlink = isRight ? max(curBlink, curWink) : curBlink;

            // SMOOTH WINK: Right eye curves shut playfully
            if (isRight && curWink >= 75) {
              int dx = col - curCenterX;
              int targetY = eyeCenterY + 2 + (dx * dx) / 26;
              for (uint8_t bit = 0; bit < 8; bit++) {
                int y = pageTopY + bit;
                if (abs(y - targetY) <= 1) pixelByte |= (1 << bit);
              }
            }
            // HAPPY CRESCENTS: Squinting smile eyes
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
            // ORGANIC EYEBALL: Glowing Outer Sclera + Moving Pupil + Glass Sparkles!
            else {
              int curTopY = eyeCenterY - curLidH;
              int curBotY = eyeCenterY + curLidH;
              int dx = abs(col - curCenterX);
              int distFromEdgeX = eyeHalfW - dx;

              // Actual Moving Pupil Center
              int pupCenterX = curCenterX + pX;
              int pupCenterY = eyeCenterY + pY;
              int pupRadiusSq = (currentMood == MOOD_SURPRISED) ? 16 : 28;

              for (uint8_t bit = 0; bit < 8; bit++) {
                int y = pageTopY + bit;

                if (y >= curTopY && y <= curBotY) {
                  bool cornerCut = false;
                  if (curEyeBlink < 80) {
                    int distFromEdgeY = min(y - curTopY, curBotY - y);
                    if (distFromEdgeX < 5 && distFromEdgeY < 5) {
                      int cx = 5 - distFromEdgeX;
                      int cy = 5 - distFromEdgeY;
                      if (cx*cx + cy*cy > 25) cornerCut = true;
                    }
                  }

                  if (!cornerCut) {
                    int pdx = col - pupCenterX;
                    int pdy = y - pupCenterY;
                    bool isPupil = (pdx*pdx + pdy*pdy <= pupRadiusSq);

                    bool isSparkle = false;
                    if (curEyeBlink < 25) {
                      if (col >= curCenterX + 3 && col <= curCenterX + 6 && y >= eyeCenterY - 11 && y <= eyeCenterY - 8) {
                        isSparkle = true;
                      }
                      if (col >= curCenterX - 7 && col <= curCenterX - 5 && y >= eyeCenterY + 5 && y <= eyeCenterY + 7) {
                        isSparkle = true;
                      }
                    }

                    if (isSparkle) {
                      // Sparkle
                    } else if (isPupil) {
                      continue; // Dark pupil
                    }
                    pixelByte |= (1 << bit);
                  }
                }
              }
            }
          }
        }

        // --- CUTE MOUTHS & ROSY BLUSH ---
        for (uint8_t bit = 0; bit < 8; bit++) {
          int y = pageTopY + bit;

          if (currentMood == MOOD_HAPPY || currentMood == MOOD_HEART) {
            int dx = col - 64; int dy = y - 48;
            int r2 = dx*dx + dy*dy;
            if (r2 <= 49 && r2 >= 25 && y >= 49) pixelByte |= (1 << bit);
            if (y == 44 && ((col >= 22 && col <= 26) || (col >= 102 && col <= 106))) pixelByte |= (1 << bit);
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
// 8. CONTINUOUS EYE SACCADES, WINKING & BLINKING PHYSICS
// ----------------------------------------------------------------------------

void updateEyePhysics() {
  unsigned long now = millis();

  pupilX += (targetPupilX - pupilX) * 0.22;
  pupilY += (targetPupilY - pupilY) * 0.22;

  // Smooth Animated Wink Physics
  if (winkDirection == 1) {
    winkProgress += 14.0;
    if (winkProgress >= 100.0) {
      winkProgress = 100.0;
      winkDirection = 2;
      winkHoldTimer = now;
    }
  } else if (winkDirection == 2) {
    if (now - winkHoldTimer >= 500) {
      winkDirection = -1;
    }
  } else if (winkDirection == -1) {
    winkProgress -= 16.0;
    if (winkProgress <= 0.0) {
      winkProgress = 0.0;
      winkDirection = 0;
      currentMood = MOOD_NORMAL;
    }
  }

  // Natural Eyelid Blinking
  if (blinkDirection == 0 && (now - lastBlinkTime > nextBlinkInterval)) {
    if (winkDirection == 0 && currentMood != MOOD_HEART) {
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
        nextBlinkInterval = random(2200, 4500);
      }
    }
  }
}

// ----------------------------------------------------------------------------
// 9. CONTINUOUS CHOREOGRAPHED STORY ANIMATION
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

void updateStoryChoreography() {
  unsigned long now = millis();

  if (currentFace == FACE_TEXT_INTRO) {
    if (now - introStartTime > 3500) {
      currentFace = FACE_ORGANIC_ROBOT;
      storyStep = 0;
      storyStepTimer = now;
    }
    return;
  }

  if (!autoDanceMode || calibrationMode || isStopped) return;
  if (lastIrDetected) return; // Keep Woody in interactive greeting while hand is present!

  // The Continuous Choreographed Story:
  // Step 0: Glance Left & Blink naturally (2.6s)
  // Step 1: Glance Right & Blink naturally (2.6s)
  // Step 2: Smooth Playful Wink 😉 (2.6s)
  // Step 3: HUGE GLOWING HEARTS ❤️❤️ + Both Arms Swing UP! 🙌 (3.6s)
  // Repeat seamlessly!

  if (storyStep == 0) {
    if (now - storyStepTimer >= 2600) {
      storyStep = 1;
      storyStepTimer = now;
      targetPupilX = 6;  // Look Right!
      targetPupilY = 0;
      targetServoAngle = ARM_DOWN_ANGLE + 15;
      armSpeed = 1.8;
      Serial.println(F("[STORY] Glancing Right while Blinking"));
    }
  }
  else if (storyStep == 1) {
    if (now - storyStepTimer >= 2600) {
      storyStep = 2;
      storyStepTimer = now;
      targetPupilX = 0;
      targetPupilY = 0;
      targetServoAngle = 80;
      armSpeed = 2.2;
      triggerSmoothWink();
      Serial.println(F("[STORY] Smooth Playful Wink! 😉"));
    }
  }
  else if (storyStep == 2) {
    if (now - storyStepTimer >= 2600) {
      storyStep = 3;
      storyStepTimer = now;
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_HEART; // HUGE GLOWING HEARTS! ❤️❤️
      targetPupilX = 0;
      targetPupilY = 0;
      targetServoAngle = ARM_UP_ANGLE; // ARMS UP! 🙌
      armSpeed = 2.5;
      Serial.println(F("[STORY] HUGE GLOWING HEARTS ❤️❤️ + ARMS UP! 🙌"));
    }
  }
  else if (storyStep == 3) {
    if (now - storyStepTimer >= 3600) {
      storyStep = 0;
      storyStepTimer = now;
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
      targetPupilX = -6; // Look Left!
      targetPupilY = 0;
      targetServoAngle = ARM_DOWN_ANGLE;
      armSpeed = 1.6;
      Serial.println(F("[STORY] Glancing Left while Blinking"));
    }
  }
}

// ----------------------------------------------------------------------------
// 10. IR SENSOR (HAND DETECTION & GREETING)
// ----------------------------------------------------------------------------

void checkIrSensor() {
  bool rawReading = (digitalRead(IR_SENSOR_PIN) == LOW);
  unsigned long now = millis();

  // 50ms Debounce Filter: Prevents rapid flickering from room light noise
  if (rawReading) {
    if (irDetectStartTime == 0) irDetectStartTime = now;
  } else {
    irDetectStartTime = 0;
  }

  bool isDetected = (irDetectStartTime > 0 && (now - irDetectStartTime >= 50));

  if (isDetected && !lastIrDetected) {
    lastIrDetected = true;
    irTriggerTime = now;
    currentFace = FACE_HAIII;        // 📺 DISPLAY "HAIII! I SEE YOU!" SCREEN!
    targetServoAngle = ARM_UP_ANGLE;  // Both arms swing UP in excitement! 🙌
    armSpeed = 3.2;
    Serial.println(F("\n👋 [IR SENSOR DETECTED]: >>> HAIII! I SEE YOU! <<< ❤️🙌"));
  }
  else if (!isDetected && lastIrDetected) {
    // Hold "HAIII!" greeting for at least 2 seconds so it is clearly readable
    if (now - irTriggerTime >= 2000) {
      lastIrDetected = false;
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
      targetServoAngle = ARM_DOWN_ANGLE;
      armSpeed = 1.6;
      storyStep = 0;
      storyStepTimer = now;
      Serial.println(F("👋 [IR SENSOR]: Hand cleared. Resuming normal story loop."));
    }
  }
}

// ----------------------------------------------------------------------------
// 11. SERIAL COMMAND CENTER
// ----------------------------------------------------------------------------

void printCommandMenu() {
  Serial.println(F("\n========================================================"));
  Serial.println(F("   🤖 WOODY THE ROBOT - BATCH 4 TEAM COMMAND CENTER 🤖"));
  Serial.println(F("========================================================"));
  Serial.println(F("  [DEFAULT STORY SEQUENCE ACTIVE]"));
  Serial.println(F("    1. Moving Eyeballs Left/Right + Blinking Naturally"));
  Serial.println(F("    2. Playful One-Eye Wink 😉"));
  Serial.println(F("    3. HUGE GLOWING HEARTS in BOTH Eyes ❤️❤️ + Arms UP! 🙌"));
  Serial.println(F("    (Loops continuously!)"));
  Serial.println(F(""));
  Serial.println(F("  [IR SENSOR DETECT]: Wave hand in front -> Instant HUGE Hearts & Arms UP! 🙌"));
  Serial.println(F(""));
  Serial.println(F("  [COMMANDS]"));
  Serial.println(F("    'T' or 'INTRO'   -> Replay 'HELLO, I AM WOODY BATCH 4' Screen"));
  Serial.println(F("    '3' or 'ROBOT'   -> Resume Default Story Animation"));
  Serial.println(F("    'K' or 'WINK'    -> Instant Animated Wink 😉"));
  Serial.println(F("    'H' or 'HEART'   -> Instant HUGE Heart Eyes ❤️❤️"));
  Serial.println(F("    'O' or 'WOW'     -> Surprised Alert Eyes 😲"));
  Serial.println(F("    '1' or 'CREEPER' -> 1:1 Pixel Minecraft Creeper 🟩"));
  Serial.println(F("    '2' or 'HEROBRINE'-> Glowing Herobrine Face 👻"));
  Serial.println(F("    'S'              -> Emergency STOP / Freeze Arms 🛑"));
  Serial.println(F("    'W'              -> Resume Auto Dance / Wave 👋"));
  Serial.println(F("    'U' / 'D'        -> Arms UP / DOWN"));
  Serial.println(F("    'C'              -> Calibrate Arms at 90 deg 🎯"));
  Serial.println(F("    'L+' / 'R+'      -> Fine-tune arm angles"));
  Serial.println(F("========================================================\n"));
}

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "?" || cmd == "HELP" || cmd == "MENU") {
      printCommandMenu();
    }
    else if (cmd == "T" || cmd == "INTRO" || cmd == "HELLO" || cmd == "WOODY") {
      currentFace = FACE_TEXT_INTRO;
      introStartTime = millis();
      Serial.println(F("\n📺 [DISPLAY]: Showing 'HELLO, I AM WOODY - BATCH 4 TEAM' Screen!"));
    }
    else if (cmd == "HAI" || cmd == "HI" || cmd == "HAIII") {
      currentFace = FACE_HAIII;
      irTriggerTime = millis();
      targetServoAngle = ARM_UP_ANGLE;
      Serial.println(F("\n👋 [DISPLAY]: Showing 'HAIII! I SEE YOU!' Screen!"));
    }
    else if (cmd == "3" || cmd == "ROBOT") {
      currentFace = FACE_ORGANIC_ROBOT;
      autoDanceMode = true;
      isStopped = false;
      storyStep = 0;
      storyStepTimer = millis();
      Serial.println(F("\n🤖 [STORY]: Resumed Full Continuous Animation Loop!"));
    }
    else if (cmd == "K" || cmd == "WINK") {
      triggerSmoothWink();
      Serial.println(F("\n😉 [EXPRESSION]: Smooth Animated Wink!"));
    }
    else if (cmd == "H" || cmd == "HEART") {
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_HEART;
      Serial.println(F("\n💖 [EXPRESSION]: HUGE Glowing Heart Eyes in BOTH eyes! ❤️❤️"));
    }
    else if (cmd == "O" || cmd == "WOW" || cmd == "SURPRISED") {
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_SURPRISED;
      Serial.println(F("\n😲 [EXPRESSION]: Surprised Wide Eyes!"));
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
      storyStep = 0;
      storyStepTimer = millis();
      Serial.println(F("[COMMAND] Auto Dance / Story Loop Resumed! 👋"));
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
// 11. SETUP & MAIN LOOP
// ----------------------------------------------------------------------------

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.begin(115200);
  printCommandMenu();

  // 1. Initialize Servos
  leftServo.attach(LEFT_SERVO_PIN);
  rightServo.attach(RIGHT_SERVO_PIN);

  leftServo.write(ARM_DOWN_ANGLE);
  rightServo.write(MIRROR_RIGHT_ARM ? (180 - ARM_DOWN_ANGLE) : ARM_DOWN_ANGLE);
  delay(300);

  // 2. Initialize OLED Display at 400kHz Fast Speed
  initSH1106();

  // 3. Show Batch 4 Team Intro Screen on Boot!
  introStartTime = millis();
  drawIntroTextScreen();

  lastBlinkTime = millis();
  targetPupilX = -6;

  // 4. Initialize IR Sensor (Pin 2 = Signal, Pin 3 = Ground, Pin 4 = 5V Power)
  pinMode(IR_GND_PIN, OUTPUT);
  digitalWrite(IR_GND_PIN, LOW);    // Pure 0V Ground for the IR sensor!
  pinMode(IR_POWER_PIN, OUTPUT);
  digitalWrite(IR_POWER_PIN, HIGH); // Pure 5V Power for the IR sensor!
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
}

void loop() {
  // 1. Heartbeat LED
  digitalWrite(LED_PIN, (millis() / 500) % 2);

  // 2. Check Serial Commands & IR Sensor
  checkSerialCommands();
  checkIrSensor();

  // 3. Update Organic Eyeballs & Saccades
  updateEyePhysics();

  // 4. Update Arm Positions & Continuous Story Choreography
  updateArms();
  updateStoryChoreography();

  // 5. Render Screen at 35+ FPS
  if (millis() - lastOledDraw >= 28) {
    lastOledDraw = millis();
    if (currentFace == FACE_TEXT_INTRO) {
      drawIntroTextScreen();
    } else if (currentFace == FACE_HAIII) {
      drawHaiiiScreen();
    } else {
      renderFastScreen();
    }
  }
}
