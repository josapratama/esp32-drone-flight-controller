# 📘 Panduan Dasar Drone Quadcopter untuk Pemula

Panduan komprehensif mengenai konsep fundamental, aerodinamika penerbangan, terminologi, dan anatomi komponen pada drone quadcopter berbasis ESP32.

---

## 📑 Daftar Isi
1. [Apa itu Drone Quadcopter?](#1-apa-itu-drone-quadcopter)
2. [Prinsip Fisika Penerbangan (Bagaimana Drone Terangkat)](#2-prinsip-fisika-penerbangan)
3. [Dinamika Gerak: Roll, Pitch, Yaw, dan Throttle](#3-dinamika-gerak-roll-pitch-yaw-dan-throttle)
4. [Konfigurasi Quad-X dan Arah Rotasi Motor](#4-konfigurasi-quad-x-dan-arah-rotasi-motor)
5. [Anatomi dan Komponen Utama Drone](#5-anatomi-dan-komponen-utama-drone)
6. [Otak Drone: Flight Controller & Sensor IMU](#6-otak-drone-flight-controller--sensor-imu)
7. [Algoritma Keseimbangan: Sistem Kontrol PID](#7-algoritma-keseimbangan-sistem-kontrol-pid)
8. [ESC (Electronic Speed Controller) & Protokol Kontrol Motor](#8-esc-electronic-speed-controller--protokol-kontrol-motor)
9. [Sistem Komunikasi Radio (Transmitter & Receiver)](#9-sistem-komunikasi-radio-transmitter--receiver)
10. [Sensor Pendukung Otonom (Barometer, ToF Lidar, GPS)](#10-sensor-pendukung-otonom-barometer-tof-lidar-gps)
11. [Manajemen dan Keselamatan Baterai LiPo](#11-manajemen-dan-keselamatan-baterai-lipo)
12. [Glosarium Istilah Penting](#12-glosarium-istilah-penting)

---

## 1. Apa itu Drone Quadcopter?
Drone (Unmanned Aerial Vehicle / UAV) quadcopter adalah wahana terbang multirotor yang menggunakan **4 motor listrik** dengan baling-baling independen yang disusun dalam pola simetris (umumnya membentuk huruf "X" atau "+").

Berbeda dengan helikopter konvensional yang mengandalkan kemiringan bilah baling-baling (*swashplate mechanism*) yang rumit secara mekanikal, quadcopter bergerak sepenuhnya dengan **mengatur variasi kecepatan putaran (RPM)** dari masing-masing 4 motor secara simultan melalui algoritma komputer digital (Flight Controller).

---

## 2. Prinsip Fisika Penerbangan
Penerbangan quadcopter tunduk pada Hukum Newton:
- **Hukum Newton III (Aksi-Reaksi):** Ketika baling-baling berputar kencang, bilah baling-baling membelah dan mendorong udara ke bawah (aksi). Udara memberikan dorongan balik ke atas pada drone dengan kekuatan yang sama (reaksi). Gaya reaksi inilah yang disebut **Thrust (Gaya Angkat)**.
- **Kondisi Melayang (*Hovering*):** Tercapai saat total daya angkat (*Total Thrust*) dari 4 motor tepat sama dengan total bobot gravitasi drone ($Thrust = Berat$).
- **Naik (*Ascending*):** Terjadi saat $Thrust > Berat$.
- **Turun (*Descending*):** Terjadi saat $Thrust < Berat$.

---

## 3. Dinamika Gerak: Roll, Pitch, Yaw, dan Throttle

Untuk mengendalikan posisi dan orientasi di udara, drone menggunakan 4 sumbu kendali utama (*4 Degrees of Freedom*):

```
       ▲ Z (Yaw - Putaran Poros)
       │
       │       ▲ X (Pitch - Maju/Mundur)
       │      /
       │     /
       │    /
       └───┼────────► Y (Roll - Miring Kiri/Kanan)
```

| Gerakan | Sumbu | Aksi Drone | Cara Flight Controller Mewujudkannya |
| :--- | :--- | :--- | :--- |
| **Throttle** | Vertikal (Z) | Naik / Turun ketinggian | Menaikkan / menurunkan RPM keempat motor secara serentak |
| **Roll** | Longitudinal (X) | Miring ke kiri / kanan | Motor sisi kanan berputar lebih cepat dari motor kiri (atau sebaliknya) |
| **Pitch** | Lateral (Y) | Maju ke depan / mundur ke belakang | Motor belakang berputar lebih cepat dari motor depan (maju), atau sebaliknya |
| **Yaw** | Poros Vertikal (Z) | Berputar hadap kiri / kanan di tempat | Memanfaatkan gaya torsi lawan (*torque reaction*) antar pasangan motor CW dan CCW |

---

## 4. Konfigurasi Quad-X dan Arah Rotasi Motor

Pada konfigurasi **Quad-X**, badan drone menghadap lurus ke depan di antara Motor 1 dan Motor 2:

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

### Mengapa Arah Putaran Harus Berlawanan?
Ketika motor berputar searah jarum jam (CW), hukum aksi-reaksi membuat badan drone terdorong berputar ke arah sebaliknya (CCW). Untuk menetralkan putaran liar ini, drone menggunakan:
- **2 Motor CW (Clockwise - Searah Jarum Jam):** M2 dan M3.
- **2 Motor CCW (Counter-Clockwise - Berlawanan Jarum Jam):** M1 dan M4.

Jika semua motor berputar pada kecepatan yang sama, total torsi menjadi **0 (seimbang)**, dan drone tidak akan berputar sendiri. Jika pilot ingin drone berputar ke kanan (*Yaw Right*), FC akan mempercepat motor CCW dan memperlambat motor CW.

---

## 5. Anatomi dan Komponen Utama Drone

```
                                  ┌─────────────┐
                                  │ Baterai LiPo│ (11.1V - 14.8V)
                                  └──────┬──────┘
                                         │
                         ┌───────────────┴───────────────┐
                         ▼                               ▼
                 ┌──────────────┐                 ┌─────────────┐
                 │  Power Board │                 │   BEC 5V    │ (Regulator Step-down)
                 │  (4x ESC)    │                 └──────┬──────┘
                 └───────┬──────┘                        │
                         │                   ┌───────────┴───────────┐
                         ▼                   ▼                       ▼
                    4x Brushless       ┌───────────┐           ┌───────────┐
                       Motor           │ ESP32-S3  │◄──────────┤ Receiver  │◄-.- [Transmitter]
                                       │ (Flight   │           │ (iBUS RX) │   (Remote Radio)
                                       │Controller)│           └───────────┘
                                       └─────┬─────┘
                                             │ (I2C / UART Bus)
                         ┌───────────────────┼───────────────────┐
                         ▼                   ▼                   ▼
                  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
                  │ IMU MPU9250 │     │ Baro BMP280 │     │ GPS Neo-M8N │
                  │(Gyro + Accel│     │ (Altimeter) │     │(Navigasi/RTH│
                  └─────────────┘     └─────────────┘     └─────────────┘
```

1. **Rangka (Frame):** Struktur penopang (misal F450 / 250mm carbon fiber/nylon).
2. **Motor Brushless:** Menggerakkan propeller dengan efisiensi tinggi dan RPM tinggi.
3. **Propeller:** Mengubah putaran kinetik motor menjadi dorongan fluida udara.
4. **Electronic Speed Controller (ESC):** Saklar daya 3-fase berbasis MOSFET yang mengatur kecepatan motor.
5. **Flight Controller (ESP32-S3):** Mikrokontroler berkecepatan 240 MHz yang mengeksekusi kontrol stabilisasi pada frekuensi tinggi (500 Hz).
6. **IMU (Inertial Measurement Unit):** Sensor giroskop dan akselerometer pengukur derajat kemiringan.
7. **Baterai LiPo:** Sumber daya dengan *discharge rate* tinggi (misal 35C).

---

## 6. Otak Drone: Flight Controller & Sensor IMU
Flight controller berfungsi membaca sensor berkala, menghitung koreksi, dan mengirimkan sinyal kendali baru ke ESC setiap **2 milidetik (500 Hz)**.

### Sensor IMU (MPU9250 / MPU6050)
- **Gyrometer (Kecepatan Sudut):** Mengukur seberapa cepat drone miring dalam satuan derajat/detik ($\text{deg/s}$). Sangat cepat dan responsif, tetapi rentan terhadap *drift* (akumulasi kesalahan dari waktu ke waktu).
- **Akselerometer (Gaya Gravitasi):** Mengukur arah gravitasi bumi ($1g$). Akurat dalam jangka panjang, namun sangat sensitif terhadap getaran (*vibration noise*) motor.
- **Sensor Fusion (Complementary Filter):** Firmware menggabungkan keunggulan keduanya:
  $$\text{Angle} = 0.98 \times (\text{Angle} + \text{Gyro} \times \Delta t) + 0.02 \times \text{Accel}$$
  Hasilnya adalah pembacaan sudut roll dan pitch yang cepat, halus, dan stabil tanpa drift.

---

## 7. Algoritma Keseimbangan: Sistem Kontrol PID
PID (Proportional, Integral, Derivative) adalah algoritma kalkulasi yang menghitung seberapa besar tenaga koreksi yang harus diberikan ke motor saat terjadi penyimpangan posisi.

$$\text{Output PID} = K_p \cdot e(t) + K_i \int e(t)\,dt + K_d \frac{de(t)}{dt}$$

- **P (Proportional - Kekuatan Respons):** Seberapa keras motor melawan saat drone miring dari target. Jika $K_p$ terlalu rendah drone lamban; jika terlalu tinggi drone bergetar cepat (*fast oscillations*).
- **I (Integral - Koreksi Angin/Beban):** Menghilangkan kesalahan konstan (seperti tiupan angin stabil atau bobot baterai yang condong ke belakang).
- **D (Derivative - Peredam / Shock Absorber):** Memprediksi masa depan berdasarkan laju perubahan sudut. Meredam osilasi agar gerakan berhenti tepat tanpa terpental (*overshoot*).

---

## 8. ESC & Protokol Kontrol Motor
Motor brushless tidak bisa berputar hanya dengan tegangan DC langsung. ESC mengubah DC menjadi pulsa 3-fase AC berkecepatan tinggi.

Pada firmware ini, ESP32-S3 menggunakan modul **MCPWM (Motor Control Pulse Width Modulation)** pada frekuensi **400 Hz** (periode 2.5 ms):
- **1000 µs (1.0 ms):** Motor Mati / Idle Minimum.
- **1500 µs (1.5 ms):** Throttle 50% (Daya Sedang).
- **2000 µs (2.0 ms):** Throttle 100% (Daya Maksimal Penuh).

---

## 9. Sistem Komunikasi Radio (Transmitter & Receiver)
Menggunakan protokol serial digital **FlySky iBUS** melalui pin Hardware UART:
- **Latensi Sangat Rendah:** Seluruh data 14 channel diperbarui setiap 7 ms secara digital.
- **Tahan Interferensi:** Data diverifikasi dengan *checksum*, berbeda dengan sinyal analog PPM lama yang mudah goyang (*jitter*).
- **Channel Mapping:**
  - CH1: Roll (Kanan / Kiri)
  - CH2: Pitch (Maju / Mundur)
  - CH3: Throttle (Daya Naik / Turun)
  - CH4: Yaw (Putar Hadap Kiri / Kanan)
  - CH5: Saklar Arming (SWA: Siaga / Matikan Motor)
  - CH6: Saklar Mode Terbang (SWB: Manual / AltHold / RTH)

---

## 10. Sensor Pendukung Otonom
1. **Barometer (BMP280):** Mengukur tekanan atmosferik dengan presisi hingga rentang $\pm 12\text{ cm}$. Digunakan untuk mode **Altitude Hold** (drone mempertahankan ketinggian otomatis).
2. **ToF Lidar (VL53L1X):** Mengukur jarak tanah menggunakan pulsa laser infra merah presisi milimeter hingga ketinggian 2–4 meter. Sangat krusial untuk fitur **Auto-Landing** yang lembut.
3. **GPS (U-Blox Neo-M8N):** Menerima sinyal satelit GPS/GLONASS untuk mengetahui koordinat bumi, kecepatan jelajah, dan eksekusi **Return-To-Home (RTH)** saat sinyal remote terputus.

---

## 11. Manajemen dan Keselamatan Baterai LiPo
Baterai Lithium Polymer (LiPo) memiliki densitas energi tinggi namun memerlukan penanganan disiplin:
- **Tegangan Nominal Sel:** $3.7\text{V}$ per sel.
- **Tegangan Penuh (*Full Charge*):** $4.20\text{V}$ per sel ($12.6\text{V}$ untuk baterai 3S).
- **Batas Kritis (*Under-voltage*):** **JANGAN PERNAH** biarkan sel turun di bawah $3.3\text{V}$ ($9.9\text{V}$ pada 3S). Baterai dapat rusak permanen atau kembung.
- **Tingkat Discharge (C-Rating):** Baterai 2200mAh 35C mampu menyalurkan arus kontinu:
  $$I_{\max} = 2.2\text{ Ah} \times 35 = 77\text{ Ampere}$$
  Cukup untuk mencukupi kebutuhan 4 motor yang mengonsumsi masing-masing 12–15A saat akselerasi penuh.

---

## 12. Glosarium Istilah Penting
- **Arming:** Tindakan mengaktifkan saklar remote untuk menyalakan putaran motor siaga sebelum terbang.
- **Disarming:** Mematikan putaran motor secara instan (prosedur darurat saat mendarat atau jatuh).
- **AirMode:** Fitur kontrol pada throttle 0% yang tetap menjaga kontrol stabilisasi PID aktif sehingga drone tidak kehilangan kendali saat jatuh bebas.
- **Failsafe:** Prosedur darurat otomatis saat sinyal remote hilang (misal mendarat perlahan atau mengaktifkan RTH).
- **Drift:** Fenomena drone perlahan bergeser sendiri karena ketidaksempurnaan titik gravitasi atau sensor.
- **Jitter:** Fluktuasi nilai sinyal secara acak yang disebabkan oleh derau listrik (*electrical noise*).

