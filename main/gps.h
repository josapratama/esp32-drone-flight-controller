#pragma once
#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_GPS

struct GPSData {
  double   lat;           // latitude
  double   lng;           // longitude
  float    altitudeM;     // altitude GPS (meter)
  float    speedKmh;      // kecepatan
  float    heading;       // arah (0-360 derajat)
  uint8_t  satellites;    // jumlah satelit
  bool     fix;           // apakah ada GPS fix
  bool     healthy;
  uint32_t lastFixMs;
};

struct GPSPoint {
  double lat;
  double lng;
};

bool    gpsInit();
void    gpsUpdate();
GPSData gpsGet();

void      gpsSetHome();           // simpan posisi home (saat arm)
GPSPoint  gpsGetHome();
bool      gpsHasFix();
float     gpsDistanceTo(double lat, double lng);   // jarak ke titik (meter)
float     gpsBearingTo(double lat, double lng);    // arah ke titik (derajat)

#endif // ENABLE_GPS
