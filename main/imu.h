#pragma once
#include <Arduino.h>

struct IMUData {
  // Sudut (derajat)
  float roll;
  float pitch;
  float yaw;

  // Gyro rate (deg/s)
  float gyroX;
  float gyroY;
  float gyroZ;

  // Accelerometer (g)
  float accX;
  float accY;
  float accZ;

  bool healthy;
};

bool    imuInit();
void    imuCalibrate();
bool    imuLoadCalibration();
void    imuSaveCalibration();
void    imuUpdate(float dt);
IMUData imuGet();
