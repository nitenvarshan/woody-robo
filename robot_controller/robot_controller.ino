/*
 * ============================================================================
 *   MINECRAFT CREEPER ROBOT COMPANION! 🟩💥
 *   Dual-Arm Synchronized Servos + Expressive Creeper OLED Face
 * ============================================================================
 *   Board: Arduino Uno R3 (Direct Pin Connections - No Breadboard Needed!)
 *   Display: 1.3" or 0.96" I2C OLED (128x64)
 *   Actuators: 
 *     - Left Arm Servo:  Pin 9
 *     - Right Arm Servo: Pin 10
 * ============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

// ----------------------------------------------------------------------------
// 1. PIN & DISPLAY CONFIGURATION
// ----------------------------------------------------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

#define LEFT_SERVO_PIN  9   // Left Arm Servo
#define RIGHT_SERVO_PIN 10  // Right Arm Servo
#define LED_PIN         13  // Built-in "L" LED

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Servo leftServo;
Servo rightServo;

bool oledFound = false;

// ----------------------------------------------------------------------------
// 2. DUAL ARM SETTINGS
// ----------------------------------------------------------------------------
const int ARM_DOWN_ANGLE = 20;   // Rest position down
const int ARM_UP_ANGLE   = 130;  // Raised up in the air!

float currentServoAngle  = ARM_DOWN_ANGLE;
float targetServoAngle   = ARM_DOWN_ANGLE;
float armSpeed           = 1.5;

const bool MIRROR_RIGHT_ARM = true;

// ----------------------------------------------------------------------------
// 3. ANIMATION & EMOTIONS
// ----------------------------------------------------------------------------
enum RobotEmotion {
  EMO_CREEPER_NORMAL,
  EMO_CREEPER_HAPPY,
  EMO_CREEPER_SURPRISED,
  EMO_CREEPER_BLINK
};

RobotEmotion currentEmotion = EMO_CREEPER_NORMAL;

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
// 4. DRAWING THE MINECRAFT CREEPER FACE
// ----------------------------------------------------------------------------

// Draws the iconic 8x8 Minecraft Creeper Face, perfectly scaled!
void drawCreeperFace(int centerX, int centerY, int blockSize, bool inverted, int eyeShiftX, int eyeShiftY) {
  int totalSize = blockSize * 8;
  int startX = centerX - totalSize / 2;
  int startY = centerY - totalSize / 2;

  uint16_t skinColor = inverted ? SSD1306_BLACK : SSD1306_WHITE;
  uint16_t featureColor = inverted ? SSD1306_WHITE : SSD1306_BLACK;

  // 1. Draw Creeper Head Block
  display.fillRect(startX, startY, totalSize, totalSize, skinColor);

  // 2. Pixelated Skin Texture Dots (gives it that authentic Minecraft pixel look!)
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      if ((x + y) % 3 == 0) {
        display.drawRect(startX + x * blockSize, startY + y * blockSize, blockSize, blockSize, featureColor);
      }
    }
  }

  // If Blinking, close the eyes into a horizontal slit
  if (isBlinking) {
    display.fillRect(startX + 1 * blockSize, startY + 1 * blockSize + blockSize / 2, 2 * blockSize, 3, featureColor);
    display.fillRect(startX + 5 * blockSize, startY + 1 * blockSize + blockSize / 2, 2 * blockSize, 3, featureColor);
  } else {
    // 3. Left Eye (2x2 blocks at cell 1,1)
    display.fillRect(startX + (1 * blockSize) + eyeShiftX, startY + (1 * blockSize) + eyeShiftY, 2 * blockSize, 2 * blockSize, featureColor);

    // 4. Right Eye (2x2 blocks at cell 5,1)
    display.fillRect(startX + (5 * blockSize) + eyeShiftX, startY + (1 * blockSize) + eyeShiftY, 2 * blockSize, 2 * blockSize, featureColor);
  }

  // 5. Nose Bridge (2x1 blocks at cell 3,3)
  display.fillRect(startX + 3 * blockSize, startY + 3 * blockSize, 2 * blockSize, 1 * blockSize, featureColor);

  // 6. Upper Mouth (4x2 blocks at cell 2,4)
  display.fillRect(startX + 2 * blockSize, startY + 4 * blockSize, 4 * blockSize, 2 * blockSize, featureColor);

  // 7. Left Mouth Fang (1x2 blocks at cell 2,6)
  display.fillRect(startX + 2 * blockSize, startY + 6 * blockSize, 1 * blockSize, 2 * blockSize, featureColor);

  // 8. Right Mouth Fang (1x2 blocks at cell 5,6)
  display.fillRect(startX + 5 * blockSize, startY + 6 * blockSize, 1 * blockSize, 2 * blockSize, featureColor);
}

// Full screen Creeper layout with cool side banners
void renderScreen() {
  if (!oledFound) return;

  display.clearDisplay();

  // Draw 56x56 pixel authentic Minecraft Creeper Face in the center!
  drawCreeperFace(64, 32, 7, false, gazeOffsetX, gazeOffsetY);

  // Side text banners
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  if (currentEmotion == EMO_CREEPER_HAPPY) {
    display.setCursor(4, 22);
    display.println(F("HI!"));
    display.setCursor(4, 34);
    display.println(F("FRIEND"));

    display.setCursor(96, 22);
    display.println(F("ARMS"));
    display.setCursor(96, 34);
    display.println(F("UP! ^_^"));
  } else if (currentEmotion == EMO_CREEPER_SURPRISED) {
    display.setCursor(4, 28);
    display.println(F("WHAT?"));

    display.setCursor(96, 28);
    display.println(F("BOOM!"));
  } else {
    display.setCursor(4, 28);
    display.println(F("CREEPER"));

    display.setCursor(98, 28);
    display.println(F("BOT 🤖"));
  }

  display.display();
}

// ----------------------------------------------------------------------------
// 5. UPDATE FUNCTIONS
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
  unsigned long now = millis();

  if (now - lastActionTime > 4000) {
    lastActionTime = now;
    currentRoutine = (currentRoutine + 1) % 4;

    switch (currentRoutine) {
      // 0: Arms Resting Down + Creeper looking straight ahead
      case 0:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.2;
        currentEmotion = EMO_CREEPER_NORMAL;
        gazeOffsetX = 0;
        gazeOffsetY = 0;
        break;

      // 1: Both arms swing UP together! Creeper is HAPPY!
      case 1:
        targetServoAngle = ARM_UP_ANGLE;
        armSpeed = 2.0;
        currentEmotion = EMO_CREEPER_HAPPY;
        gazeOffsetX = 0;
        gazeOffsetY = -1;
        break;

      // 2: Arms hold halfway, Creeper looks to the LEFT!
      case 2:
        targetServoAngle = 75;
        armSpeed = 2.8;
        currentEmotion = EMO_CREEPER_SURPRISED;
        gazeOffsetX = -2;
        gazeOffsetY = 0;
        break;

      // 3: Creeper looks to the RIGHT & lowers arms gently
      case 3:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.5;
        currentEmotion = EMO_CREEPER_NORMAL;
        gazeOffsetX = 2;
        gazeOffsetY = 1;
        break;
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
  Serial.println(F("Starting Minecraft Creeper Robot..."));

  Wire.begin();
  #if defined(WIRE_HAS_TIMEOUT)
    Wire.setWireTimeout(3000, true);
  #endif

  // Try address 0x3C first (most common), then 0x3D
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledFound = true;
    Serial.println(F("OLED found at 0x3C!"));
  } else if (display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    oledFound = true;
    Serial.println(F("OLED found at 0x3D!"));
  } else {
    Serial.println(F("WARNING: OLED not detected yet. Check SDA/SCL wires!"));
  }

  // Startup Splash: Show Welcome Creeper!
  if (oledFound) {
    display.clearDisplay();
    display.drawRoundRect(2, 2, 124, 60, 4, SSD1306_WHITE);
    drawCreeperFace(40, 32, 6, false, 0, 0);

    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(72, 16);
    display.println(F("HI!"));

    display.setTextSize(1);
    display.setCursor(72, 38);
    display.println(F("WELCOME"));

    display.display();
    delay(2000);
  }

  // Attach Both Servos
  leftServo.attach(LEFT_SERVO_PIN);
  rightServo.attach(RIGHT_SERVO_PIN);

  leftServo.write(ARM_DOWN_ANGLE);
  rightServo.write(MIRROR_RIGHT_ARM ? (180 - ARM_DOWN_ANGLE) : ARM_DOWN_ANGLE);
  delay(300);

  lastBlinkTime = millis();
  lastActionTime = millis();
}

void loop() {
  // 1. Heartbeat indicator: Blinks the "L" LED smoothly every 1 second so you KNOW it's running!
  digitalWrite(LED_PIN, (millis() / 500) % 2);

  // 2. Continuous background robot tasks
  updateArms();          // Synchronized arm swing
  updateBlinking();      // Creeper eye blinking
  updateRobotBehavior(); // Moods & routines

  // 3. Render Creeper Face at ~30 FPS
  if (millis() - lastScreenUpdateTime >= 33) {
    lastScreenUpdateTime = millis();
    renderScreen();
  }
}
