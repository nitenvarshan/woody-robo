/*
 * ============================================================================
 *   SUPER COOL WOODEN ROBOT COMPANION (DUAL SERVO VERSION!)
 *   Dual-Arm Synchronized Robot + Expressive OLED Face
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
// 1. PIN & DISPLAY SETUP
// ----------------------------------------------------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// Most OLEDs use 0x3C
#define OLED_I2C_ADDRESS 0x3C

#define LEFT_SERVO_PIN  9   // Servo 1 (Left Arm)
#define RIGHT_SERVO_PIN 10  // Servo 2 (Right Arm)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Servo leftServo;
Servo rightServo;

// ----------------------------------------------------------------------------
// 2. DUAL ARM MOVEMENT SETTINGS (Angles in Degrees)
// ----------------------------------------------------------------------------
const int ARM_DOWN_ANGLE = 20;   // Rest position down
const int ARM_UP_ANGLE   = 130;  // Raised up position (hands in the air!)

float currentServoAngle  = ARM_DOWN_ANGLE;
float targetServoAngle   = ARM_DOWN_ANGLE;
float armSpeed           = 1.5;  // Smooth gliding speed

// Set to 'true' if servos are mounted facing opposite directions (standard)
// This makes BOTH arms raise UP together at the same time!
const bool MIRROR_RIGHT_ARM = true;

// ----------------------------------------------------------------------------
// 3. EYE ANIMATIONS & EMOTIONS
// ----------------------------------------------------------------------------
enum RobotEmotion {
  EMO_NORMAL,
  EMO_HAPPY,
  EMO_SURPRISED,
  EMO_BLINK
};

RobotEmotion currentEmotion = EMO_NORMAL;

// Eye dimensions on the 128x64 screen
const int EYE_WIDTH       = 32;
const int EYE_HEIGHT      = 38;
const int EYE_CORNER_RAD  = 8;
const int LEFT_EYE_X      = 22;
const int RIGHT_EYE_X     = 74;
const int EYE_Y           = 13;

int gazeOffsetX = 0;
int gazeOffsetY = 0;

bool isBlinking = false;
int blinkProgress = 0;
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkInterval = 3000;

// Timing clocks (non-blocking millis)
unsigned long lastArmUpdateTime = 0;
unsigned long lastScreenUpdateTime = 0;
unsigned long lastActionTime = 0;

int currentRoutine = 0;

// ----------------------------------------------------------------------------
// 4. DRAWING THE FACES ON THE OLED SCREEN
// ----------------------------------------------------------------------------
void drawFace() {
  display.clearDisplay();

  // If blinking, shrink the eye height down to a thin cute slit
  if (isBlinking) {
    int eyeH = map(blinkProgress, 0, 100, EYE_HEIGHT, 4);
    int eyeY = EYE_Y + (EYE_HEIGHT - eyeH) / 2;
    display.fillRoundRect(LEFT_EYE_X + gazeOffsetX, eyeY, EYE_WIDTH, eyeH, 3, SSD1306_WHITE);
    display.fillRoundRect(RIGHT_EYE_X + gazeOffsetX, eyeY, EYE_WIDTH, eyeH, 3, SSD1306_WHITE);
    display.display();
    return;
  }

  // --- HAPPY FACE: Cute upward smiling eyes! ---
  if (currentEmotion == EMO_HAPPY) {
    display.fillRoundRect(LEFT_EYE_X, EYE_Y, EYE_WIDTH, EYE_HEIGHT, EYE_CORNER_RAD, SSD1306_WHITE);
    display.fillCircle(LEFT_EYE_X + EYE_WIDTH / 2, EYE_Y + EYE_HEIGHT + 6, EYE_WIDTH / 1.5, SSD1306_BLACK);

    display.fillRoundRect(RIGHT_EYE_X, EYE_Y, EYE_WIDTH, EYE_HEIGHT, EYE_CORNER_RAD, SSD1306_WHITE);
    display.fillCircle(RIGHT_EYE_X + EYE_WIDTH / 2, EYE_Y + EYE_HEIGHT + 6, EYE_WIDTH / 1.5, SSD1306_BLACK);

    // Little cute smile
    display.drawCircle(64, 48, 8, SSD1306_WHITE);
    display.fillRect(54, 40, 20, 8, SSD1306_BLACK);
  }
  // --- SURPRISED FACE: Wide round eyes with pupils! ---
  else if (currentEmotion == EMO_SURPRISED) {
    display.fillCircle(LEFT_EYE_X + EYE_WIDTH / 2, EYE_Y + EYE_HEIGHT / 2, 20, SSD1306_WHITE);
    display.fillCircle(RIGHT_EYE_X + EYE_WIDTH / 2, EYE_Y + EYE_HEIGHT / 2, 20, SSD1306_WHITE);

    display.fillCircle(LEFT_EYE_X + EYE_WIDTH / 2 + gazeOffsetX, EYE_Y + EYE_HEIGHT / 2 + gazeOffsetY, 7, SSD1306_BLACK);
    display.fillCircle(RIGHT_EYE_X + EYE_WIDTH / 2 + gazeOffsetX, EYE_Y + EYE_HEIGHT / 2 + gazeOffsetY, 7, SSD1306_BLACK);

    display.drawCircle(64, 52, 4, SSD1306_WHITE);
  }
  // --- NORMAL / IDLE FACE: Classic adorable robot eyes ---
  else {
    display.fillRoundRect(LEFT_EYE_X + gazeOffsetX, EYE_Y + gazeOffsetY, EYE_WIDTH, EYE_HEIGHT, EYE_CORNER_RAD, SSD1306_WHITE);
    display.fillRoundRect(RIGHT_EYE_X + gazeOffsetX, EYE_Y + gazeOffsetY, EYE_WIDTH, EYE_HEIGHT, EYE_CORNER_RAD, SSD1306_WHITE);

    // Shiny light reflection dot in eyes
    display.fillCircle(LEFT_EYE_X + gazeOffsetX + 6, EYE_Y + gazeOffsetY + 7, 3, SSD1306_BLACK);
    display.fillCircle(RIGHT_EYE_X + gazeOffsetX + 6, EYE_Y + gazeOffsetY + 7, 3, SSD1306_BLACK);
  }

  display.display();
}

// ----------------------------------------------------------------------------
// 5. UPDATE FUNCTIONS (Smooth Movement)
// ----------------------------------------------------------------------------

// Moves BOTH servos smoothly toward target angle
void updateArms() {
  if (millis() - lastArmUpdateTime >= 20) { // 50Hz smooth updates
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
      // Mirror the right arm so both raise UP together!
      int rightAngle = MIRROR_RIGHT_ARM ? (180 - leftAngle) : leftAngle;

      leftServo.write(leftAngle);
      rightServo.write(rightAngle);
    }
  }
}

// Natural eye blinking
void updateBlinking() {
  unsigned long now = millis();

  if (!isBlinking && (now - lastBlinkTime > nextBlinkInterval)) {
    isBlinking = true;
    blinkProgress = 0;
  }

  if (isBlinking) {
    blinkProgress += 20;
    if (blinkProgress >= 100) {
      isBlinking = false;
      lastBlinkTime = now;
      nextBlinkInterval = random(2500, 5000);
    }
  }
}

// High-level synchronized robot behavior
void updateRobotBehavior() {
  unsigned long now = millis();

  if (now - lastActionTime > 4000) {
    lastActionTime = now;
    currentRoutine = (currentRoutine + 1) % 4;

    switch (currentRoutine) {
      // 0: Arms Resting Down + Robot looking straight ahead (Normal)
      case 0:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.2;
        currentEmotion = EMO_NORMAL;
        gazeOffsetX = 0;
        gazeOffsetY = 0;
        break;

      // 1: Robot raises BOTH arms in the air! Face turns Super HAPPY! 🙌
      case 1:
        targetServoAngle = ARM_UP_ANGLE;
        armSpeed = 2.0;
        currentEmotion = EMO_HAPPY;
        gazeOffsetX = 0;
        gazeOffsetY = -2; // Looking up proudly!
        break;

      // 2: Robot swings arms halfway & looks surprised to the LEFT! 😲
      case 2:
        targetServoAngle = 75;
        armSpeed = 2.8;
        currentEmotion = EMO_SURPRISED;
        gazeOffsetX = -6;
        gazeOffsetY = 0;
        break;

      // 3: Robot looks to the RIGHT & lowers arms gently
      case 3:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.5;
        currentEmotion = EMO_NORMAL;
        gazeOffsetX = 6;
        gazeOffsetY = 2;
        break;
    }
  }
}

// ----------------------------------------------------------------------------
// 6. SETUP & MAIN LOOP
// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting Dual-Servo Wooden Robot..."));

  // 1. Start the OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("OLED not found! Check SDA/SCL wires."));
    for (;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(20, 28);
  display.println(F("HELLO FRIEND!"));
  display.display();
  delay(1200);

  // 2. Attach Both Servos
  leftServo.attach(LEFT_SERVO_PIN);
  rightServo.attach(RIGHT_SERVO_PIN);

  leftServo.write(ARM_DOWN_ANGLE);
  rightServo.write(MIRROR_RIGHT_ARM ? (180 - ARM_DOWN_ANGLE) : ARM_DOWN_ANGLE);
  delay(300);

  lastBlinkTime = millis();
  lastActionTime = millis();
}

void loop() {
  updateArms();          // 1. Smoothly glide BOTH synchronized arms
  updateBlinking();      // 2. Natural eye blinking
  updateRobotBehavior(); // 3. Mood and arm choreographies

  // Draw face at ~30 FPS
  if (millis() - lastScreenUpdateTime >= 33) {
    lastScreenUpdateTime = millis();
    drawFace();
  }
}
