# ⚖️ Kalkulasi Thrust, Beban, dan Rasio Daya Angkat Drone

Panduan teknis mengenai perhitungan gaya dorong (*thrust*), penentuan rasio daya terhadap bobot (*Thrust-to-Weight Ratio*), pemilihan motor/propeller, serta estimasi durasi terbang.

---

## 📑 Daftar Isi
1. [Konsep Dasar Gaya Angkat (Thrust)](#1-konsep-dasar-gaya-angkat-thrust)
2. [Hukum Rasio Daya Angkat terhadap Bobot (TWR)](#2-hukum-rasio-daya-angkat-terhadap-bobot-twr)
3. [Spesifikasi Motor A2212 1400KV & Propeller](#3-spesifikasi-motor-a2212-1400kv--propeller)
4. [Tabel Rincian Bobot Komponen (All-Up Weight / AUW)](#4-tabel-rincian-bobot-komponen)
5. [Kalkulasi Titik Melayang (Hovering Point)](#5-kalkulasi-titik-melayang-hovering-point)
6. [Beban Muatan Maksimal (Payload Capacity)](#6-beban-muatan-maksimal-payload-capacity)
7. [Rumus Estimasi Durasi Terbang (Flight Time)](#7-rumus-estimasi-durasi-terbang-flight-time)
8. [Tips Optimasi Efisiensi Energi](#8-tips-optimasi-efisiensi-energi)

---

## 1. Konsep Dasar Gaya Angkat (Thrust)

**Thrust** adalah gaya mekanis yang dihasilkan oleh putaran baling-baling yang memindahkan massa udara ke bawah dengan kecepatan tinggi, menciptakan dorongan ke atas yang melawan gravitasi bumi.

Dalam dunia multirotor, thrust umumnya diukur dalam satuan **gram (g)** atau **kilogram (kg)** gaya angkat statis:
$$F = \dot{m} \cdot v_e$$
- $\dot{m}$ = Laju aliran massa udara yang ditarik baling-baling.
- $v_e$ = Kecepatan hembusan udara ke bawah (*exit velocity*).

---

## 2. Hukum Rasio Daya Angkat terhadap Bobot (TWR)

**Thrust-to-Weight Ratio (TWR)** adalah perbandingan antara total daya dorong maksimal seluruh motor pada throttle 100% terhadap total bobot keseluruhan drone (AUW):

$$\text{TWR} = \frac{\text{Total Thrust Maksimal (4 Motor)}}{\text{Total Berat Drone Termasuk Baterai (AUW)}}$$

```
                ▲ TOTAL THRUST MAKSIMAL (contoh: 2400g)
                │
         ┌──────┴──────┐
         │             │
         │    DRONE    │  Rasio Ideal = 2.0 : 1
         │             │
         └──────┬──────┘
                │
                ▼ TOTAL BERAT DRONE / AUW (contoh: 1000g)
```

### Standar Karakteristik TWR Multirotor:
| Nilai Rasio TWR | Kategori Penerbangan | Karakteristik Wahana |
| :---: | :--- | :--- |
| **$< 1.2 : 1$** | **Gagal / Sangat Bahaya** | Tidak mampu terbang stabil. Saat angin kencang drone akan terseret jatuh karena motor kehabisan tenaga untuk bermanuver. |
| **$1.5 : 1$** | Minimal Penerbangan Santai | Mampu terbang tenang (*gentle cruising*), tetapi lamban bereaksi saat pemulihan darurat. |
| **$2.0 : 1$** | **Target Standar Ideal (Platform Ini)** | Drone melayang stabil tepat di throttle **$50\%$**, memiliki cadangan tenaga $50\%$ untuk koreksi PID cepat dan melawan angin kencang. |
| **$3.0 : 1 - 5.0 : 1$** | Drone Balap (FPV Racing / Acro) | Akselerasi ekstrem, sangat lincah, manuver freestyle akrobatik. |

---

## 3. Spesifikasi Motor A2212 1400KV & Propeller

Konfigurasi standar drone ini menggunakan motor **A2212/10T 1400KV** dengan baterai **LiPo 3S (11.1V - 12.6V)**:

### Data Uji Statis Motor A2212 1400KV (Per Motor pada 11.1V):
| Ukuran Propeller | Throttle 50% (Hover) | Throttle 100% (Maksimal) | Arus Maksimal (Ampere) | Efisiensi (g/Watt) |
| :--- | :---: | :---: | :---: | :---: |
| **8045 (8x4.5 inci)** | **$300\text{g}$** | **$620\text{g}$** | $12.5\text{ A}$ | $4.5\text{ g/W}$ |
| **9047 (9x4.7 inci)** | **$380\text{g}$** | **$780\text{g}$** | $16.0\text{ A}$ | $4.2\text{ g/W}$ |
| **1045 (10x4.5 inci)** | **$450\text{g}$** | **$880\text{g}$** | $19.5\text{ A}$ *(Perlu ESC $\ge 30$A)* | $3.9\text{ g/W}$ |

> [!TIP]
> Untuk motor 1400KV pada baterai LiPo 3S, baling-baling ukuran **8045 atau 9047** adalah titik keseimbangan paling ideal antara gaya angkat dan suhu kerja motor. Baling-baling 1045 memberikan gaya angkat terbesar, tetapi membuat motor cepat panas jika digeber terus menerus.

---

## 4. Tabel Rincian Bobot Komponen

Berikut adalah estimasi berat seluruh komponen drone saat siap terbang (*All-Up Weight / AUW*):

| Kategori Komponen | Komponen Spesifik | Qty | Bobot Satuan | Total Bobot |
| :--- | :--- | :---: | :---: | :---: |
| **Rangka & Struktur** | Frame Quadcopter F450 (Nylon + PCB) | 1 | $280\text{g}$ | $280\text{g}$ |
| **Propulsi** | Motor Brushless A2212 1400KV + Mount | 4 | $48\text{g}$ | $192\text{g}$ |
| **Kontrol Daya** | ESC 30A SimonK / Generic | 4 | $25\text{g}$ | $100\text{g}$ |
| **Baling-baling** | Propeller 8045 / 9047 (Nylon) | 4 | $9\text{g}$ | $36\text{g}$ |
| **Baterai Utama** | LiPo 3S 2200mAh 35C XT60 | 1 | $185\text{g}$ | $185\text{g}$ |
| **Komputer FC** | Board ESP32-S3 + Shield/Breakout | 1 | $25\text{g}$ | $25\text{g}$ |
| **Regulator BEC** | Step-down Buck Converter 5V 3A | 1 | $12\text{g}$ | $12\text{g}$ |
| **Komunikasi** | Receiver FlySky FS-iA6B | 1 | $15\text{g}$ | $15\text{g}$ |
| **Sensor IMU & Alt** | MPU9250 + BMP280 | 1 set | $6\text{g}$ | $6\text{g}$ |
| **Sensor Navigasi** | GPS U-Blox Neo-M8N + Tiang Mount | 1 | $35\text{g}$ | $35\text{g}$ |
| **Sensor Jarak** | ToF Lidar VL53L1X | 1 | $3\text{g}$ | $3\text{g}$ |
| **Aksesoris** | Kabel silikon, konektor XT60, kapasitor, baut | 1 set | $45\text{g}$ | $45\text{g}$ |
| **TOTAL BOBOT (AUW)** | **Siap Terbang (Tanpa Payload Tambahan)** | - | - | **$\approx 934\text{ gram}$** |

---

## 5. Kalkulasi Titik Melayang (Hovering Point)

Menggunakan propeller **8045** pada baterai LiPo 3S:
- **Total Thrust Maksimal (4 Motor):**
  $$\text{Thrust}_{\text{Total}} = 4 \times 620\text{g} = 2480\text{ gram}$$
- **Rasio TWR pada Bobot $934\text{g}$:**
  $$\text{TWR} = \frac{2480\text{g}}{934\text{g}} = \mathbf{2.65 : 1}$$

### Menghitung Posisi Stick Throttle saat Hover:
$$\text{Thrust Per Motor saat Hover} = \frac{934\text{g}}{4} = 233.5\text{ gram per motor}$$
Dengan kurva performa motor A2212 1400KV, gaya dorong $233.5\text{g}$ dicapai pada throttle **$\approx 42\% - 45\%$**.
- **Hasil Analisis:** Sangat ideal! Stick throttle berada tepat di bawah titik tengah (50%), menandakan drone sangat lincah dan memiliki cadangan daya dorong yang melimpah untuk stabilisasi angin kencang.

---

## 6. Beban Muatan Maksimal (Payload Capacity)

Untuk menjaga batas aman penerbangan stabil (TWR tidak boleh kurang dari $1.8 : 1$):
$$\text{Berat Total Maksimal yang Diizinkan} = \frac{\text{Total Thrust}}{1.8} = \frac{2480\text{g}}{1.8} \approx 1377\text{ gram}$$

$$\text{Kapasitas Muatan Beban (Payload)} = 1377\text{g} - 934\text{g} = \mathbf{443\text{ gram}}$$

> **Artinya:** Drone ini sanggup membawa muatan tambahan seperti **Kamera Aksi (GoPro/Insta360 $\approx 120\text{g}$)**, Gimbal 2-axis ($\approx 150\text{g}$), atau baterai kedua secara paralel dengan aman tanpa kehilangan stabilitas kendali.

---

## 7. Rumus Estimasi Durasi Terbang (Flight Time)

Estimasi durasi melayang (*Hover Flight Time*) dihitung berdasarkan konsumsi daya saat melayang:

1. **Konsumsi Arus Saat Melayang (Hover Current):**
   - Tiap motor menghasilkan $233.5\text{g}$ dorongan pada arus $\approx 4.2\text{ Ampere}$.
   - Total arus 4 motor + sistem elektronik:
     $$I_{\text{hover}} = (4 \times 4.2\text{A}) + 0.3\text{A (ESP32 + Sensor)} = 17.1\text{ Ampere}$$
2. **Kapasitas Baterai Efektif (Aturan Aman 80% DoD):**
   - Baterai LiPo $2200\text{mAh} = 2.2\text{Ah}$.
   - Kapasitas yang boleh dipakai agar baterai tidak rusak ($80\%$):
     $$C_{\text{efektif}} = 2.2\text{Ah} \times 0.80 = 1.76\text{ Ah}$$
3. **Kalkulasi Menit Terbang:**
   $$\text{Waktu Terbang} = \frac{C_{\text{efektif}}}{I_{\text{hover}}} \times 60\text{ menit} = \frac{1.76}{17.1} \times 60 \approx \mathbf{6.17\text{ menit}}$$

> Jika menggunakan baterai LiPo 3S **3000mAh**, waktu terbang meningkat menjadi **$\approx 8.5 - 9.5$ menit**.

---

## 8. Tips Optimasi Efisiensi Energi
1. **Pilih Propeller Lebih Besar dengan KV Rendah:** Jika tujuan utama adalah terbang durasi panjang (*endurance*), gunakan motor 920KV–1000KV dengan propeller 1045.
2. **Kerapian Kabel & Struktur Ringan:** Potong kabel yang terlalu panjang untuk memangkas bobot gram demi gram.
3. **Hindari Manuver Throttle Liar:** Penerbangan dengan gerakan stick yang halus menghemat konsumsi energi hingga 25% dibanding akselerasi mendadak.

