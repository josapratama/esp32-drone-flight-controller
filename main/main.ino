/*
 * DRONE FLIGHT CONTROLLER - CUSTOM
 * ESP32-S3 + MPU9250 + FS-iA6B (Quad X)
 * Sensor opsional: BMP280, VL53L1X, Neo M8N
 *
 * Fitur:
 *   - 500Hz Fast Loop (IMU, PID PT1 Filter, MCPWM 400Hz)
 *   - AirMode (Attitude stability at zero throttle)
 *   - RC Expo & Dual Rates
 *   - Smooth Altitude Cross-Fading (ToF + Baro)
 *   - Pre-Arm Safety Diagnostics & Lost Drone Beacon
 *   - Crash Detection & Auto-Disarm
 *   - Voltage Sag Thrust Scaling
 *   - Blackbox Flight Logger (LittleFS Flash)
 *   - WiFi Live Tuning Dashboard (Smartphone Web UI)
 */

#include "config.h"
#include "imu.h"
#include "receiver.h"
#include "pid.h"
#include "motor.h"

#ifdef ENABLE_BARO
  #include "baro.h"
#endif
#ifdef ENABLE_TOF
  #include "tof.h"
#endif
#ifdef ENABLE_GPS
  #include "gps.h"
#endif
#ifdef ENABLE_NAV
  #include "nav.h"
#endif
#ifdef ENABLE_SAFETY
  #include "safety.h"
#endif
#ifdef ENABLE_BLACKBOX
  #include "blackbox.h"
#endif
#ifdef ENABLE_WIFI_TUNING
  #include "wifi_tuning.h"
#endif

// ── PID Attitude dengan PT1 D-Term Filter ─────────
PID pidRoll  (ROLL_KP,  ROLL_KI,  ROLL_KD,  PID_ITERM_MAX, PID_MAX_ROLL,  D_FILTER_CUTOFF_HZ);
PID pidPitch (PITCH_KP, PITCH_KI, PITCH_KD, PID_ITERM_MAX, PID_MAX_PITCH, D_FILTER_CUTOFF_HZ);
PID pidYaw   (YAW_KP,   YAW_KI,   YAW_KD,   PID_ITERM_MAX, PID_MAX_YAW,   D_FILTER_CUTOFF_HZ);

#ifdef ENABLE_BARO
PID pidAltitude(ALT_KP, ALT_KI, ALT_KD, PID_ITERM_MAX, PID_MAX_ALT);
#endif

// ── State ─────────────────────────────────────────
bool     isArmed        = false;
int      flightMode     = MODE_STABILIZE;
int      prevFlightMode = -1;

#ifdef ENABLE_BARO
float    targetAltitude = 0;
float    baseThrottle   = 0.5f;
#endif

// ── Timing ───────────────────────────────────────
uint32_t lastLoopUs  = 0;
uint32_t lastSlowMs  = 0;
uint32_t lastDebugMs = 0;

// ─────────────────────────────────────────────────
void resetAllPID() {
  pidRoll.reset();
  pidPitch.reset();
  pidYaw.reset();
  #ifdef ENABLE_BARO
    pidAltitude.reset();
  #endif
}

// ─────────────────────────────────────────────────
// Ambil altitude terbaik dengan Smooth Cross-Fading (ToF <-> Baro)
// ─────────────────────────────────────────────────
float getCurrentAlt() {
  #if defined(ENABLE_TOF) && defined(ENABLE_BARO)
    if (tofIsReliable()) {
      float d = tofGet().distanceM;
      if (d < 1.5f) return d; // ToF presisi di bawah 1.5 meter
      if (d <= 2.8f) {
        // Cross-fading mulus dari 1.5m ke 2.8m agar tidak ada hentakan throttle
        float blend = (d - 1.5f) / (2.8f - 1.5f);
        return (1.0f - blend) * d + blend * baroGet().altitude;
      }
    }
    return baroGet().altitude;
  #elif defined(ENABLE_BARO)
    return baroGet().altitude;
  #elif defined(ENABLE_TOF)
    return tofGet().distanceM;
  #else
    return 0.0f;
  #endif
}

// ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== DRONE FC BOOT ===");

  Serial.print("Fitur: IMU=ON RC=ON EXPO=ON MOTOR=400Hz");
  #ifdef ENABLE_AIRMODE
    Serial.print(" AIRMODE=ON");
  #endif
  #ifdef ENABLE_BLACKBOX
    Serial.print(" BLACKBOX=ON");
  #endif
  #ifdef ENABLE_BARO
    Serial.print(" BARO=ON");
  #endif
  #ifdef ENABLE_TOF
    Serial.print(" TOF=ON");
  #endif
  #ifdef ENABLE_GPS
    Serial.print(" GPS=ON");
  #endif
  #ifdef ENABLE_NAV
    Serial.print(" NAV=ON");
  #endif
  #ifdef ENABLE_SAFETY
    Serial.print(" SAFETY=ON");
  #endif
  #ifdef ENABLE_WIFI_TUNING
    Serial.print(" WIFI_TUNING=ON");
  #endif
  Serial.println();

  // Init modul inti
  bool imuOk = imuInit();
  receiverInit();
  motorInit();

  if (!imuOk) {
    Serial.println("[FATAL] IMU gagal!");
    while (1) delay(1000);
  }

  // Init modul opsional
  #ifdef ENABLE_BLACKBOX
    blackboxInit();
  #endif
  #ifdef ENABLE_BARO
    baroInit();
  #endif
  #ifdef ENABLE_TOF
    tofInit();
  #endif
  #ifdef ENABLE_GPS
    gpsInit();
  #endif
  #ifdef ENABLE_NAV
    navInit();
  #endif
  #ifdef ENABLE_SAFETY
    safetyInit();
  #endif
  #ifdef ENABLE_WIFI_TUNING
    wifiTuningInit(pidRoll, pidPitch, pidYaw);
  #endif

  #ifdef ENABLE_BARO
    baroSetHome();
  #endif

  Serial.println("=== FC SIAP TERBANG ===");
  lastLoopUs = micros();
}

// ─────────────────────────────────────────────────
void updateFlightMode() {
  int sw = rcGetRaw(CH_MODE);

  #if defined(ENABLE_NAV)
    if      (sw < 1200) flightMode = MODE_STABILIZE;
    else if (sw < 1500) flightMode = MODE_ACRO;
    else if (sw < 1800) flightMode = MODE_ALT_HOLD;
    else                flightMode = MODE_GPS_HOLD;
  #elif defined(ENABLE_BARO)
    if      (sw < 1300) flightMode = MODE_STABILIZE;
    else if (sw < 1700) flightMode = MODE_ACRO;
    else                flightMode = MODE_ALT_HOLD;
  #else
    if (sw < 1500) flightMode = MODE_STABILIZE;
    else           flightMode = MODE_ACRO;
  #endif

  if (flightMode == prevFlightMode) return;

  #ifdef ENABLE_BARO
    float currentAlt = getCurrentAlt();
  #endif

  switch (flightMode) {
    case MODE_STABILIZE:
      #ifdef ENABLE_NAV
        navSetMode(NAV_IDLE);
      #endif
      #ifdef ENABLE_BARO
        pidAltitude.reset();
      #endif
      Serial.println("[MODE] Stabilize (Angle Auto-Level)");
      break;

    case MODE_ACRO:
      #ifdef ENABLE_NAV
        navSetMode(NAV_IDLE);
      #endif
      #ifdef ENABLE_BARO
        pidAltitude.reset();
      #endif
      Serial.println("[MODE] Acro (Rate Mode)");
      break;

    #ifdef ENABLE_BARO
    case MODE_ALT_HOLD:
      #ifdef ENABLE_NAV
        navSetMode(NAV_IDLE);
      #endif
      targetAltitude = currentAlt;
      baseThrottle   = (float)(rcGetRaw(CH_THROTTLE) - RC_MIN) / (RC_MAX - RC_MIN);
      pidAltitude.reset();
      Serial.printf("[MODE] Alt Hold, target: %.2f m\n", targetAltitude);
      break;
    #endif

    #ifdef ENABLE_NAV
    case MODE_GPS_HOLD:
      if (gpsHasFix()) {
        navHoldPosition();
        navSetMode(NAV_GPS_HOLD);
        targetAltitude = currentAlt;
        baseThrottle   = (float)(rcGetRaw(CH_THROTTLE) - RC_MIN) / (RC_MAX - RC_MIN);
        pidAltitude.reset();
        Serial.println("[MODE] GPS Hold");
      } else {
        flightMode = MODE_ALT_HOLD;
        targetAltitude = currentAlt;
        pidAltitude.reset();
        Serial.println("[MODE] GPS Hold no-fix -> Fallback Alt Hold");
      }
      break;
    #endif
  }

  prevFlightMode = flightMode;
}

// ─────────────────────────────────────────────────
#ifdef ENABLE_BARO
float computeAltHold(float dt) {
  float currentAlt = getCurrentAlt();
  float stick = rcGetNormalized(CH_THROTTLE);
  if (fabsf(stick) > 0.1f) {
    targetAltitude += stick * 0.5f * dt;
    targetAltitude  = constrain(targetAltitude, 0.2f, 15.0f);
  }
  float correction = pidAltitude.compute(targetAltitude, currentAlt, dt);
  return constrain(baseThrottle + (correction / 1000.0f), 0.0f, 1.0f);
}
#endif

// ─────────────────────────────────────────────────
void loop() {
  uint32_t now = micros();

  // ── Fast loop 500Hz (2000 µs) ────────────────
  if (now - lastLoopUs >= LOOP_FAST_US) {
    float dt = (now - lastLoopUs) / 1000000.0f;
    lastLoopUs = now;
    dt = constrain(dt, 0.001f, 0.01f);

    // 1. Update sensor IMU & RC
    imuUpdate(dt);
    receiverUpdate();
    IMUData imu = imuGet();

    // 2. Safety & Crash Detection
    #ifdef ENABLE_SAFETY
      float accMag = sqrtf(imu.accX * imu.accX + imu.accY * imu.accY + imu.accZ * imu.accZ);
      safetyUpdate(imu.roll, imu.pitch, isArmed, accMag);
      SafetyData safety = safetyGet();

      if (isArmed && (safety.angleExceeded || safety.crashDetected)) {
        Serial.println("[SAFETY] EMERGENCY: Crash / Angle Exceeded! Emergency Disarm.");
        motorDisarm();
        isArmed = false;
        safetyNotifyDisarm();
        #ifdef ENABLE_NAV
          navSetMode(NAV_IDLE);
        #endif
        #ifdef ENABLE_BLACKBOX
          blackboxFlush();
        #endif
        resetAllPID();
        buzzerBeep(5, 50, 50);
        return;
      }
    #endif

    // 3. Failsafe RC
    if (rcIsFailsafe() && isArmed) {
      Serial.println("[FAILSAFE] RC signal hilang!");
      #ifdef ENABLE_NAV
        if (gpsHasFix()) { navStartRTH(); flightMode = MODE_GPS_HOLD; }
        else             { navStartLand(); flightMode = MODE_ALT_HOLD; }
      #elif defined(ENABLE_BARO)
        navStartLand(); flightMode = MODE_ALT_HOLD;
      #else
        motorDisarm(); isArmed = false;
      #endif
    }

    // 4. Pre-Arm Verification & Arming
    if (!isArmed && rcIsArmed()) {
      bool allowArm = false;

      #ifdef ENABLE_SAFETY
        bool isGpsMode = (flightMode == MODE_GPS_HOLD);
        bool gpsFix = false;
        #ifdef ENABLE_GPS
          gpsFix = gpsHasFix();
        #endif

        PreArmReason prearm = safetyCheckPreArm(imu.roll, imu.pitch, rcGetRaw(CH_THROTTLE), isGpsMode, gpsFix);
        if (prearm == PREARM_OK) {
          allowArm = true;
        } else {
          safetyPlayPreArmFailureBeep(prearm);
        }
      #else
        if (rcGetRaw(CH_THROTTLE) < RC_MIN + 100) allowArm = true;
      #endif

      if (allowArm) {
        motorArm();
        isArmed = true;
        #ifdef ENABLE_SAFETY
          safetyNotifyArm();
          buzzerBeep(1, 500, 0);
        #endif
        #ifdef ENABLE_GPS
          if (gpsHasFix()) gpsSetHome();
        #endif
        #ifdef ENABLE_BARO
          baroSetHome();
        #endif
        resetAllPID();
        #ifdef ENABLE_NAV
          navSetMode(NAV_IDLE);
        #endif
        Serial.println("[ARM] Armed - Siap Terbang");
      }
    }

    // Disarm
    if (isArmed && !rcIsArmed() && !rcIsFailsafe()) {
      motorDisarm();
      isArmed = false;
      #ifdef ENABLE_SAFETY
        safetyNotifyDisarm();
        buzzerBeep(2, 200, 100);
      #endif
      #ifdef ENABLE_NAV
        navSetMode(NAV_IDLE);
      #endif
      #ifdef ENABLE_BLACKBOX
        blackboxFlush();
      #endif
      Serial.println("[ARM] Disarmed");
    }

    // 5. Update flight mode
    if (isArmed) updateFlightMode();

    // 6. Setpoint dari Remote Control dengan Kurva RC Expo
    float setRoll, setPitch;
    if (flightMode == MODE_ACRO) {
      setRoll  =  rcGetNormalizedExpo(CH_ROLL,  RC_EXPO_ROLL_PITCH) * MAX_RATE_ROLL;
      setPitch = -rcGetNormalizedExpo(CH_PITCH, RC_EXPO_ROLL_PITCH) * MAX_RATE_PITCH;
    } else {
      setRoll  =  rcGetNormalizedExpo(CH_ROLL,  RC_EXPO_ROLL_PITCH) * MAX_ANGLE_ROLL;
      setPitch = -rcGetNormalizedExpo(CH_PITCH, RC_EXPO_ROLL_PITCH) * MAX_ANGLE_PITCH;
    }
    float setYaw   =  rcGetNormalizedExpo(CH_YAW,   RC_EXPO_YAW)        * MAX_RATE_YAW;
    float throttle = (float)(rcGetRaw(CH_THROTTLE) - RC_MIN) / (RC_MAX - RC_MIN);
    throttle = constrain(throttle, 0.0f, 1.0f);

    // 7. Override throttle (altitude hold)
    #ifdef ENABLE_BARO
      if (isArmed && flightMode != MODE_STABILIZE && flightMode != MODE_ACRO) {
        #ifdef ENABLE_NAV
          NavMode nm = navGetMode();
          if (nm == NAV_RTH_LAND || nm == NAV_LAND) {
            NavOutput nav = navGet();
            throttle = constrain(baseThrottle + nav.throttleCorrection, 0.0f, 0.6f);
          } else {
            throttle = computeAltHold(dt);
          }
        #else
          throttle = computeAltHold(dt);
        #endif
      }
    #endif

    // 8. Override setpoint (GPS Hold / RTH)
    #ifdef ENABLE_NAV
      if (isArmed && flightMode == MODE_GPS_HOLD) {
        NavOutput nav = navGet();
        setRoll  = constrain(setRoll  + nav.rollCorrection  * MAX_ANGLE_ROLL,
                             -MAX_ANGLE_ROLL,  MAX_ANGLE_ROLL);
        setPitch = constrain(setPitch + nav.pitchCorrection * MAX_ANGLE_PITCH,
                             -MAX_ANGLE_PITCH, MAX_ANGLE_PITCH);
      }
    #endif

    // 9. Hitung PID attitude (dengan AirMode)
    float outRoll = 0, outPitch = 0, outYaw = 0;
    static bool hasTakenOff = false;
    if (throttle > 0.15f) hasTakenOff = true;
    if (!isArmed) hasTakenOff = false;

    #ifdef ENABLE_AIRMODE
      bool runPID = isArmed && (throttle > 0.05f || hasTakenOff);
    #else
      bool runPID = isArmed && (throttle > 0.05f);
    #endif

    if (runPID) {
      if (flightMode == MODE_ACRO) {
        outRoll  = pidRoll.compute(setRoll,  imu.gyroX, dt);
        outPitch = pidPitch.compute(setPitch, imu.gyroY, dt);
      } else {
        outRoll  = pidRoll.compute(setRoll,  imu.roll,  dt);
        outPitch = pidPitch.compute(setPitch, imu.pitch, dt);
      }
      outYaw   = pidYaw.compute(setYaw,   -imu.gyroZ, dt); // Meredam yaw
    } else {
      pidRoll.reset(); pidPitch.reset(); pidYaw.reset();
    }

    // 10. Output ke motor (dengan Voltage Sag Compensation)
    if (isArmed) {
      float thrustMultiplier = 1.0f;
      #ifdef ENABLE_SAFETY
        thrustMultiplier = safetyGetThrustMultiplier();
      #endif

      motorWrite(throttle, outRoll, outPitch, outYaw, thrustMultiplier);

      #ifdef ENABLE_NAV
        if (navIsLanded()) {
          motorDisarm();
          isArmed = false;
          #ifdef ENABLE_SAFETY
            safetyNotifyDisarm();
            buzzerBeep(3, 300, 100);
          #endif
          Serial.println("[NAV] Landing selesai, auto-disarm.");
        }
      #endif
    } else {
      motorStop();
    }

    // 11. Debug Serial 10Hz
    if (millis() - lastDebugMs > 100) {
      lastDebugMs = millis();
      Serial.print("R:"); Serial.print(imu.roll, 1);
      Serial.print(" P:"); Serial.print(imu.pitch, 1);
      Serial.print(" Y:"); Serial.print(imu.yaw, 1);
      #ifdef ENABLE_BARO
        Serial.print(" | Alt:"); Serial.print(getCurrentAlt(), 2);
      #endif
      #ifdef ENABLE_SAFETY
        SafetyData sf = safetyGet();
        Serial.print(" | Batt:"); Serial.print(sf.battVoltage, 2); Serial.print("V");
      #endif
      Serial.print(" | Thr:"); Serial.print(throttle, 2);
      Serial.print(" | M:");
      Serial.print(motorGetPWM(0)); Serial.print(" ");
      Serial.print(motorGetPWM(1)); Serial.print(" ");
      Serial.print(motorGetPWM(2)); Serial.print(" ");
      Serial.print(motorGetPWM(3));
      Serial.print(" | "); Serial.println(isArmed ? "ARMED" : "DISARM");
    }
  }

  // ── Slow loop 50Hz (20 ms) ───────────────────
  if (millis() - lastSlowMs >= LOOP_SLOW_MS) {
    float dtSlow = LOOP_SLOW_MS / 1000.0f;
    lastSlowMs = millis();

    #ifdef ENABLE_BARO
      baroUpdate();
    #endif
    #ifdef ENABLE_TOF
      tofUpdate();
    #endif
    #ifdef ENABLE_GPS
      gpsUpdate();
    #endif
    #ifdef ENABLE_NAV
      if (isArmed) navUpdate(dtSlow);
    #endif
    #ifdef ENABLE_SAFETY
      buzzerUpdate();
      ledUpdate();
    #endif
    #ifdef ENABLE_WIFI_TUNING
      wifiTuningUpdate();
    #endif

    // Blackbox Flight Logging
    #ifdef ENABLE_BLACKBOX
      if (isArmed) {
        float battV = 0.0f;
        #ifdef ENABLE_SAFETY
          battV = safetyGet().battVoltage;
        #endif
        blackboxLog(millis(), imu.roll, imu.pitch, imu.yaw,
                    (float)(rcGetRaw(CH_THROTTLE) - RC_MIN) / (RC_MAX - RC_MIN),
                    motorGetPWM(0), motorGetPWM(1), motorGetPWM(2), motorGetPWM(3),
                    battV, flightMode);
      }
    #endif
  }
}
