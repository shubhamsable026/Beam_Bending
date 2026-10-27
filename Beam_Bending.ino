
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "HX711.h"

#define DT_PIN  4
#define SCK_PIN 5

LiquidCrystal_I2C lcd(0x27, 16, 2);
HX711 scale;

// Load cell calibration
const float CAL_FACTOR = 769.58;

// Overload warning threshold
const float WARNING_LIMIT_G = 900.0;

// Beam dimensions (mm): change to actual dimensions
const float L = 300.0;        // Support span
const float b = 20.0;         // Width
const float t = 3.0;          // Thickness
const float E = 200000.0;     // Approximate steel modulus

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Startup screen for 7 seconds
  lcd.setCursor(0, 0);
  lcd.print("Beam Bending");
  lcd.setCursor(0, 1);
  lcd.print("Strength Test");
  delay(7000);

  lcd.clear();
  lcd.print("Initializing...");

  scale.begin(DT_PIN, SCK_PIN);

  unsigned long startTime = millis();

  while (!scale.is_ready()) {
    if (millis() - startTime > 5000) {
      lcd.clear();
      lcd.print("HX711 Error!");
      lcd.setCursor(0, 1);
      lcd.print("Check Wiring");

      Serial.println("ERROR: Check HX711 wiring!");

      while (true) {
        delay(100);
      }
    }
    delay(10);
  }

  scale.set_scale(CAL_FACTOR);

  lcd.clear();
  lcd.print("Remove all load");
  lcd.setCursor(0, 1);
  lcd.print("Zeroing...");
  delay(2000);

  scale.tare(20);

  lcd.clear();
  lcd.print("System Ready");
  Serial.println("Beam Bending Test Ready");
  delay(1000);
}

void loop() {
  if (!scale.is_ready()) {
    lcd.clear();
    lcd.print("HX711 Error!");
    lcd.setCursor(0, 1);
    lcd.print("Check Wiring");

    Serial.println("ERROR: HX711 not ready");
    delay(500);
    return;
  }

  // Read load in grams
  float load_g = scale.get_units(10);

  // Use absolute load for overload detection
  float absLoad_g = abs(load_g);

  // Overload warning
  if (absLoad_g >= WARNING_LIMIT_G) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("!! OVERLOAD !!");
    lcd.setCursor(0, 1);
    lcd.print("Please Remove");

    Serial.println("!!! OVERLOAD WARNING !!!");
    Serial.println("PLEASE REMOVE LOAD!");

    delay(300);
    return;
  }

  // Ignore small negative zero drift for display
  if (load_g < 0) {
    load_g = 0;
  }

  // Convert grams to force (Newtons)
  float P = load_g * 9.80665 / 1000.0;

  // Second moment of area (mm^4)
  float I = b * t * t * t / 12.0;

  // Bending stress (N/mm^2)
  float stress = (3.0 * P * L) /
                 (2.0 * b * t * t);

  // Theoretical central deflection (mm)
  float deflection = (P * L * L * L) /
                     (48.0 * E * I);

  // Serial Monitor results
  Serial.println("------------------------");

  Serial.print("Load: ");
  Serial.print(load_g, 2);
  Serial.println(" g");

  Serial.print("Force: ");
  Serial.print(P, 4);
  Serial.println(" N");

  Serial.print("Bending Stress: ");
  Serial.print(stress, 4);
  Serial.println(" N/mm^2");

  Serial.print("Theoretical Deflection: ");
  Serial.print(deflection, 4);
  Serial.println(" mm");

  // Screen 1: Load and force
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Load:");
  lcd.print(load_g, 1);
  lcd.print(" g");

  lcd.setCursor(0, 1);
  lcd.print("Force:");
  lcd.print(P, 3);
  lcd.print(" N");
  delay(2000);

  // Screen 2: Bending stress
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bending Stress");
  lcd.setCursor(0, 1);
  lcd.print(stress, 3);
  lcd.print(" N/mm2");
  delay(2000);

  // Screen 3: Theoretical deflection
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Deflection:");
  lcd.setCursor(0, 1);
  lcd.print(deflection, 4);
  lcd.print(" mm");
  delay(2000);
}
