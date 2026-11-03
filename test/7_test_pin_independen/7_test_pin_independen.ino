/*
 * TEST PIN INDEPENDEN
 * Cek apakah GPIO 4, 5, 6, 7 benar-benar independen
 * di ESP32-S3 tanpa library Servo.
 *
 * Buka Serial Monitor 115200
 * Ukur tegangan tiap pin dengan multimeter saat test berjalan
 */

#define PIN_M1  4
#define PIN_M2  5
#define PIN_M3  6
#define PIN_M4  7

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_M1, OUTPUT);
  pinMode(PIN_M2, OUTPUT);
  pinMode(PIN_M3, OUTPUT);
  pinMode(PIN_M4, OUTPUT);

  // Semua LOW dulu
  digitalWrite(PIN_M1, LOW);
  digitalWrite(PIN_M2, LOW);
  digitalWrite(PIN_M3, LOW);
  digitalWrite(PIN_M4, LOW);

  Serial.println("=== TEST PIN INDEPENDEN ===");
  Serial.println("Ukur tegangan tiap pin dengan multimeter.");
  Serial.println("Hanya pin yang disebut yang boleh HIGH (~3.3V).");
  Serial.println("Pin lain harus LOW (~0V).");
  Serial.println();
}

void loop() {
  // Test GPIO 4 saja
  Serial.println("GPIO 4 = HIGH, sisanya LOW");
  Serial.println("Ukur: GPIO4=3.3V, GPIO5=0V, GPIO6=0V, GPIO7=0V");
  digitalWrite(PIN_M1, HIGH);
  digitalWrite(PIN_M2, LOW);
  digitalWrite(PIN_M3, LOW);
  digitalWrite(PIN_M4, LOW);
  delay(3000);

  // Test GPIO 5 saja
  Serial.println("GPIO 5 = HIGH, sisanya LOW");
  Serial.println("Ukur: GPIO4=0V, GPIO5=3.3V, GPIO6=0V, GPIO7=0V");
  digitalWrite(PIN_M1, LOW);
  digitalWrite(PIN_M2, HIGH);
  digitalWrite(PIN_M3, LOW);
  digitalWrite(PIN_M4, LOW);
  delay(3000);

  // Test GPIO 6 saja
  Serial.println("GPIO 6 = HIGH, sisanya LOW");
  Serial.println("Ukur: GPIO4=0V, GPIO5=0V, GPIO6=3.3V, GPIO7=0V");
  digitalWrite(PIN_M1, LOW);
  digitalWrite(PIN_M2, LOW);
  digitalWrite(PIN_M3, HIGH);
  digitalWrite(PIN_M4, LOW);
  delay(3000);

  // Test GPIO 7 saja
  Serial.println("GPIO 7 = HIGH, sisanya LOW");
  Serial.println("Ukur: GPIO4=0V, GPIO5=0V, GPIO6=0V, GPIO7=3.3V");
  digitalWrite(PIN_M1, LOW);
  digitalWrite(PIN_M2, LOW);
  digitalWrite(PIN_M3, LOW);
  digitalWrite(PIN_M4, HIGH);
  delay(3000);

  // Semua LOW
  Serial.println("Semua LOW");
  digitalWrite(PIN_M1, LOW);
  digitalWrite(PIN_M2, LOW);
  digitalWrite(PIN_M3, LOW);
  digitalWrite(PIN_M4, LOW);
  Serial.println("──────────────────────────────");
  delay(2000);
}
