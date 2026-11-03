#include <Arduino.h>
#include <Wire.h>
#include <MPU9250.h>
#include <ESP32Servo.h>

// ================== IBUS ==================
HardwareSerial IBUS(1);
static const int IBUS_RX_PIN = 33;
uint16_t ibusCh[14];

// ================== ESC ===================
Servo esc1, esc2, esc3, esc4;
#define ESC1_PIN 25
#define ESC2_PIN 14
#define ESC3_PIN 26
#define ESC4_PIN 27

// ================== IMU ===================
MPU9250 mpu;

float gyroX_offset = 0;
float gyroY_offset = 0;
float gyroZ_offset = 0;

float roll = 0;
float pitch = 0;

unsigned long lastTime;

// ================== SETTINGS ==================
float Kp_angle = 4.0;
float Kd_rate  = 0.15;
float Kp_yaw   = 0.8;

float maxAngle   = 20.0;
float maxYawRate = 150.0;

// ================== IBUS READ ==================
bool readIbusFrame() {
  static uint8_t buf[32];
  static uint8_t idx = 0;

  while (IBUS.available()) {
    uint8_t b = IBUS.read();

    if (idx == 0) {
      if (b != 0x20) continue;
      buf[idx++] = b;
      continue;
    }

    buf[idx++] = b;

    if (idx == 2 && buf[1] != 0x40) {
      idx = 0;
      continue;
    }

    if (idx >= 32) {
      uint16_t sum = 0;
      for (int i = 0; i < 30; i++) sum += buf[i];
      uint16_t calc = 0xFFFF - sum;
      uint16_t rx = (uint16_t)buf[30] | ((uint16_t)buf[31] << 8);
      idx = 0;

      if (calc != rx) return false;

      for (int ch = 0; ch < 14; ch++) {
        ibusCh[ch] =
          (uint16_t)buf[2 + ch * 2] |
          ((uint16_t)buf[3 + ch * 2] << 8);
      }
      return true;
    }
  }
  return false;
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== QUAD FC WITH YAW ===");
  Serial.println("REMOVE PROPS!");

  IBUS.begin(115200, SERIAL_8N1, IBUS_RX_PIN, -1);

  Wire.begin(21, 22);

  if (!mpu.setup(0x68)) {
    Serial.println("MPU FAILED");
    while (1);
  }

  // ===== Gyro Calibration =====
  Serial.println("Calibrating gyro...");
  float sumX = 0, sumY = 0, sumZ = 0;

  for (int i = 0; i < 2000; i++) {
    mpu.update();
    sumX += mpu.getGyroX();
    sumY += mpu.getGyroY();
    sumZ += mpu.getGyroZ();
    delay(2);
  }

  gyroX_offset = sumX / 2000.0;
  gyroY_offset = sumY / 2000.0;
  gyroZ_offset = sumZ / 2000.0;

  Serial.println("Gyro calibration done");

  // ESC setup
  esc1.setPeriodHertz(50);
  esc2.setPeriodHertz(50);
  esc3.setPeriodHertz(50);
  esc4.setPeriodHertz(50);

  esc1.attach(ESC1_PIN, 1000, 2000);
  esc2.attach(ESC2_PIN, 1000, 2000);
  esc3.attach(ESC3_PIN, 1000, 2000);
  esc4.attach(ESC4_PIN, 1000, 2000);

  esc1.writeMicroseconds(1000);
  esc2.writeMicroseconds(1000);
  esc3.writeMicroseconds(1000);
  esc4.writeMicroseconds(1000);

  lastTime = micros();
}

// ================== LOOP ==================
void loop() {

  readIbusFrame();
  if (!mpu.update()) return;

  unsigned long now = micros();
  float dt = (now - lastTime) / 1000000.0;
  lastTime = now;

  // ===== Gyro corrected =====
  float gx = mpu.getGyroX() - gyroX_offset;
  float gy = mpu.getGyroY() - gyroY_offset;
  float gz = mpu.getGyroZ() - gyroZ_offset;

  roll  += gx * dt;
  pitch += gy * dt;

  // ===== Accelerometer =====
  float ax = mpu.getAccX();
  float ay = mpu.getAccY();
  float az = mpu.getAccZ();

  float rollAcc  = atan2(ay, az) * 180 / PI;
  float pitchAcc = atan2(-ax, sqrt(ay * ay + az * az)) * 180 / PI;

  roll  = roll * 0.98 + rollAcc * 0.02;
  pitch = pitch * 0.98 + pitchAcc * 0.02;

  // ===== PID storage =====
  static float iRoll = 0;
  static float iPitch = 0;
  static float iYaw = 0;

  // ===== Receiver =====
  uint16_t chRoll  = ibusCh[0];
  uint16_t chPitch = ibusCh[1];
  uint16_t chThr   = ibusCh[2];
  uint16_t chYaw   = ibusCh[3];
  uint16_t chArm   = ibusCh[4];

  bool armed = (chArm > 1500);
  int baseThrottle = armed ? chThr : 1000;

  // ===== Target =====
  float targetRoll =
    ((int)chRoll - 1500) / 500.0 * maxAngle;

  float targetPitch =
    ((int)chPitch - 1500) / 500.0 * maxAngle;

  float targetYawRate =
    ((int)chYaw - 1500) / 500.0 * maxYawRate;

  // ===== Errors =====
  float errorRoll  = targetRoll  - roll;
  float errorPitch = targetPitch - pitch;
  float errorYaw   = targetYawRate - gz;

  // ===== Integrator =====
  if (armed && baseThrottle > 1100) {
    iRoll  += errorRoll  * Ki_angle * dt;
    iPitch += errorPitch * Ki_angle * dt;
    iYaw   += errorYaw   * Ki_yaw   * dt;
  }

  // Anti-windup
  iRoll  = constrain(iRoll,  -200, 200);
  iPitch = constrain(iPitch, -200, 200);
  iYaw   = constrain(iYaw,   -200, 200);

  // ===== PID Output =====
  float rollCorrection  =
    errorRoll * Kp_angle + iRoll - gx * Kd_rate;

  float pitchCorrection =
    errorPitch * Kp_angle + iPitch - gy * Kd_rate;

  float yawCorrection =
    errorYaw * Kp_yaw + iYaw;

  // ===== Quad X Mixing =====
  int m1 = baseThrottle - rollCorrection + pitchCorrection;
  int m2 = baseThrottle + rollCorrection + pitchCorrection;
  int m3 = baseThrottle + rollCorrection - pitchCorrection;
  int m4 = baseThrottle - rollCorrection - pitchCorrection;

  // YAW (M1,M4 CCW / M2,M3 CW)
  m1 -= yawCorrection;
  m4 -= yawCorrection;
  m2 += yawCorrection;
  m3 += yawCorrection;

  if (!armed) {
    m1 = m2 = m3 = m4 = 1000;
    iRoll = iPitch = iYaw = 0; // reset integrator saat disarm
  }

  m1 = constrain(m1, 1000, 2000);
  m2 = constrain(m2, 1000, 2000);
  m3 = constrain(m3, 1000, 2000);
  m4 = constrain(m4, 1000, 2000);

  writeAllMotorsUs(m1, m2, m3, m4);

  esc1.writeMicroseconds(m1);
  esc2.writeMicroseconds(m2);
  esc3.writeMicroseconds(m3);
  esc4.writeMicroseconds(m4);

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 300) {
    lastPrint = millis();
    Serial.printf("ARM=%s THR=%d | R=%.2f P=%.2f | M:%d %d %d %d\n",
                  armed ? "ON" : "OFF",
                  baseThrottle,
                  roll, pitch,
                  m1, m2, m3, m4);
  }
}