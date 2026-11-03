/*
 * TEST 5 - IMU MPU9250
 * Baca data gyroscope, accelerometer, dan magnetometer.
 *
 * Wiring:
 *   VCC → 3.3V ESP32
 *   GND → GND
 *   SDA → GPIO 21
 *   SCL → GPIO 22
 *
 * Library: MPU9250 by hideakitai
 * Install via Library Manager: "MPU9250" by hideakitai
 */

#include <Wire.h>
#include <MPU9250.h>

MPU9250 mpu;

// Cek WHO_AM_I register secara manual
uint8_t readWhoAmI() {
  Wire.beginTransmission(0x68);
  Wire.write(0x75);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)0x68, (uint8_t)1);
  if (Wire.available()) return Wire.read();
  return 0xFF;
}

void setup() {
  Serial.begin(115200);
  delay(1000); // beri waktu sensor power up penuh

  Wire.begin(8, 9);
  delay(200);

  Serial.println("=== TEST IMU ===");

  // Cek WHO_AM_I dulu sebelum init library
  uint8_t whoami = readWhoAmI();
  Serial.print("WHO_AM_I: 0x");
  Serial.println(whoami, HEX);

  if (whoami == 0xFF) {
    Serial.println("GAGAL: Tidak ada respons dari 0x68");
    Serial.println("Kemungkinan: kabel lepas, VCC salah, atau SDA/SCL terbalik");
    while (1) delay(1000);
  }

  // Identifikasi chip berdasarkan WHO_AM_I
  Serial.print("Chip terdeteksi: ");
  switch (whoami) {
    case 0x71: Serial.println("MPU9250 (asli)"); break;
    case 0x70: Serial.println("MPU6500"); break;
    case 0x68: Serial.println("MPU6050"); break;
    case 0x98: Serial.println("ICM-20689"); break;
    case 0x19: Serial.println("MPU9255"); break;
    default:
      Serial.print("Tidak dikenal (0x");
      Serial.print(whoami, HEX);
      Serial.println(") - coba tetap init");
      break;
  }

  // Init library
  if (!mpu.setup(0x68)) {
    Serial.println("GAGAL: Library tidak bisa init sensor");
    Serial.println("Jika chip bukan MPU9250 asli, coba ganti library");
    while (1) delay(1000);
  }

  Serial.println("IMU OK. Warm-up sensor 5 detik...");
  delay(5000); // warm-up lebih lama agar sensor stabil secara termal

  Serial.println("Kalibrasi gyro & acc...");
  Serial.println("Letakkan drone DIAM di permukaan rata, jangan disentuh selama 5 detik");
  mpu.calibrateAccelGyro();
  Serial.println("Kalibrasi gyro & acc selesai.");
  Serial.println();

  Serial.println("Kalibrasi magnetometer...");
  Serial.println("Putar drone perlahan ke semua arah (8 detik) - gerakkan seperti angka 8");
  mpu.calibrateMag();
  Serial.println("Kalibrasi mag selesai.");
  Serial.println();

  // Tampilkan offset hasil kalibrasi untuk verifikasi
  Serial.println("--- Offset Kalibrasi ---");
  Serial.print("Gyro offset  X:"); Serial.print(mpu.getGyroBiasX(), 2);
  Serial.print(" Y:"); Serial.print(mpu.getGyroBiasY(), 2);
  Serial.print(" Z:"); Serial.println(mpu.getGyroBiasZ(), 2);
  Serial.print("Accel offset X:"); Serial.print(mpu.getAccBiasX(), 2);
  Serial.print(" Y:"); Serial.print(mpu.getAccBiasY(), 2);
  Serial.print(" Z:"); Serial.println(mpu.getAccBiasZ(), 2);
  Serial.println("------------------------");
  Serial.println();
  Serial.println();
  Serial.println("AccX  AccY  AccZ | GyrX  GyrY  GyrZ | Roll  Pitch  Yaw | MagX  MagY  MagZ");
}

void loop() {
  if (mpu.update()) {
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 100) {
      lastPrint = millis();

      // Accelerometer (g)
      Serial.print("Acc  X:"); Serial.print(mpu.getAccX(), 2);
      Serial.print(" Y:");     Serial.print(mpu.getAccY(), 2);
      Serial.print(" Z:");     Serial.print(mpu.getAccZ(), 2);

      // Gyroscope (deg/s)
      Serial.print(" | Gyr X:"); Serial.print(mpu.getGyroX(), 1);
      Serial.print(" Y:");        Serial.print(mpu.getGyroY(), 1);
      Serial.print(" Z:");        Serial.print(mpu.getGyroZ(), 1);

      // Euler angles
      Serial.print(" | Roll:");  Serial.print(mpu.getRoll(), 1);
      Serial.print(" Pitch:");   Serial.print(mpu.getPitch(), 1);
      Serial.print(" Yaw:");     Serial.print(mpu.getYaw(), 1);

      // Magnetometer (uT)
      Serial.print(" | Mag X:"); Serial.print(mpu.getMagX(), 1);
      Serial.print(" Y:");        Serial.print(mpu.getMagY(), 1);
      Serial.print(" Z:");        Serial.print(mpu.getMagZ(), 1);

      Serial.println();

      // Cek nilai accelerometer wajar
      // Saat diam, total acc harusnya mendekati 1g (gravitasi)
      float accTotal = sqrt(
        mpu.getAccX() * mpu.getAccX() +
        mpu.getAccY() * mpu.getAccY() +
        mpu.getAccZ() * mpu.getAccZ()
      );

      if (accTotal < 0.5 || accTotal > 2.0) {
        Serial.print("WARNING: acc total = ");
        Serial.print(accTotal, 2);
        Serial.println("g (harusnya ~1.0g saat diam)");
      }
    }
  }
}
