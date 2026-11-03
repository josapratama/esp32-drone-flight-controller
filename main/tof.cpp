#include "tof.h"
#include "config.h"
#include <Wire.h>
#include <VL53L1X.h>

#ifdef ENABLE_TOF

static VL53L1X sensor;
static ToFData data;

// Batas jarak yang dianggap reliable untuk altitude hold
#define TOF_MAX_RELIABLE_CM  300   // 3 meter
#define TOF_MIN_CM             2   // minimum valid (hindari false 0)

bool tofInit() {
  sensor.setTimeout(500);

  if (!sensor.init()) {
    Serial.println("[TOF] GAGAL: VL53L1X tidak terdeteksi");
    data.healthy = false;
    return false;
  }

  // Mode jarak: SHORT (lebih akurat, max ~1.3m) atau LONG (max ~4m)
  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(33000); // 33ms per pengukuran

  sensor.startContinuous(50); // baca setiap 50ms

  data.healthy      = true;
  data.valid        = false;
  data.distanceCm   = 0;
  data.distanceM    = 0;
  data.lastUpdateMs = 0;

  Serial.println("[TOF] OK");
  return true;
}

void tofUpdate() {
  if (!data.healthy) return;
  if (!sensor.dataReady()) return;

  uint16_t raw = sensor.read(false); // non-blocking

  if (sensor.ranging_data.range_status == VL53L1X::RangeValid) {
    data.distanceCm   = raw / 10.0f;  // mm → cm
    data.distanceM    = raw / 1000.0f; // mm → m
    data.valid        = (data.distanceCm > TOF_MIN_CM);
    data.lastUpdateMs = millis();
  } else {
    data.valid = false;
  }
}

ToFData tofGet() {
  return data;
}

bool tofIsReliable() {
  if (!data.healthy || !data.valid) return false;
  if (millis() - data.lastUpdateMs > 200) return false; // data stale
  return data.distanceCm < TOF_MAX_RELIABLE_CM;
}

#endif // ENABLE_TOF
