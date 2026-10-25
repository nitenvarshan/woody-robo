/*
 * ============================================================================
 *   MINECRAFT CREEPER ROBOT (1.3" SH1106 OLED + DUAL SERVO CONTROL!)
 *   Interactive Robot: Auto-Dance + Manual Keyboard Control via Serial
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
#include <Adafruit_SH110X.h>
#include <Servo.h>

// ----------------------------------------------------------------------------
// 1. PIN & DISPLAY CONFIGURATION (1.3" SH1106)
// ----------------------------------------------------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

#define LEFT_SERVO_PIN  9   // Left Arm Servo
#define RIGHT_SERVO_PIN 10  // Right Arm Servo
#define LED_PIN         13  // Built-in "L" LED

// 1.3" OLED Driver (Clears the dot-dot static!)
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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
float armSpeed           = 1.8;

const bool MIRROR_RIGHT_ARM = true;
bool autoMode = true; // Set to false when user types manual angles

// ----------------------------------------------------------------------------
// 3. ANIMATION & EMOTIONS
// ----------------------------------------------------------------------------
enum RobotEmotion {
  EMO_CREEPER_NORMAL,
  EMO_CREEPER_HAPPY,
  EMO_CREEPER_SURPRISED
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
void drawCreeperFace(int centerX, int centerY, int blockSize, int eyeShiftX, int eyeShiftY) {
  int totalSize = blockSize * 8;
  int startX = centerX - totalSize / 2;
  int startY = centerY - totalSize / 2;

  // 1. Outer Glowing Head
  display.fillRect(startX, startY, totalSize, totalSize, SH110X_WHITE);

  // 2. Pixelated Skin Texture Dots
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      if ((x + y) % 3 == 0) {
        display.drawRect(startX + x * blockSize, startY + y * blockSize, blockSize, blockSize, SH110X_BLACK);
      }
    }
  }

  // 3. Eyes (Blinking vs Open)
  if (isBlinking) {
    display.fillRect(startX + 1 * blockSize, startY + 1 * blockSize + blockSize / 2, 2 * blockSize, 3, SH110X_BLACK);
    display.fillRect(startX + 5 * blockSize, startY + 1 * blockSize + blockSize / 2, 2 * blockSize, 3, SH110X_BLACK);
  } else {
    // Left Eye (2x2 blocks at cell 1,1)
    display.fillRect(startX + (1 * blockSize) + eyeShiftX, startY + (1 * blockSize) + eyeShiftY, 2 * blockSize, 2 * blockSize, SH110X_BLACK);
    // Right Eye (2x2 blocks at cell 5,1)
    display.fillRect(startX + (5 * blockSize) + eyeShiftX, startY + (1 * blockSize) + eyeShiftY, 2 * blockSize, 2 * blockSize, SH110X_BLACK);
  }

  // 4. Nose Bridge (2x1 blocks at cell 3,3)
  display.fillRect(startX + 3 * blockSize, startY + 3 * blockSize, 2 * blockSize, 1 * blockSize, SH110X_BLACK);

  // 5. Upper Mouth (4x2 blocks at cell 2,4)
  display.fillRect(startX + 2 * blockSize, startY + 4 * blockSize, 4 * blockSize, 2 * blockSize, SH110X_BLACK);

  // 6. Left Mouth Fang (1x2 blocks at cell 2,6)
  display.fillRect(startX + 2 * blockSize, startY + 6 * blockSize, 1 * blockSize, 2 * blockSize, SH110X_BLACK);

  // 7. Right Mouth Fang (1x2 blocks at cell 5,6)
  display.fillRect(startX + 5 * blockSize, startY + 6 * blockSize, 1 * blockSize, 2 * blockSize, SH110X_BLACK);
}

void renderScreen() {
  if (!oledFound) return;

  display.clearDisplay();

  // Draw 56x56 pixel Creeper Face in the center!
  drawCreeperFace(64, 32, 7, gazeOffsetX, gazeOffsetY);

  // Side text banners
  display.setTextColor(SH110X_WHITE);
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
  if (!autoMode) return; // If user is manually controlling, don't auto-override

  unsigned long now = millis();

  if (now - lastActionTime > 4000) {
    lastActionTime = now;
    currentRoutine = (currentRoutine + 1) % 4;

    switch (currentRoutine) {
      // 0: Arms Resting Down
      case 0:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.2;
        currentEmotion = EMO_CREEPER_NORMAL;
        gazeOffsetX = 0;
        gazeOffsetY = 0;
        break;

      // 1: Both arms swing UP together! 🙌
      case 1:
        targetServoAngle = ARM_UP_ANGLE;
        armSpeed = 2.0;
        currentEmotion = EMO_CREEPER_HAPPY;
        gazeOffsetX = 0;
        gazeOffsetY = -1;
        break;

      // 2: Arms hold halfway, Creeper looks to the LEFT! 😲
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
// 6. KEYBOARD SERIAL COMMANDS (Control Arms from Laptop!)
// ----------------------------------------------------------------------------
void checkSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "U") {
      autoMode = false;
      targetServoAngle = ARM_UP_ANGLE;
      currentEmotion = EMO_CREEPER_HAPPY;
      Serial.println(F("[COMMAND] Arms UP! 🙌"));
    } else if (cmd == "D") {
      autoMode = false;
      targetServoAngle = ARM_DOWN_ANGLE;
      currentEmotion = EMO_CREEPER_NORMAL;
      Serial.println(F("[COMMAND] Arms DOWN! 👇"));
    } else if (cmd == "W") {
      autoMode = true;
      Serial.println(F("[COMMAND] Auto Dance / Wave Mode ON! 👋"));
    } else if (cmd.toInt() > 0 || cmd == "0") {
      autoMode = false;
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
  Serial.println(F("   Commands: 'U' = Arms Up, 'D' = Down"));
  Serial.println(F("             'W' = Auto Wave, or type angle (0-180)"));
  Serial.println(F("========================================\n"));

  Wire.begin();

  // Initialize 1.3" SH1106 Display (Address 0x3C is standard)
  if (display.begin(0x3C, true)) {
    oledFound = true;
    display.clearDisplay();
    display.display();
    Serial.println(F("1.3\" SH1106 OLED Initialized Successfully!"));
  } else {
    Serial.println(F("OLED not detected at 0x3C. Trying 0x3D..."));
    if (display.begin(0x3D, true)) {
      oledFound = true;
      display.clearDisplay();
      display.display();
      Serial.println(F("1.3\" SH1106 OLED Initialized at 0x3D!"));
    }
  }

  // Startup Splash
  if (oledFound) {
    display.clearDisplay();
    display.drawRoundRect(2, 2, 124, 60, 4, SH110X_WHITE);
    drawCreeperFace(40, 32, 6, 0, 0);

    display.setTextColor(SH110X_WHITE);
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
  // 1. Heartbeat LED
  digitalWrite(LED_PIN, (millis() / 500) % 2);

  // 2. Check Keyboard / Serial commands from user
  checkSerialCommands();

  // 3. Update Robot Motion
  updateArms();
  updateBlinking();
  updateRobotBehavior();

  // 4. Render Screen
  if (millis() - lastScreenUpdateTime >= 33) {
    lastScreenUpdateTime = millis();
    renderScreen();
  }
}
