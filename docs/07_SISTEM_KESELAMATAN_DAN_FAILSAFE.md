# 🛡️ Sistem Keselamatan Penerbangan dan Logika Failsafe

Panduan komprehensif mengenai prosedur keselamatan, sistem interlock pencegah kecelakaan (*arming safety*), deteksi tabrakan darurat (*crash detection*), dan mitigasi hilang sinyal radio (*failsafe*).

---

## 📑 Daftar Isi
1. [Filosofi Keselamatan Wahana Terbang](#1-filosofi-keselamatan-wahana-terbang)
2. [Sistem Interlock Arming (Syarat Aman Menyalakan Motor)](#2-sistem-interlock-arming)
3. [Deteksi Tabrakan & Sudut Miring Darurat (Crash Detection)](#3-deteksi-tabrakan--sudut-miring-darurat)
4. [Logika Failsafe Hilang Sinyal Radio (Lost Signal Mitigation)](#4-logika-failsafe-hilang-sinyal-radio)
5. [Proteksi Baterai Drop (Low Battery Voltage Warning)](#5-proteksi-baterai-drop)
6. [Checklist Pra-Terbang (Pre-Flight Safety Checklist)](#6-checklist-pra-terbang)
7. [Prosedur Tanggap Darurat di Lapangan](#7-prosedur-tanggap-darurat-di-lapangan)

---

## 1. Filosofi Keselamatan Wahana Terbang

> [!WARNING]
> Baling-baling drone yang berputar dengan motor brushless 1400KV pada tegangan 11.1V memiliki kecepatan linear ujung bilah (*tip speed*) melebihi **$200\text{ km/jam}$**. Bilah baling-baling tersebut memiliki energi kinetik yang cukup tajam untuk melukai tubuh manusia atau merusak properti.

Oleh karena itu, firmware mengadopsi prinsip **"Fail-to-Safe"** (Gagal dalam Kondisi Aman): jika terdeteksi anomali pada sensor, baterai, orientasi, atau sinyal radio, sistem secara otomatis memilih untuk **mematikan motor (*cut-off throttle*)** demi mencegah drone terbang liar tanpa kendali (*flyaway*).

---

## 2. Sistem Interlock Arming

Motor tidak akan pernah bisa di-arm (dinyalakan) jika salah satu dari kondisi pengaman berikut tidak terpenuhi:

```
                      [Pilot Menekan Saklar ARM]
                                  │
                                  ▼
           ┌──────────────────────────────────────────────┐
           │ 1. Apakah Stick Throttle Berada di Bawah?    │───► TIDAK: Tolak ARM!
           │    (Nilai iBUS < 1050µs)                     │
           └──────────────────────┬───────────────────────┘
                                  │ YA
                                  ▼
           ┌──────────────────────────────────────────────┐
           │ 2. Apakah Kemiringan Drone Datar?            │───► TIDAK: Tolak ARM!
           │    (Kemiringan Roll & Pitch < 15°)           │     (Buzzer Beep 2x)
           └──────────────────────┬───────────────────────┘
                                  │ YA
                                  ▼
           ┌──────────────────────────────────────────────┐
           │ 3. Apakah Sensor IMU Bekerja Normal?         │───► TIDAK: Tolak ARM!
           │    (I2C Sensor Terbaca, Kalibrasi Selesai)   │
           └──────────────────────┬───────────────────────┘
                                  │ YA
                                  ▼
           ┌──────────────────────────────────────────────┐
           │ 4. Apakah Sinyal Radio Receiver Stabil?      │───► TIDAK: Tolak ARM!
           │    (Paket iBUS Terverifikasi Checksum)       │
           └──────────────────────┬───────────────────────┘
                                  │ YA
                                  ▼
                   [STATUS: ARMED DIIZINKAN]
            (Buzzer Beep Panjang, Motor Idle Berputar)
```

---

## 3. Deteksi Tabrakan & Sudut Miring Darurat

Ketika drone berada di udara dan menabrak pohon, dinding, atau kehilangan keseimbangan drastis:
- **Batas Sudut Kritis:** $\mathbf{65^\circ}$ pada sumbu Roll maupun Pitch.
- **Waktu Respons Darurat:** $\mathbf{< 40\text{ milidetik}}$.
- **Aksi Firmware:**
  - Status langsung beralih ke `STATE_CRASH_DISARM`.
  - Keempat motor dihentikan instan ($1000\mu\text{s}$ cut-off).
  - Melindungi motor dan ESC dari panas berlebih akibat propeller tertahan objek (*stalled rotor protection*).

---

## 4. Logika Failsafe Hilang Sinyal Radio

Jika transmitter remote kehabisan baterai, sinyal terhalang gedung, atau antena rusak:

1. **Deteksi Time-out Non-Blocking:**
   - Driver `receiver.cpp` memeriksa cap waktu (*timestamp*) paket terakhir dari receiver iBUS.
   - Jika tidak ada paket valid selama **$> 500\text{ ms}$**, status `FAILSAFE` aktif.
2. **Tahap 1 — Penurunan Perlahan (Gentle Descent):**
   - Jika fitur GPS Navigasi belum aktif, throttle diturunkan perlahan secara bertahap selama 5 detik agar drone turun ke tanah dan tidak jatuh bebas.
3. **Tahap 2 — Auto Return-to-Home (Jika GPS Aktif):**
   - Jika satelit GPS terkunci $\ge 8$ satelit saat awal arming, drone akan otomatis naik ke ketinggian aman ($15\text{ meter}$) dan terbang lurus kembali ke koordinat lepas landas (*Home Location*).
4. **Tahap 3 — Full Cut-off:**
   - Setelah mendarat atau setelah batas waktu 10 detik, motor dimatikan permanen.

---

## 5. Proteksi Baterai Drop

Pemantauan tegangan LiPo 3S real-time melalui rangkaian pembagi tegangan:

| Tingkat Peringatan | Tegangan Total 3S | Rata-rata Per Sel | Tindakan Sistem |
| :--- | :---: | :---: | :--- |
| **Normal / Penuh** | $11.4\text{V} - 12.6\text{V}$ | $3.80\text{V} - 4.20\text{V}$ | Operasi penerbangan penuh. |
| **Peringatan 1 (Low Batt)** | **$10.5\text{V}$** | **$3.50\text{V}$** | Buzzer berbunyi bip berulang-ulang; pilot diimbau segera membawa drone pulang dan mendarat. |
| **Kritis (Critical Cut-off)** | **$9.9\text{V}$** | **$3.30\text{V}$** | Drone menonaktifkan manuver agresif dan memulai penurunan ketinggian otomatis untuk mencegah kerusakan baterai. |

---

## 6. Checklist Pra-Terbang (Pre-Flight Safety Checklist)

Sebelum menghubungkan baterai di lapangan, lakukan pengecekan berikut:

1. [ ] **Inspeksi Fisik:** Tidak ada retakan pada lengan frame (*arm*), motor terpasang kokoh, dan tidak ada kabel terkelupas.
2. [ ] **Propeller:** Dipasang dengan kencang, orientasi CW/CCW benar, dan tidak ada gompel/retak pada bilah.
3. [ ] **Remote Control:** Pastikan baterai remote penuh dan saklar arming berada pada posisi **OFF / DISARM**.
4. [ ] **Area Penerbangan:** Area lapang terbuka dengan radius aman minimal 20 meter dari penonton.
5. [ ] **Prosedur Power-up:**
   - Nyalakan transmitter remote control terlebih dahulu.
   - Letakkan drone di tanah datar.
   - Colokkan baterai LiPo drone dan **JANGAN GERAKKAN DRONE** selama 5 detik pertama agar sensor IMU selesai mengkalibrasi offset gyro.
   - Dengarkan nada buzzer pembuka siap terbang.

---

## 7. Prosedur Tanggap Darurat di Lapangan

- **Jika Drone Mulai Berosilasi Liar:** Segera matikan saklar arming (*Emergency Disarm*) agar drone jatuh ke rumput sebelum terbang semakin tinggi atau mengenai objek.
- **Jika Drone Mendarat Keras / Crash:**
  1. Pastikan saklar remote pada posisi **DISARM**.
  2. Jangan langsung menyentuh baling-baling; cabut kabel baterai LiPo terlebih dahulu.
  3. Raba suhu keempat motor. Jika ada salah satu motor yang sangat panas, periksa kemungkinan debu masuk atau kabel tertekan.

