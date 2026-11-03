#include "blackbox.h"

#ifdef ENABLE_BLACKBOX

#include <LittleFS.h>

static const char* LOG_PATH = "/flight_log.csv";
static File logFile;
static bool fsReady = false;
static char writeBuf[1024];
static size_t bufLen = 0;
static uint32_t lastFlushMs = 0;

void blackboxInit() {
  if (!LittleFS.begin(true)) {
    Serial.println("[BLACKBOX] LittleFS gagal dimount!");
    fsReady = false;
    return;
  }
  fsReady = true;

  // Cek apakah file sudah ada, jika belum tulis header CSV
  if (!LittleFS.exists(LOG_PATH)) {
    logFile = LittleFS.open(LOG_PATH, FILE_WRITE);
    if (logFile) {
      logFile.println("time_ms,roll,pitch,yaw,throttle,m1,m2,m3,m4,batt_v,mode");
      logFile.close();
    }
  }

  Serial.printf("[BLACKBOX] Ready! Log size saat ini: %u bytes\n", (unsigned int)blackboxGetSize());
}

void blackboxFlush() {
  if (!fsReady || bufLen == 0) return;

  logFile = LittleFS.open(LOG_PATH, FILE_APPEND);
  if (logFile) {
    logFile.write((const uint8_t*)writeBuf, bufLen);
    logFile.close();
  }
  bufLen = 0;
  lastFlushMs = millis();
}

void blackboxLog(uint32_t nowMs, float roll, float pitch, float yaw, float throttle,
                 int m1, int m2, int m3, int m4, float battV, int mode) {
  if (!fsReady) return;

  char line[128];
  int len = snprintf(line, sizeof(line), "%lu,%.1f,%.1f,%.1f,%.2f,%d,%d,%d,%d,%.2f,%d\n",
                     nowMs, roll, pitch, yaw, throttle, m1, m2, m3, m4, battV, mode);

  if (len > 0) {
    if (bufLen + len >= sizeof(writeBuf)) {
      blackboxFlush();
    }
    memcpy(writeBuf + bufLen, line, len);
    bufLen += len;
  }

  // Flush otomatis setiap 2 detik jika ada buffer
  if (millis() - lastFlushMs > 2000) {
    blackboxFlush();
  }
}

void blackboxClear() {
  if (!fsReady) return;
  bufLen = 0;
  LittleFS.remove(LOG_PATH);
  logFile = LittleFS.open(LOG_PATH, FILE_WRITE);
  if (logFile) {
    logFile.println("time_ms,roll,pitch,yaw,throttle,m1,m2,m3,m4,batt_v,mode");
    logFile.close();
  }
  Serial.println("[BLACKBOX] Log dibersihkan.");
}

size_t blackboxGetSize() {
  if (!fsReady || !LittleFS.exists(LOG_PATH)) return 0;
  File f = LittleFS.open(LOG_PATH, FILE_READ);
  if (!f) return 0;
  size_t sz = f.size();
  f.close();
  return sz;
}

String blackboxGetSummary() {
  size_t total = LittleFS.totalBytes();
  size_t used  = LittleFS.usedBytes();
  size_t logSz = blackboxGetSize();
  return "Log Size: " + String(logSz / 1024.0f, 1) + " KB | Flash Used: " +
         String(used / 1024.0f, 1) + " / " + String(total / 1024.0f, 1) + " KB";
}

#endif // ENABLE_BLACKBOX

