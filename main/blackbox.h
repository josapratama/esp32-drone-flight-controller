#pragma once
#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_BLACKBOX

void     blackboxInit();
void     blackboxLog(uint32_t nowMs, float roll, float pitch, float yaw, float throttle,
                     int m1, int m2, int m3, int m4, float battV, int mode);
void     blackboxFlush();
void     blackboxClear();
size_t   blackboxGetSize();
String   blackboxGetSummary();

#endif // ENABLE_BLACKBOX

