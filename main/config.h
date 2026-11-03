#pragma once

// ═══════════════════════════════════════════════
// FEATURE FLAGS
// Comment untuk nonaktifkan fitur, uncomment untuk aktifkan.
// Semua kode terkait fitur yang di-comment tidak akan dikompilasi.
// ═══════════════════════════════════════════════

// #define ENABLE_BARO               // BMP280  — altitude hold
// #define ENABLE_TOF                // VL53L1X — altitude akurat jarak dekat
// #define ENABLE_GPS                // Neo M8N — GPS hold & RTH
// #define ENABLE_NAV                // GPS Hold & RTH (butuh ENABLE_GPS)
// #define ENABLE_SAFETY             // battery monitor, crash detect, angle limit, buzzer, LED

#define ENABLE_AIRMODE               // Kendali sikap drone penuh saat throttle nol di udara
#define ENABLE_VOLTAGE_COMPENSATION  // Kompensasi daya dorong otomatis saat voltase baterai turun
#define ENABLE_BLACKBOX              // Rekam telemetri & PID ke Flash internal (LittleFS)
#define ENABLE_LOST_BEACON           // Alarm beeper otomatis jika hilang > 2 menit
#define ENABLE_GEOFENCE              // Batas radius & ketinggian virtual drone
// #define ENABLE_WIFI_TUNING        // Hotspot Web Tuning "Drone-FC-Setup" (IP 192.168.4.1)

// Catatan dependensi:
//   ENABLE_NAV   butuh ENABLE_GPS aktif
//   ENABLE_NAV   butuh ENABLE_BARO aktif (untuk RTH climb)
//   MODE_ALT_HOLD butuh ENABLE_BARO aktif
//   MODE_GPS_HOLD butuh ENABLE_GPS + ENABLE_NAV aktif

// ═══════════════════════════════════════════════
// PIN MAPPING — ESP32-S3 N16R8
// ═══════════════════════════════════════════════
#define PIN_M1   4   // Front Left  CCW
#define PIN_M2   5   // Front Right CW
#define PIN_M3   6   // Rear Left   CW
#define PIN_M4   7   // Rear Right  CCW

#define PIN_SDA  8
#define PIN_SCL  9

#define PIN_IBUS_RX 13   // UART1 RX
#define PIN_GPS_RX  18   // UART2 RX (ESP32-S3, GPIO 16/17 dihindari karena internal PSRAM)
#define PIN_GPS_TX  15   // UART2 TX (ESP32-S3)

// ═══════════════════════════════════════════════
// ESC
// ═══════════════════════════════════════════════
#define ESC_MIN   1000   // microseconds
#define ESC_MAX   2000
#define ESC_ARM    700
#define ESC_IDLE  1100   // throttle saat arm, motor berputar pelan

// ═══════════════════════════════════════════════
// LOOP TIMING
// ═══════════════════════════════════════════════
#define LOOP_FAST_US   2000   // 500Hz  → IMU, PID, Motor
#define LOOP_SLOW_MS     20   // 50Hz   → Baro, ToF, GPS, Web Tuning, Blackbox

// ═══════════════════════════════════════════════
// IMU FILTER & D-TERM PT1
// ═══════════════════════════════════════════════
#define CF_ALPHA            0.98f    // 98% gyro + 2% accelerometer
#define GYRO_DEADBAND       0.5f     // deg/s
#define D_FILTER_CUTOFF_HZ  35.0f    // PT1 filter untuk redam getaran motor pada D-term

// ═══════════════════════════════════════════════
// PID — ROLL & PITCH & YAW
// ═══════════════════════════════════════════════
#define ROLL_KP   1.0f
#define ROLL_KI   0.02f
#define ROLL_KD   0.1f

#define PITCH_KP  1.0f
#define PITCH_KI  0.02f
#define PITCH_KD  0.1f

#define YAW_KP    2.0f
#define YAW_KI    0.02f
#define YAW_KD    0.0f

#define ALT_KP    1.5f
#define ALT_KI    0.05f
#define ALT_KD    0.8f

// ═══════════════════════════════════════════════
// PID LIMITS
// ═══════════════════════════════════════════════
#define PID_MAX_ROLL    300.0f
#define PID_MAX_PITCH   300.0f
#define PID_MAX_YAW     200.0f
#define PID_MAX_ALT     200.0f
#define PID_ITERM_MAX   100.0f   // anti windup

// ═══════════════════════════════════════════════
// RECEIVER & RC EXPO
// ═══════════════════════════════════════════════
#define RC_MIN              1000
#define RC_MAX              2000
#define RC_MID              1500
#define RC_DEADBAND           20   // deadband tengah stick

// Kurva kurva eksponensial stick (0.0 = linear murni, 0.35 = halus di tengah, lincah di tepi)
#define RC_EXPO_ROLL_PITCH  0.35f
#define RC_EXPO_YAW         0.25f

// Channel mapping iBUS
#define CH_ROLL       0
#define CH_PITCH      1
#define CH_THROTTLE   2
#define CH_YAW        3
#define CH_ARM        4   // SWA: < 1500 = disarm, > 1500 = arm
#define CH_MODE       5   // SWB: mode terbang

// ═══════════════════════════════════════════════
// ANGLE & RATE LIMITS
// ═══════════════════════════════════════════════
#define MAX_ANGLE_ROLL   35.0f   // derajat (Angle Mode)
#define MAX_ANGLE_PITCH  35.0f
#define MAX_RATE_ROLL   200.0f   // deg/s (Acro Mode)
#define MAX_RATE_PITCH  200.0f   // deg/s (Acro Mode)
#define MAX_RATE_YAW    150.0f   // deg/s

// ═══════════════════════════════════════════════
// GEOFENCE & BEACON
// ═══════════════════════════════════════════════
#define GEOFENCE_MAX_RADIUS_M  50.0f   // Radius terbang maksimal 50 meter dari titik Home
#define GEOFENCE_MAX_ALT_M     25.0f   // Ketinggian maksimal 25 meter
#define BEACON_TIMEOUT_MS      120000  // Alarm beeper berbunyi jika drone hilang > 2 menit

// ═══════════════════════════════════════════════
// FAILSAFE
// ═══════════════════════════════════════════════
#define FAILSAFE_TIMEOUT_MS  1000   // motor stop jika RC hilang > 1 detik

// ═══════════════════════════════════════════════
// FLIGHT MODES
// ═══════════════════════════════════════════════
#define MODE_STABILIZE   0   // Angle mode (Auto-leveling)
#define MODE_ACRO        1   // Rate mode (FPV Freestyle / manual penuh)
#define MODE_ALT_HOLD    2   // Altitude Hold (Baro / ToF)
#define MODE_GPS_HOLD    3   // GPS Position Hold
