# CrossPoint WeAct37 — ESP32-S3 N16R8

**English:** A CrossPoint fork for a DIY e-reader with a WeActStudio 3.7-inch display. Includes an adapted UC8253 display driver, board configuration, themes, button navigation, compact fonts, dictionary improvements, text preview, and a software clock with Wi-Fi synchronization, without an external RTC.

**Indonesia:** Fork CrossPoint untuk e-reader DIY dengan layar WeActStudio 3,7 inci. Mencakup penyesuaian driver UC8253, konfigurasi board, tema, navigasi tombol, font kompak, perbaikan kamus, preview teks, serta jam tanpa RTC eksternal dengan sinkronisasi melalui Wi-Fi.

## Hardware / Perangkat

| Component / Komponen | Specification / Spesifikasi |
|---|---|
| MCU | ESP32-S3 N16R8 |
| Flash / PSRAM | 16 MB / 8 MB |
| Display / Layar | WeActStudio 3.7", GDEY037T03, UC8253 |
| Resolution / Resolusi | 240 × 416 pixels |
| Storage / Penyimpanan | microSD melalui SPI |
| Controls / Kontrol | Back, Confirm, Nav1, Nav2, Power |
| External RTC / RTC eksternal | Not required / Tidak diperlukan |

### GPIO wiring / Sambungan GPIO

| Component / Komponen | ESP32-S3 GPIO |
|---|---|
| Display / Layar | SCK=12, DIN/MOSI=11, CS=10, DC=17, RST=16, BUSY=4 |
| microSD | SCK=12, MOSI=11, MISO=13, CS=9 |
| Back / Confirm | 38 / 39 |
| Nav1 right / kanan | 1 |
| Nav2 left / kiri | 2 |
| Power | 21 |

GPIO numbers are not physical connector positions. Connect power and ground according to your module specifications.

Nomor GPIO bukan nomor urutan pin konektor. Sambungkan daya dan ground sesuai spesifikasi modul.

## Install — Windows / Cara pasang

[**Download firmware / Unduh firmware**](https://github.com/dvorak946/crosspoint-weact37/releases/tag/weact37-bw-v0.1.0)

### English

1. Open the release link above. Under **Assets**, download **firmware.factory.bin**. For a first installation, only this file is needed.
2. Install [Python](https://www.python.org/downloads/) 3.10 or newer. Enable **Add Python to PATH** if offered.
3. Connect the board with a **USB data cable**. Open **Device Manager → Ports (COM & LPT)** and note its port, such as `COM10`. Close any serial monitor.
4. Open the folder containing the downloaded file in File Explorer. Type `powershell` in the address bar and press Enter.
5. Run the two commands below in order. Replace `COM10` with your actual port.

### Bahasa Indonesia

1. Buka tautan Release di atas. Pada bagian **Assets**, unduh **firmware.factory.bin**. Untuk pemasangan pertama, cukup file ini.
2. Pasang [Python](https://www.python.org/downloads/) versi 3.10 atau lebih baru. Aktifkan **Add Python to PATH** jika pilihan tersebut tersedia.
3. Sambungkan board dengan **kabel USB data**. Buka **Device Manager → Ports (COM & LPT)**, lalu catat portnya, misalnya `COM10`. Tutup serial monitor.
4. Buka folder berisi file unduhan melalui File Explorer. Ketik `powershell` pada bilah alamat, lalu tekan Enter.
5. Jalankan dua perintah berikut secara berurutan. Ganti `COM10` dengan port perangkat Anda.

```powershell
python -m pip install esptool
```

```powershell
python -m esptool --chip esp32s3 --port COM10 --baud 115200 write-flash 0x0 firmware.factory.bin
```

**English:** Keep USB connected until writing and verification finish successfully. Press RESET if necessary. If connection fails, hold BOOT, press and release RESET, release BOOT, check the COM port again, and retry. Insert a microSD containing your EPUB books while powered off; start the device and open **Files**.

**Indonesia:** Biarkan USB tersambung hingga penulisan dan verifikasi berhasil. Tekan RESET jika diperlukan. Jika gagal tersambung, tahan BOOT, tekan lalu lepaskan RESET, lepaskan BOOT, periksa kembali port COM, lalu ulangi. Pasang microSD berisi buku EPUB saat perangkat mati; hidupkan perangkat dan buka **Files**.

## Release notes / Catatan Release

- **Black-and-white / Hitam-putih.**
- Local build succeeded; the new release binary has not yet been retested on hardware. / Build lokal berhasil; binary Release baru belum diuji ulang pada perangkat.
- Factory flashing may reset internal settings. / Pemasangan factory dapat mereset pengaturan internal.
- This binary requires the hardware and wiring above. / Binary ini memerlukan hardware dan sambungan di atas.
- SDK and PlatformIO are unnecessary for installing the downloaded binary. / SDK dan PlatformIO tidak diperlukan untuk memasang binary unduhan.
- `firmware.bin` is for application updates at `0x10000` only with a compatible partition table. / `firmware.bin` untuk pembaruan aplikasi pada `0x10000` hanya dengan tabel partisi yang sesuai.

## Credits / Kredit

Based on / Dikembangkan dari [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader) and / dan [FreeInk SDK](https://github.com/Free-Ink/freeink-sdk). Existing licenses remain applicable. / Lisensi yang tersedia dalam repository tetap berlaku.
