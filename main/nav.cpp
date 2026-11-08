#include "nav.h"
#include "config.h"
#include "gps.h"
#include "imu.h"
#include "baro.h"
#include "tof.h"
#include "pid.h"

#ifdef ENABLE_NAV

// ── Konfigurasi navigasi ─────────────────────────
#define NAV_MAX_LEAN_ANGLE    20.0f   // derajat, max kemiringan saat navigasi
#define NAV_SPEED_MAX         3.0f    // m/s, kecepatan max GPS hold
#define NAV_ACCEPT_RADIUS     1.5f    // meter, radius dianggap sudah di titik
#define RTH_SAFE_ALTITUDE     15.0f   // meter, ketinggian aman sebelum RTH (di atas pepohonan/atap)
#define RTH_LAND_RADIUS       1.5f    // meter, mulai landing jika dalam radius ini
#define LAND_DESCENT_RATE     0.3f    // m/s, kecepatan turun saat landing
#define LAND_DETECT_ALT       0.15f   // meter, dianggap sudah landed

// ── PID posisi (output = kecepatan target) ───────
static PID pidPosNorth(1.2f, 0.0f, 0.3f, 50.0f, NAV_SPEED_MAX);
static PID pidPosEast (1.2f, 0.0f, 0.3f, 50.0f, NAV_SPEED_MAX);

// ── PID kecepatan (output = sudut lean) ──────────
static PID pidVelNorth(0.8f, 0.05f, 0.1f, 30.0f, NAV_MAX_LEAN_ANGLE);
static PID pidVelEast (0.8f, 0.05f, 0.1f, 30.0f, NAV_MAX_LEAN_ANGLE);

// ── State ─────────────────────────────────────────
static NavMode    currentMode   = NAV_IDLE;
static NavOutput  navOut        = {0, 0, 0, NAV_IDLE};

static double     holdLat       = 0;
static double     holdLng       = 0;
static bool       holdSet       = false;

static float      rthAltTarget  = RTH_SAFE_ALTITUDE;
static bool       rthClimbDone  = false;
static bool       rthLanded     = false;
static bool       landed        = false;

// Estimasi kecepatan dari GPS (m/s, North & East)
static float      velNorth      = 0;
static float      velEast       = 0;
static double     prevLat       = 0;
static double     prevLng       = 0;
static uint32_t   prevVelMs     = 0;

// ─────────────────────────────────────────────────
// Konversi bearing + jarak ke komponen North/East
// ─────────────────────────────────────────────────
static void distanceToNE(double lat1, double lng1,
                          double lat2, double lng2,
                          float &north, float &east) {
  const float R = 6371000.0f;
  north = (float)(radians(lat2 - lat1) * R);
  east  = (float)(radians(lng2 - lng1) * R * cosf(radians(lat1)));
}

// ─────────────────────────────────────────────────
// Rotasi koreksi dari frame dunia ke frame drone
// (kompensasi heading drone)
// ─────────────────────────────────────────────────
static void rotateToBody(float north, float east,
                          float heading,
                          float &roll, float &pitch) {
  float headRad = radians(heading);
  // Pitch = maju/mundur (North), Roll = kiri/kanan (East)
  pitch = -(north * cosf(headRad) + east * sinf(headRad));
  roll  =  (-north * sinf(headRad) + east * cosf(headRad));
}

// ─────────────────────────────────────────────────
void navInit() {
  currentMode = NAV_IDLE;
  holdSet     = false;
  rthLanded   = false;
  landed      = false;
  Serial.println("[NAV] Init OK");
}

void navSetMode(NavMode mode) {
  if (mode == currentMode) return;
  Serial.printf("[NAV] Mode: %d → %d\n", currentMode, mode);

  // Reset PID saat ganti mode
  pidPosNorth.reset(); pidPosEast.reset();
  pidVelNorth.reset(); pidVelEast.reset();

  currentMode = mode;
  navOut      = {0, 0, 0, mode};
}

NavMode navGetMode() {
  return currentMode;
}

void navHoldPosition() {
  GPSData gps = gpsGet();
  if (!gps.fix) {
    Serial.println("[NAV] GPS Hold gagal: tidak ada fix");
    return;
  }
  holdLat = gps.lat;
  holdLng = gps.lng;
  holdSet = true;
  pidPosNorth.reset(); pidPosEast.reset();
  pidVelNorth.reset(); pidVelEast.reset();
  Serial.printf("[NAV] Hold position: %.6f, %.6f\n", holdLat, holdLng);
}

void navSetHoldTarget(double lat, double lng) {
  holdLat = lat;
  holdLng = lng;
  holdSet = true;
}

void navStartRTH() {
  GPSData gps = gpsGet();
  if (!gps.fix) {
    Serial.println("[NAV] RTH gagal: tidak ada GPS fix");
    return;
  }
  rthClimbDone = false;
  rthLanded    = false;

  // Tentukan ketinggian aman RTH
  BaroData baro = baroGet();
  rthAltTarget  = max(baro.altitude, RTH_SAFE_ALTITUDE);

  navSetMode(NAV_RTH_CLIMB);
  Serial.printf("[NAV] RTH dimulai, naik ke %.1f m\n", rthAltTarget);
}

bool navIsRTHComplete() {
  return rthLanded;
}

void navStartLand() {
  navSetMode(NAV_LAND);
  Serial.println("[NAV] Landing dimulai");
}

bool navIsLanded() {
  return landed;
}

// ─────────────────────────────────────────────────
// Update estimasi kecepatan dari GPS
// ─────────────────────────────────────────────────
static void updateVelocity() {
  GPSData gps = gpsGet();
  if (!gps.fix) return;

  uint32_t now = millis();
  float dt = (now - prevVelMs) / 1000.0f;

  if (prevVelMs > 0 && dt > 0.1f && dt < 2.0f) {
    float dNorth, dEast;
    distanceToNE(prevLat, prevLng, gps.lat, gps.lng, dNorth, dEast);
    // Low-pass filter kecepatan
    velNorth = 0.7f * velNorth + 0.3f * (dNorth / dt);
    velEast  = 0.7f * velEast  + 0.3f * (dEast  / dt);
  }

  prevLat   = gps.lat;
  prevLng   = gps.lng;
  prevVelMs = now;
}

// ─────────────────────────────────────────────────
// GPS Hold logic
// ─────────────────────────────────────────────────
static void updateGPSHold(float dt) {
  GPSData gps = gpsGet();
  if (!gps.fix || !holdSet) {
    navOut.rollCorrection  = 0;
    navOut.pitchCorrection = 0;
    return;
  }

  // Hitung error posisi (meter)
  float errNorth, errEast;
  distanceToNE(gps.lat, gps.lng, holdLat, holdLng, errNorth, errEast);

  // PID posisi → target kecepatan
  float targetVelNorth = pidPosNorth.compute(0, -errNorth, dt);
  float targetVelEast  = pidPosEast.compute(0, -errEast,  dt);

  // PID kecepatan → sudut lean
  float leanNorth = pidVelNorth.compute(targetVelNorth, velNorth, dt);
  float leanEast  = pidVelEast.compute(targetVelEast,  velEast,  dt);

  // Rotasi ke frame drone (kompensasi heading menggunakan sensor IMU yaw sejati)
  float rollCorr, pitchCorr;
  IMUData imu = imuGet();
  rotateToBody(leanNorth, leanEast, imu.yaw, rollCorr, pitchCorr);

  // Normalisasi ke -1 ~ 1
  navOut.rollCorrection  = constrain(rollCorr  / NAV_MAX_LEAN_ANGLE, -1.0f, 1.0f);
  navOut.pitchCorrection = constrain(pitchCorr / NAV_MAX_LEAN_ANGLE, -1.0f, 1.0f);
}

// ─────────────────────────────────────────────────
// Landing logic (dipakai RTH dan manual land)
// ─────────────────────────────────────────────────
static void updateRTH_Land(float dt, float currentAlt) {
  // Tahan posisi horizontal saat turun
  if (holdSet) updateGPSHold(dt);

  // Turun pelan
  navOut.throttleCorrection = -LAND_DESCENT_RATE * dt;

  // Deteksi sudah landed
  if (currentAlt < LAND_DETECT_ALT) {
    landed    = true;
    rthLanded = true;
    navSetMode(NAV_IDLE);
    Serial.println("[NAV] Landed!");
  }
}

// ─────────────────────────────────────────────────
// RTH logic
// ─────────────────────────────────────────────────
static void updateRTH(float dt) {
  GPSData  gps  = gpsGet();
  BaroData baro = baroGet();
  ToFData  tof  = tofGet();

  float currentAlt = tofIsReliable() ? tof.distanceM : baro.altitude;

  switch (currentMode) {

    case NAV_RTH_CLIMB:
      // Naik dulu ke ketinggian aman
      navOut.rollCorrection  = 0;
      navOut.pitchCorrection = 0;
      if (currentAlt >= rthAltTarget - 0.3f) {
        rthClimbDone = true;
        // Set target ke posisi home
        GPSPoint home = gpsGetHome();
        navSetHoldTarget(home.lat, home.lng);
        navSetMode(NAV_RTH_FLY);
        Serial.println("[NAV] RTH: ketinggian aman, terbang ke home");
      }
      break;

    case NAV_RTH_FLY: {
      // Terbang ke home
      GPSPoint home = gpsGetHome();
      float dist = gpsDistanceTo(home.lat, home.lng);

      updateGPSHold(dt); // pakai GPS hold untuk navigasi ke home

      if (dist < RTH_LAND_RADIUS) {
        navSetMode(NAV_RTH_LAND);
        Serial.println("[NAV] RTH: sudah di home, mulai landing");
      }
      break;
    }

    case NAV_RTH_LAND:
      // Landing di home
      updateRTH_Land(dt, currentAlt);
      break;

    default:
      break;
  }
}

// ─────────────────────────────────────────────────
void navUpdate(float dt) {
  updateVelocity();

  navOut.throttleCorrection = 0;
  navOut.currentMode        = currentMode;

  switch (currentMode) {
    case NAV_GPS_HOLD:
      updateGPSHold(dt);
      break;

    case NAV_RTH_CLIMB:
    case NAV_RTH_FLY:
    case NAV_RTH_LAND:
      updateRTH(dt);
      break;

    case NAV_LAND: {
      BaroData baro = baroGet();
      ToFData  tof  = tofGet();
      float alt = tofIsReliable() ? tof.distanceM : baro.altitude;
      updateRTH_Land(dt, alt);
      break;
    }

    case NAV_IDLE:
    default:
      navOut.rollCorrection  = 0;
      navOut.pitchCorrection = 0;
      break;
  }
}

NavOutput navGet() {
  return navOut;
}

#endif // ENABLE_NAV
