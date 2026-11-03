#include "safety.h"
#include "config.h"
#include <Arduino.h>

#ifdef ENABLE_SAFETY

static SafetyData data;

// ── Baterai ──────────────────────────────────────
#define BATT_SAMPLES     20
#define BATT_LPF_ALPHA   0.95f

static float battFiltered = 0;
static bool  is4S         = false;   // auto-detect 3S atau 4S
static uint32_t crashTimerMs = 0;
static uint32_t lastDisarmMs = 0;
static uint32_t lastBeaconMs = 0;

static float readBattVoltage() {
  long sum = 0;
  for (int i = 0; i < BATT_SAMPLES; i++) {
    sum += analogRead(PIN_BATTERY_ADC);
    delayMicroseconds(100);
  }
  float adc     = sum / (float)BATT_SAMPLES;
  float voltage = (adc / BATT_ADC_MAX) * BATT_ADC_REF * BATT_DIVIDER_RATIO;
  return voltage;
}

static float battToPercent(float voltage) {
  float full     = is4S ? BATT_4S_FULL     : BATT_3S_FULL;
  float critical = is4S ? BATT_4S_CRITICAL : BATT_3S_CRITICAL;
  float pct = (voltage - critical) / (full - critical) * 100.0f;
  return constrain(pct, 0.0f, 100.0f);
}

float safetyGetThrustMultiplier() {
  #ifdef ENABLE_VOLTAGE_COMPENSATION
    if (battFiltered <= 6.0f) return 1.0f; // Sensor baterai tidak tersambung
    float nominal = is4S ? BATT_4S_FULL : BATT_3S_FULL;
    float factor  = nominal / battFiltered;
    return constrain(factor, 1.0f, 1.25f);
  #else
    return 1.0f;
  #endif
}

// ── Buzzer (non-blocking) ─────────────────────────
struct BuzzerState {
  int      count;
  int      remaining;
  int      onMs;
  int      offMs;
  bool     isOn;
  uint32_t lastMs;
};
static BuzzerState buz = {0, 0, 0, 0, false, 0};

void buzzerBeep(int count, int onMs, int offMs) {
  buz.count     = count;
  buz.remaining = count;
  buz.onMs      = onMs;
  buz.offMs     = offMs;
  buz.isOn      = false;
  buz.lastMs    = millis();
  digitalWrite(PIN_BUZZER, LOW);
}

void buzzerUpdate() {
  if (buz.remaining <= 0) return;

  uint32_t now = millis();
  if (!buz.isOn && (now - buz.lastMs >= (uint32_t)buz.offMs)) {
    digitalWrite(PIN_BUZZER, HIGH);
    buz.isOn   = true;
    buz.lastMs = now;
  } else if (buz.isOn && (now - buz.lastMs >= (uint32_t)buz.onMs)) {
    digitalWrite(PIN_BUZZER, LOW);
    buz.isOn   = false;
    buz.lastMs = now;
    buz.remaining--;
  }
}

// ── LED (non-blocking) ────────────────────────────
static int      ledInterval = 0;
static uint32_t ledLastMs   = 0;
static bool     ledState    = false;

void ledSet(bool on) {
  ledInterval = 0;
  ledState    = on;
  digitalWrite(PIN_LED, on ? HIGH : LOW);
}

void ledBlink(int intervalMs) {
  ledInterval = intervalMs;
}

void ledUpdate() {
  if (ledInterval <= 0) return;
  uint32_t now = millis();
  if (now - ledLastMs >= (uint32_t)ledInterval) {
    ledLastMs = now;
    ledState  = !ledState;
    digitalWrite(PIN_LED, ledState ? HIGH : LOW);
  }
}

// ── Pre-Arm Safety Checklist ─────────────────────
PreArmReason safetyCheckPreArm(float roll, float pitch, int throttleRaw, bool isGPSMode, bool gpsFix) {
  if (!data.imuHealthy) {
    return PREARM_FAIL_IMU;
  }
  if (fabsf(roll) >= SAFETY_MAX_ANGLE_ARM || fabsf(pitch) >= SAFETY_MAX_ANGLE_ARM) {
    return PREARM_FAIL_NOT_LEVELED;
  }
  if (data.battLand) {
    return PREARM_FAIL_LOW_BATT;
  }
  if (throttleRaw >= RC_MIN + 100) {
    return PREARM_FAIL_THROTTLE_HIGH;
  }
  if (isGPSMode && !gpsFix) {
    return PREARM_FAIL_NO_GPS_LOCK;
  }
  return PREARM_OK;
}

void safetyPlayPreArmFailureBeep(PreArmReason reason) {
  switch (reason) {
    case PREARM_FAIL_LOW_BATT:
      Serial.println("[PREARM] Ditolak: Baterai rendah!");
      buzzerBeep(2, 250, 100);
      break;
    case PREARM_FAIL_NOT_LEVELED:
      Serial.println("[PREARM] Ditolak: Drone miring di atas 15 derajat!");
      buzzerBeep(3, 150, 100);
      break;
    case PREARM_FAIL_NO_GPS_LOCK:
      Serial.println("[PREARM] Ditolak: Mode GPS aktif tapi satelit belum 3D-Fix!");
      buzzerBeep(4, 100, 80);
      break;
    case PREARM_FAIL_IMU:
      Serial.println("[PREARM] Ditolak: Sensor IMU tidak merespons!");
      buzzerBeep(5, 80, 80);
      break;
    case PREARM_FAIL_THROTTLE_HIGH:
      Serial.println("[PREARM] Ditolak: Throttle stick belum di posisi minimum!");
      buzzerBeep(1, 600, 0);
      break;
    default:
      break;
  }
}

void safetyNotifyArm() {
  lastDisarmMs = 0;
  lastBeaconMs = 0;
}

void safetyNotifyDisarm() {
  lastDisarmMs = millis();
}

// ─────────────────────────────────────────────────
void safetyInit() {
  pinMode(PIN_BATTERY_ADC, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED, LOW);

  float initVolt = readBattVoltage();
  battFiltered   = initVolt;
  is4S           = (initVolt > 13.0f);

  data.battVoltage   = initVolt;
  data.battPercent   = battToPercent(initVolt);
  data.battWarn      = false;
  data.battCritical  = false;
  data.battLand      = false;
  data.angleExceeded = false;
  data.crashDetected = false;
  data.imuHealthy    = true;
  data.armAllowed    = true;
  data.status        = SAFETY_OK;

  lastDisarmMs = millis();
  lastBeaconMs = millis();

  Serial.printf("[SAFETY] Init OK, Batt: %.2fV (%s), %.0f%%\n",
    initVolt, is4S ? "4S" : "3S", data.battPercent);

  // Beep 2x tanda boot OK
  buzzerBeep(2, 100, 100);
}

// ─────────────────────────────────────────────────
void safetyUpdate(float roll, float pitch, bool armed, float accG) {
  // ── Baca baterai ─────────────────────────────
  float rawVolt  = readBattVoltage();
  battFiltered   = BATT_LPF_ALPHA * battFiltered + (1.0f - BATT_LPF_ALPHA) * rawVolt;
  data.battVoltage = battFiltered;
  data.battPercent = battToPercent(battFiltered);

  float warnV     = is4S ? BATT_4S_WARN     : BATT_3S_WARN;
  float criticalV = is4S ? BATT_4S_CRITICAL : BATT_3S_CRITICAL;
  float landV     = is4S ? BATT_4S_LAND     : BATT_3S_LAND;

  bool prevWarn = data.battWarn;
  data.battWarn     = (battFiltered < warnV);
  data.battCritical = (battFiltered < criticalV);
  data.battLand     = (battFiltered < landV);

  if (data.battWarn && !prevWarn) {
    Serial.printf("[SAFETY] Baterai LOW: %.2fV\n", battFiltered);
    buzzerBeep(3, 200, 200);
  }

  // ── Cek angle & Crash Detection ──────────────
  bool prevAngle     = data.angleExceeded;
  data.angleExceeded = (fabsf(roll) > SAFETY_MAX_ANGLE ||
                        fabsf(pitch) > SAFETY_MAX_ANGLE);

  bool severeTilt   = (fabsf(roll) > SAFETY_CRASH_ANGLE || fabsf(pitch) > SAFETY_CRASH_ANGLE);
  bool severeImpact = (accG > SAFETY_CRASH_G);

  if (armed && (severeTilt || severeImpact)) {
    if (crashTimerMs == 0) crashTimerMs = millis();
    if (millis() - crashTimerMs > 300 || severeImpact) {
      data.crashDetected = true;
    }
  } else {
    crashTimerMs = 0;
  }

  if (data.angleExceeded && !prevAngle && armed) {
    Serial.printf("[SAFETY] Angle exceeded! R:%.1f P:%.1f\n", roll, pitch);
    buzzerBeep(5, 50, 50);
  }

  data.armAllowed = (fabsf(roll)  < SAFETY_MAX_ANGLE_ARM &&
                     fabsf(pitch) < SAFETY_MAX_ANGLE_ARM);

  // ── Lost Drone Beacon ────────────────────────
  #ifdef ENABLE_LOST_BEACON
  if (!armed && lastDisarmMs > 0) {
    if (millis() - lastDisarmMs > BEACON_TIMEOUT_MS) {
      if (millis() - lastBeaconMs > 5000) {
        lastBeaconMs = millis();
        buzzerBeep(3, 100, 100); // 3x beep penunjuk lokasi
      }
    }
  }
  #endif

  // ── Tentukan status keseluruhan ──────────────
  if (data.battLand || (data.angleExceeded && armed) || data.crashDetected) {
    data.status = SAFETY_EMERGENCY;
  } else if (data.battCritical) {
    data.status = SAFETY_CRITICAL;
  } else if (data.battWarn) {
    data.status = SAFETY_WARN;
  } else {
    data.status = SAFETY_OK;
  }

  // ── Update LED berdasarkan status ────────────
  if (!armed) {
    ledBlink(500);           // disarm: blink lambat
  } else {
    switch (data.status) {
      case SAFETY_OK:        ledBlink(1000); break;
      case SAFETY_WARN:      ledBlink(300);  break;
      case SAFETY_CRITICAL:  ledBlink(100);  break;
      case SAFETY_EMERGENCY: ledSet(true);   break;
    }
  }

  buzzerUpdate();
  ledUpdate();
}

SafetyData safetyGet() {
  return data;
}

SafetyStatus safetyGetStatus() {
  return data.status;
}

#endif // ENABLE_SAFETY
