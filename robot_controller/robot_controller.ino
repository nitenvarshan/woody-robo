/*
 * ============================================================================
 *   SUPER COOL WOODEN ROBOT COMPANION
 *   Dual-Arm Synchronized Swing + Animated Expressive OLED Face
 * ============================================================================
 *   Board: Arduino Uno R3 (or ESP32 / compatible)
 *   Display: 0.96" (or 1.3") I2C OLED (128x64 SSD1306)
 *   Actuator: 1x RC Servo Motor (Pin 9) driving both arms together via cross-bar!
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

// Most 0.96" OLEDs use 0x3C. If yours doesn't turn on, try 0x3D!
#define OLED_I2C_ADDRESS 0x3C

#define SERVO_PIN 9 // Servo signal wire (Yellow or Orange) goes to Pin 9

// Create display and servo objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Servo armServo;

// ----------------------------------------------------------------------------
// 2. ARM MOVEMENT SETTINGS (Angles in Degrees)
// ----------------------------------------------------------------------------
const int ARM_DOWN_ANGLE = 25;   // Resting down position
const int ARM_UP_ANGLE   = 125;  // Raised up position

float currentServoAngle  = ARM_DOWN_ANGLE;
float targetServoAngle   = ARM_DOWN_ANGLE;
float armSpeed           = 1.5;  // How fast the arm glides (smooth steps)

// ----------------------------------------------------------------------------
// 3. EYE ANIMATION & EMOTION SETTINGS
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
const int EYE_CORNER_RAD  = 8;  // Rounded corner radius
const int LEFT_EYE_X      = 22; // Center X for left eye
const int RIGHT_EYE_X     = 74; // Center X for right eye
const int EYE_Y           = 13; // Top Y position

// Gaze direction offsets (in pixels)
int gazeOffsetX = 0;
int gazeOffsetY = 0;

// Eyelid blink state
bool isBlinking = false;
int blinkProgress = 0; // 0 = fully open, 100 = fully closed
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkInterval = 3000;

// Timing clocks (non-blocking millis)
unsigned long lastArmUpdateTime = 0;
unsigned long lastScreenUpdateTime = 0;
unsigned long lastActionTime = 0;

int currentRoutine = 0; // Tracks what the robot is currently doing

// ----------------------------------------------------------------------------
// 4. DRAWING THE FACES ON THE OLED SCREEN
// ----------------------------------------------------------------------------

// Helper to draw one rounded cartoon eye
void drawOneEye(int x, int y, int width, int height, int radius) {
  display.fillRoundRect(x, y, width, height, radius, SSD1306_WHITE);
}

// Draw the entire face based on mood, gaze, and blinking!
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

  // --- HAPPY FACE: Cute upward smiling crescent eyes! ---
  if (currentEmotion == EMO_HAPPY) {
    // Left Happy Eye (inverted arc / triangle cut out)
    display.fillRoundRect(LEFT_EYE_X, EYE_Y, EYE_WIDTH, EYE_HEIGHT, EYE_CORNER_RAD, SSD1306_WHITE);
    display.fillCircle(LEFT_EYE_X + EYE_WIDTH / 2, EYE_Y + EYE_HEIGHT + 6, EYE_WIDTH / 1.5, SSD1306_BLACK);

    // Right Happy Eye
    display.fillRoundRect(RIGHT_EYE_X, EYE_Y, EYE_WIDTH, EYE_HEIGHT, EYE_CORNER_RAD, SSD1306_WHITE);
    display.fillCircle(RIGHT_EYE_X + EYE_WIDTH / 2, EYE_Y + EYE_HEIGHT + 6, EYE_WIDTH / 1.5, SSD1306_BLACK);

    // Little cute smile in the center
    display.drawCircle(64, 48, 8, SSD1306_WHITE);
    display.fillRect(54, 40, 20, 8, SSD1306_BLACK); // Keep bottom half of circle
  }
  // --- SURPRISED FACE: Big round wide-open eyes with pupils! ---
  else if (currentEmotion == EMO_SURPRISED) {
    display.fillCircle(LEFT_EYE_X + EYE_WIDTH / 2, EYE_Y + EYE_HEIGHT / 2, 20, SSD1306_WHITE);
    display.fillCircle(RIGHT_EYE_X + EYE_WIDTH / 2, EYE_Y + EYE_HEIGHT / 2, 20, SSD1306_WHITE);

    // Black pupils in the center
    display.fillCircle(LEFT_EYE_X + EYE_WIDTH / 2 + gazeOffsetX, EYE_Y + EYE_HEIGHT / 2 + gazeOffsetY, 7, SSD1306_BLACK);
    display.fillCircle(RIGHT_EYE_X + EYE_WIDTH / 2 + gazeOffsetX, EYE_Y + EYE_HEIGHT / 2 + gazeOffsetY, 7, SSD1306_BLACK);

    // Little "o" mouth
    display.drawCircle(64, 52, 4, SSD1306_WHITE);
  }
  // --- NORMAL / IDLE FACE: Classic adorable modern robot eyes ---
  else {
    // Outer white eye box
    display.fillRoundRect(LEFT_EYE_X + gazeOffsetX, EYE_Y + gazeOffsetY, EYE_WIDTH, EYE_HEIGHT, EYE_CORNER_RAD, SSD1306_WHITE);
    display.fillRoundRect(RIGHT_EYE_X + gazeOffsetX, EYE_Y + gazeOffsetY, EYE_WIDTH, EYE_HEIGHT, EYE_CORNER_RAD, SSD1306_WHITE);

    // Little shiny reflective highlight dot in top-right corner of each eye
    display.fillCircle(LEFT_EYE_X + gazeOffsetX + 6, EYE_Y + gazeOffsetY + 7, 3, SSD1306_BLACK);
    display.fillCircle(RIGHT_EYE_X + gazeOffsetX + 6, EYE_Y + gazeOffsetY + 7, 3, SSD1306_BLACK);
  }

  display.display();
}

// ----------------------------------------------------------------------------
// 5. UPDATE FUNCTIONS (Running in background smoothly)
// ----------------------------------------------------------------------------

// Smoothly moves the servo toward the target angle without jerking
void updateArm() {
  if (millis() - lastArmUpdateTime >= 20) { // Update every 20 milliseconds (50Hz)
    lastArmUpdateTime = millis();

    if (abs(currentServoAngle - targetServoAngle) > 0.5) {
      if (currentServoAngle < targetServoAngle) {
        currentServoAngle += armSpeed;
        if (currentServoAngle > targetServoAngle) currentServoAngle = targetServoAngle;
      } else {
        currentServoAngle -= armSpeed;
        if (currentServoAngle < targetServoAngle) currentServoAngle = targetServoAngle;
      }
      armServo.write((int)currentServoAngle);
    }
  }
}

// Handles natural blinking every 3 to 5 seconds
void updateBlinking() {
  unsigned long now = millis();

  if (!isBlinking && (now - lastBlinkTime > nextBlinkInterval)) {
    isBlinking = true;
    blinkProgress = 0;
  }

  if (isBlinking) {
    blinkProgress += 20; // Blinks quickly in 5 steps (fast & natural!)
    if (blinkProgress >= 100) {
      isBlinking = false;
      lastBlinkTime = now;
      nextBlinkInterval = random(2500, 5000); // Random wait until next blink
    }
  }
}

// ----------------------------------------------------------------------------
// 6. ROBOT BRAIN (Fun Choreographed Routines!)
// ----------------------------------------------------------------------------
void updateRobotBehavior() {
  unsigned long now = millis();

  // Switch between actions every few seconds
  if (now - lastActionTime > 4000) {
    lastActionTime = now;
    currentRoutine = (currentRoutine + 1) % 4; // Cycle through 4 cool states

    switch (currentRoutine) {
      // Routine 0: Arms Resting Down + Robot looking straight ahead (Normal)
      case 0:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.2;
        currentEmotion = EMO_NORMAL;
        gazeOffsetX = 0;
        gazeOffsetY = 0;
        break;

      // Routine 1: Robot raises both arms high! Face turns Super HAPPY!
      case 1:
        targetServoAngle = ARM_UP_ANGLE;
        armSpeed = 2.0;
        currentEmotion = EMO_HAPPY;
        gazeOffsetX = 0;
        gazeOffsetY = -2; // Looking up proudly!
        break;

      // Routine 2: Robot swings arms halfway & looks surprised to the LEFT!
      case 2:
        targetServoAngle = 75;
        armSpeed = 3.0; // Quick motion
        currentEmotion = EMO_SURPRISED;
        gazeOffsetX = -6; // Look left
        gazeOffsetY = 0;
        break;

      // Routine 3: Robot looks to the RIGHT & wiggles arms back to resting
      case 3:
        targetServoAngle = ARM_DOWN_ANGLE;
        armSpeed = 1.5;
        currentEmotion = EMO_NORMAL;
        gazeOffsetX = 6; // Look right
        gazeOffsetY = 2;
        break;
    }
  }
}

// ----------------------------------------------------------------------------
// 7. SETUP & MAIN LOOP
// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting Animated Wooden Robot..."));

  // 1. Start the OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("OLED not found! Check SDA/SCL wires."));
    for (;;); // Halt if screen is not connected
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(20, 28);
  display.println(F("HELLO FRIEND!"));
  display.display();
  delay(1200); // Quick friendly boot splash

  // 2. Attach Servo to Pin 9 and gently set to starting position
  armServo.attach(SERVO_PIN);
  armServo.write(ARM_DOWN_ANGLE);
  delay(300);

  lastBlinkTime = millis();
  lastActionTime = millis();
}

void loop() {
  // All three tasks run simultaneously without stopping or freezing!
  updateArm();           // 1. Smoothly glide the synchronized arms
  updateBlinking();      // 2. Natural eye blinking
  updateRobotBehavior(); // 3. High-level mood and arm choreographies

  // Redraw face at ~30 FPS (every 33 milliseconds)
  if (millis() - lastScreenUpdateTime >= 33) {
    lastScreenUpdateTime = millis();
    drawFace();
  }
}
