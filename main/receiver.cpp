#include "receiver.h"
#include "config.h"
#include <HardwareSerial.h>

static HardwareSerial ibusSerial(1);
static RCData rcData;

static uint8_t  buf[32];
static int      bufIdx = 0;

void receiverInit() {
  ibusSerial.begin(115200, SERIAL_8N1, PIN_IBUS_RX, -1);

  rcData.available    = false;
  rcData.failsafe     = true;
  rcData.lastPacketMs = 0;

  // Set nilai default aman
  for (int i = 0; i < 10; i++) {
    rcData.channel[i] = RC_MID;
  }
  rcData.channel[CH_THROTTLE] = RC_MIN; // throttle default bawah
  rcData.channel[CH_ARM]      = RC_MIN; // default disarm

  Serial.println("[RC] Receiver init OK");
}

static bool parsePacket() {
  if (buf[0] != 0x20 || buf[1] != 0x40) return false;

  // Verifikasi checksum
  uint16_t checksum = 0xFFFF;
  for (int i = 0; i < 30; i++) checksum -= buf[i];

  uint16_t rxCheck = buf[30] | (buf[31] << 8);
  if (checksum != rxCheck) return false;

  for (int i = 0; i < 10; i++) {
    rcData.channel[i] = buf[2 + i * 2] | (buf[3 + i * 2] << 8);
    // Clamp ke range valid
    rcData.channel[i] = constrain(rcData.channel[i], 900, 2100);
  }

  rcData.available    = true;
  rcData.failsafe     = false;
  rcData.lastPacketMs = millis();
  return true;
}

void receiverUpdate() {
  // Cek failsafe
  if (millis() - rcData.lastPacketMs > FAILSAFE_TIMEOUT_MS) {
    rcData.failsafe = true;
  }

  // Baca data serial dengan verifikasi sinkronisasi 2-byte header (0x20 0x40)
  while (ibusSerial.available()) {
    uint8_t c = ibusSerial.read();

    if (bufIdx == 0) {
      if (c == 0x20) buf[bufIdx++] = c; // Byte 1 header iBUS
    } else if (bufIdx == 1) {
      if (c == 0x40) {
        buf[bufIdx++] = c;              // Byte 2 header iBUS
      } else {
        bufIdx = (c == 0x20) ? 1 : 0;   // Resync cepat jika ada derau
      }
    } else {
      buf[bufIdx++] = c;
      if (bufIdx == 32) {
        parsePacket();
        bufIdx = 0;
      }
    }
  }
}

RCData receiverGet() {
  return rcData;
}

float rcApplyExpo(float input, float expo) {
  expo = constrain(expo, 0.0f, 1.0f);
  // Kurva kubik simetris: (1 - e)*x + e*x^3
  return (1.0f - expo) * input + expo * (input * input * input);
}

float rcGetNormalized(int ch) {
  int raw = rcData.channel[ch];
  int mid = RC_MID;

  // Terapkan deadband tengah
  if (abs(raw - mid) < RC_DEADBAND) return 0.0f;

  if (raw < mid) {
    return (float)(raw - mid + RC_DEADBAND) / (float)(mid - RC_MIN - RC_DEADBAND);
  } else {
    return (float)(raw - mid - RC_DEADBAND) / (float)(RC_MAX - mid - RC_DEADBAND);
  }
}

float rcGetNormalizedExpo(int ch, float expo) {
  float linear = rcGetNormalized(ch);
  return rcApplyExpo(linear, expo);
}

int rcGetRaw(int ch) {
  return rcData.channel[ch];
}

bool rcIsArmed() {
  return rcData.channel[CH_ARM] > RC_MID && !rcData.failsafe;
}

bool rcIsFailsafe() {
  return rcData.failsafe;
}
