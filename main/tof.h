#pragma once
#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_TOF

struct ToFData {
  float    distanceCm;    // jarak ke tanah dalam cm
  float    distanceM;     // jarak ke tanah dalam meter
  bool     valid;         // apakah pembacaan valid
  bool     healthy;       // apakah sensor OK
  uint32_t lastUpdateMs;
};

bool    tofInit();
void    tofUpdate();
ToFData tofGet();

// Apakah jarak valid untuk dipakai altitude hold
// (VL53L1X akurat sampai ~400cm, pakai di bawah 3m)
bool    tofIsReliable();

#endif // ENABLE_TOF
