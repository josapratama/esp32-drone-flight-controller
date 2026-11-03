# 📶 Panduan Tuning PID dan Telemetri Nirkabel WiFi

Panduan sistematis untuk menyetel konstanta PID (Proportional, Integral, Derivative) serta memanfaatkan antarmuka web telemetri nirkabel bawaan ESP32 untuk penyesuaian parameter tanpa kabel.

---

## 📑 Daftar Isi
1. [Pengantar Penyetelan PID Multirotor](#1-pengantar-penyetelan-pid-multirotor)
2. [Tabel Nilai Awal PID (Baseline Configuration)](#2-tabel-nilai-awal-pid)
3. [Gejala dan Solusi Parameter P, I, dan D](#3-gejala-dan-solusi-parameter-p-i-dan-d)
4. [Langkah-Langkah Praktis Tuning di Lapangan](#4-langkah-langkah-praktis-tuning-di-lapangan)
5. [Tuning Penahan Ketinggian (Altitude Hold PID)](#5-tuning-penahan-ketinggian-altitude-hold-pid)
6. [Penggunaan Web Dashboard Telemetri WiFi ESP32](#6-penggunaan-web-dashboard-telemetri-wifi-esp32)
7. [Penyimpanan Parameter ke Flash NVS](#7-penyimpanan-parameter-ke-flash-nvs)

---

## 1. Pengantar Penyetelan PID Multirotor

Sistem PID adalah algoritma inti yang menjaga drone tetap tegak. Menyetel (*tuning*) PID adalah proses mencari angka keseimbangan agar respons drone cepat, patuh pada stick pilot, tetapi tidak berosilasi atau bergetar liar.

```
       Nilai Terlalu Rendah              NILAI IDEAL              Nilai Terlalu Tinggi
   ◄──────────────────────────────┼──────────────────────────────►
   Respons lambat / "lembek"      Terkunci kokoh (*locked-in*)    Osilasi cepat / bergetar
   Drone bergoyang tersapu angin   Tanpa getaran & overshoot      Motor panas & boros baterai
```

---

## 2. Tabel Nilai Awal PID

Gunakan nilai dasar berikut untuk drone ukuran frame 450mm dengan motor A2212 1400KV:

| Sumbu Kendali | Parameter | Nilai Awal | Rentang Aman | Fungsi Utama |
| :--- | :--- | :---: | :---: | :--- |
| **Roll (Sumbu X)** | **Kp** | **1.20** | $0.80 - 2.50$ | Kecepatan reaksi melawan kemiringan kiri/kanan |
| | **Ki** | **0.02** | $0.01 - 0.08$ | Mempertahankan sudut konstan saat ada angin samping |
| | **Kd** | **0.12** | $0.05 - 0.25$ | Meredam osilasi getaran saat berhenti miring |
| **Pitch (Sumbu Y)** | **Kp** | **1.20** | $0.80 - 2.50$ | Kecepatan reaksi melawan kemiringan maju/mundur |
| | **Ki** | **0.02** | $0.01 - 0.08$ | Mengoreksi ketidakseimbangan titik gravitasi baterai |
| | **Kd** | **0.12** | $0.05 - 0.25$ | Meredam osilasi setelah drone diperintahkan berhenti maju |
| **Yaw (Sumbu Z)** | **Kp** | **2.50** | $1.50 - 4.50$ | Respons putaran hadap haluan drone |
| | **Ki** | **0.05** | $0.02 - 0.12$ | Menahan kepala drone agar tidak bergeser sendiri |
| | **Kd** | **0.00** | $0.00 - 0.05$ | Umumnya 0 (karena torsi motor sudah memiliki redaman alami) |

---

## 3. Gejala dan Solusi Parameter P, I, dan D

### Proportional (P):
- **Terlalu Rendah:** Drone terasa lambat seperti "mengambang di atas agar-agar", susah dikendalikan, dan mudah tertiup angin.
  - *Solusi:* Naikkan Kp bertahap $+0.1$ hingga terasa responsif.
- **Terlalu Tinggi:** Drone bergetar cepat dengan frekuensi tinggi (*fast shaking / jitter*).
  - *Solusi:* Turunkan Kp sebesar $-0.2$.

### Integral (I):
- **Terlalu Rendah:** Saat diterbangkan maju lalu stick dilepas, posisi drone perlahan melayang condong ke belakang atau sudutnya bergeser sendiri (*angle wandering*).
  - *Solusi:* Naikkan Ki bertahap $+0.01$.
- **Terlalu Tinggi:** Drone bergoyang lambat (*slow wobble / oscillation* pada frekuensi $\approx 1-2\text{ Hz}$).
  - *Solusi:* Turunkan Ki sebesar $-0.01$.

### Derivative (D):
- **Terlalu Rendah:** Setelah manuver cepat lalu stick dilepas, drone memantul sesaat (*bounce-back overshoot*) sebelum diam.
  - *Solusi:* Naikkan Kd bertahap $+0.02$.
- **Terlalu Tinggi:** Suara motor mendesis kasar, dan motor terasa **sangat panas** setelah terbang 1 menit akibat mengamplifikasi derau getaran frekuensi tinggi.
  - *Solusi:* Segera turunkan Kd.

---

## 4. Langkah-Langkah Praktis Tuning di Lapangan

Lakukan proses penyetelan dengan urutan berikut:

```
┌──────────────────┐     ┌──────────────────┐     ┌──────────────────┐     ┌──────────────────┐
│  Langkah 1:      │────►│  Langkah 2:      │────►│  Langkah 3:      │────►│  Langkah 4:      │
│  Tuning Kp Roll  │     │  Tuning Kd Roll  │     │  Tuning Ki Roll  │     │  Salin ke Pitch  │
└──────────────────┘     └──────────────────┘     └──────────────────┘     └──────────────────┘
```

1. **Uji Sumbu Roll Dahulu:**
   - Atur $Ki = 0$ dan $Kd = 0$.
   - Naikkan $Kp$ perlahan sampai drone mulai menunjukkan sedikit getaran cepat (*oscillation threshold*).
   - Setelah getaran muncul, kurangi $Kp$ sekitar $20\%$ dari angka tersebut.
2. **Tambahkan Kd:**
   - Berikan sedikit sentuhan stick Roll ke kanan lalu lepas mendadak.
   - Jika badan drone memantul (*overshoot*), naikkan $Kd$ sampai drone berhenti terkunci secara instan tanpa pantulan.
3. **Tambahkan Ki:**
   - Terbangkan drone maju-mundur atau tahan di hembusan angin. Naikkan $Ki$ sampai sudut terkunci tanpa drift.
4. **Penyamaan ke Sumbu Pitch:**
   - Pada frame simetris Quad-X, karakteristik fisik Roll dan Pitch hampir identik. Anda dapat menyalin nilai akhir Roll ke Pitch.

---

## 5. Tuning Penahan Ketinggian (Altitude Hold PID)

Mode Altitude Hold menggunakan data Barometer BMP280 dan sensor ToF VL53L1X:
- **Alt Kp ($2.0$):** Menentukan seberapa cepat throttle motor dinaikkan/diturunkan saat ketinggian melenceng dari target.
- **Alt Ki ($0.5$):** Mengoreksi penurunan daya dorong saat tegangan baterai perlahan melemah.
- **Alt Kd ($0.8$):** Mencegah drone memantul naik-turun seperti bola bekel (*yo-yo effect*).

---

## 6. Penggunaan Web Dashboard Telemetri WiFi ESP32

Firmware dilengkapi antarmuka web mandiri yang di-host langsung oleh chip ESP32 tanpa memerlukan koneksi internet:

```
[Smartphone / Laptop Pilot]
            │
            │ (Koneksi WiFi: "ESP32-Drone-FC")
            ▼
┌────────────────────────┐
│     ESP32 SoftAP       │
│     192.168.4.1        │
├────────────────────────┤
│ • Live Angle Gauge     │
│ • Live Battery Voltage │
│ • Slider PID Kp,Ki,Kd  │
│ • Tombol SAVE TO NVS   │
└────────────────────────┘
```

### Prosedur Koneksi:
1. Nyalakan drone (dalam keadaan **DISARMED / Motor Mati**).
2. Di smartphone atau laptop Anda, cari jaringan WiFi bernama:
   - **SSID:** `ESP32-Drone-FC`
   - **Password:** `drone123` (atau sesuai konfigurasi di `config.h`)
3. Buka browser web (Chrome / Safari / Firefox) dan buka alamat:
   ```text
   http://192.168.4.1
   ```
4. Dashboard web interaktif akan menampilkan orientasi Roll/Pitch real-time, tegangan baterai LiPo, dan slider PID.

---

## 7. Penyimpanan Parameter ke Flash NVS

- Ketika Anda mengubah nilai slider PID pada Web Dashboard, nilai tersebut langsung diuji coba secara real-time ke memori RAM flight controller.
- Setelah menemukan karakter terbang yang paling pas, klik tombol **"SAVE TO NVS"** pada halaman web.
- Parameter akan ditulis secara permanen ke chip Flash internal ESP32 (Non-Volatile Storage) dan akan otomatis dimuat setiap kali drone dinyalakan kembali.
