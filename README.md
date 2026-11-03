# 🛸 ESP32 DIY Quadcopter Flight Controller (FC)

[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3%20%2F%20ESP32-blue.svg)](https://www.espressif.com/)
[![Framework: Arduino / C++](https://img.shields.io/badge/Framework-Arduino%20C%2B%2B-00979D.svg)](https://www.arduino.cc/)
[![Loop Rate: 500Hz](https://img.shields.io/badge/Flight%20Loop-500Hz%20(2ms)-brightgreen.svg)]()
[![Motor Control: 400Hz MCPWM](https://img.shields.io/badge/ESC%20Protocol-400Hz%20MCPWM-orange.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

Sistem pengendali terbang (*Flight Controller*) quadcopter otonom berkinerja tinggi yang dibangun dari nol menggunakan mikrokontroler **ESP32 / ESP32-S3**. Mengimplementasikan komputasi multi-core FreeRTOS, filter peredam getaran (D-term PT1 LPF), sensor fusion IMU 9-Axis, sistem keselamatan auto-disarm, telemetri nirkabel WiFi, dan navigasi GPS otonom.

---

## 🌟 Fitur Unggulan

- **Arsitektur Dual-Core FreeRTOS:**
  - **Core 1 (Real-Time 500Hz / 2ms):** Sampling sensor IMU MPU9250 (I2C 400kHz), Complementary Filter, dual-stage PID controller, dan output pulsa motor.
  - **Core 0 (Background & Network):** Server Web SoftAP untuk tuning PID live via smartphone, telemetri nirkabel, parser GPS Neo-M8N, dan blackbox logging.
- **Hardware MCPWM Driver (400Hz):** Menggunakan periferal Motor Control PWM bawaan silikon ESP32-S3 dengan timer independen untuk 4 unit ESC (tanpa konflik timer servo).
- **Advanced Flight Control Dynamics:**
  - **D-Term PT1 Low-Pass Filter ($f_{\text{cut}} = 40\text{ Hz}$):** Melindungi motor dari panas akibat amplifikasi getaran mekanis baling-baling.
  - **Dynamic AirMode:** Menjaga otoritas penuh kontrol stabilisasi PID bahkan saat stick throttle ditarik ke 0%.
- **Sistem Keselamatan Multi-Lapis:**
  - **Arming Interlock Safety:** Menolak start motor jika throttle tidak di posisi nol, kemiringan $> 15^\circ$, atau sensor gagal inisialisasi.
  - **Emergency Crash Detection ($<40\text{ ms}$):** Memutus putaran motor secara instan saat drone mengalami kemiringan ekstrem ($> 65^\circ$).
  - **Radio Signal Loss Failsafe:** Deteksi timeout iBUS digital ($> 500\text{ ms}$) dengan opsi penurunan bertahap (*controlled descent*) atau Return-To-Home.
- **Telemetri WiFi & Tuning PID Tanpa Kabel:** Web dashboard interaktif langsung dari smartphone (`http://192.168.4.1`) untuk melihat orientasi real-time dan menyimpan konstanta Kp, Ki, Kd langsung ke memori Flash NVS.
- **Sensor Lengkap Siap Navigasi Otonom:** Mendukung Barometer BMP280 (Altitude Hold), Laser ToF VL53L1X (Precision Landing), dan GPS U-Blox Neo-M8N (Waypoint/RTH).

---

## 📐 Layout Motor Quad-X & Pinout

```
                  DEPAN (Arah Terbang)
                          ▲
                          │
             M1 [CCW] ◄───┼───► M2 [CW]
              (Kiri Depan)│    (Kanan Depan)
                          │
                   ┌──────┴──────┐
                   │   ESP32-S3  │
                   │     (FC)    │
                   └──────┬──────┘
                          │
             M3 [CW]  ◄───┼───► M4 [CCW]
             (Kiri Belakang)   (Kanan Belakang)
                          │
                      BELAKANG
```

### Pemetaan Pin GPIO:

| Jalur / Perangkat | Fungsi | Pin ESP32 Classic | Pin ESP32-S3 N16R8 | Keterangan |
| :--- | :--- | :---: | :---: | :--- |
| **ESC Motor 1** | Pulsa MCPWM M1 (CCW) | **GPIO 25** | **GPIO 4** | Front-Left |
| **ESC Motor 2** | Pulsa MCPWM M2 (CW) | **GPIO 14** | **GPIO 5** | Front-Right |
| **ESC Motor 3** | Pulsa MCPWM M3 (CW) | **GPIO 26** | **GPIO 6** | Rear-Left |
| **ESC Motor 4** | Pulsa MCPWM M4 (CCW) | **GPIO 27** | **GPIO 7** | Rear-Right |
| **I2C SDA** | Bus Data Sensor | **GPIO 21** | **GPIO 8** | Pull-up 4.7kΩ ke 3.3V |
| **I2C SCL** | Bus Clock Sensor | **GPIO 22** | **GPIO 9** | Pull-up 4.7kΩ ke 3.3V |
| **iBUS RX** | Sinyal Radio Remote | **GPIO 33** | **GPIO 18** | FlySky FS-iA6B |
| **GPS RX / TX** | Komunikasi Serial NMEA | **GPIO 17 / 16** | **GPIO 16 / 17** | U-Blox Neo-M8N |
| **Voltage ADC** | Pemantau LiPo 3S | **GPIO 34** | **GPIO 1** | R1=33kΩ, R2=10kΩ |
| **Buzzer Aktif** | Indikator Suara & Alarm | **GPIO 13** | **GPIO 21** | Active Buzzer 5V |

---

## 🛠️ Spesifikasi Hardware (BOM)

- **Flight Controller:** Board ESP32 / ESP32-S3 N16R8 (Xtensa LX7 Dual-Core 240MHz).
- **Frame:** Multirotor F450 (Jarak sumbu motor 450mm, bobot 280g).
- **Motor Brushless:** 4x A2212/10T 1400KV.
- **ESC:** 4x 30A SimonK / BLHeli (Support 2S-3S LiPo).
- **Propeller:** 8045 atau 9047 Nylon (2x CW + 2x CCW).
- **Baterai:** LiPo 3S 2200mAh 35C dengan konektor XT60.
- **Regulator Daya:** BEC Step-Down 5V 3A (Low-Noise).
- **Sensor IMU:** MPU9250 (Gyro 3-Axis + Accelerometer 3-Axis + Magnetometer AK8963).
- **Sensor Ketinggian:** Barometer BMP280 + Laser ToF Lidar VL53L1X.
- **Modul GPS:** U-Blox Neo-M8N dengan antena keramik terintegrasi.
- **Transmitter & Receiver:** FlySky FS-i6X + FS-iA6B (Protokol Serial Digital iBUS).

---

## 📁 Struktur Repositori

```
esp32-drone-flight-controller/
├── docs/                     → Dokumentasi komprehensif & modular
│   ├── 01_PANDUAN_PEMULA.md              → Konsep dasar, aerodinamika, dan komponen
│   ├── 02_WIRING_DAN_PINOUT.md           → Skematik perkabelan, common ground & kapasitor
│   ├── 03_THRUST_DAN_KALKULASI_BEBAN.md  → Perhitungan TWR (2:1), daya angkat & flight time
│   ├── 04_ARSITEKTUR_FIRMWARE.md         → FreeRTOS multi-core, 500Hz loop, mixer & filter
│   ├── 05_PANDUAN_TEST_HARDWARE.md       → Uji bertahap Test 1-7 & checklist propeller
│   ├── 06_TUNING_PID_DAN_WIFI_TELEMETRI.md→ Langkah penyetelan PID & Web Dashboard
│   ├── 07_SISTEM_KESELAMATAN_DAN_FAILSAFE.md→ Interlock arming, crash detect & low batt
│   └── 08_PROGRES_DAN_CATATAN_PENGEMBANGAN.md→ Riwayat pengujian & solusi kendala MCPWM
├── main/                     → Source code utama firmware (ESP32 / ESP32-S3)
│   ├── config.h              → Konfigurasi master & pinout
│   ├── main.ino              → Setup, task FreeRTOS, dan main flight loop
│   ├── imu.cpp / imu.h       → Driver MPU9250 & complementary filter
│   ├── pid.cpp / pid.h       → Algoritma PID dual-stage & D-term LPF
│   ├── motor.cpp / motor.h   → Driver hardware MCPWM 400Hz & Quad-X mixing
│   ├── receiver.cpp / .h     → Parser digital serial iBUS non-blocking
│   ├── safety.cpp / safety.h → Logika arming, crash detection, dan failsafe
│   ├── baro.cpp / baro.h     → Driver BMP280 untuk Altitude Hold
│   ├── tof.cpp / tof.h       → Driver VL53L1X untuk Precision Landing
│   ├── gps.cpp / gps.h       → Driver U-Blox GPS & parser TinyGPSPlus
│   ├── nav.cpp / nav.h       → State machine navigasi waypoint & Return-To-Home
│   ├── blackbox.cpp / .h     → Logger data penerbangan internal SPIFFS
│   └── wifi_tuning.cpp / .h  → Server Web Asinkron untuk tuning nirkabel
├── test/                     → Sketsa uji hardware bertahap (Validasi Unit)
│   ├── 0_prototype/          → Sketsa prototipe awal
│   ├── 1_test_i2c/           → Uji pemindaian alamat sensor bus I2C
│   ├── 2_test_esc/           → Kalibrasi rentang pulsa throttle ESC (1000-2000µs)
│   ├── 3_test_receiver/      → Verifikasi pembacaan 6 channel iBUS FlySky
│   ├── 4_test_gps/           → Uji modul GPS dan penerimaan kalimat NMEA
│   ├── 5_test_imu/           → Uji orientasi dan kalibrasi offset sensor gerak
│   ├── 6_test_motor/         → Uji putaran dan arah rotasi motor Quad-X
│   └── 7_test_pin_independen/→ Uji independensi timer channel hardware MCPWM
├── .gitignore
├── LICENSE
└── README.md
```

---

## 🚀 Panduan Memulai Cepat (Quick Start)

### 1. Prasyarat Library Arduino IDE
Pastikan board **esp32 by Espressif Systems** sudah terpasang di Arduino IDE. Pasang library berikut melalui *Library Manager*:
- `MPU9250` (by hideakitai)
- `Adafruit BMP280 Library` (by Adafruit)
- `VL53L1X` (by Pololu)
- `TinyGPSPlus` (by Mikal Hart)
- `ESPAsyncWebServer` & `AsyncTCP`

### 2. Validasi Hardware Bertahap (Tanpa Propeller!)
Sebelum meng-upload firmware utama, jalankan pengujian berurutan di folder `test/`:
1. Upload `test/1_test_i2c/` $\rightarrow$ Pastikan alamat `0x68`, `0x0C`, `0x76`, dan `0x29` terdeteksi.
2. Upload `test/2_test_esc/` $\rightarrow$ Lakukan kalibrasi batas atas dan bawah ESC.
3. Upload `test/3_test_receiver/` $\rightarrow$ Pastikan stik remote dan saklar terbaca normal ($1000 - 2000$).
4. Upload `test/6_test_motor/` $\rightarrow$ Pastikan arah putaran motor tepat (M1/M4 CCW, M2/M3 CW).

Detail panduan pengujian dapat dibaca di [docs/05_PANDUAN_TEST_HARDWARE.md](docs/05_PANDUAN_TEST_HARDWARE.md).

### 3. Kompilasi & Upload Firmware Utama
1. Buka file `main/main.ino` di Arduino IDE.
2. Pilih board: **ESP32S3 Dev Module** (atau **ESP32 Dev Module**).
3. Pastikan konfigurasi Flash:
   - *USB CDC On Boot:* Enabled (jika menggunakan USB native ESP32-S3)
   - *Partition Scheme:* Default 4MB with SPIFFS / 16MB
4. Klik **Upload**.

---

## 📶 Web Dashboard & Telemetri WiFi

1. Nyalakan drone (posisi **DISARMED**).
2. Hubungkan smartphone / laptop ke jaringan WiFi:
   - **SSID:** `ESP32-Drone-FC`
   - **Password:** `drone123`
3. Buka browser web dan tuju alamat:
   ```text
   http://192.168.4.1
   ```
4. Anda dapat memantau orientasi sudut Roll/Pitch secara grafis, memeriksa tegangan baterai LiPo, mengubah konstanta PID via slider, dan menyimpan konfigurasi permanen ke Flash NVS.

Detail panduan tuning dapat dibaca di [docs/06_TUNING_PID_DAN_WIFI_TELEMETRI.md](docs/06_TUNING_PID_DAN_WIFI_TELEMETRI.md).

---

## 📚 Indeks Dokumentasi Lengkap

Untuk panduan mendalam, silakan baca dokumentasi di folder `docs/`:
- [01. Panduan Dasar Drone Quadcopter untuk Pemula](docs/01_PANDUAN_PEMULA.md)
- [02. Skematik Perkabelan, Distribusi Daya, dan Pinout](docs/02_WIRING_DAN_PINOUT.md)
- [03. Perhitungan Gaya Angkat (Thrust), Bobot, dan Rasio TWR](docs/03_THRUST_DAN_KALKULASI_BEBAN.md)
- [04. Arsitektur Firmware, FreeRTOS Dual-Core, dan Kontrol Loop](docs/04_ARSITEKTUR_FIRMWARE.md)
- [05. Prosedur Uji Coba Hardware Step-by-Step & Propeller Checklist](docs/05_PANDUAN_TEST_HARDWARE.md)
- [06. Panduan Penyetelan PID dan Telemetri Nirkabel WiFi](docs/06_TUNING_PID_DAN_WIFI_TELEMETRI.md)
- [07. Sistem Keselamatan Penerbangan, Interlock, dan Failsafe](docs/07_SISTEM_KESELAMATAN_DAN_FAILSAFE.md)
- [08. Log Riwayat Progres dan Catatan Teknis Pengembangan](docs/08_PROGRES_DAN_CATATAN_PENGEMBANGAN.md)

---

## ⚠️ Peringatan Keselamatan (Safety Disclaimer)

> Drone adalah wahana terbang berdaya tinggi. Selalu utamakan keselamatan diri dan orang lain. Jangan pernah memasang baling-baling saat melakukan pengujian software atau kalibrasi di meja kerja. Pengembang tidak bertanggung jawab atas kerusakan fisik, material, atau cedera yang diakibatkan oleh penggunaan firmware ini di luar prosedur keselamatan yang telah ditentukan.

---

## 📄 Lisensi

Proyek ini dirilis di bawah lisensi terbuka [MIT License](LICENSE). Bebas digunakan, dimodifikasi, dan dikembangkan untuk keperluan edukasi, hobi, maupun komersial.

