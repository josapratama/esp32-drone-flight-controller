# 🏗️ Arsitektur Firmware dan Sistem Kontrol

Dokumentasi arsitektur perangkat lunak, pembagian beban multi-core FreeRTOS, algoritma filter sinyal sensor, matriks pencampuran motor (*motor mixing*), dan hierarki modul firmware.

---

## 📑 Daftar Isi
1. [Struktur dan Modul Firmware](#1-struktur-dan-modul-firmware)
2. [Alokasi FreeRTOS Dual-Core (Core 0 vs Core 1)](#2-alokasi-freertos-dual-core)
3. [Hierarki Loop Kontrol (500Hz, 50Hz, 10Hz)](#3-hierarki-loop-kontrol)
4. [State Machine Mode Penerbangan](#4-state-machine-mode-penerbangan)
5. [Sensor Fusion & Filter Derau Getaran (D-Term LPF)](#5-sensor-fusion--filter-derau-getaran)
6. [Matriks Pencampuran Motor (Quad-X Motor Mixer)](#6-matriks-pencampuran-motor)
7. [Mekanisme AirMode](#7-mekanisme-airmode)

---

## 1. Struktur dan Modul Firmware

Firmware dirancang modular dengan standar C++ modern untuk memisahkan logika hardware, sensor, algoritma kontrol, dan komunikasi:

```
drone/main/
├── config.h          → Konfigurasi master (pinout, konstanta PID, batasan sudut, frekuensi)
├── main.ino          → Inisialisasi sistem, setup timer FreeRTOS, dan eksekusi loop utama
├── imu.h / imu.cpp   → Pembacaan raw sensor MPU9250, kalibrasi offset, dan complementary filter
├── pid.h / pid.cpp   → Algoritma PID dual-stage (Angle mode dan Rate mode) + D-filter
├── motor.h / motor.cpp → Driver hardware MCPWM 400Hz, kalibrasi rentang, dan mixing motor
├── receiver.h / .cpp → Parser serial iBUS FlySky non-blocking dengan deteksi sinyal hilang
├── safety.h / .cpp   → Interlock arming, deteksi sudut flip darurat (>65°), dan failsafe
├── baro.h / baro.cpp → Driver I2C BMP280, smoothing ketinggian, dan variometer
├── tof.h / tof.cpp   → Driver sensor laser VL53L1X untuk estimasi ketinggian rendah (<2m)
├── gps.h / gps.cpp   → Parser NMEA TinyGPSPlus untuk koordinat, kecepatan, dan jumlah satelit
├── nav.h / nav.cpp   → State machine navigasi otomatis waypoint dan algoritma Return-To-Home
├── blackbox.h / .cpp → Perekam telemetri ke flash SPIFFS internal untuk analisis crash
└── wifi_tuning.h/.cpp→ Server Web Asinkron ESP32 untuk tuning PID real-time via smartphone
```

---

## 2. Alokasi FreeRTOS Dual-Core

ESP32-S3 memiliki prosesor **Xtensa Dual-Core 240 MHz**. Agar eksekusi loop penyeimbang 500 Hz tidak pernah terganggu oleh beban jaringan WiFi atau penulisan data lambat, sistem dibagi ke dua core:

```
┌──────────────────────────────────────┐  ┌──────────────────────────────────────┐
│        CORE 1 (Dedicated Real-Time)  │  │       CORE 0 (Background & Network)  │
├──────────────────────────────────────┤  ├──────────────────────────────────────┤
│ • 500Hz Flight Loop (2 ms interval)  │  │ • WiFi SoftAP Web Dashboard Server   │
│ • Baca IMU (MPU9250 via I2C 400kHz)  │  │ • WebSocket Telemetry Stream         │
│ • Sensor Fusion & Angle Estimation   │  │ • Blackbox SPIFFS Data Logging       │
│ • Perhitungan PID (Roll, Pitch, Yaw) │  │ • Parser Modul GPS Neo-M8N (UART)    │
│ • MCPWM Hardware Motor Pulse Output  │  │ • Komputasi Waypoint Navigasi Jauh   │
│ • Safety Crash Tilt Angle Detection  │  │ • Manajemen Konfigurasi NVS Flash    │
└──────────────────────────────────────┘  └──────────────────────────────────────┘
```

---

## 3. Hierarki Loop Kontrol

```
WAKTU (ms) ──► 0       2       4       6       8      10 ... 20 ms
               │       │       │       │       │       │      │
FAST LOOP ────►[IMU+PID][IMU+PID][IMU+PID][IMU+PID][IMU+PID]... (500 Hz / Setiap 2 ms)
(Core 1)       │
               │
MEDIUM LOOP ───┼─────────────────────────────────────────────► [Baro + ToF + Receiver] (50 Hz / 20 ms)
(Core 1/0)     │
               │
SLOW LOOP ─────┴────────────────────────────────────────────────────────────────────────► [GPS + WiFi + Batt] (10 Hz / 100 ms)
(Core 0)
```

1. **Fast Inner Loop ($500\text{ Hz} / 2.0\text{ ms}$):**
   - Mengambil data raw gyro dan akselerometer.
   - Menghitung orientasi aktual.
   - Menghitung keluaran koreksi PID Roll, Pitch, dan Yaw.
   - Mengirim pulsa mikrodetik ke ESC melalui unit periferal MCPWM.
2. **Medium Loop ($50\text{ Hz} / 20.0\text{ ms}$):**
   - Mendekode paket data remote iBUS (target perintah pilot).
   - Membaca Barometer BMP280 dan ToF VL53L1X untuk pembaruan ketinggian vertikal.
   - Memperbarui perhitungan PID penahan ketinggian (*Altitude Hold*).
3. **Slow Loop ($10\text{ Hz} / 100.0\text{ ms}$):**
   - Membaca koordinat GPS U-Blox.
   - Mengukur tegangan baterai LiPo melalui pin ADC.
   - Mengirim paket telemetri ke antarmuka web WiFi smartphone.

---

## 4. State Machine Mode Penerbangan

```
                   ┌──────────────┐
                   │ POWER UP     │
                   └──────┬───────┘
                          │ (Inisialisasi Sensor & Kalibrasi Gyro)
                          ▼
            ┌───────────────────────────┐
            │       DISARMED            │◄───────────────────────┐
            │   (Motor Mati Penuh)      │                        │
            └─────────────┬─────────────┘                        │
                          │ (Stick Throttle Min + Saklar SWA ON) │
                          ▼                                      │
            ┌───────────────────────────┐                        │
            │       ARMED (IDLE)        │                        │
            │ (Motor Berputar Rendah)   │                        │
            └─────────────┬─────────────┘                        │
                          │                                      │
         ┌────────────────┴────────────────┐                     │
         │ Throttle > 1050µs               │                     │
         ▼                                 ▼                     │
┌─────────────────┐               ┌─────────────────┐            │
│  ANGLE MODE     │               │ ALT_HOLD MODE   │            │
│ (Self-Leveling) │               │ (Otomatis Baro) │            │
└────────┬────────┘               └────────┬────────┘            │
         │                                 │                     │
         └────────────────┬────────────────┘                     │
                          │                                      │
             ┌────────────┴────────────┐                         │
             │     EMERGENCY COND?     │                         │
             └────────────┬────────────┘                         │
                          │                                      │
             ┌────────────┴────────────┐                         │
             │ Kemiringan > 65° ATAU   │                         │
             │ Hilang Sinyal Remote TX │                         │
             └────────────┬────────────┘                         │
                          ▼                                      │
            ┌───────────────────────────┐                        │
            │         FAILSAFE          │────────────────────────┘
            │  (Cut Throttle / Disarm)  │
            └───────────────────────────┘
```

---

## 5. Sensor Fusion & Filter Derau Getaran

### A. Complementary Filter (Orientasi Sudut)
Untuk mencegah keterlambatan (*lag*) dan melenyapkan getaran motor:
$$\theta_{k} = \alpha \cdot (\theta_{k-1} + \omega_{\text{gyro}} \cdot \Delta t) + (1 - \alpha) \cdot \theta_{\text{accel}}$$
- $\alpha = 0.98$: Memberikan bobot $98\%$ pada kelincahan gyro, dan $2\%$ pada acuan gravitasi akselerometer.

### B. D-Term Low-Pass Filter (PT1 Filter)
Komponen Derivative (D) pada PID sangat rentan mengamplifikasi getaran frekuensi tinggi dari baling-baling, yang dapat menyebabkan motor cepat panas:
$$D_{\text{filtered}} = D_{\text{last}} + \beta \cdot (D_{\text{raw}} - D_{\text{last}})$$
$$\beta = \frac{2\pi \cdot f_{\text{cutoff}} \cdot \Delta t}{1 + 2\pi \cdot f_{\text{cutoff}} \cdot \Delta t}$$
Firmware menggunakan frekuensi cut-off $f_{\text{cutoff}} = 40\text{ Hz}$ sehingga getaran mekanis tidak diteruskan ke motor.

---

## 6. Matriks Pencampuran Motor (Quad-X Mixer)

Keluaran perintah throttle pilot dan output koreksi PID Roll, Pitch, dan Yaw didistribusikan ke masing-masing 4 motor dengan rumus:

```
        DEPAN (Maju)
    M1 (CCW)    M2 (CW)
        \      /
         \    /
          [FC]
         /    \
        /      \
    M3 (CW)     M4 (CCW)
       BELAKANG (Mundur)
```

$$\begin{aligned}
\text{M1 (Front Left)}   &= \text{Throttle} + \text{Roll}_{\text{PID}} + \text{Pitch}_{\text{PID}} + \text{Yaw}_{\text{PID}} \\
\text{M2 (Front Right)}  &= \text{Throttle} - \text{Roll}_{\text{PID}} + \text{Pitch}_{\text{PID}} - \text{Yaw}_{\text{PID}} \\
\text{M3 (Rear Left)}    &= \text{Throttle} + \text{Roll}_{\text{PID}} - \text{Pitch}_{\text{PID}} - \text{Yaw}_{\text{PID}} \\
\text{M4 (Rear Right)}   &= \text{Throttle} - \text{Roll}_{\text{PID}} - \text{Pitch}_{\text{PID}} + \text{Yaw}_{\text{PID}}
\end{aligned}$$

---

## 7. Mekanis AirMode

Pada drone konvensional tanpa AirMode, ketika pilot menarik stick throttle ke posisi $0\%$ (paling bawah), motor akan mati sehingga otoritas kendali PID hilang total dan drone akan berguling tidak terkendali saat jatuh bebas.

**Solusi AirMode pada Firmware Ini:**
- Ketika throttle berada di bawah batas idle ($< 1050\mu\text{s}$), loop PID tetap dipertahankan aktif $100\%$.
- Motor diizinkan berputar pada kecepatan idle dinamis untuk menghasilkan momen kendali penyeimbang meskipun drone sedang dalam kondisi deselerasi bebas.
