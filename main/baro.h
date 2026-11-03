#pragma once
#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_BARO

struct BaroData {
  float pressure;      // hPa
  float temperature;   // Celsius
  float altitude;      // meter (relatif dari takeoff)
  float altitudeRaw;   // meter (absolut dari sea level)
  bool  healthy;
};

bool     baroInit();
void     baroUpdate();
void     baroSetHome();       // set titik 0 altitude (saat takeoff)
BaroData baroGet();

#endif // ENABLE_BARO
