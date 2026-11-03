/*
 * TEST 1 - I2C SCANNER
 * Scan semua alamat I2C, pastikan muncul:
 *   0x29 → VL53L1X (ToF)
 *   0x68 → MPU9250 (IMU)
 *   0x76 → BMP280 (Barometer)
 *
 * Wiring I2C:
 *   SDA → GPIO 21
 *   SCL → GPIO 22
 *   Pull-up 4.7kΩ ke 3.3V pada SDA dan SCL
 */

#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9); // SDA=21, SCL=22
  delay(500);

  Serial.println("=== I2C SCANNER ===");
  Serial.println("Scanning...");

  int found = 0;

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Ditemukan di: 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);

      // Identifikasi sensor
      if (addr == 0x29) Serial.print("  → VL53L1X (ToF)");
      if (addr == 0x68) Serial.print("  → MPU9250 (IMU)");
      if (addr == 0x69) Serial.print("  → MPU9250 (IMU, AD0=HIGH)");
      if (addr == 0x76) Serial.print("  → BMP280 (Barometer)");
      if (addr == 0x77) Serial.print("  → BMP280 (Barometer, SDO=HIGH)");

      Serial.println();
      found++;
    }
  }

  Serial.println("──────────────────────");
  Serial.print("Total ditemukan: ");
  Serial.println(found);

  if (found == 3) {
    Serial.println("STATUS: OK - Semua sensor terdeteksi");
  } else if (found == 0) {
    Serial.println("STATUS: GAGAL - Tidak ada sensor terdeteksi");
    Serial.println("Cek: kabel SDA/SCL, pull-up resistor, power 3.3V");
  } else {
    Serial.println("STATUS: SEBAGIAN - Ada sensor yang tidak terdeteksi");
    Serial.println("Cek sensor yang tidak muncul");
  }
}

void loop() {
  // Scan ulang setiap 5 detik
  delay(5000);
  Serial.println("\nScan ulang...");

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);
      if (addr == 0x29) Serial.print(" VL53L1X");
      if (addr == 0x68) Serial.print(" MPU9250");
      if (addr == 0x76) Serial.print(" BMP280");
      Serial.println();
    }
  }
}
