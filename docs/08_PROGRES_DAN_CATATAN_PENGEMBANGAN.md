# 📝 Log Progres Pengembangan dan Catatan Teknis

Catatan riwayat pengujian hardware, solusi kendala teknis krusial yang telah diselesaikan, dan peta jalan (*roadmap*) pengembangan drone ESP32-S3.

---

## 📑 Daftar Isi
1. [Status Pengujian Terkini (Hardware Validation Status)](#1-status-pengujian-terkini)
2. [Catatan Solusi Kendala Teknis Krusial](#2-catatan-solusi-kendala-teknis-krusial)
3. [Daftar Komponen Hardware Terverifikasi](#3-daftar-komponen-hardware-terverifikasi)
4. [Ringkasan Peningkatan Fitur Firmware Terbaru](#4-ringkasan-peningkatan-fitur-firmware-terbaru)
5. [Peta Jalan Langkah Selanjutnya (Roadmap)](#5-peta-jalan-langkah-selanjutnya)

---

## 1. Status Pengujian Terkini

```
Test 1 - I2C Sensor Bus Scan          [✅ LULUS / Selesai]
Test 2 - Kalibrasi Rentang Throttle ESC[✅ LULUS / Selesai]
Test 3 - Dekode Sinyal iBUS Receiver   [✅ LULUS / Selesai]
Test 4 - Pembacaan NMEA Modul GPS      [✅ LULUS / Selesai]
Test 5 - Kalibrasi Offset IMU MPU9250  [✅ LULUS / Selesai]
Test 6 - Pengujian Arah Putar Motor    [✅ LULUS / Selesai]
Test 7 - Uji Independensi Pin MCPWM    [✅ SIAP / Terverifikasi]
Audit Firmware & D-Filter / AirMode    [✅ Selesai]
Pemasangan Propeller & Uji Melayang    [⏳ Langkah Selanjutnya]
```

---

## 2. Catatan Solusi Kendala Teknis Krusial

### 1. Masalah Konflik Timer LEDC Servo pada ESP32-S3 (Terselesaikan)
- **Gejala Masalah:** Saat menggunakan library `ESP32Servo`, memerintahkan Motor 1 berputar menyebabkan Motor 3 ikut berputar secara bersamaan (begitu juga Motor 2 dan Motor 4).
- **Akar Masalah:** Modul LEDC timer pada arsitektur SoC ESP32-S3 berbagi prescaler channel secara berpasangan jika dialokasikan secara dinamis oleh library servo generik.
- **Solusi Permanen:** Firmware dirombak menggunakan modul **MCPWM (Motor Control Pulse Width Modulation)** bawaan silikon ESP32-S3 (`driver/mcpwm.h`). Setiap pin GPIO motor kini memiliki generator hardware independen pada frekuensi **400 Hz**, melenyapkan konflik timer dan memberikan respons sinyal ESC yang presisi tanpa interferensi silang.

### 2. Permasalahan Baterai LiPo Drop (Terselesaikan)
- **Gejala Masalah:** Baterai LiPo lama mengalami penurunan tegangan drastis ke $2.8\text{V}$ saat diberi beban motor, dan fisik baterai kembung.
- **Solusi Permanen:** Penggantian ke unit baterai baru **LiPo 3S 2200mAh 35C** dengan konektor standar **XT60** dan kapasitor peredam $1000\mu\text{F}$ 25V Low-ESR pada bus daya.

### 3. Deteksi Alamat Pass-Through Magnetometer (Terselesaikan)
- **Hasil:** Pada pemindaian I2C muncul alamat `0x0C`. Dikonfirmasi bahwa ini adalah chip magnetometer **AK8963** yang terintegrasi di dalam wadah sensor MPU9250 saat mode *bypass/pass-through* aktif. Kondisi ini normal dan valid.

---

## 3. Daftar Komponen Hardware Terverifikasi

- **Sistem Komputasi:** ESP32-S3 N16R8 (Dual-Core 240 MHz, 16MB Flash, 8MB PSRAM).
- **Motor Penggerak:** 4x Brushless Motor A2212/10T 1400KV.
- **Pengatur Kecepatan:** 4x ESC 30A SimonK Firmware support 2S-3S.
- **Sumber Daya:** LiPo 3S 2200mAh 35C XT60 + BEC Step-Down 5V 3A.
- **Sensor IMU:** MPU9250 (Gyrometer 3-Axis + Accelerometer 3-Axis + Magnetometer 3-Axis).
- **Sensor Ketinggian:** Barometer BMP280 + Laser ToF Lidar VL53L1X.
- **Sistem Navigasi:** GPS U-Blox Neo-M8N dengan antena keramik aktif.
- **Sistem Radio:** FlySky FS-i6X Transmitter + FS-iA6B 6-Channel iBUS Receiver.

---

## 4. Ringkasan Peningkatan Fitur Firmware Terbaru

1. **Frekuensi Loop Penyeimbang 500 Hz:** Loop eksekusi IMU dan PID berjalan setiap $2.0\text{ milidetik}$ di Core 1 tanpa interupsi.
2. **PT1 Low-Pass Filter pada D-Term:** Meredam lonjakan kebisingan getaran motor di atas 40 Hz sehingga motor tetap dingin.
3. **Mekanisme AirMode Dinamis:** Mempertahankan kemampuan manuver stabilisasi penuh bahkan saat throttle pilot berada di 0%.
4. **Telemetri WiFi Asinkron:** Web dashboard interaktif di Core 0 (`http://192.168.4.1`) untuk tuning slider Kp, Ki, Kd dan penyimpanan langsung ke NVS Flash.
5. **Crash Detection & Auto-Disarm:** Memutus daya motor dalam waktu $<40\text{ ms}$ jika kemiringan drone melebihi batas kritis $65^\circ$.

---

## 5. Peta Jalan Langkah Selanjutnya

```
               [Uji Terbang Perdana (Maiden Flight)]
                                 │
                                 ▼
                 Tuning Lapangan PID Roll & Pitch
                                 │
                                 ▼
              Aktivasi Penahan Ketinggian (Baro BMP280)
                                 │
                                 ▼
             Uji Precision Landing Menggunakan ToF Lidar
                                 │
                                 ▼
              Uji Navigasi Posisi GPS & Return-To-Home
```

