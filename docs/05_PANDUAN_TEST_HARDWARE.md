# 🧪 Panduan Pengujian Hardware Step-by-Step

Panduan pengujian bertahap (*hardware validation*) dari komponen paling mendasar hingga perakitan propeller untuk memastikan keamanan total sebelum terbang.

> [!IMPORTANT]
> **ATURAN EMAS KESELAMATAN:**
> Pengujian Test 1 sampai Test 7 **WAJIB DILAKUKAN TANPA MEMASANG BALING-BALING (PROPELLER LEPAS)**! Baling-baling hanya boleh dipasang pada tahapan terakhir setelah seluruh 7 tes lulus $100\%$.

---

## 📑 Daftar Isi
1. [Ikhtisar Urutan Pengujian](#1-ikhtisar-urutan-pengujian)
2. [Test 1 — I2C Bus Sensor Scanner](#2-test-1--i2c-bus-sensor-scanner)
3. [Test 2 — Kalibrasi Rentang Throttle ESC](#3-test-2--kalibrasi-rentang-throttle-esc)
4. [Test 3 — Verifikasi Receiver iBUS FlySky](#4-test-3--verifikasi-receiver-ibus-flysky)
5. [Test 4 — Uji Modul GPS Neo-M8N](#5-test-4--uji-modul-gps-neo-m8n)
6. [Test 5 — Uji Keseimbangan & Kalibrasi IMU](#6-test-5--uji-keseimbangan--kalibrasi-imu)
7. [Test 6 — Uji Putaran dan Arah Rotasi Motor](#7-test-6--uji-putaran-dan-arah-rotasi-motor)
8. [Test 7 — Uji Independensi Pin Motor (MCPWM)](#8-test-7--uji-independensi-pin-motor-mcpwm)
9. [Checklist Akhir Pemasangan Propeller](#9-checklist-akhir-pemasangan-propeller)

---

## 1. Ikhtisar Urutan Pengujian

```
┌───────────┐     ┌───────────┐     ┌───────────┐     ┌───────────┐
│  Test 1   │────►│  Test 2   │────►│  Test 3   │────►│  Test 4   │
│ I2C Scan  │     │ ESC Calib │     │ iBUS RX   │     │ GPS Fix   │
└───────────┘     └───────────┘     └───────────┘     └───────────┘
                                                            │
┌───────────┐     ┌───────────┐     ┌───────────┐           │
│ Propeller │◄────│  Test 7   │◄────│  Test 6   │◄────┌─────┴─────┐
│ Checklist │     │ MCPWM Pin │     │ Motor Dir │     │  Test 5   │
└───────────┘     └───────────┘     └───────────┘     │ IMU Check │
                                                      └───────────┘
```

---

## 2. Test 1 — I2C Bus Sensor Scanner

- **Folder Sketsa:** `test/1_test_i2c/1_test_i2c.ino`
- **Tujuan:** Memastikan semua sensor yang terhubung ke bus I2C (GPIO 8/9 pada ESP32-S3 atau GPIO 21/22 pada ESP32) merespons dan memiliki alamat yang valid.

### Langkah Uji:
1. Hubungkan ESP32 ke komputer via kabel USB (Baterai LiPo belum perlu dicolok).
2. Buka Serial Monitor pada baudrate **115200**.
3. **Hasil yang Diharapkan:**
   ```text
   Scanning I2C Addresses...
   [V] 0x0C : Magnetometer AK8963 (Internal MPU9250) - NORMAL
   [V] 0x29 : Sensor ToF Laser Lidar (VL53L1X) - TERDETEKSI
   [V] 0x68 : Sensor IMU 9-Axis (MPU9250) - TERDETEKSI
   [V] 0x76 : Sensor Barometer Tekanan Udara (BMP280) - TERDETEKSI
   Total 4 perangkat I2C ditemukan. Status: SUKSES!
   ```

---

## 3. Test 2 — Kalibrasi Rentang Throttle ESC

- **Folder Sketsa:** `test/2_test_esc/2_test_esc.ino`
- **Tujuan:** Mengajari 4 unit ESC batas pulsa sinyal maksimum ($2000\mu\text{s}$) dan minimum ($1000\mu\text{s}$) agar semua motor merespons throttle pada putaran yang seragam.

### Prosedur Wajib (Ikuti Urutan):
1. **Cabut baterai LiPo.**
2. Upload sketsa `2_test_esc.ino`.
3. Buka Serial Monitor (115200). Sketsa akan otomatis mengirimkan sinyal **MAX ($2000\mu\text{s}$)**.
4. **Colokkan baterai LiPo.**
5. ESC akan berbunyi nada pembuka: *beep-beep* (menandai batas atas terdeteksi).
6. Tekan tombol Enter pada Serial Monitor (sketsa akan mengirimkan sinyal **MIN ($1000\mu\text{s}$)**).
7. ESC akan berbunyi nada konfirmasi: *beep---beep-beep-beeep* panjang.
8. Kalibrasi ESC selesai dan tersimpan permanen di memori ESC.

---

## 4. Test 3 — Verifikasi Receiver iBUS FlySky

- **Folder Sketsa:** `test/3_test_receiver/3_test_receiver.ino`
- **Tujuan:** Memvalidasi penerimaan paket digital iBUS dari remote control FlySky FS-i6 / FS-i6X.

### Langkah Uji:
1. Nyalakan transmitter remote control FlySky.
2. Beri daya pada receiver (via BEC 5V atau pin 5V ESP32).
3. Buka Serial Monitor (115200).
4. Gerakkan stick remote dan periksa rentang angka:
   - Stick Throttle Bawah: `1000 - 1050`
   - Stick Throttle Atas: `1950 - 2000`
   - Stick Roll / Pitch / Yaw Tengah: `1495 - 1505`
   - Saklar SWA / SWB: Berpindah antara `1000` (Off) dan `2000` (On).

---

## 5. Test 4 — Uji Modul GPS Neo-M8N

- **Folder Sketsa:** `test/4_test_gps/4_test_gps.ino`
- **Tujuan:** Memeriksa komunikasi serial UART dengan modul GPS U-Blox dan penguncian satelit (*3D Sat Lock*).

### Langkah Uji:
1. Bawa drone ke area terbuka (dekat jendela atau luar ruangan).
2. Periksa LED biru/hijau pada modul GPS berkedip (indikasi data terkirim).
3. **Hasil yang Diharapkan:**
   - Deteksi kalimat NMEA valid.
   - Satelit terkunci: $\ge 6\text{ Satelit}$.
   - Data lintang (*latitude*), bujur (*longitude*), dan ketinggian (*altitude*) tampil presisi.

---

## 6. Test 5 — Uji Keseimbangan & Kalibrasi IMU

- **Folder Sketsa:** `test/5_test_imu/5_test_imu.ino`
- **Tujuan:** Membaca sudut Roll, Pitch, dan Yaw secara real-time dan memastikan tidak ada drift liar saat drone diletakkan di permukaan datar.

### Langkah Uji:
1. Letakkan drone di atas meja yang benar-benar rata air (*level surface*).
2. Upload sketsa dan biarkan diam selama 5 detik untuk kalibrasi offset gyro.
3. Miringkan drone ke depan: Nilai **Pitch** harus bertambah positif.
4. Miringkan drone ke kanan: Nilai **Roll** harus bertambah positif.
5. Putar drone ke kanan: Nilai **Yaw** harus bertambah.

---

## 7. Test 6 — Uji Putaran dan Arah Rotasi Motor

- **Folder Sketsa:** `test/6_test_motor/6_test_motor.ino`
- **Tujuan:** Memverifikasi masing-masing motor berputar perlahan satu per satu dan memastikan arah putaran sesuai skema Quad-X.

### Diagram Arah Wajib:
```
         DEPAN (Arah Terbang)

   M1 [CCW] ◄──●       ●──► M2 [CW]
  (Kiri Depan)           (Kanan Depan)

   M3 [CW]  ──►●       ●◄── M4 [CCW]
 (Kiri Belakang)         (Kanan Belakang)

               BELAKANG
```

> **Cara Membalik Arah Rotasi Motor:**
> Jika salah satu motor berputar ke arah yang salah, **tukar sembarang 2 dari 3 kabel** yang menghubungkan ESC ke motor brushless tersebut.

---

## 8. Test 7 — Uji Independensi Pin Motor (MCPWM)

- **Folder Sketsa:** `test/7_test_pin_independen/7_test_pin_independen.ino`
- **Tujuan:** Memastikan perbaikan driver hardware MCPWM bekerja sempurna dan tidak ada timer yang tertukar antar motor.

### Verifikasi:
- Uji mengirim perintah putar hanya ke **Motor 1** $\rightarrow$ Pastikan hanya Motor 1 yang berputar; Motor 2, 3, dan 4 harus diam total.
- Ulangi untuk Motor 2, Motor 3, dan Motor 4 secara bergantian.

---

## 9. Checklist Akhir Pemasangan Propeller

Lakukan checklist ini **HANYA JIKA TEST 1 SAMPAI TEST 7 SUDAH LULUS SEMUA**:

- [ ] Seluruh baut frame, motor mount, dan arm sudah dikencangkan dengan threadlocker (Loctite).
- [ ] Baterai LiPo terikat kencang menggunakan velcro strap ganda.
- [ ] **Propeller CW (Biasanya bertanda R / Reverse):** Dipasang di **Motor 2** dan **Motor 3**.
- [ ] **Propeller CCW (Propeller Standar):** Dipasang di **Motor 1** dan **Motor 4**.
- [ ] Mur pengunci baling-baling dikencangkan dengan kunci pas (tidak boleh longgar).
- [ ] Baling-baling tidak menyentuh kabel, antena GPS, atau receiver saat berputar.
- [ ] Siapkan lapangan terbuka yang aman, bebas dari kerumunan orang atau hewan peliharaan sebelum uji terbang pertama!

