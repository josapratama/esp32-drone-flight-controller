/*
 * TEST 4 - GPS NEO M8N
 * Baca data GPS: koordinat, altitude, jumlah satelit, fix status.
 *
 * Wiring:
 *   GPS TX → GPIO 16 (ESP32 UART2 RX)
 *   GPS RX → GPIO 17 (ESP32 UART2 TX)
 *   VCC    → 5V
 *   GND    → GND
 *
 * Library: TinyGPSPlus
 * Install via Library Manager: "TinyGPSPlus" by Mikal Hart
 */

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

HardwareSerial gpsSerial(2); // UART2
TinyGPSPlus gps;

#define GPS_BAUD 9600

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, 15, 18); // RX=16, TX=17
  delay(500);

  Serial.println("=== TEST GPS NEO M8N ===");
  Serial.println("Taruh GPS di tempat terbuka (dekat jendela/luar)");
  Serial.println("Cold start butuh 1-3 menit untuk fix pertama");
  Serial.println();
}

unsigned long lastPrint = 0;

void loop() {
  // Feed data GPS ke TinyGPSPlus
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // Print setiap 1 detik
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();

    Serial.println("──────────────────────────────");

    // Jumlah satelit
    Serial.print("Satelit    : ");
    if (gps.satellites.isValid()) {
      Serial.println(gps.satellites.value());
    } else {
      Serial.println("Mencari...");
    }

    // Fix status
    Serial.print("Fix        : ");
    if (gps.location.isValid() && gps.location.age() < 2000) {
      Serial.println("OK");
    } else {
      Serial.println("Belum fix");
    }

    // Koordinat
    Serial.print("Latitude   : ");
    if (gps.location.isValid()) {
      Serial.println(gps.location.lat(), 6);
    } else {
      Serial.println("--");
    }

    Serial.print("Longitude  : ");
    if (gps.location.isValid()) {
      Serial.println(gps.location.lng(), 6);
    } else {
      Serial.println("--");
    }

    // Altitude
    Serial.print("Altitude   : ");
    if (gps.altitude.isValid()) {
      Serial.print(gps.altitude.meters(), 1);
      Serial.println(" m");
    } else {
      Serial.println("--");
    }

    // Kecepatan
    Serial.print("Kecepatan  : ");
    if (gps.speed.isValid()) {
      Serial.print(gps.speed.kmph(), 1);
      Serial.println(" km/h");
    } else {
      Serial.println("--");
    }

    // Waktu UTC
    Serial.print("Waktu UTC  : ");
    if (gps.time.isValid()) {
      Serial.printf("%02d:%02d:%02d\n",
        gps.time.hour(),
        gps.time.minute(),
        gps.time.second());
    } else {
      Serial.println("--");
    }

    // Warning jika tidak ada data sama sekali
    if (millis() > 10000 && gps.charsProcessed() < 10) {
      Serial.println("WARNING: Tidak ada data dari GPS!");
      Serial.println("Cek kabel TX/RX dan power GPS");
    }
  }
}
