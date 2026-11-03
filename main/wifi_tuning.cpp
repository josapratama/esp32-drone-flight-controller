#include "wifi_tuning.h"

#ifdef ENABLE_WIFI_TUNING

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "imu.h"
#include "motor.h"
#include "receiver.h"

#ifdef ENABLE_SAFETY
  #include "safety.h"
#endif

#ifdef ENABLE_BLACKBOX
  #include <LittleFS.h>
  #include "blackbox.h"
#endif

static WebServer server(80);
static PID* pRoll = nullptr;
static PID* pPitch = nullptr;
static PID* pYaw = nullptr;

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Drone FC - Live Tuning</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: #0f172a; color: #f8fafc; padding: 16px; }
    .container { max-width: 500px; margin: 0 auto; }
    h1 { font-size: 1.25rem; font-weight: 700; color: #38bdf8; margin-bottom: 12px; display: flex; justify-content: space-between; align-items: center; }
    .badge { font-size: 0.75rem; padding: 3px 8px; border-radius: 9999px; background: #334155; }
    .badge.armed { background: #ef4444; color: #fff; }
    .badge.disarm { background: #10b981; color: #fff; }
    .card { background: #1e293b; border-radius: 12px; padding: 14px; margin-bottom: 14px; border: 1px solid #334155; }
    .card-title { font-size: 0.85rem; font-weight: 600; color: #94a3b8; text-transform: uppercase; margin-bottom: 10px; }
    .grid-3 { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; text-align: center; }
    .metric { background: #0f172a; padding: 8px; border-radius: 8px; }
    .metric-val { font-size: 1.1rem; font-weight: 700; color: #38bdf8; }
    .metric-lbl { font-size: 0.7rem; color: #64748b; margin-top: 2px; }
    .form-group { margin-bottom: 10px; }
    .form-group label { display: flex; justify-content: space-between; font-size: 0.8rem; color: #cbd5e1; margin-bottom: 4px; }
    input[type=number] { width: 100%; background: #0f172a; border: 1px solid #475569; color: #fff; padding: 8px; border-radius: 6px; font-size: 0.9rem; }
    .btn { display: block; width: 100%; padding: 10px; border-radius: 8px; font-weight: 600; font-size: 0.9rem; border: none; cursor: pointer; margin-top: 8px; }
    .btn-primary { background: #0284c7; color: #fff; }
    .btn-primary:active { background: #0369a1; }
    .btn-secondary { background: #334155; color: #cbd5e1; }
    .btn-warning { background: #d97706; color: #fff; }
    #msg { font-size: 0.8rem; text-align: center; margin-top: 8px; min-height: 1.2rem; }
  </style>
</head>
<body>
  <div class="container">
    <h1>
      <span>Drone FC Tuning</span>
      <span id="armBadge" class="badge disarm">DISARM</span>
    </h1>

    <div class="card">
      <div class="card-title">Live Telemetry</div>
      <div class="grid-3">
        <div class="metric"><div id="valRoll" class="metric-val">0.0°</div><div class="metric-lbl">ROLL</div></div>
        <div class="metric"><div id="valPitch" class="metric-val">0.0°</div><div class="metric-lbl">PITCH</div></div>
        <div class="metric"><div id="valYaw" class="metric-val">0.0°</div><div class="metric-lbl">YAW</div></div>
      </div>
      <div class="grid-3" style="margin-top: 8px;">
        <div class="metric"><div id="valBatt" class="metric-val">-- V</div><div class="metric-lbl">BATERAI</div></div>
        <div class="metric"><div id="valThr" class="metric-val">0%</div><div class="metric-lbl">THROTTLE</div></div>
        <div class="metric"><div id="valMotors" class="metric-val">1000</div><div class="metric-lbl">M1 PWM</div></div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Roll PID</div>
      <div class="grid-3">
        <div><label>Kp</label><input type="number" step="0.05" id="r_kp"></div>
        <div><label>Ki</label><input type="number" step="0.005" id="r_ki"></div>
        <div><label>Kd</label><input type="number" step="0.01" id="r_kd"></div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Pitch PID</div>
      <div class="grid-3">
        <div><label>Kp</label><input type="number" step="0.05" id="p_kp"></div>
        <div><label>Ki</label><input type="number" step="0.005" id="p_ki"></div>
        <div><label>Kd</label><input type="number" step="0.01" id="p_kd"></div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Yaw PID</div>
      <div class="grid-3">
        <div><label>Kp</label><input type="number" step="0.1" id="y_kp"></div>
        <div><label>Ki</label><input type="number" step="0.005" id="y_ki"></div>
        <div><label>Kd</label><input type="number" step="0.01" id="y_kd"></div>
      </div>
    </div>

    <button class="btn btn-primary" onclick="savePID(false)">Terapkan PID (Live RAM)</button>
    <button class="btn btn-warning" onclick="savePID(true)">Simpan Permanen ke Flash (NVS)</button>
    <button class="btn btn-secondary" onclick="calibrateIMU()">Kalibrasi IMU Meja Datar</button>

    <div class="card" style="margin-top: 14px;">
      <div class="card-title">Blackbox Flight Recorder</div>
      <a href="/api/blackbox/download" class="btn btn-secondary" style="text-align:center; text-decoration:none;">Download Flight Log (.csv)</a>
      <button class="btn btn-secondary" style="margin-top:6px; color:#ef4444;" onclick="clearBlackbox()">Bersihkan Log Flash</button>
    </div>

    <div id="msg"></div>
  </div>

  <script>
    async function loadTelemetry() {
      try {
        const res = await fetch('/api/status');
        const d = await res.json();
        document.getElementById('valRoll').textContent = d.roll.toFixed(1) + '°';
        document.getElementById('valPitch').textContent = d.pitch.toFixed(1) + '°';
        document.getElementById('valYaw').textContent = d.yaw.toFixed(1) + '°';
        document.getElementById('valBatt').textContent = d.batt.toFixed(2) + 'V';
        document.getElementById('valThr').textContent = Math.round(d.throttle * 100) + '%';
        document.getElementById('valMotors').textContent = d.m1;

        const badge = document.getElementById('armBadge');
        if (d.armed) {
          badge.textContent = 'ARMED';
          badge.className = 'badge armed';
        } else {
          badge.textContent = 'DISARM';
          badge.className = 'badge disarm';
        }

        if (!window.loadedOnce) {
          document.getElementById('r_kp').value = d.rkp;
          document.getElementById('r_ki').value = d.rki;
          document.getElementById('r_kd').value = d.rkd;
          document.getElementById('p_kp').value = d.pkp;
          document.getElementById('p_ki').value = d.pki;
          document.getElementById('p_kd').value = d.pkd;
          document.getElementById('y_kp').value = d.ykp;
          document.getElementById('y_ki').value = d.yki;
          document.getElementById('y_kd').value = d.ykd;
          window.loadedOnce = true;
        }
      } catch (e) {}
    }

    async function savePID(persist) {
      const body = {
        rkp: parseFloat(document.getElementById('r_kp').value),
        rki: parseFloat(document.getElementById('r_ki').value),
        rkd: parseFloat(document.getElementById('r_kd').value),
        pkp: parseFloat(document.getElementById('p_kp').value),
        pki: parseFloat(document.getElementById('p_ki').value),
        pkd: parseFloat(document.getElementById('p_kd').value),
        ykp: parseFloat(document.getElementById('y_kp').value),
        yki: parseFloat(document.getElementById('y_ki').value),
        ykd: parseFloat(document.getElementById('y_kd').value),
        persist: persist
      };
      const res = await fetch('/api/set_pid', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body)
      });
      const d = await res.json();
      const msg = document.getElementById('msg');
      msg.textContent = d.message;
      msg.style.color = d.success ? '#38bdf8' : '#ef4444';
      setTimeout(() => msg.textContent = '', 4000);
    }

    async function calibrateIMU() {
      if (!confirm("Pastikan drone diam tegak di tempat datar! Lanjutkan kalibrasi?")) return;
      const res = await fetch('/api/calibrate', { method: 'POST' });
      const d = await res.json();
      alert(d.message);
    }

    async function clearBlackbox() {
      if (!confirm("Yakin ingin menghapus seluruh log blackbox di Flash?")) return;
      const res = await fetch('/api/blackbox/clear', { method: 'POST' });
      const d = await res.json();
      alert(d.message);
    }

    setInterval(loadTelemetry, 250);
  </script>
</body>
</html>
)rawliteral";

void wifiTuningSavePIDToNVS() {
  if (!pRoll || !pPitch || !pYaw) return;
  Preferences prefs;
  if (prefs.begin("drone_pid", false)) {
    prefs.putFloat("rkp", pRoll->getKp());
    prefs.putFloat("rki", pRoll->getKi());
    prefs.putFloat("rkd", pRoll->getKd());

    prefs.putFloat("pkp", pPitch->getKp());
    prefs.putFloat("pki", pPitch->getKi());
    prefs.putFloat("pkd", pPitch->getKd());

    prefs.putFloat("ykp", pYaw->getKp());
    prefs.putFloat("yki", pYaw->getKi());
    prefs.putFloat("ykd", pYaw->getKd());
    prefs.putBool("saved", true);
    prefs.end();
    Serial.println("[WIFI] PID berhasil disimpan permanen ke NVS Flash!");
  }
}

void wifiTuningLoadPIDFromNVS(PID &roll, PID &pitch, PID &yaw) {
  Preferences prefs;
  if (prefs.begin("drone_pid", true)) {
    if (prefs.getBool("saved", false)) {
      roll.setGains(prefs.getFloat("rkp", ROLL_KP),
                    prefs.getFloat("rki", ROLL_KI),
                    prefs.getFloat("rkd", ROLL_KD));
      pitch.setGains(prefs.getFloat("pkp", PITCH_KP),
                     prefs.getFloat("pki", PITCH_KI),
                     prefs.getFloat("pkd", PITCH_KD));
      yaw.setGains(prefs.getFloat("ykp", YAW_KP),
                   prefs.getFloat("yki", YAW_KI),
                   prefs.getFloat("ykd", YAW_KD));
      Serial.println("[WIFI] PID berhasil dimuat dari NVS Flash!");
    }
    prefs.end();
  }
}

void wifiTuningInit(PID &roll, PID &pitch, PID &yaw) {
  pRoll  = &roll;
  pPitch = &pitch;
  pYaw   = &yaw;

  // Cek apakah ada custom PID di NVS
  wifiTuningLoadPIDFromNVS(roll, pitch, yaw);

  // Jalankan SoftAP
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Drone-FC-Setup", "12345678");
  Serial.print("[WIFI] Hotspot aktif: SSID 'Drone-FC-Setup' | IP: ");
  Serial.println(WiFi.softAPIP());

  // Routing
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", INDEX_HTML);
  });

  server.on("/api/status", HTTP_GET, []() {
    IMUData imu = imuGet();
    float battV = 0.0f;
    #ifdef ENABLE_SAFETY
      SafetyData sf = safetyGet();
      battV = sf.battVoltage;
    #endif

    float thr = (float)(rcGetRaw(CH_THROTTLE) - RC_MIN) / (RC_MAX - RC_MIN);
    thr = constrain(thr, 0.0f, 1.0f);

    String json = "{";
    json += "\"roll\":" + String(imu.roll, 2) + ",";
    json += "\"pitch\":" + String(imu.pitch, 2) + ",";
    json += "\"yaw\":" + String(imu.yaw, 2) + ",";
    json += "\"batt\":" + String(battV, 2) + ",";
    json += "\"throttle\":" + String(thr, 2) + ",";
    json += "\"m1\":" + String(motorGetPWM(0)) + ",";
    json += "\"armed\":" + String(motorIsArmed() ? "true" : "false") + ",";
    json += "\"rkp\":" + String(pRoll->getKp(), 3) + ",";
    json += "\"rki\":" + String(pRoll->getKi(), 4) + ",";
    json += "\"rkd\":" + String(pRoll->getKd(), 4) + ",";
    json += "\"pkp\":" + String(pPitch->getKp(), 3) + ",";
    json += "\"pki\":" + String(pPitch->getKi(), 4) + ",";
    json += "\"pkd\":" + String(pPitch->getKd(), 4) + ",";
    json += "\"ykp\":" + String(pYaw->getKp(), 3) + ",";
    json += "\"yki\":" + String(pYaw->getKi(), 4) + ",";
    json += "\"ykd\":" + String(pYaw->getKd(), 4);
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/api/set_pid", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing body\"}");
      return;
    }
    String body = server.arg("plain");

    auto extract = [&](const String& key) -> float {
      int idx = body.indexOf("\"" + key + "\":");
      if (idx == -1) return 0;
      int start = idx + key.length() + 3;
      int end = body.indexOf(",", start);
      if (end == -1) end = body.indexOf("}", start);
      return body.substring(start, end).toFloat();
    };

    float rkp = extract("rkp");
    float rki = extract("rki");
    float rkd = extract("rkd");
    float pkp = extract("pkp");
    float pki = extract("pki");
    float pkd = extract("pkd");
    float ykp = extract("ykp");
    float yki = extract("yki");
    float ykd = extract("ykd");
    bool persist = body.indexOf("\"persist\":true") != -1;

    pRoll->setGains(rkp, rki, rkd);
    pPitch->setGains(pkp, pki, pkd);
    pYaw->setGains(ykp, yki, ykd);

    if (persist) {
      wifiTuningSavePIDToNVS();
    }

    server.send(200, "application/json", "{\"success\":true,\"message\":\"PID berhasil diperbarui!\"}");
  });

  server.on("/api/calibrate", HTTP_POST, []() {
    if (motorIsArmed()) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"Gagal: Drone sedang ARMED!\"}");
      return;
    }
    imuCalibrate();
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Kalibrasi IMU selesai & disimpan ke NVS!\"}");
  });

  #ifdef ENABLE_BLACKBOX
    server.on("/api/blackbox/download", HTTP_GET, []() {
      if (!LittleFS.exists("/flight_log.csv")) {
        server.send(404, "text/plain", "Belum ada file flight_log.csv.");
        return;
      }
      File f = LittleFS.open("/flight_log.csv", FILE_READ);
      server.sendHeader("Content-Disposition", "attachment; filename=\"flight_log.csv\"");
      server.streamFile(f, "text/csv");
      f.close();
    });

    server.on("/api/blackbox/clear", HTTP_POST, []() {
      blackboxClear();
      server.send(200, "application/json", "{\"success\":true,\"message\":\"Blackbox log berhasil dibersihkan!\"}");
    });
  #endif

  server.begin();
  Serial.println("[WIFI] WebServer tuning siap!");
}

void wifiTuningUpdate() {
  server.handleClient();
}

#endif // ENABLE_WIFI_TUNING
