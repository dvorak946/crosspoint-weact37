# CrossPoint for WeActStudio — ESP32-S3

**Language:** English | [Bahasa Indonesia](README-WeAct37.id.md)

A development fork of [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader) for a DIY e-reader built around an **ESP32-S3 N16R8** and a **WeActStudio 3.7-inch GDEY037T03 / UC8253 e-paper display**.

This project adapts the display driver, board configuration, button navigation, themes, and reader features. The current development baseline is **black and white**, restored from the backup taken before the grayscale experiments on October 11, 2026.

## Upstream projects

- Main firmware: [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader).
- Hardware and UI SDK: [FreeInk SDK](https://github.com/Free-Ink/freeink-sdk).
- Display module: [WeActStudio EpaperModule](https://github.com/WeActStudio/WeActStudio.EpaperModule).

Upstream Git history, copyright notices, and license files are preserved. These modifications target this DIY hardware configuration; compatibility with every original CrossPoint device has not been tested.

## Hardware adaptations and features

| Area | Changes in this WeAct project |
|---|---|
| Display driver | Adapts the FreeInk SDK UC8253 driver for the WeActStudio GDEY037T03 panel and its black-and-white refresh path. |
| Board configuration | Dedicated PlatformIO environment `dv0_esp32s3_weact37` for the ESP32-S3 N16R8, display, SD card, and hardware buttons. |
| Navigation | Adapts the two-direction controls: GPIO1 / Nav1 moves right and GPIO2 / Nav2 moves left. Virtual keyboard and dictionary navigation are also adjusted. |
| RoundedRaff theme | Adapts the layout to the WeAct display, aligns tabs, refines selected states, and adjusts rounded button corners. Shorter menu labels: Files, Books, Share, and Sett. |
| Grid theme and covers | Adapts icons, cover layout, and navigation to the hardware controls. Grid behavior still needs to be retested against the published baseline. |
| Library | Book titles use up to two lines in bold size 8; authors use regular size 8. Category tabs are also adjusted. |
| Dictionary | Adjusts presentation, word selection navigation, compact dictionary fonts, and dictionary settings. |
| Text settings | Adds compact fonts and a text settings preview to inspect changes before reading. |
| Clock without an external RTC | A clock feature designed for use without an additional external RTC module, with clock/timezone settings and time synchronization over Wi-Fi. Behavior after deep sleep or complete power loss needs to be documented through device testing. |
| Wi-Fi and device information | Adjusts the Wi-Fi clock synchronization flow and information shown on the About screen. |
| Book covers | Adjusts BMP/JPEG/PNG cover processing, cover sizing, and cover caching for the device layout. |

This table summarizes the work on the DIY project. It is not a list of every built-in CrossPoint feature or a claim that every feature has passed regression testing on the hardware.

## The FreeInk SDK is modified too

`freeink-sdk` is a Git submodule. Committing only the main firmware does not save its local source changes.

The SDK fork needs to preserve modifications to:

- `libs/display/FreeInkDisplay/src/driver/Uc8253MurphyDriver.cpp`
- `libs/display/FreeInkDisplay/src/driver/Uc8253MurphyDriver.h`
- `libs/hardware/BoardConfig/include/BoardConfig.h`
- `libs/ui/FreeInkUI/include/components/bars/tab-bar.h`
- `libs/ui/FreeInkUI/include/components/keyboard/keyboard.h`

The main firmware repository must reference the SDK fork URL and an SDK commit that has been pushed. Download the submodule as well when fetching the project from GitHub.

## Build

Run PlatformIO from the project root:

```powershell
pio run --environment dv0_esp32s3_weact37
```

If PlatformIO is not on your Windows PATH:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
& "$pio" run --environment dv0_esp32s3_weact37
```

Application binary: `.pio/build/dv0_esp32s3_weact37/firmware.bin`.

## Validation status

- Local black-and-white build: **SUCCESS**, October 11, 2026.
- The pre-grayscale backup firmware was restored successfully; logs show the SD card reading dictionaries and an EPUB.
- The new build after source restoration had not been uploaded when this note was written.
- A build from a fresh GitHub clone has not yet been validated.
- Grayscale experiments are archived separately and are not the current development baseline.

## Local files excluded from source uploads

The `.cache/` cache, `.pio/` PlatformIO directory, patch backups, and `*.bak` files are excluded using `.gitignore`. Firmware binaries intended for flashing can be distributed through GitHub Releases.

This note supplements the original CrossPoint README.

