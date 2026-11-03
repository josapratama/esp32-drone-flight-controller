# 🔌 Panduan Skematik Perkabelan (Wiring) dan Pinout

Panduan detail diagram sambungan kabel, distribusi daya, filtering kebisingan elektrik, dan peta pin GPIO untuk board **ESP32 DevKit** dan **ESP32-S3 N16R8**.

---

## 📑 Daftar Isi
1. [Arsitektur Distribusi Daya (Power System)](#1-arsitektur-distribusi-daya)
2. [Pencegahan Noise: Common Ground & Kapasitor](#2-pencegahan-noise-common-ground--kapasitor)
3. [Tabel Pinout Lengkap ESP32 vs ESP32-S3](#3-tabel-pinout-lengkap-esp32-vs-esp32-s3)
4. [Diagram Sambungan Motor ESC (Quad-X)](#4-diagram-sambungan-motor-esc)
5. [Diagram Sambungan Bus Sensor I2C](#5-diagram-sambungan-bus-sensor-i2c)
6. [Diagram Sambungan Komunikasi Serial (Receiver & GPS)](#6-diagram-sambungan-komunikasi-serial)
7. [Rangkaian Sensor Pembagi Tegangan Baterai (Voltage Divider)](#7-rangkaian-sensor-pembagi-tegangan-baterai)
8. [Checklist Pengujian Perkabelan Sebelum Nyala](#8-checklist-pengujian-perkabelan-sebelum-nyala)

---

## 1. Arsitektur Distribusi Daya

Sistem kelistrikan drone dibagi menjadi dua domain tegangan:
1. **High-Voltage High-Current Domain (11.1V - 12.6V / Up to 60A-80A):** Berasal langsung dari baterai LiPo 3S untuk memberi daya ke 4 unit ESC dan motor brushless.
2. **Low-Voltage Regulated Domain (5.0V / 3A):** Dihasilkan oleh modul BEC (Battery Eliminator Circuit) atau DC-DC Buck Converter untuk memberi daya stabil ke ESP32, Receiver, dan sensor-sensor.

```
┌────────────────────────────────────────────────────────┐
│                   Baterai LiPo 3S                      │
│            (11.1V - 12.6V / XT60 Connector)            │
└───────────────────────────┬────────────────────────────┘
                            │
            ┌───────────────┴───────────────┐
            │ (+) Positif (Merah)           │ (-) Negatif (Hitam)
            ▼                               ▼
    ┌────────────────┐              ┌────────────────┐
    │ PDB / Harness  │              │ PDB / Common   │
    │ ESC Power Bus  ├──────────────┤ Ground Bus     │
    └───────┬────────┘              └───────┬────────┘
            │                               │
            ├─► ESC M1 (VCC/GND)            ├─► ESC M1 GND
            ├─► ESC M2 (VCC/GND)            ├─► ESC M2 GND
            ├─► ESC M3 (VCC/GND)            ├─► ESC M3 GND
            ├─► ESC M4 (VCC/GND)            ├─► ESC M4 GND
            │                               │
            ▼                               ▼
     ┌──────────────┐                ┌──────────────┐
     │  BEC StepDown├────────────────┤  BEC GND     │
     │  (In: 12V)   │                └──────┬───────┘
     └──────┬───────┘                       │
            │ (Output 5.0V Tergulasi)       │
            ├───────────────────────────────┤ (Common Ground)
            ▼                               ▼
    ┌───────────────┐               ┌───────────────┐
    │ 5V ESP32      │               │ GND ESP32     │
    │ 5V FS-iA6B RX │               │ GND Receiver  │
    │ 5V GPS Neo-M8N│               │ GND GPS       │
    │ 3.3V/5V I2C   │               │ GND I2C Sensor│
    └───────────────┘               └───────────────┘
```

---

## 2. Pencegahan Noise: Common Ground & Kapasitor

> [!CAUTION]
> Motor brushless yang berputar cepat menghasilkan lonjakan arus (*voltage spikes*) dan derau elektromagnetik (*EMF noise*) yang dapat me-restart (brown-out) mikrokontroler ESP32 jika perkabelan tidak terisolasi dengan benar!

### Aturan Wajib Perkabelan:
1. **Common Ground (Satu Titik Acuan Negatif):** Seluruh pin GND (ESP32, Baterai, ESC, Receiver, dan semua sensor) **WAJIB** terhubung secara elektrik. Beda potensial GND dapat menyebabkan sinyal kontrol ESC rusak atau pembacaan sensor kacau.
2. **Kapasitor Peredam Low-ESR:** Pasang 1 unit kapasitor elektrolit berkapasitas **$1000\mu\text{F} - 2200\mu\text{F}$ (rating minimal 25V Low-ESR)** tepat pada titik percabangan kabel daya utama baterai masuk ke PDB/ESC.
   - Kaki Panjang (+) $\rightarrow$ Terhubung ke kabel Merah (Positif Baterai).
   - Kaki Pendek (-) $\rightarrow$ Terhubung ke kabel Hitam (Negatif Baterai).
3. **Pemisahan Jalur Daya dan Jalur Sinyal:** Jangan menyejajarkan kabel sinyal I2C (SDA/SCL) bersentuhan langsung dengan kabel daya 3-fase motor untuk menghindari induksi medan magnet.

---

## 3. Tabel Pinout Lengkap ESP32 vs ESP32-S3

Firmware mendukung dua jenis mikrokontroler target. Pinout telah dioptimasi untuk menghindari pin strapping bawaan ESP32:

| Perangkat / Jalur | Fungsi | Pin ESP32 Classic | Pin ESP32-S3 N16R8 | Keterangan |
| :--- | :--- | :--- | :--- | :--- |
| **Motor 1 (Front Left)** | MCPWM Sinyal ESC | **GPIO 25** | **GPIO 4** | Rotasi CCW |
| **Motor 2 (Front Right)** | MCPWM Sinyal ESC | **GPIO 14** | **GPIO 5** | Rotasi CW |
| **Motor 3 (Rear Left)** | MCPWM Sinyal ESC | **GPIO 26** | **GPIO 6** | Rotasi CW |
| **Motor 4 (Rear Right)** | MCPWM Sinyal ESC | **GPIO 27** | **GPIO 7** | Rotasi CCW |
| **I2C SDA** | Data Sensor Bus | **GPIO 21** | **GPIO 8** | Butuh pull-up 4.7kΩ ke 3.3V |
| **I2C SCL** | Clock Sensor Bus | **GPIO 22** | **GPIO 9** | Butuh pull-up 4.7kΩ ke 3.3V |
| **Receiver iBUS RX** | Serial Radio Remote | **GPIO 33** | **GPIO 18** | Terhubung ke pin iBUS FS-iA6B |
| **GPS Neo-M8N RX** | Serial GPS (ESP32 TX) | **GPIO 17** | **GPIO 16** | Ke pin RX modul GPS |
| **GPS Neo-M8N TX** | Serial GPS (ESP32 RX) | **GPIO 16** | **GPIO 17** | Ke pin TX modul GPS |
| **Battery Voltage ADC** | Monitor LiPo 3S | **GPIO 34** | **GPIO 1** | Melalui Voltage Divider |
| **Status Buzzer / LED** | Indikator Siaga/Fail | **GPIO 13** | **GPIO 21** | Active Buzzer 5V |

---

## 4. Diagram Sambungan Motor ESC

```
                     [DEPAN DRONE]

          M1 (CCW)                   M2 (CW)
         (Kiri Depan)              (Kanan Depan)
      ┌──────────────┐            ┌──────────────┐
      │  ESC M1 30A  │            │  ESC M2 30A  │
      └──────┬───────┘            └──────┬───────┘
             │ Putih (Sinyal)            │ Putih (Sinyal)
             │ ke GPIO 4                 │ ke GPIO 5
             │ (atau GPIO 25)            │ (atau GPIO 14)
             │                           │
             └───────────┬───────────────┘
                         │
                  ┌──────┴──────┐
                  │  ESP32-S3   │
                  │  FC BOARD   │
                  └──────┬──────┘
                         │
             ┌───────────┴───────────────┐
             │                           │
             │ Putih (Sinyal)            │ Putih (Sinyal)
             │ ke GPIO 6                 │ ke GPIO 7
             │ (atau GPIO 26)            │ (atau GPIO 27)
      ┌──────┴───────┐            ┌──────┴───────┐
      │  ESC M3 30A  │            │  ESC M4 30A  │
      └──────────────┘            └──────────────┘
          M3 (CW)                   M4 (CCW)
       (Kiri Belakang)           (Kanan Belakang)

                    [BELAKANG DRONE]
```

> **Catatan Kabel ESC Generic:**
> - Kabel Putih / Oranye = **Sinyal PWM** (masuk ke GPIO ESP32).
> - Kabel Hitam / Cokelat = **GND Sinyal** (gabungkan ke GND ESP32).
> - Kabel Merah (5V BEC internal ESC) = **LEPASKAN / ISOLASI** jika Anda menggunakan BEC eksternal terpisah. Jangan hubungkan 4 kabel 5V dari 4 ESC secara paralel ke pin 5V ESP32 karena dapat menimbulkan *loop current*.

---

## 5. Diagram Sambungan Bus Sensor I2C

Seluruh sensor gerak dan lingkungan berada pada jalur bus I2C yang sama:

```
                ESP32 / ESP32-S3
               ┌────────────────┐
               │ 3.3V       GND │
               │ SDA        SCL │
               └───┬───┬────┬───┘
       3.3V        │   │    │   GND
   ┌───────────────┤   │    │   ├──────────────┐
   │               │   │    │   │              │
   ▼               ▼   ▼    ▼   ▼              ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│  MPU9250     │ │  BMP280      │ │  VL53L1X     │
│  (Alamat     │ │  (Alamat     │ │  (Alamat     │
│   0x68)      │ │   0x76)      │ │   0x29)      │
└──────────────┘ └──────────────┘ └──────────────┘
```

- **Resistor Pull-up I2C:** Jika modul breakout sensor Anda belum memiliki pull-up internal, pasang resistor **$4.7\text{ k}\Omega$** antara pin **SDA ke 3.3V** dan **SCL ke 3.3V**.
- **Alamat I2C Terdaftar:**
  - `0x68`: Sensor Gyro & Accelerometer MPU9250
  - `0x0C`: Magnetometer AK8963 (Internal pass-through pada MPU9250)
  - `0x76`: Sensor Barometer BMP280
  - `0x29`: Sensor ToF Laser Lidar VL53L1X

---

## 6. Diagram Sambungan Komunikasi Serial

### A. Receiver FlySky FS-iA6B (Protokol iBUS)
Port iBUS mengirim paket serial satu arah berkecepatan 115200 bps:
```
Receiver FS-iA6B                  ESP32 / ESP32-S3
┌────────────────┐                ┌────────────────┐
│ [Port iBUS]    │                │                │
│ S (Sinyal) ────┼────────────────┼─► GPIO 18 (S3) │
│                │                │   (GPIO 33)    │
│ + (VCC 5V) ────┼─── BEC 5V      │                │
│ - (GND)    ────┼─── Common GND ─┼─► GND          │
└────────────────┘                └────────────────┘
```

### B. Modul GPS U-Blox Neo-M8N
Menggunakan jalur serial silang (TX ke RX, RX ke TX):
```
GPS Neo-M8N                       ESP32 / ESP32-S3
┌────────────────┐                ┌────────────────┐
│ VCC (5V)   ────┼─── BEC 5V      │                │
│ GND        ────┼─── Common GND ─┼─► GND          │
│ TX         ────┼────────────────┼─► GPIO 17 (RX2)│
│ RX         ◄───┼────────────────┼─── GPIO 16 (TX2)│
└────────────────┘                └────────────────┘
```

---

## 7. Rangkaian Sensor Pembagi Tegangan Baterai

Pin ADC ESP32 hanya aman menerima tegangan maksimal **$3.3\text{V}$**. Untuk membaca tegangan baterai LiPo 3S (hingga $12.6\text{V}$), diperlukan rangkaian pembagi tegangan (*voltage divider*):

```
LiPo (+) (12.6V Max)
    │
    │
   ┌┴┐
   │ │  R1 = 33 kΩ (1% Presisi)
   │ │
   └┬┘
    ├───────────────────────► Pin ADC ESP32 (GPIO 1 / GPIO 34)
   ┌┴┐
   │ │  R2 = 10 kΩ (1% Presisi)
   │ │
   └┬┘
    │
    │
LiPo (-) / GND
```

### Rasio Perhitungan Tegangan:
$$V_{\text{ADC}} = V_{\text{Baterai}} \times \frac{R_2}{R_1 + R_2} = 12.6\text{V} \times \frac{10}{33 + 10} = 2.93\text{V}$$
Tegangan $2.93\text{V}$ sangat aman di bawah batas $3.3\text{V}$. Pada firmware, nilai dikonversi kembali dengan pengali $\approx 4.30$.

---

## 8. Checklist Pengujian Perkabelan Sebelum Nyala

Sebelum mencolokkan baterai LiPo untuk pertama kali, lakukan pemeriksaan multi-tester:
- [ ] **Uji Hubungan Singkat (Short Circuit):** Ukur hambatan (Ohm meter) antara kabel Merah (+) dan Hitam (-) pada konektor XT60. Nilai harus menunjukkan tidak ada kontak langsung (*Open Circuit* atau hambatan naik bertahap mengisi kapasitor).
- [ ] **Koneksi Common Ground:** Pastikan pin GND ESP32, GND ESC, GND Receiver, dan GND Sensor tersambung nol Ohm ke kutub negatif baterai.
- [ ] **Polaritas BEC:** Pastikan output BEC tepat mengalirkan tegangan $5.0\text{V} \pm 0.1\text{V}$ sebelum kabel daya dihubungkan ke ESP32.
- [ ] **Pemberian Daya Awal Tanpa Propeller:** **LEPASKAN SEMUA PROPELLER** sebelum melakukan uji daya pertama kali!
