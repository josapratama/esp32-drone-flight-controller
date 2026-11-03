#include "imu.h"
#include "config.h"
#include <Wire.h>
#include <MPU9250.h>
#include <Preferences.h>

static MPU9250 mpu;
static IMUData data;

// Offset kalibrasi gyro
static float gyroOffX = 0, gyroOffY = 0, gyroOffZ = 0;

void imuSaveCalibration() {
  Preferences prefs;
  if (prefs.begin("drone_imu", false)) {
    prefs.putFloat("gx", gyroOffX);
    prefs.putFloat("gy", gyroOffY);
    prefs.putFloat("gz", gyroOffZ);
    prefs.putBool("cal_done", true);
    prefs.end();
    Serial.println("[IMU] Offset gyro berhasil disimpan ke Flash (NVS)!");
  }
}

bool imuLoadCalibration() {
  Preferences prefs;
  if (prefs.begin("drone_imu", true)) {
    bool done = prefs.getBool("cal_done", false);
    if (done) {
      gyroOffX = prefs.getFloat("gx", 0.0f);
      gyroOffY = prefs.getFloat("gy", 0.0f);
      gyroOffZ = prefs.getFloat("gz", 0.0f);
      prefs.end();
      Serial.printf("[IMU] Loaded NVS offset: X=%.2f Y=%.2f Z=%.2f (Boot Instan!)\n",
                    gyroOffX, gyroOffY, gyroOffZ);
      return true;
    }
    prefs.end();
  }
  return false;
}

void imuCalibrate() {
  Serial.println("[IMU] Kalibrasi gyro, jangan gerakkan drone...");
  mpu.calibrateAccelGyro();

  // Ambil offset gyro manual untuk deadband
  float sumX = 0, sumY = 0, sumZ = 0;
  const int samples = 500;
  for (int i = 0; i < samples; i++) {
    if (mpu.update()) {
      sumX += mpu.getGyroX();
      sumY += mpu.getGyroY();
      sumZ += mpu.getGyroZ();
    }
    delay(2);
  }
  gyroOffX = sumX / samples;
  gyroOffY = sumY / samples;
  gyroOffZ = sumZ / samples;

  Serial.printf("[IMU] Gyro offset: X=%.2f Y=%.2f Z=%.2f\n",
    gyroOffX, gyroOffY, gyroOffZ);
  Serial.println("[IMU] Kalibrasi selesai");

  // Simpan ke Flash secara otomatis
  imuSaveCalibration();
}

bool imuInit() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);  // 400kHz fast mode
  delay(100);             // tunggu I2C bus stabil

  // Coba setup sampai 3 kali, MPU9250 kadang butuh retry setelah power on
  for (int attempt = 1; attempt <= 3; attempt++) {
    if (mpu.setup(0x68)) {
      data.healthy = true;
      data.roll = data.pitch = data.yaw = 0;
      Serial.println("[IMU] OK");

      // Cek apakah ada offset tersimpan di NVS
      if (!imuLoadCalibration()) {
        Serial.println("[IMU] Belum ada kalibrasi di NVS, menjalankan kalibrasi pertama kali...");
        imuCalibrate();
      }
      return true;
    }
    Serial.printf("[IMU] Attempt %d gagal, retry...\n", attempt);
    delay(200);
  }

  Serial.println("[IMU] GAGAL: MPU9250 tidak terdeteksi");
  data.healthy = false;
  return false;
}

void imuUpdate(float dt) {
  if (!mpu.update()) return;

  // Raw gyro (deg/s) dikurangi offset
  float gx = mpu.getGyroX() - gyroOffX;
  float gy = mpu.getGyroY() - gyroOffY;
  float gz = mpu.getGyroZ() - gyroOffZ;

  // Terapkan deadband
  if (fabsf(gx) < GYRO_DEADBAND) gx = 0;
  if (fabsf(gy) < GYRO_DEADBAND) gy = 0;
  if (fabsf(gz) < GYRO_DEADBAND) gz = 0;

  data.gyroX = gx;
  data.gyroY = gy;
  data.gyroZ = gz;

  // Raw accelerometer
  data.accX = mpu.getAccX();
  data.accY = mpu.getAccY();
  data.accZ = mpu.getAccZ();

  // Sudut dari accelerometer
  float accRoll  = atan2f(data.accY, data.accZ) * RAD_TO_DEG;
  float accPitch = atan2f(-data.accX,
    sqrtf(data.accY * data.accY + data.accZ * data.accZ)) * RAD_TO_DEG;

  // Complementary filter
  // Gabungkan gyro (cepat, drift) + accelerometer (lambat, akurat jangka panjang)
  data.roll  = CF_ALPHA * (data.roll  + gx * dt) + (1.0f - CF_ALPHA) * accRoll;
  data.pitch = CF_ALPHA * (data.pitch + gy * dt) + (1.0f - CF_ALPHA) * accPitch;

  // Yaw hanya dari gyro (accelerometer tidak bisa ukur yaw)
  data.yaw += gz * dt;

  // Wrap yaw ke -180 ~ 180
  if (data.yaw >  180.0f) data.yaw -= 360.0f;
  if (data.yaw < -180.0f) data.yaw += 360.0f;
}

IMUData imuGet() {
  return data;
}
