/*
 * Quick I2C Scanner to test your OLED screen wires!
 * Upload this sketch, open the Serial Monitor at 115200 baud,
 * and it will tell you the exact I2C address of your OLED screen.
 */
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n--- I2C OLED Scanner Started ---");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning for OLED screen...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("SUCCESS! Found device at address: 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  <-- Use this address in your robot code!");
      nDevices++;
    }
  }

  if (nDevices == 0) {
    Serial.println("No I2C devices found. Check if SDA/SCL and 5V/GND are connected properly.");
  }

  delay(4000); // Wait 4 seconds before scanning again
}
