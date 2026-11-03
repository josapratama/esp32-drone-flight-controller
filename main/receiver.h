#pragma once
#include <Arduino.h>

struct RCData {
  uint16_t channel[10];   // nilai raw 1000-2000
  bool     available;
  bool     failsafe;
  uint32_t lastPacketMs;
};

void   receiverInit();
void   receiverUpdate();
RCData receiverGet();

// Ambil nilai channel sudah di-map ke range tertentu
float  rcGetNormalized(int ch);                      // -1.0 ~ 1.0 (linear)
float  rcApplyExpo(float input, float expo);         // Rumus kurva eksponensial
float  rcGetNormalizedExpo(int ch, float expo);      // -1.0 ~ 1.0 dengan kurva expo
int    rcGetRaw(int ch);                             // 1000 ~ 2000
bool   rcIsArmed();                                  // cek switch arm
bool   rcIsFailsafe();                               // cek failsafe
