#include "baro.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>

#ifdef ENABLE_BARO

static Adafruit_BMP280 bmp;
static BaroData        data;
static float           homeAltitude = 0;

// Low-pass filter untuk altitude (redam noise sensor)
static float altFiltered = 0;
#define BARO_LPF_ALPHA  0.85f   // 0=no filter, 1=max filter

bool baroInit() {
  if (!bmp.begin(0x76)) {
    // Coba alamat alternatif
    if (!bmp.begin(0x77)) {
      Serial.println("[BARO] GAGAL: BMP280 tidak terdeteksi");
      data.healthy = false;
      return false;
    }
  }

  // Setting sampling untuk drone (balance antara noise dan kecepatan)
  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,    // temperature
    Adafruit_BMP280::SAMPLING_X16,   // pressure
    Adafruit_BMP280::FILTER_X16,     // IIR filter
    Adafruit_BMP280::STANDBY_MS_1    // 1ms standby
  );

  data.healthy = true;

  // Baca beberapa kali untuk stabilisasi awal
  for (int i = 0; i < 20; i++) {
    baroUpdate();
    delay(10);
  }

  baroSetHome();
  Serial.printf("[BARO] OK, altitude awal: %.2f m\n", data.altitudeRaw);
  return true;
}

void baroUpdate() {
  if (!data.healthy) return;

  data.pressure    = bmp.readPressure() / 100.0f; // Pa → hPa
  data.temperature = bmp.readTemperature();

  // Hitung altitude dari tekanan (formula barometrik)
  // Referensi sea level 1013.25 hPa
  float rawAlt = 44330.0f * (1.0f - powf(data.pressure / 1013.25f, 0.1903f));

  // Low-pass filter untuk redam noise
  altFiltered = BARO_LPF_ALPHA * altFiltered + (1.0f - BARO_LPF_ALPHA) * rawAlt;

  data.altitudeRaw = altFiltered;
  data.altitude    = altFiltered - homeAltitude;
}

void baroSetHome() {
  homeAltitude     = data.altitudeRaw;
  data.altitude    = 0;
  Serial.printf("[BARO] Home altitude set: %.2f m\n", homeAltitude);
}

BaroData baroGet() {
  return data;
}

#endif // ENABLE_BARO
