#pragma once
#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_SAFETY

// ── Pin (ESP32-S3 Compatible) ────────────────────
#define PIN_BATTERY_ADC   1    // ADC1_CH0 (GPIO 1 pada ESP32-S3)
#define PIN_BUZZER        21   // GPIO 21 (Output bebas pada ESP32-S3)
#define PIN_LED           2    // GPIO 2

// ── Baterai ──────────────────────────────────────
#define BATT_DIVIDER_RATIO   11.0f
#define BATT_ADC_REF         3.3f
#define BATT_ADC_MAX         4095.0f

#define BATT_3S_FULL         12.6f
#define BATT_3S_WARN         11.1f
#define BATT_3S_CRITICAL     10.5f
#define BATT_3S_LAND         10.2f

#define BATT_4S_FULL         16.8f
#define BATT_4S_WARN         14.8f
#define BATT_4S_CRITICAL     14.0f
#define BATT_4S_LAND         13.6f

// ── Angle limit & Crash Detection ────────────────
#define SAFETY_MAX_ANGLE     55.0f
#define SAFETY_CRASH_ANGLE   75.0f
#define SAFETY_CRASH_G       4.5f
#define SAFETY_MAX_ANGLE_ARM 15.0f

// ── Status ───────────────────────────────────────
enum SafetyStatus {
  SAFETY_OK        = 0,
  SAFETY_WARN      = 1,
  SAFETY_CRITICAL  = 2,
  SAFETY_EMERGENCY = 3,
};

enum PreArmReason {
  PREARM_OK = 0,
  PREARM_FAIL_NOT_LEVELED,   // Drone miring > 15 derajat
  PREARM_FAIL_LOW_BATT,      // Baterai terlalu rendah
  PREARM_FAIL_THROTTLE_HIGH, // Stick throttle belum di posisi minimum
  PREARM_FAIL_NO_GPS_LOCK,   // Arming di mode GPS tapi belum fix
  PREARM_FAIL_IMU            // IMU tidak sehat
};

struct SafetyData {
  float        battVoltage;
  float        battPercent;
  bool         battWarn;
  bool         battCritical;
  bool         battLand;
  bool         angleExceeded;
  bool         crashDetected;
  bool         imuHealthy;
  bool         armAllowed;
  SafetyStatus status;
};

void         safetyInit();
void         safetyUpdate(float roll, float pitch, bool armed, float accG = 1.0f);
void         safetyUpdateFast(float roll, float pitch, bool armed, float accG = 1.0f);
void         safetyUpdateSlow(bool armed = false);
SafetyData   safetyGet();
SafetyStatus safetyGetStatus();
float        safetyGetThrustMultiplier();

PreArmReason safetyCheckPreArm(float roll, float pitch, int throttleRaw, bool isGPSMode, bool gpsFix);
void         safetyPlayPreArmFailureBeep(PreArmReason reason);
void         safetyNotifyArm();
void         safetyNotifyDisarm();

void         buzzerBeep(int count, int onMs, int offMs);
void         buzzerUpdate();
void         ledSet(bool on);
void         ledBlink(int intervalMs);
void         ledUpdate();

#endif // ENABLE_SAFETY
