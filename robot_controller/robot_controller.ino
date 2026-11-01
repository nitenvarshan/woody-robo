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
 *   Interactive Serial Commands (9600 baud):
 *     - 'T' or 'INTRO'   -> Show Batch 4 Team Intro Screen
 *     - '3' or 'ROBOT'   -> Resume Default Story Animation
 *     - '1' or 'CREEPER' -> 1:1 Pixel Minecraft Creeper 🟩
 *     - '2' or 'HEROBRINE'-> Glowing Herobrine Face 👻
 *     - 'S'              -> Emergency STOP / Freeze Arms 🛑
 *     - 'W'              -> Resume Story Animation Loop 👋
 *     - 'U' / 'D'        -> Both Arms UP / DOWN in sync 🙌
 *     - 'L <angle>'      -> Test Left Arm only (e.g. 'L 90')
 *     - 'R <angle>'      -> Test Right Arm only (e.g. 'R 90')
 *     - 'TEST'           -> Step-by-step diagnostic test of each servo
 *     - 'M'              -> Toggle Mirror Mode (for back-to-back vs side-by-side) 🔄
 *     - 'C'              -> Calibration Mode (Locks both at 90°) 🎯
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
#define IR_GND_PIN      3   // Ground pin for direct-plug sensor (0V)
#define IR_POWER_PIN    4   // 5V VCC power pin for direct-plug sensor

Servo leftServo;
Servo rightServo;

// ----------------------------------------------------------------------------
// 1. MODES & MOODS
// ----------------------------------------------------------------------------
enum FaceMode {
  FACE_ORGANIC_ROBOT, // Default: Lifelike moving eyes, wink & hearts!
  FACE_TEXT_INTRO,    // "HELLO, I AM WOODY - BATCH 4 TEAM"
  FACE_CREEPER,       // Minecraft Creeper
  FACE_HEROBRINE      // Glowing Herobrine
};

enum RobotMood {
  MOOD_NORMAL,        // Natural glancing eyeballs
  MOOD_HAPPY,         // Happy crescent eyes + smile + blush
  MOOD_WINK,          // Smooth animated wink 😉
  MOOD_HEART,         // HUGE glowing heart eyes in BOTH eyes ❤️❤️
  MOOD_SURPRISED,     // Wide alert dilated eyes 😲
  MOOD_GIGACHAD       // 😎 Ultra Confident Giga Chad Wink & Smirk!
};

FaceMode currentFace = FACE_ORGANIC_ROBOT; // Default: Lifelike moving eyes & story animation loop!
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
// 3. SYNCHRONIZED SERVO MOTION SETTINGS
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
float armSpeed           = 2.4;   // Proven 2.4°/frame smooth speed

// Both servos take identical angle values (e.g. 25° DOWN, 135° UP):
// Left motor on -X axis, Right motor on +X axis oriented to rotate in matching direction.
// Can be toggled anytime in Serial Monitor with 'M' command!
bool MIRROR_RIGHT_ARM    = false;
bool faceStoryActive     = true;  // Continuous OLED story animation loop active
bool calibrationMode     = false;
bool isStopped           = false;

void setSyncTarget(float targetAngle, float speed = 2.4) {
  targetServoAngle = constrain(targetAngle, 0, 180);
  armSpeed = speed;
}

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
bool irSensorEnabled = true;  // Default: IR ACTIVE (Interactive hand detection active on boot!)
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
// 7. HUGE 39x40 PIXEL GLOWING CARTOON HEARTS (Full Face Sockets!)
// ----------------------------------------------------------------------------

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
  // Bouncy giggling motion: hearts bob up and down playfully between -3 and +3 pixels
  int giggleY  = (currentMood == MOOD_HEART) ? (int)round(sin(millis() * 0.022) * 3.5) : 0;

  int leftCenterX  = 38;
  int rightCenterX = 90;
  int eyeCenterY   = 28;

  int eyeHalfW = 14; // Sclera width = 28px
  int eyeHalfH = (currentMood == MOOD_SURPRISED) ? 19 : ((currentMood == MOOD_GIGACHAD) ? 13 : 17); // Sclera height

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
          // Playful giggle bounce: hearts smoothly bob up and down
          unsigned long pulseTime = millis() % 750;
          bool isPulsing = (pulseTime < 120) || (pulseTime > 200 && pulseTime < 320);
          int heartTopY = 6 + giggleY;
          if (heartTopY < 2) heartTopY = 2;
          if (heartTopY > 10) heartTopY = 10;

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

            // SMOOTH WINK: Right eye curves shut playfully when winking
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

        // --- GIGA CHAD EYEBROWS ---
        if (currentMood == MOOD_GIGACHAD) {
          for (uint8_t bit = 0; bit < 8; bit++) {
            int y = pageTopY + bit;
            // Left Eyebrow: Confident fierce downward slope toward center (col 24 to 50, y from 11 to 15)
            if (col >= 24 && col <= 50) {
              int browY = 11 + ((col - 24) * 4) / 26;
              if (y == browY || y == browY - 1) pixelByte |= (1 << bit);
            }
            // Right Eyebrow: Cocked / Raised high above wink (col 76 to 104, arched from y 14 to y 8)
            if (col >= 76 && col <= 104) {
              int rdx = col - 90;
              int browY = 8 + (rdx * rdx) / 36;
              if (y == browY || y == browY - 1) pixelByte |= (1 << bit);
            }
          }
        }

        // --- CUTE MOUTHS & ROSY BLUSH ---
        for (uint8_t bit = 0; bit < 8; bit++) {
          int y = pageTopY + bit;

          if (currentMood == MOOD_GIGACHAD) {
            // Iconic Giga Chad Smirk (sharp, cocky upward curve on the right side)
            if (col >= 55 && col <= 63 && (y == 52 || y == 53)) pixelByte |= (1 << bit);
            if (col >= 64 && col <= 67 && (y == 51 || y == 52)) pixelByte |= (1 << bit);
            if (col >= 68 && col <= 71 && (y == 49 || y == 50)) pixelByte |= (1 << bit);
            if (col >= 72 && col <= 74 && (y == 47 || y == 48)) pixelByte |= (1 << bit);
            if (col == 75 && (y >= 45 && y <= 47)) pixelByte |= (1 << bit); // Cheeky smirk crease
            // Chiseled chin cleft
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
    winkProgress += 10.0;
    if (winkProgress >= 100.0) {
      winkProgress = 100.0;
      winkDirection = 2;
      winkHoldTimer = now;
    }
  } else if (winkDirection == 2) {
    // Hold the cute wink for 1.1s so it is clearly seen!
    if (now - winkHoldTimer >= 1100) {
      winkDirection = -1;
    }
  } else if (winkDirection == -1) {
    winkProgress -= 12.0;
    if (winkProgress <= 0.0) {
      winkProgress = 0.0;
      winkDirection = 0;
      if (currentMood == MOOD_WINK) currentMood = MOOD_NORMAL;
    }
  }

  // Natural Eyelid Blinking
  if (blinkDirection == 0 && (now - lastBlinkTime > nextBlinkInterval)) {
    if (winkDirection == 0 && currentMood != MOOD_HEART && currentMood != MOOD_GIGACHAD) {
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
    // Both servos locked at 90° in processSerialCommand. Return to keep Timer 1 clean!
    return;
  }

  if (isStopped) return;

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

    int leftAngle = constrain((int)round(currentServoAngle) + leftOffset, 0, 180);
    int baseRight = MIRROR_RIGHT_ARM ? (180 - (int)round(currentServoAngle)) : (int)round(currentServoAngle);
    int rightAngle = constrain(baseRight + rightOffset, 0, 180);

    leftServo.write(leftAngle);
    rightServo.write(rightAngle);
  }
}

void setStoryStep(int newStep, unsigned long now) {
  storyStep = newStep;
  storyStepTimer = now;

  switch (storyStep) {
    case 0: // 1. Glance Left #1
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
      targetPupilX = -6;
      targetPupilY = 0;
      setSyncTarget(ARM_DOWN_ANGLE, 2.0); // Arms resting down
      break;
    case 1: // 2. Glance Right #1
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
      targetPupilX = 6;
      targetPupilY = 0;
      break;
    case 2: // 3. Glance Left #2 (Looking left & right twice)
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
      targetPupilX = -6;
      targetPupilY = 0;
      break;
    case 3: // 4. Glance Right #2
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
      targetPupilX = 6;
      targetPupilY = 0;
      break;
    case 4: // 5. Look Straight / Center
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
      targetPupilX = 0;
      targetPupilY = 0;
      break;
    case 5: // 6. Smooth Playful Wink 😉
      targetPupilX = 0;
      targetPupilY = 0;
      triggerSmoothWink();
      break;
    case 6: // 7. Cute Confident Smile 😎 (Both eyes open, cocked eyebrow, confident smirk!)
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_GIGACHAD;
      winkProgress = 0.0;
      winkDirection = 0;
      targetPupilX = 0;
      targetPupilY = 0;
      break;
    case 7: // 8. Cute Giggling Heart Eyes! ❤️❤️ (Bounces up & down playfully!)
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_HEART;
      winkProgress = 0.0;
      winkDirection = 0;
      targetPupilX = 0;
      targetPupilY = 0;
      setSyncTarget(ARM_UP_ANGLE, 2.4); // Both arms swing UP with hearts! 🙌
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

  // Story runs continuously on OLED unless temporarily showing a specific face or IR trigger
  if (!faceStoryActive) return;
  if (lastIrDetected) return; // Hold heart eyes while IR object is present!

  // Durations for each step in milliseconds:
  unsigned long stepDuration = 1200; // Glance duration
  if (storyStep == 4) stepDuration = 1100;      // Look Straight
  else if (storyStep == 5) stepDuration = 1500; // Playful Wink 😉
  else if (storyStep == 6) stepDuration = 2000; // Cute Confident Smile 😎
  else if (storyStep == 7) stepDuration = 2400; // Giggling Heart Eyes ❤️❤️ (Bounce up & down!)

  if (now - storyStepTimer >= stepDuration) {
    int nextStep = (storyStep + 1) % 8;
    setStoryStep(nextStep, now);
  }
}

// ----------------------------------------------------------------------------
// 10. IR SENSOR (HAND DETECTION -> HUGE GLOWING HEART EYES ❤️❤️)
// ----------------------------------------------------------------------------

void checkIrSensor() {
  if (!irSensorEnabled) return; // Default: IR OFF (Toggle with 'IR' or 'IR ON')

  bool rawReading = (digitalRead(IR_SENSOR_PIN) == LOW);
  unsigned long now = millis();

  // 50ms Debounce Filter
  if (rawReading) {
    if (irDetectStartTime == 0) irDetectStartTime = now;
  } else {
    irDetectStartTime = 0;
  }

  bool isDetected = (irDetectStartTime > 0 && (now - irDetectStartTime >= 50));

  if (isDetected && !lastIrDetected) {
    lastIrDetected = true;
    irTriggerTime = now;
    currentFace = FACE_ORGANIC_ROBOT;
    currentMood = MOOD_HEART;             // ❤️❤️ Display heart emoji in eyes!
    targetPupilX = 0;
    targetPupilY = 0;
    setSyncTarget(ARM_UP_ANGLE, 2.4); // Both arms swing UP in excitement! 🙌
    Serial.println(F("\n💖 [IR DETECTED]: Heart Eyes ❤️❤️ + Both Arms UP! 🙌"));
  }
  else if (!isDetected && lastIrDetected) {
    // Hold heart eyes for at least 1.8s after object clears
    if (now - irTriggerTime >= 1800) {
      lastIrDetected = false;
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
      setSyncTarget(ARM_DOWN_ANGLE, 2.0);
      faceStoryActive = true;
      setStoryStep(0, now);
      Serial.println(F("💖 [IR CLEARED]: Resumed default animation loop."));
    }
  }
}

// ----------------------------------------------------------------------------
// 11. SERIAL COMMAND CENTER
// ----------------------------------------------------------------------------

void printCommandMenu() {
  Serial.println(F("\n========================================================"));
  Serial.println(F("   🤖 WOODY THE ROBOT - COMMAND MENU (9600 BAUD) 🤖"));
  Serial.println(F("========================================================"));
  Serial.println(F("  [SERVO CONTROLS - HIGH PRIORITY]"));
  Serial.println(F("    'U' / 'UP'       -> Both Arms UP in sync 🙌"));
  Serial.println(F("    'D' / 'DOWN'     -> Both Arms DOWN in sync 👇"));
  Serial.println(F("    'C' or 'CAL'     -> Calibration Mode (Locks both arms at 90.0°) 🎯"));
  Serial.println(F("    'TEST'           -> Diagnostic sweep: Left arm then Right arm 🔬"));
  Serial.println(F("    'M' or 'MIRROR'  -> Toggle Mirror Mode (for back-to-back vs side-by-side) 🔄"));
  Serial.println(F("    'L <angle>'      -> Move Left arm directly (e.g. 'L 90', 'L 25')"));
  Serial.println(F("    'R <angle>'      -> Move Right arm directly (e.g. 'R 90', 'R 25')"));
  Serial.println(F("    '0' - '180'      -> Move both arms to angle in sync (e.g. '90')"));
  Serial.println(F("    'L+' / 'L-'      -> Fine-tune Left arm offset"));
  Serial.println(F("    'R+' / 'R-'      -> Fine-tune Right arm offset"));
  Serial.println(F("    'S' or 'STOP'    -> Stop / Freeze arms 🛑"));
  Serial.println(F(""));
  Serial.println(F("  [DEFAULT STORY ANIMATION]"));
  Serial.println(F("    'W' or '3'       -> Resume Default Story Loop 👋"));
  Serial.println(F("       (Look Left/Right twice -> Straight -> Wink -> Confident Smile -> Giggling Hearts!)"));
  Serial.println(F("    '1' / 'CREEPER'  -> Minecraft Creeper face 🟩"));
  Serial.println(F("    '2' / 'HEROBRINE'-> Minecraft Herobrine face 👻"));
  Serial.println(F("    'K' / 'WINK'     -> Smooth Animated Wink 😉"));
  Serial.println(F("    'G' / 'SMILE'    -> Cute Confident Smile 😎"));
  Serial.println(F("    'H' / 'HEART'    -> Heart Eyes ❤️❤️"));
  Serial.println(F(""));
  Serial.println(F("  [IR SENSOR - DEFAULT: ACTIVE]"));
  Serial.println(F("    'IRTEST'         -> Diagnostic check of IR sensor reading on Pin 2 🔍"));
  Serial.println(F("    'IR ON' / 'IR'   -> Enable IR Detection"));
  Serial.println(F("    'IR OFF'         -> Disable IR Detection"));
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

  Serial.print(F(">>> [COMMAND]: '"));
  Serial.print(cmd);
  Serial.println(F("'"));

  if (cmd == "?" || cmd == "HELP" || cmd == "MENU") {
    printCommandMenu();
  }
  // SERVO COMMANDS (PRIORITY 1)
  // 1. FINE TUNING TRIMS (MUST BE BEFORE cmd.startsWith("L") / "R"!)
  else if (cmd == "L+" || cmd == "LEFT+") {
    leftOffset++;
    Serial.print(F("Left Offset: ")); Serial.println(leftOffset);
  } else if (cmd == "L-" || cmd == "LEFT-") {
    leftOffset--;
    Serial.print(F("Left Offset: ")); Serial.println(leftOffset);
  } else if (cmd == "R+" || cmd == "RIGHT+") {
    rightOffset++;
    Serial.print(F("Right Offset: ")); Serial.println(rightOffset);
  } else if (cmd == "R-" || cmd == "RIGHT-") {
    rightOffset--;
    Serial.print(F("Right Offset: ")); Serial.println(rightOffset);
  }
  // 2. DIRECT SERVO COMMANDS (PRIORITY 1)
  else if (cmd == "U" || cmd == "UP") {
    isStopped = false;
    calibrationMode = false;
    setSyncTarget(ARM_UP_ANGLE, 2.4);
    Serial.println(F("🙌 [SERVO]: Both Arms UP! (Position locked)"));
  }
  else if (cmd == "D" || cmd == "DOWN") {
    isStopped = false;
    calibrationMode = false;
    setSyncTarget(ARM_DOWN_ANGLE, 2.0);
    Serial.println(F("👇 [SERVO]: Both Arms DOWN! (Position locked)"));
  }
  else if (cmd == "C" || cmd == "CAL" || cmd == "CALIBRATE") {
    calibrationMode = !calibrationMode;
    isStopped = false;
    if (calibrationMode) {
      currentServoAngle = 90;
      targetServoAngle = 90;
      int leftCalib  = constrain(90 + leftOffset, 0, 180);
      int rightCalib = constrain((MIRROR_RIGHT_ARM ? (180 - 90) : 90) + rightOffset, 0, 180);
      leftServo.write(leftCalib);
      rightServo.write(rightCalib);
      Serial.println(F("🎯 [CALIBRATION MODE]: Both motors actively LOCKED at 90.0°!"));
      Serial.println(F("   (Steady 90° hold - align/push horns on now. Type 'C' to exit)"));
    } else {
      setSyncTarget(ARM_DOWN_ANGLE, 2.0);
      Serial.println(F("✅ [CALIBRATION COMPLETE]: Exited. Both arms returning DOWN."));
    }
  }
  else if (cmd == "TEST" || cmd == "SWEEP" || cmd == "CHECK") {
    isStopped = false;
    calibrationMode = false;
    Serial.println(F("\n🔬 [HARDWARE DIAGNOSTIC SWEEP]:"));
    Serial.println(F("  1. Left Arm: 25° -> 90° -> 135° -> 25°..."));
    leftServo.write(25 + leftOffset); delay(400);
    leftServo.write(90 + leftOffset); delay(400);
    leftServo.write(135 + leftOffset); delay(500);
    leftServo.write(25 + leftOffset); delay(400);

    int rDown = MIRROR_RIGHT_ARM ? (180 - 25) : 25;
    int rMid  = MIRROR_RIGHT_ARM ? (180 - 90) : 90;
    int rUp   = MIRROR_RIGHT_ARM ? (180 - 135) : 135;
    Serial.print(F("  2. Right Arm: "));
    Serial.print(rDown); Serial.print(F("° -> "));
    Serial.print(rMid);  Serial.print(F("° -> "));
    Serial.print(rUp);   Serial.print(F("° -> "));
    Serial.print(rDown); Serial.println(F("°..."));
    rightServo.write(rDown + rightOffset); delay(400);
    rightServo.write(rMid + rightOffset); delay(400);
    rightServo.write(rUp + rightOffset); delay(500);
    rightServo.write(rDown + rightOffset); delay(400);

    currentServoAngle = ARM_DOWN_ANGLE;
    targetServoAngle  = ARM_DOWN_ANGLE;
    Serial.println(F("  ✅ Test Complete! Arms resting DOWN.\n"));
  }
  else if (cmd == "M" || cmd == "MIRROR") {
    MIRROR_RIGHT_ARM = !MIRROR_RIGHT_ARM;
    Serial.print(F("🔄 Right arm mirror mode is now: "));
    Serial.println(MIRROR_RIGHT_ARM ? F("MIRRORED (Back-to-back)") : F("DIRECT (Side-by-side)"));
  }
  else if (cmd.startsWith("LEFT") || cmd.startsWith("L")) {
    int angle = extractFirstNumber(cmd);
    if (angle >= 0) {
      angle = constrain(angle, 0, 180);
      isStopped = false;
      calibrationMode = false;
      leftServo.write(angle + leftOffset);
      Serial.print(F("👈 [LEFT SERVO DIRECT]: Written to "));
      Serial.print(angle);
      Serial.println(F("°"));
    } else {
      Serial.println(F("❓ Specify angle for Left arm, e.g. 'L 90', 'L 135', 'L 25'"));
    }
  }
  else if (cmd.startsWith("RIGHT") || cmd.startsWith("R")) {
    int angle = extractFirstNumber(cmd);
    if (angle >= 0) {
      angle = constrain(angle, 0, 180);
      isStopped = false;
      calibrationMode = false;
      rightServo.write(angle + rightOffset);
      Serial.print(F("👉 [RIGHT SERVO DIRECT]: Written to "));
      Serial.print(angle);
      Serial.println(F("°"));
    } else {
      Serial.println(F("❓ Specify angle for Right arm, e.g. 'R 90', 'R 135', 'R 25'"));
    }
  }
  else if (cmd == "S" || cmd == "STOP" || cmd == "FREEZE") {
    isStopped = true;
    calibrationMode = false;
    targetServoAngle = currentServoAngle;
    Serial.println(F("🛑 [SERVO]: Motion stopped / frozen."));
  }
  // 3. OLED FACIAL EXPRESSIONS & STORY ANIMATION (MUST BE BEFORE NUMERIC MATCHING!)
  else if (cmd == "3" || cmd == "ROBOT" || cmd == "W" || cmd == "PLAY" || cmd == "RESUME") {
    currentFace = FACE_ORGANIC_ROBOT;
    currentMood = MOOD_NORMAL;
    faceStoryActive = true;
    calibrationMode = false;
    isStopped = false;
    setStoryStep(0, millis());
    setSyncTarget(ARM_DOWN_ANGLE, 2.0);
    Serial.println(F("🤖 [STORY]: Resumed Default Animation Loop!"));
  }
  else if (cmd == "1" || cmd == "CREEPER") {
    faceStoryActive = false;
    currentFace = FACE_CREEPER;
    Serial.println(F("🟩 [OLED]: Minecraft Creeper!"));
  }
  else if (cmd == "2" || cmd == "HEROBRINE") {
    faceStoryActive = false;
    currentFace = FACE_HEROBRINE;
    Serial.println(F("👻 [OLED]: Glowing Herobrine Face!"));
  }
  else if (cmd == "K" || cmd == "WINK") {
    faceStoryActive = false;
    triggerSmoothWink();
    Serial.println(F("😉 [OLED]: Smooth Animated Wink!"));
  }
  else if (cmd == "H" || cmd == "HEART" || cmd == "HEARTS") {
    faceStoryActive = false;
    currentFace = FACE_ORGANIC_ROBOT;
    currentMood = MOOD_HEART;
    setSyncTarget(ARM_UP_ANGLE, 2.4);
    Serial.println(F("💖 [OLED]: HUGE Glowing Heart Eyes! ❤️❤️"));
  }
  else if (cmd == "G" || cmd == "CHAD" || cmd == "SMILE") {
    faceStoryActive = false;
    currentFace = FACE_ORGANIC_ROBOT;
    currentMood = MOOD_GIGACHAD;
    winkProgress = 0.0;
    winkDirection = 0;
    targetPupilX = 0;
    targetPupilY = 0;
    Serial.println(F("😎 [OLED]: Confident Smile & Smirk!"));
  }
  else if (cmd == "T" || cmd == "INTRO") {
    faceStoryActive = false;
    currentFace = FACE_TEXT_INTRO;
    introStartTime = millis();
    Serial.println(F("📺 [OLED]: Batch 4 Intro Screen."));
  }
  // 4. IR SENSOR CONTROLS & LIVE DIAGNOSTIC TEST
  else if (cmd == "IRTEST" || cmd == "IRCHECK" || cmd == "IRDEBUG") {
    int val = digitalRead(IR_SENSOR_PIN);
    Serial.print(F("🔍 [IR SENSOR TEST]: Pin "));
    Serial.print(IR_SENSOR_PIN);
    Serial.print(F(" State = "));
    Serial.print(val);
    if (val == LOW) {
      Serial.println(F(" (LOW -> OBSTACLE DETECTED! Hand in front) ✋"));
    } else {
      Serial.println(F(" (HIGH -> CLEAR / No obstacle)"));
    }
    Serial.print(F("   IR Sensor Enabled: "));
    Serial.println(irSensorEnabled ? F("YES (ON)") : F("NO (OFF)"));
    Serial.println(F("   Tip: If reading doesn't change, turn the small potentiometer screw on the IR sensor module!"));
  }
  else if (cmd == "IR OFF" || cmd == "IOFF" || cmd == "IR 0" || cmd == "I0" || cmd == "OFF" || cmd == "IROFF") {
    irSensorEnabled = false;
    lastIrDetected = false;
    if (currentMood == MOOD_HEART) {
      currentFace = FACE_ORGANIC_ROBOT;
      currentMood = MOOD_NORMAL;
    }
    Serial.println(F("🚫 [IR SENSOR]: DISABLED."));
  }
  else if (cmd == "IR ON" || cmd == "ION" || cmd == "IR 1" || cmd == "I1" || cmd == "ON" || cmd == "IRON") {
    irSensorEnabled = true;
    Serial.println(F("✅ [IR SENSOR]: ENABLED! Hand detection active (Heart Eyes ❤️❤️)."));
  }
  else if (cmd == "IR") {
    irSensorEnabled = !irSensorEnabled;
    if (irSensorEnabled) {
      Serial.println(F("✅ [IR SENSOR]: ENABLED! Hand detection active (Heart Eyes ❤️❤️)."));
    } else {
      lastIrDetected = false;
      if (currentMood == MOOD_HEART) {
        currentFace = FACE_ORGANIC_ROBOT;
        currentMood = MOOD_NORMAL;
      }
      Serial.println(F("🚫 [IR SENSOR]: DISABLED."));
    }
  }
  // 5. NUMERIC ANGLE FOR BOTH ARMS (0 - 180) (MUST BE AFTER 1, 2, 3!)
  else if (extractFirstNumber(cmd) >= 0) {
    int angle = constrain(extractFirstNumber(cmd), 0, 180);
    isStopped = false;
    calibrationMode = false;
    setSyncTarget(angle, 2.4);
    Serial.print(F("🙌 [SERVO]: Both arms moved to: "));
    Serial.print(angle);
    Serial.println(F("°"));
  }
  else {
    Serial.println(F("❓ Unknown command. Type '?' for menu."));
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

  // Handle "No line ending" in Arduino Serial Monitor after 60ms pause
  if (serialCmdBuffer.length() > 0 && (millis() - lastSerialCharTime > 60)) {
    processSerialCommand(serialCmdBuffer);
    serialCmdBuffer = "";
  }
}

// ----------------------------------------------------------------------------
// 11. SETUP & MAIN LOOP
// ----------------------------------------------------------------------------

void setup() {
  pinMode(LED_PIN, OUTPUT);
  // 3 quick blinks on boot (visual indicator if Arduino restarts due to power brownout)
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH); delay(60);
    digitalWrite(LED_PIN, LOW);  delay(60);
  }

  Serial.begin(9600); // Standard 9600 baud (matches Arduino IDE default!)
  Serial.setTimeout(50);
  Serial.println(F("\n========================================================"));
  Serial.println(F("   🤖 WOODY ROBOT READY (9600 BAUD) | Type '?' for menu"));
  Serial.println(F("========================================================\n"));

  // 1. Initialize OLED Display (Fast 400kHz I2C)
  initSH1106();

  // 2. Initialize Servos (Staggered boot to avoid power drop!)
  currentServoAngle = ARM_DOWN_ANGLE;
  targetServoAngle  = ARM_DOWN_ANGLE;

  leftServo.attach(LEFT_SERVO_PIN);
  rightServo.attach(RIGHT_SERVO_PIN);

  int initLeft  = constrain(ARM_DOWN_ANGLE + leftOffset, 0, 180);
  int initRight = constrain((MIRROR_RIGHT_ARM ? (180 - ARM_DOWN_ANGLE) : ARM_DOWN_ANGLE) + rightOffset, 0, 180);
  leftServo.write(initLeft);
  delay(150); // Let Left servo settle before powering Right servo
  rightServo.write(initRight);
  delay(150); // Let Right servo settle

  // 3. Initialize IR Sensor (Pin 2 = Signal, Pin 3 = GND, Pin 4 = 5V VCC)
  pinMode(IR_GND_PIN, OUTPUT);
  digitalWrite(IR_GND_PIN, LOW);    // Pure 0V Ground for direct-plug sensor
  pinMode(IR_POWER_PIN, OUTPUT);
  digitalWrite(IR_POWER_PIN, HIGH); // Pure 5V VCC Power for direct-plug sensor
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);

  // 4. Start directly in the Default Cute Animation Sequence!
  faceStoryActive = true;
  setStoryStep(0, millis());
  lastBlinkTime = millis();
  renderFastScreen();
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
    } else {
      renderFastScreen();
    }
  }
}
