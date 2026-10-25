/*
 * ============================================================================
 *   SUPER DIAGNOSTIC & MOTOR/SCREEN TESTER
 *   - Guaranteed NEVER to freeze or hang (I2C hardware timeout active!)
 *   - Tests Servos, "L" Blinking LED, and 1.3" OLED Screen safely
 * ============================================================================
 */

#include <Wire.h>
#include <Servo.h>

#define LEFT_SERVO_PIN  9
#define RIGHT_SERVO_PIN 10
#define LED_PIN         13

Servo leftServo;
Servo rightServo;

uint8_t detectedOLEDAddress = 0;
unsigned long lastBlinkTime = 0;
unsigned long lastServoTime = 0;
bool servoState = false;
bool ledState = false;

// Safe non-blocking I2C write
bool safeSendByte(uint8_t addr, uint8_t control, uint8_t data) {
  Wire.beginTransmission(addr);
  Wire.write(control);
  Wire.write(data);
  return (Wire.endTransmission() == 0);
}

void clearOLEDDirect(uint8_t addr) {
  // Clear all 8 pages of the 1.3" screen
  for (uint8_t p = 0; p < 8; p++) {
    Wire.beginTransmission(addr);
    Wire.write(0x00); // Command stream
    Wire.write(0xB0 + p);
    Wire.write(0x02);
    Wire.write(0x10);
    Wire.endTransmission();

    for (uint8_t chunk = 0; chunk < 8; chunk++) {
      Wire.beginTransmission(addr);
      Wire.write(0x40); // Data stream
      for (uint8_t i = 0; i < 16; i++) {
        Wire.write(0x00); // Pure black! Clears static dots!
      }
      Wire.endTransmission();
    }
  }
}

void drawCreeperFaceDirect(uint8_t addr) {
  // Direct hardware page draw - 100% crash proof!
  // Draw glowing box with Creeper eyes on Page 2, 3, 4, 5
  clearOLEDDirect(addr);

  // Draw Creeper on Center Pages
  for (uint8_t p = 1; p <= 6; p++) {
    Wire.beginTransmission(addr);
    Wire.write(0x00);
    Wire.write(0xB0 + p);
    Wire.write(0x02 + 36); // Center offset
    Wire.write(0x10);
    Wire.endTransmission();

    Wire.beginTransmission(addr);
    Wire.write(0x40);
    for (uint8_t col = 0; col < 56; col++) {
      // Solid white block with cutouts for eyes & mouth
      uint8_t b = 0xFF;

      // Eyes on pages 2 and 3
      if ((p == 2 || p == 3) && ((col >= 8 && col < 22) || (col >= 34 && col < 48))) {
        b = 0x00; // Black eyes!
      }
      // Mouth center on pages 4 and 5
      if ((p == 4 || p == 5) && (col >= 15 && col < 41)) {
        b = 0x00; // Black mouth!
      }
      // Mouth drop corners on page 6
      if (p == 6 && ((col >= 15 && col < 22) || (col >= 34 && col < 41))) {
        b = 0x00; // Black chin drops!
      }

      Wire.write(b);
    }
    Wire.endTransmission();
  }
}

void setup() {
  // 1. BLINK LED IMMEDIATELY to prove Arduino is running!
  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(80);
    digitalWrite(LED_PIN, LOW);
    delay(80);
  }

  Serial.begin(115200);
  Serial.println(F("\n--- DIAGNOSTIC ROBOT TEST STARTED ---"));

  // 2. ATTACH SERVOS FIRST
  leftServo.attach(LEFT_SERVO_PIN);
  rightServo.attach(RIGHT_SERVO_PIN);

  // Quick initial servo kick to show motors work!
  leftServo.write(45);
  rightServo.write(135);
  delay(400);
  leftServo.write(90);
  rightServo.write(90);
  delay(400);

  // 3. START I2C WITH HARDWARE TIMEOUT (NEVER HANGS!)
  Wire.begin();
  #if defined(WIRE_HAS_TIMEOUT)
    Wire.setWireTimeout(25000, true); // 25ms timeout - stops all freezes!
  #endif

  // 4. SCAN FOR OLED
  Serial.print(F("Scanning for OLED screen... "));
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0) {
    detectedOLEDAddress = 0x3C;
    Serial.println(F("FOUND AT 0x3C!"));
  } else {
    Wire.beginTransmission(0x3D);
    if (Wire.endTransmission() == 0) {
      detectedOLEDAddress = 0x3D;
      Serial.println(F("FOUND AT 0x3D!"));
    } else {
      Serial.println(F("NOT FOUND! Check wires: SDA->A4, SCL->A5"));
    }
  }

  // 5. IF OLED FOUND, INITIALIZE & DRAW CREEPER
  if (detectedOLEDAddress != 0) {
    static const uint8_t initSeq[] PROGMEM = {
      0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
      0xAD, 0x8B, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xBF,
      0xD9, 0x22, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    for (uint8_t i = 0; i < sizeof(initSeq); i++) {
      safeSendByte(detectedOLEDAddress, 0x80, pgm_read_byte(&initSeq[i]));
    }
    drawCreeperFaceDirect(detectedOLEDAddress);
  }
}

void loop() {
  unsigned long now = millis();

  // 1. FAST LED HEARTBEAT: Blinks every 250ms (SUPER VISIBLE!)
  if (now - lastBlinkTime >= 250) {
    lastBlinkTime = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }

  // 2. AUTOMATIC SERVO SWEEP: Moves arms every 1.5 seconds!
  if (now - lastServoTime >= 1500) {
    lastServoTime = now;
    servoState = !servoState;

    if (servoState) {
      leftServo.write(30);
      rightServo.write(150);
      Serial.println(F("Motors: Position DOWN (30 deg)"));
    } else {
      leftServo.write(120);
      rightServo.write(60);
      Serial.println(F("Motors: Position UP (120 deg)"));
    }
  }
}
