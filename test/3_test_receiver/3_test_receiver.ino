/*
 * TEST 3 - RECEIVER FS-iA6B (iBUS)
 * Baca semua channel dari receiver via protokol iBUS.
 * Pastikan receiver sudah ter-bind dengan transmitter.
 *
 * Wiring:
 *   iBUS Signal → GPIO 33 (UART1 RX)
 *   VCC         → 5V BEC
 *   GND         → GND ESP32
 *
 * Channel default FlySky:
 *   CH1 → Aileron  (Roll)
 *   CH2 → Elevator (Pitch)
 *   CH3 → Throttle
 *   CH4 → Rudder   (Yaw)
 *   CH5 → SWA (switch)
 *   CH6 → SWB (switch)
 */

#include <HardwareSerial.h>

HardwareSerial ibusSerial(1); // UART1

#define IBUS_BUFFSIZE  32
#define IBUS_CHANNELS  10

uint8_t  ibusBuffer[IBUS_BUFFSIZE];
uint16_t channels[IBUS_CHANNELS];
int      bufIndex = 0;

// Parse paket iBUS
bool parseIBUS() {
  // iBUS packet: 0x20 0x40 + 20 bytes data + 2 bytes checksum
  if (ibusBuffer[0] != 0x20 || ibusBuffer[1] != 0x40) return false;

  uint16_t checksum = 0xFFFF;
  for (int i = 0; i < 30; i++) {
    checksum -= ibusBuffer[i];
  }

  uint16_t rxChecksum = ibusBuffer[30] | (ibusBuffer[31] << 8);
  if (checksum != rxChecksum) return false;

  for (int i = 0; i < IBUS_CHANNELS; i++) {
    channels[i] = ibusBuffer[2 + i * 2] | (ibusBuffer[3 + i * 2] << 8);
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  ibusSerial.begin(115200, SERIAL_8N1, 33, -1); // RX=33, TX tidak dipakai
  delay(500);

  Serial.println("=== TEST RECEIVER iBUS ===");
  Serial.println("Gerakkan stick transmitter, nilai channel akan berubah");
  Serial.println("Range normal: 1000 (min) - 2000 (max), tengah ~1500");
  Serial.println();
}

unsigned long lastPrint = 0;

void loop() {
  while (ibusSerial.available()) {
    uint8_t c = ibusSerial.read();

    if (bufIndex == 0 && c != 0x20) return; // tunggu header
    ibusBuffer[bufIndex++] = c;

    if (bufIndex == 32) {
      if (parseIBUS()) {
        // Print setiap 200ms agar tidak flood
        if (millis() - lastPrint > 200) {
          lastPrint = millis();

          Serial.println("──────────────────────────────");
          Serial.print("CH1 Roll     : "); Serial.println(channels[0]);
          Serial.print("CH2 Pitch    : "); Serial.println(channels[1]);
          Serial.print("CH3 Throttle : "); Serial.println(channels[2]);
          Serial.print("CH4 Yaw      : "); Serial.println(channels[3]);
          Serial.print("CH5 SWA      : "); Serial.println(channels[4]);
          Serial.print("CH6 SWB      : "); Serial.println(channels[5]);

          // Cek apakah nilai dalam range normal
          bool ok = true;
          for (int i = 0; i < 6; i++) {
            if (channels[i] < 900 || channels[i] > 2100) {
              ok = false;
              Serial.print("WARNING: CH");
              Serial.print(i + 1);
              Serial.println(" di luar range!");
            }
          }
          if (ok) Serial.println("STATUS: OK");
        }
      }
      bufIndex = 0;
    }
  }
}
