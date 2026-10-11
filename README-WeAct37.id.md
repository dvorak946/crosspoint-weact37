# CrossPoint untuk WeActStudio — ESP32-S3

**Bahasa:** [English](README-WeAct37.en.md) | Bahasa Indonesia

Fork pengembangan dari [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader) untuk e-reader DIY berbasis **ESP32-S3 N16R8** dengan layar e-paper **WeActStudio 3,7 inci, GDEY037T03 / UC8253**.

Proyek ini mencakup penyesuaian driver layar, konfigurasi perangkat, navigasi tombol, tema, dan fitur pembaca. Versi pengembangan yang digunakan saat ini adalah **hitam-putih**, dipulihkan dari backup sebelum eksperimen grayscale tanggal 11 Oktober 2026.

## Asal proyek

- Firmware utama: [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader).
- SDK perangkat dan UI: [FreeInk SDK](https://github.com/Free-Ink/freeink-sdk).
- Modul layar: [WeActStudio EpaperModule](https://github.com/WeActStudio/WeActStudio.EpaperModule).

Riwayat Git, pemberitahuan hak cipta, dan file lisensi dari proyek asal dipertahankan. Penyesuaian di sini dibuat untuk konfigurasi perangkat DIY ini; dukungan semua perangkat CrossPoint asal belum diuji.

## Penyesuaian dan fitur

| Bagian | Perubahan pada proyek WeAct |
|---|---|
| Driver layar | Penyesuaian driver UC8253 di FreeInk SDK untuk panel WeActStudio GDEY037T03 dan jalur refresh hitam-putih. |
| Konfigurasi perangkat | Profil PlatformIO khusus `dv0_esp32s3_weact37` untuk ESP32-S3 N16R8, layar, SD card, dan tombol perangkat. |
| Navigasi | Penyesuaian tombol untuk perangkat dua arah: GPIO1 / Nav1 ke kanan dan GPIO2 / Nav2 ke kiri. Keyboard virtual dan navigasi kamus turut disesuaikan. |
| Tema RoundedRaff | Penyesuaian layout untuk layar WeAct, kesejajaran tab, tampilan pilihan, dan sudut membulat tombol. Label menu dibuat lebih ringkas: Files, Books, Share, dan Sett. |
| Tema Grid dan cover | Penyesuaian ikon, layout cover, serta navigasi untuk konfigurasi tombol perangkat. Implementasi Grid masih perlu diuji ulang pada baseline yang dipublikasikan. |
| Library | Judul buku maksimum dua baris, judul bold ukuran 8, penulis regular ukuran 8, serta penyesuaian tab kategori. |
| Kamus | Penyesuaian tampilan, navigasi pemilihan kata, font kamus yang lebih kecil, dan pengaturan kamus. |
| Pengaturan teks | Tambahan font kompak dan preview pengaturan teks untuk melihat perubahan sebelum membaca. |
| Jam tanpa RTC eksternal | Fitur jam yang dirancang untuk digunakan tanpa modul RTC eksternal tambahan, dengan pengaturan jam/zona waktu dan sinkronisasi waktu melalui Wi-Fi. Perilaku setelah deep sleep atau daya terputus perlu didokumentasikan dari pengujian perangkat. |
| Wi-Fi dan informasi perangkat | Penyesuaian alur sinkronisasi jam melalui Wi-Fi serta informasi pada halaman About. |
| Cover buku | Penyesuaian pemrosesan cover BMP/JPEG/PNG, ukuran cover, dan cache cover untuk tampilan perangkat. |

Daftar ini merangkum perubahan proyek DIY yang sudah dikerjakan. Ini bukan daftar seluruh fitur bawaan CrossPoint dan bukan klaim bahwa semua fitur telah lulus pengujian regresi pada perangkat.

## FreeInk SDK juga dimodifikasi

`freeink-sdk` adalah Git submodule. Perubahan lokal tidak cukup disimpan dengan commit firmware utama saja.

Fork SDK perlu menyimpan perubahan pada:

- `libs/display/FreeInkDisplay/src/driver/Uc8253MurphyDriver.cpp`
- `libs/display/FreeInkDisplay/src/driver/Uc8253MurphyDriver.h`
- `libs/hardware/BoardConfig/include/BoardConfig.h`
- `libs/ui/FreeInkUI/include/components/bars/tab-bar.h`
- `libs/ui/FreeInkUI/include/components/keyboard/keyboard.h`

Repository firmware utama harus menunjuk ke URL fork SDK dan commit SDK yang sudah diunggah. Saat mengambil source dari GitHub, submodule juga harus diunduh.

## Build

Gunakan PlatformIO dari direktori utama proyek:

```powershell
pio run --environment dv0_esp32s3_weact37
```

Jika PlatformIO tidak tersedia melalui PATH pada Windows:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
& "$pio" run --environment dv0_esp32s3_weact37
```

Hasil aplikasi: `.pio/build/dv0_esp32s3_weact37/firmware.bin`.

## Status pengujian

- Build hitam-putih pada komputer pengembang: **SUCCESS**, 11 Oktober 2026.
- Firmware backup sebelum grayscale berhasil dipulihkan; log menunjukkan SD dapat membaca kamus dan EPUB.
- Build baru setelah pemulihan source belum di-upload pada saat catatan ini dibuat.
- Build dari clone GitHub yang baru belum divalidasi.
- Eksperimen grayscale disimpan terpisah dan tidak menjadi baseline pengembangan saat ini.

## File lokal yang tidak diunggah sebagai source

Cache `.cache/`, folder PlatformIO `.pio/`, backup patch, dan file `*.bak` dikecualikan melalui `.gitignore`. Firmware yang ingin dibagikan untuk flashing dapat disediakan melalui GitHub Releases.

Catatan ini melengkapi README asli CrossPoint.

