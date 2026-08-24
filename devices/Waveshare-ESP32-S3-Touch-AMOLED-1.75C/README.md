# Waveshare ESP32-S3-Touch-AMOLED-1.75C Notes

The Waveshare `ESP32-S3-Touch-AMOLED-1.75C` is a compact ESP32-S3R8 smart-display development board in a CNC aluminum alloy case. It has a 1.75 inch 466 x 466 capacitive AMOLED touch display, Wi-Fi/BLE, IMU, power management, dual microphones, audio output, a built-in speaker, USB-C, and an internal space/header for a 3.7 V lithium battery.

Use this as a local bring-up checklist and source index before building firmware for the board.

## Links

[Product page](https://www.waveshare.com/esp32-s3-touch-amoled-1.75c.htm)

[Waveshare documentation](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75C)

[Resources and documents](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75C/Resources-And-Documents)

[Working with Arduino](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75C/Arduino)

[Working with ESP-IDF](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75C/ESP-IDF)

[GitHub repository](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C)

[Latest firmware releases](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/releases/latest)

[Arduino examples](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/tree/main/examples/arduino/examples)

[ESP-IDF examples](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/tree/main/examples/esp-idf)

[Managed BSP component](https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_1_75c)

## Official Documents

Hardware and vendor files:

- [Schematic](https://files.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75C/ESP32-S3-Touch-AMOLED-1.75C-schematic.pdf)
- [Repository schematic folder](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/tree/main/Schematic)
- [Firmware artifacts documentation](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/blob/main/docs/firmware.md)
- [Component notes](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/blob/main/docs/components.md)

Chip and peripheral manuals linked by Waveshare:

- [ESP32-S3 datasheet](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf)
- [ESP32-S3 technical reference manual](https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf)
- [QMI8658 datasheet](https://files.waveshare.com/wiki/common/QMI8658C.pdf)
- [AXP2101 datasheet](https://files.waveshare.com/wiki/common/X-power-AXP2101_SWcharge_V1.0.pdf)
- [ES8311 datasheet](https://files.waveshare.com/wiki/common/ES8311.DS.pdf)
- [ES8311 user manual](https://files.waveshare.com/wiki/common/ES8311.user.Guide.pdf)

The Waveshare resources page is the source of truth for current datasheet URLs because some files are hosted behind generated `files.waveshare.com` links.

## Quick Identity

| Item | Value |
| --- | --- |
| Product | Waveshare ESP32-S3-Touch-AMOLED-1.75C |
| SKUs | `33691` with battery, `33692` without battery |
| MCU/module | ESP32-S3R8 |
| CPU | Dual-core Xtensa LX7, up to 240 MHz |
| Wireless | 2.4 GHz Wi-Fi 802.11 b/g/n, Bluetooth 5 LE |
| SRAM/ROM | 512 KB SRAM, 384 KB ROM |
| PSRAM | 8 MB stacked PSRAM |
| Flash | Product docs list 32 MB NOR flash; GitHub overview currently says external flash is present but does not repeat the 32 MB value |
| Display | 1.75 inch AMOLED, 466 x 466, 16.7M colors |
| Display brightness | 700 cd/m2 per product page |
| Display contrast | 100000:1 per product page |
| Display bus | QSPI |
| Display driver | CO5300 |
| Touch | Capacitive touch, CST9217 over I2C |
| Power management | AXP2101 PMIC and battery charger/telemetry |
| Motion sensor | QMI8658 6-axis IMU |
| Audio input | Dual microphones via ES7210 ADC / echo-cancellation path |
| Audio output | ES8311 audio codec, built-in speaker and speaker pads |
| Buttons | PWR and BOOT side buttons |
| Battery | 3.7 V MX1.25 2-pin lithium battery connector; case has battery space |
| USB | USB-C, ESP32-S3 native USB for flashing and logs |
| Case | CNC-machined aluminum alloy case |
| Board support | ESP-IDF managed component `waveshare/esp32_s3_touch_amoled_1_75c` |

## Capabilities

The board is a good fit for:

- Compact touch dashboards and LVGL UIs.
- Watch-style or badge-style controllers.
- Wi-Fi/BLE status panels and API clients.
- Battery-powered prototypes with charge management and telemetry.
- Motion-aware interfaces using the QMI8658 accelerometer/gyro.
- Voice/audio experiments using the dual microphones and ES8311 audio output.
- AI/LLM companion UI experiments, which Waveshare explicitly targets in the product positioning.

## Software Stack

Recommended starting point:

1. Use the official GitHub examples first, because they include the board-specific display, touch, PMIC, sensor, and audio glue.
2. For ESP-IDF, prefer the managed component `waveshare/esp32_s3_touch_amoled_1_75c`.
3. For Arduino, use the libraries bundled under `examples/arduino/libraries` when reproducing vendor examples.
4. Keep LVGL versions aligned with the example set. Waveshare's Arduino setup notes currently use LVGL `v8.4.0`, while the repository also includes ESP-IDF LVGL 9 examples.

Supported toolchains from the Waveshare repository:

| Surface | Version |
| --- | --- |
| ESP-IDF | `v5.5.5` |
| ESP-IDF | `v6.0.2` |
| Arduino-ESP32 | `3.3.11` |

Arduino library versions from Waveshare's setup notes:

| Library/File | Version / note |
| --- | --- |
| GFX Library for Arduino | `v1.6.4` |
| SensorLib | `v0.3.3` |
| XPowersLib | `v0.2.6` |
| lvgl | `v8.4.0` |
| Mylibrary | Board pin macro definitions, install manually |
| lv_conf.h | Board LVGL config, install manually |

## Examples To Try First

ESP-IDF:

| Example | Purpose |
| --- | --- |
| `01_AXP2101` | Power management and battery telemetry |
| `02_lvgl_demo_v9` | LVGL 9 display demo |
| `03_esp-brookesia` | ESP-Brookesia app UI |
| `04_Immersive_block` | Motion-driven LVGL block demo |
| `05_Spec_Analyzer` | Microphone spectrum analyzer |

Arduino:

| Example | Purpose |
| --- | --- |
| `01_HelloWorld` | Display bring-up |
| `02_GFX_AsciiTable` | Text rendering |
| `03_LVGL_AXP2101_ADC_Data` | LVGL power telemetry UI |
| `04_LVGL_QMI8658_ui` | LVGL IMU UI |
| `05_LVGL_Widgets` | LVGL widgets, touch, and display interaction |
| `06_ES7210` | Microphone input |
| `07_ES8311` | Audio output |

## Bring-Up Checklist

1. Connect over USB-C and confirm the board appears as an ESP32-S3 USB serial/JTAG device.
2. Flash a current `*-combined.zip` firmware package from the GitHub releases page, or build and flash the `01_HelloWorld` Arduino example.
3. Verify display init, touch response, PMIC telemetry, IMU readings, microphone input, and speaker output with the vendor examples.
4. If flashing fails, close any serial monitor. If the board is stuck in bad firmware, fully power it off, hold `BOOT`, power it back on, then flash again.
5. After flashing from forced download mode, power-cycle or reset the board.

Ready-made combined firmware images are flashed at offset `0x0` and include the split binaries, flash arguments, helper scripts, and checksums in the release package.

## Pin and Hardware Notes

Waveshare has not published a separate `HARDWARE_REFERENCE.md` for the `1.75C` repository at the time these notes were written. Treat the schematic and the managed BSP component as authoritative for GPIO assignments.

Known internal buses and devices:

- CO5300 display uses QSPI.
- CST9217 touch controller uses I2C.
- AXP2101 PMIC is used for power rails, charging, and battery telemetry.
- QMI8658 provides accelerometer and gyroscope data.
- ES7210 handles microphone input.
- ES8311 handles audio codec output.
- USB-C is wired to the ESP32-S3 native USB interface for flashing and logs.
- BOOT is also a bootstrapping control; avoid forcing it during reset unless entering download mode.

Before using any exposed pads or repurposing pins, check the schematic for conflicts with display, touch, audio, PMIC, or boot strapping.

## Notes For This Repo

This device is distinct from the non-case `ESP32-S3-Touch-AMOLED-1.75` family. The `1.75C` variant has the aluminum case, built-in speaker, battery space, and product SKUs `33691`/`33692`. Do not assume pin maps from the existing LILYGO T4-S3 notes or the non-`C` Waveshare board without checking the schematic.
