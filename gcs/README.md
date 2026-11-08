# 🛸 Web 3D Drone Ground Control Station (GCS)

[![Platform: Raspberry Pi & Web](https://img.shields.io/badge/Platform-Raspberry%20Pi%205%20%7C%20Web-blue.svg)]()
[![Stack: FastAPI + Three.js + Leaflet](https://img.shields.io/badge/Stack-FastAPI%20%2B%20Three.js%20%2B%20Leaflet-brightgreen.svg)]()
[![Telemetry: MAVLink + SITL 6-DOF](https://img.shields.io/badge/Engine-6--DOF%20Physics%20SITL-orange.svg)]()

Stasiun Kendali Darat (*Ground Control Station / GCS*) berbasis Web 3D modern dengan antarmuka **Cockpit Cyberpunk HUD** yang dirancang khusus untuk mendampingi proyek quadcopter Anda ([`project/drone`](../drone/README.md)) maupun pengujian otonom virtual.

---

## 🌟 Fitur Utama

1. **3D Attitude & Horizon Indicator (Three.js WebGL):**
   - Model 3D Quadcopter responsif dengan 4 rotor berputar dinamis mengikuti PWM motor (M1, M2, M3, M4).
   - Sangkar gimbal 3-Axis (Roll, Pitch, Yaw) dan garis cakrawala *artificial horizon*.
2. **Primary Flight Display (PFD) HUD Canvas:**
   - Tangga pitch (*pitch ladder*) dengan kemiringan roll real-time.
   - Speedometer tape vertikal di sisi kiri & Altimeter tape (AGL/MSL) di sisi kanan.
   - Pita kompas horizontal 360° dengan jarum penunjuk heading.
3. **Peta Taktis & Perencana Misi Waypoint (Leaflet):**
   - Peta satelit/dark matter dengan pin posisi drone dan penunjuk arah rotasi SVG.
   - Jejak rekam terbang (*flight trail breadcrumbs*) berpendar (*cyan glow*).
   - Mode Perencana Misi (*Mission Planner*): Tambah waypoint WP1, WP2, WP3... langsung di peta, drag & drop posisi koordinat, dan unggah misi ke drone.
4. **Simulator Fisika 6-DOF Terintegrasi (SITL):**
   - Pengujian instan tanpa harus menyalakan hardware drone fisik.
   - Simulasi dinamika quadcopter lengkap (dorongan thrust, gravitasi, aerodinamika, respon PID, konsumsi baterai LiPo, dan hembusan angin).
5. **Dukungan Hardware Multi-Protokol:**
   - **Serial Port (USB/UART):** Terhubung ke FC via `/dev/ttyUSB0` pada Raspberry Pi atau `COM3` pada PC.
   - **UDP Telemetry (Port 14550):** MAVLink stream standar.
   - **TCP WiFi Direct:** Terhubung ke SoftAP WiFi ESP32 (`192.168.4.1:8080`).
6. **Sistem Suara & Peringatan Audio:**
   - Sintesis suara navigasi ("Motors Armed", "Return to Home Activated", "Low Battery Warning", "Emergency Motor Cut!").
   - Tombol penghenti darurat (*Emergency Motor Cut*) instan.
7. **Kontrol Keyboard Real-time:**
   - `W / S`: Kemiringan Pitch (Maju / Mundur)
   - `A / D`: Kemiringan Roll (Kiri / Kanan)
   - `Panah Atas / Bawah`: Throttle Motor
   - `Panah Kiri / Kanan`: Rotasi Yaw
   - `T`: Auto Takeoff (10m)
   - `R`: Return-to-Home (RTL)
   - `Spasi`: Emergency Cut

---

## 🚀 Cara Menjalankan

### Opsi 1: Menjalankan Langsung dengan Python
```bash
cd "project/drone-webgcs"
pip install -r requirements.txt
python -m uvicorn app.main:app --host 0.0.0.0 --port 8090
```
Buka browser di `http://localhost:8090`.

### Opsi 2: Menjalankan via Docker / MiniVPS
```bash
cd "project/drone-webgcs"
docker compose up -d --build
```
Aplikasi akan langsung online di port `8090`.

