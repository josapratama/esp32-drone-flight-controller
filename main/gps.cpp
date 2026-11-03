#include "gps.h"
#include "config.h"
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

#ifdef ENABLE_GPS

static HardwareSerial gpsSerial(2); // UART2
static TinyGPSPlus    gps;
static GPSData        data;
static GPSPoint       homePoint;
static bool           homeSet = false;

bool gpsInit() {
  gpsSerial.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);

  data.healthy    = true;
  data.fix        = false;
  data.satellites = 0;
  data.lastFixMs  = 0;
  homePoint       = {0, 0};

  Serial.println("[GPS] Init OK, menunggu fix...");
  return true;
}

void gpsUpdate() {
  if (!data.healthy) return;

  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // Update data jika ada fix valid
  if (gps.location.isValid() && gps.location.age() < 2000) {
    data.lat       = gps.location.lat();
    data.lng       = gps.location.lng();
    data.fix       = true;
    data.lastFixMs = millis();
  } else {
    // Fix hilang jika data lebih dari 3 detik
    if (millis() - data.lastFixMs > 3000) {
      data.fix = false;
    }
  }

  if (gps.altitude.isValid())   data.altitudeM  = gps.altitude.meters();
  if (gps.speed.isValid())      data.speedKmh   = gps.speed.kmph();
  if (gps.course.isValid())     data.heading    = gps.course.deg();
  if (gps.satellites.isValid()) data.satellites = gps.satellites.value();
}

GPSData gpsGet() {
  return data;
}

void gpsSetHome() {
  if (!data.fix) {
    Serial.println("[GPS] WARNING: Set home tanpa GPS fix!");
    return;
  }
  homePoint.lat = data.lat;
  homePoint.lng = data.lng;
  homeSet       = true;
  Serial.printf("[GPS] Home set: %.6f, %.6f\n", homePoint.lat, homePoint.lng);
}

GPSPoint gpsGetHome() {
  return homePoint;
}

bool gpsHasFix() {
  return data.fix && data.satellites >= 6;
}

// Hitung jarak ke titik menggunakan Haversine formula
float gpsDistanceTo(double lat2, double lng2) {
  const float R = 6371000.0f; // radius bumi meter
  float dLat = radians(lat2 - data.lat);
  float dLng = radians(lng2 - data.lng);
  float a = sinf(dLat / 2) * sinf(dLat / 2) +
            cosf(radians(data.lat)) * cosf(radians(lat2)) *
            sinf(dLng / 2) * sinf(dLng / 2);
  float c = 2 * atan2f(sqrtf(a), sqrtf(1 - a));
  return R * c;
}

// Hitung bearing (arah) ke titik dalam derajat (0=Utara, 90=Timur)
float gpsBearingTo(double lat2, double lng2) {
  float dLng  = radians(lng2 - data.lng);
  float lat1r = radians(data.lat);
  float lat2r = radians(lat2);
  float x = sinf(dLng) * cosf(lat2r);
  float y = cosf(lat1r) * sinf(lat2r) - sinf(lat1r) * cosf(lat2r) * cosf(dLng);
  float bearing = degrees(atan2f(x, y));
  return fmodf(bearing + 360.0f, 360.0f);
}

#endif // ENABLE_GPS
