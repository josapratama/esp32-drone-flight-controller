#pragma once
#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_WIFI_TUNING

#include "pid.h"

void wifiTuningInit(PID &roll, PID &pitch, PID &yaw);
void wifiTuningUpdate();
void wifiTuningSavePIDToNVS();
void wifiTuningLoadPIDFromNVS(PID &roll, PID &pitch, PID &yaw);

#endif // ENABLE_WIFI_TUNING

