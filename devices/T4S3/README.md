# LILYGO T4-S3 ESP32-S3 Prototyping Notes

The LILYGO T4-S3 is a compact ESP32-S3R8 smart-display board with a 2.41 inch AMOLED touch display, USB-C, battery charging, microSD, Qwiic expansion, and a 30-pin expansion header. The board is supported by LILYGO's `LilyGo-AMOLED-Series` Arduino/PlatformIO library and examples.

Use this as a bring-up checklist and local hardware reference before building project firmware.

## Links

[GitHub](https://github.com/Xinyuan-LilyGO)

[Wiki](https://wiki.lilygo.cc)

[YouTube](https://www.youtube.com/@lilygo)

## Quick Identity

| Item | Value |
| --- | --- |
| Product | LILYGO T4-S3 |
| MCU/module | ESP32-S3R8 |
| CPU | Dual-core Xtensa LX7, up to 240 MHz |
| Wireless | 2.4 GHz Wi-Fi 802.11 b/g/n, Bluetooth 5.0 LE |
| Flash | 16 MB |
| PSRAM | 8 MB OPI PSRAM |
| Display | 2.41 inch AMOLED touch display |
| Display resolution | Official wiki: 600 x 450; LILYGO library table: 450 x 600 |
| Display driver | RM690B0 |
| Touch controller | CST226/CST226SE family via I2C |
| Storage | microSD slot |
| Power input | USB-C and 3.7 V Li-Po battery connector |
| Battery/PMU chip | SY6970 |
| Expansion | 2 x Qwiic/STEMMA QT style JST-SH 1.0 mm I2C ports, 30-pin 1.27 mm header |
| USB | Native ESP32-S3 USB CDC/JTAG over USB-C |
| Initial hardware release | V1.0, 2023-12 per LILYGO wiki |

Display orientation is the reason the resolution appears both ways in sources: treat the panel as 450 x 600 in the LILYGO library's portrait coordinate system, then rotate in firmware as needed.

## Capabilities

The board is a good fit for:

- On-device dashboards and touch UIs using LVGL.
- Wi-Fi control panels, IoT status screens, and compact local web/API clients.
- BLE controls or provisioning flows.
- Battery-powered prototypes that need USB charging and low-power sleep.
- microSD-backed image/audio/data logging experiments.
- Qwiic/I2C sensor prototypes.
- USB CDC serial tools and USB-JTAG debugging.
- USB host experiments from the official AMOLED examples, with the caveat that USB CDC may need to be disabled for host mode.

Important ESP32-S3 capabilities available through the SoC include Wi-Fi, BLE, USB OTG/peripheral support, SPI, I2C, UART, PWM, ADC, touch-capable GPIOs, and deep sleep. The board-level display, touch, PMU, SD slot, and expansion wiring consume several pins, so check the pin map before assigning GPIOs.

## Projects In This Repo

- [FocusFrame](../../focusframe/README.md): simple Pomodoro timer firmware for this device.

## Software Stack

Recommended path:

1. Clone or install LILYGO's `LilyGo-AMOLED-Series`.
2. Use PlatformIO when possible. The official repo defaults to the `T-Display-AMOLED` environment, which covers the AMOLED family including T4-S3.
3. Start with `examples/Factory`.
4. Then run `examples/Touchpad`, `examples/LVGL_Rotation`, `examples/LVGL_SD_Images`, and `examples/PPM_Example_for_T4S3` depending on what you need to verify.

Arduino IDE settings from LILYGO:

| Setting | Value |
| --- | --- |
| Board | `ESP32S3 Dev Module` |
| USB CDC On Boot | `Enable` for serial logs over USB-C |
| CPU Frequency | `240MHz (WiFi)` |
| Flash Mode | `QIO 80MHz` |
| Flash Size | `16MB (128Mb)` |
| Partition Scheme | `16M Flash (3M APP/9.9MB FATFS)` |
| PSRAM | `OPI PSRAM` |
| Upload Mode | `UART0 / Hardware CDC` |
| Upload Speed | `921600` |
| USB Mode | `CDC and JTAG` |

Official PlatformIO baseline:

```ini
[env]
platform = espressif32@6.12.0
framework = arduino
upload_speed = 921600
monitor_speed = 115200
build_flags =
    -DBOARD_HAS_PSRAM
    -DLV_CONF_INCLUDE_SIMPLE
    -DDISABLE_ALL_LIBRARY_WARNINGS
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DCORE_DEBUG_LEVEL=1
lib_deps =
    lvgl/lvgl @ 8.4.0
    lewisxhe/XPowersLib @ 0.2.7
    lewisxhe/SensorLib @ 0.2.4
    bodmer/TFT_eSPI @ 2.5.31
```

The official README says the library is compatible with LVGL 9, but most examples are still LVGL 8.x. Start with LVGL 8.4.0 unless you are intentionally porting an example.

## Minimal Arduino Smoke Test

This checks board detection, display init, LVGL glue, and USB serial.

```cpp
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>

LilyGo_Class amoled;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  if (!amoled.beginAMOLED_241()) {
    Serial.println("T4-S3 AMOLED init failed");
    while (true) {
      delay(1000);
    }
  }

  beginLvglHelper(amoled);

  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "T4-S3 ready");
  lv_obj_center(label);
}

void loop() {
  lv_task_handler();
  delay(5);
}
```

## Pin Map From LILYGO Library

These values come from `LilyGo_AMOLED.h` in the official AMOLED library. Pins marked as internal should be treated as reserved unless you intentionally disable the related feature and understand the hardware impact.

### AMOLED Display, RM690B0

| Function | ESP32-S3 GPIO | Notes |
| --- | ---: | --- |
| Display data 0 | 14 | Internal display bus |
| Display data 1 | 10 | Internal display bus |
| Display data 2 | 16 | Internal display bus |
| Display data 3 | 12 | Internal display bus |
| Display SCK | 15 | Internal display bus |
| Display CS | 11 | Internal display bus |
| Display reset | 13 | Internal display reset |
| Display TE | 18 | Reserved; do not reuse |
| Display command bits | 8 | Library config |
| Display address bits | 24 | Library config |
| Display bus frequency | 36 MHz | Library config |

GPIO18 is explicitly called out by LILYGO as assigned to the AMOLED tearing-effect signal. Do not use it for expansion GPIO.

### Touch, PMU, and microSD

| Subsystem | Function | ESP32-S3 GPIO | Notes |
| --- | --- | ---: | --- |
| Shared I2C | SDA | 6 | Used by touch and SY6970 PMU |
| Shared I2C | SCL | 7 | Used by touch and SY6970 PMU |
| Touch CST226 | IRQ | 8 | Internal touch interrupt |
| Touch CST226 | Reset | 17 | Internal touch reset |
| PMU SY6970 | IRQ | 5 | Internal PMU interrupt |
| microSD SPI | MISO | 4 | Onboard SD slot |
| microSD SPI | MOSI | 2 | Onboard SD slot |
| microSD SPI | SCK | 3 | Onboard SD slot |
| microSD SPI | CS | 1 | Onboard SD slot |
| Button | BOOT/user button | 0 | Also ESP32 boot strap pin |

The Qwiic ports are I2C expansion ports. Because the official board library uses GPIO6/GPIO7 for the T4-S3 PMU and touch I2C bus, assume the Qwiic connectors are on the same SDA/SCL pair unless your schematic revision proves otherwise. Scan I2C before adding devices, and avoid address conflicts with the PMU/touch devices.

## Boot, Upload, and Serial

Normal upload path:

1. Connect USB-C.
2. Select the USB serial/JTAG port.
3. Build and upload at 921600 baud.
4. Monitor at 115200 baud.

Manual bootloader entry if upload fails or the USB port repeatedly appears/disappears:

1. Hold `BOOT`.
2. Press and release `RST`.
3. Release `BOOT`.
4. Upload firmware.
5. Press `RST` to leave download mode.

If powering externally or from battery with no USB terminal attached, consider disabling USB CDC on boot. LILYGO notes that USB CDC can block startup while waiting for a USB connection. In PlatformIO, replace `-DARDUINO_USB_CDC_ON_BOOT=1` with:

```ini
build_flags =
    -UARDUINO_USB_CDC_ON_BOOT
```

When USB CDC is disabled, serial output over USB-C will not appear the same way. Use the hardware UART pins from the ESP32-S3 if your firmware exposes them.

## Power and Battery Notes

The T4-S3 uses an SY6970 power-management/charger chip. LILYGO's T4-S3-specific PPM example shows the library exposing it as `amoled.SY`.

Useful calls from the official example:

```cpp
amoled.SY.setChargeTargetVoltage(4208); // mV, range 3840-4608, step 16
amoled.SY.setPrechargeCurr(64);         // mA, range 64-1024, step 64
amoled.SY.setChargerConstantCurr(832);  // mA, range 0-5056, step 64
amoled.SY.enableMeasure();
amoled.SY.enableCharge();
amoled.SY.disableCharge();
amoled.SY.disableStatLed();
amoled.SY.getVbusVoltage();
amoled.SY.getBattVoltage();
amoled.SY.getSystemVoltage();
amoled.SY.isCharging();
amoled.SY.isVbusIn();
```

Prototype cautions:

- Use a single-cell 3.7 V Li-Po battery on the battery connector; do not connect multi-cell packs.
- Confirm connector polarity before plugging in a battery.
- If no battery is connected, charger/battery readings may be misleading.
- LILYGO's example comments say USB-only operation may be cleaner with charging disabled, because charge behavior can affect VSYS.
- The official AMOLED repo lists working current around 90-230+ mA with Wi-Fi on at 240 MHz, and sleep current around 230 uA with external GPIO0 wakeup for T4-S3.

## First Bring-Up Checklist

1. Photograph both sides of your exact board revision before soldering headers.
2. Connect USB-C only, with no external modules attached.
3. Upload the official `Factory` example.
4. Verify display, touch, serial logs, and microSD detection.
5. Run `PPM_Example_for_T4S3` with and without battery attached and record expected VBUS/VBAT/VSYS values.
6. Run an I2C scanner on GPIO6/GPIO7 with no Qwiic module attached.
7. Add one Qwiic device at a time and rescan for address conflicts.
8. Only after the base board is verified, solder or connect to the 30-pin header.

## Suggested Prototyping Projects

- Touch dashboard: LVGL UI with Wi-Fi status, local controls, and battery state.
- microSD image viewer: load images from SD and display them with LVGL.
- Sensor console: Qwiic environmental sensor plus trend chart on the AMOLED.
- BLE controller: touch buttons that emit BLE HID or GATT commands.
- USB serial panel: host computer sends JSON over CDC and the board renders status.
- Battery runtime test: log VBAT/VSYS/current-state data to SD while varying brightness and Wi-Fi.

## Reserved Pins Summary

Avoid these for general expansion by default:

- GPIO0: BOOT/user button and wake source.
- GPIO1, GPIO2, GPIO3, GPIO4: onboard microSD SPI.
- GPIO5: SY6970 interrupt.
- GPIO6, GPIO7: shared internal I2C used by touch/PMU and likely Qwiic.
- GPIO8, GPIO17: touch IRQ/reset.
- GPIO10, GPIO11, GPIO12, GPIO13, GPIO14, GPIO15, GPIO16, GPIO18: AMOLED display bus/control.

Use the 30-pin header pinout diagram and schematic for final assignments. The LILYGO wiki has a pinout image, and the official repository contains the current schematic PDF.

## Source Links

- LILYGO T4-S3 wiki: https://wiki.lilygo.cc/products/t4-series/t4-s3/
- LILYGO AMOLED Series repository: https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series
- Official AMOLED README: https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series/blob/master/README.MD
- Official T4-S3 schematic in repo: https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series/blob/master/schematic/T4-S3-240719.pdf
- T4-S3 dimensions: https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series/tree/master/dimensions
- T4-S3 shell/mechanical reference: https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series/tree/master/shell
- ESP32-S3 datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf

Note: the official AMOLED README resource table currently appears to link the T4-S3 row to `T-Display-S3-AMOLED-Touch.pdf`, but the repository tree contains a dedicated `schematic/T4-S3-240719.pdf`. Prefer the dedicated T4-S3 schematic for this board.
