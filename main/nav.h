#pragma once
#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_NAV

// Mode navigasi
enum NavMode {
  NAV_IDLE       = 0,   // tidak ada navigasi aktif
  NAV_GPS_HOLD   = 1,   // tahan posisi GPS
  NAV_RTH_CLIMB  = 2,   // naik ke ketinggian aman sebelum pulang
  NAV_RTH_FLY    = 3,   // terbang menuju home
  NAV_RTH_LAND   = 4,   // turun dan landing di home
  NAV_LAND       = 5,   // landing di posisi saat ini
};

struct NavOutput {
  float   rollCorrection;
  float   pitchCorrection;
  float   throttleCorrection;
  NavMode currentMode;
};

void      navInit();
void      navUpdate(float dt);
NavOutput navGet();

void      navSetMode(NavMode mode);
NavMode   navGetMode();

void      navHoldPosition();
void      navSetHoldTarget(double lat, double lng);

void      navStartRTH();
bool      navIsRTHComplete();

void      navStartLand();
bool      navIsLanded();

#endif // ENABLE_NAV
