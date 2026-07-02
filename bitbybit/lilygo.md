# LILYGO T-Encoder-Pro ESP32-S3 Getting Started

This note is for the device ordered from:

https://www.aliexpress.us/item/3256806936883686.html

Product title: `LILYGO T-Encoder-Pro ESP32-S3 Rotary Encoder CHSC5816 ESP32-S3FN4R2`.

## What It Is

The T-Encoder-Pro is a compact ESP32-S3 rotary encoder board with a round display, capacitive touch, a rotary encoder with push button, and a small buzzer. It is useful for knobs, control panels, macro pads, menus, MIDI-style controllers, smart home controls, and small LVGL interfaces.

The exact Pro board is newer than the older LilyGO `T-Encoder` repository. Use Pro-specific examples and pin definitions when possible.

## Key Hardware

Confirmed from Pro-targeted firmware and the product title:

| Part | Detail |
| --- | --- |
| MCU | ESP32-S3, product title says `ESP32-S3FN4R2` |
| Display | 390 x 390 round QSPI display, SH8601-compatible driver used by community firmware |
| Touch | CHSC5816 capacitive touch controller on I2C |
| Input | Rotary encoder A/B plus encoder push button |
| Buzzer | GPIO-driven buzzer output |
| USB | ESP32-S3 USB programming/serial, depending on board boot mode and Arduino settings |

Important caveat: reseller listings and LilyGO revisions can change. When the board arrives, compare its silkscreen, included QR/docs, and any shipped test firmware against the pin map below.

## Pro Pin Map

These pins come from the `pins_config.h` in a community Arduino sketch made specifically for the LilyGO T-Encoder-Pro.

| Function | GPIO |
| --- | --- |
| Display QSPI CS | 10 |
| Display QSPI SCK | 12 |
| Display QSPI D0 | 11 |
| Display QSPI D1 | 13 |
| Display QSPI D2 | 7 |
| Display QSPI D3 | 14 |
| Display reset | 4 |
| Display power enable / VCI enable | 3 |
| I2C SDA | 5 |
| I2C SCL | 6 |
| Touch interrupt | 9 |
| Touch reset | 8 |
| Rotary encoder A | 1 |
| Rotary encoder B | 2 |
| Rotary encoder push button | 0 |
| Buzzer | 17 |

Display resolution constants:

```cpp
#define TFT_WIDTH 390
#define TFT_HEIGHT 390
```

## Recommended Development Paths

### Arduino IDE

This is the fastest path for initial bring-up.

1. Install Arduino IDE 2.x.
2. Add the Espressif ESP32 board package URL in Arduino preferences:

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

3. Install the `esp32` board package.
4. For the T-Encoder-Pro community LVGL example, use:

| Arduino Option | Value |
| --- | --- |
| Board | `ESP32S3 Dev Module` |
| USB CDC On Boot | `Enabled` |
| Flash Size | `16M` |
| Partition Scheme | `Huge App` |
| PSRAM | `OPI PSRAM` |
| Serial monitor | `115200` baud |

5. Install libraries as needed:

| Library | Notes |
| --- | --- |
| `LVGL` | Community Pro example was tested with LVGL `8.3.11`; LVGL 9 needs porting work. |
| `Ai Esp32 Rotary Encoder` | Useful for encoder-only tests and older LilyGO examples. |

For LVGL 8.3.x, the Pro smart-watch example notes these `lv_conf.h` settings:

```cpp
#if 1
#define LV_COLOR_16_SWAP 1
#define LV_MEM_CUSTOM 1
#define LV_TICK_CUSTOM 1
#define LV_FONT_MONTSERRAT_14 1
```

### PlatformIO

PlatformIO is a good option once basic USB flashing works.

For older LilyGO `T-Encoder` examples, the official repo uses:

```ini
[env:pico32]
platform = espressif32
board = pico32
framework = arduino
monitor_speed = 115200
lib_deps = igorantolic/Ai Esp32 Rotary Encoder@^1.4
```

For the Pro board, start with an ESP32-S3 board target instead of `pico32`, for example:

```ini
[env:t-encoder-pro]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
board_build.flash_size = 16MB
board_build.arduino.memory_type = qio_opi
build_flags =
  -DBOARD_HAS_PSRAM
```

You may need to adjust PlatformIO flash/PSRAM settings based on the installed Espressif platform version and the board revision.

## First Power-On Checklist

1. Use a known-good USB-C data cable, not a charge-only cable.
2. Plug the board into the computer and confirm a serial device appears.
3. If the board does not enter upload mode automatically, hold the encoder push button or the board `BOOT` button if exposed, tap reset, then upload.
4. Open serial monitor at `115200`.
5. Before flashing over factory firmware, note whether the shipped firmware demonstrates display, touch, encoder, buzzer, battery, or sleep behavior.

## Minimal Bring-Up Order

Bring up one subsystem at a time:

1. Serial-only blink/logging sketch.
2. Encoder A/B and push button.
3. Buzzer.
4. I2C scan for CHSC5816.
5. Touch coordinate read.
6. Display init and full-screen color fill.
7. LVGL UI.

This order makes failures much easier to isolate than starting with a full UI demo.

## Encoder Test Sketch

This tests the rotary encoder and push button without display or LVGL.

```cpp
#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>

#define KNOB_DATA_A 1
#define KNOB_DATA_B 2
#define KNOB_KEY 0
#define ROTARY_STEPS 4

AiEsp32RotaryEncoder encoder(
  KNOB_DATA_A,
  KNOB_DATA_B,
  KNOB_KEY,
  -1,
  ROTARY_STEPS
);

void IRAM_ATTR readEncoderISR() {
  encoder.readEncoder_ISR();
}

void setup() {
  Serial.begin(115200);
  encoder.begin();
  encoder.setup(readEncoderISR);
  encoder.setBoundaries(-100, 100, false);
  encoder.setAcceleration(0);
  Serial.println("T-Encoder-Pro encoder test");
}

void loop() {
  if (encoder.encoderChanged()) {
    Serial.println(encoder.readEncoder());
  }

  if (encoder.isEncoderButtonClicked()) {
    Serial.println("button");
  }

  delay(10);
}
```

## I2C Touch Sanity Check

The CHSC5816 touch controller is expected on the I2C bus:

```cpp
#include <Arduino.h>
#include <Wire.h>

#define IIC_SDA 5
#define IIC_SCL 6

void setup() {
  Serial.begin(115200);
  Wire.begin(IIC_SDA, IIC_SCL);
  Serial.println("I2C scan");

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found I2C device at 0x%02X\n", addr);
    }
  }
}

void loop() {}
```

If no I2C device appears, check that the correct board profile is selected, the board is powered correctly, and the GPIO map matches your revision.

## Display Notes

The Pro examples use a lean SH8601 display driver rather than the more generic Arduino_GFX stack. The display needs its hardware power rail enabled before initialization:

```cpp
pinMode(3, OUTPUT);
digitalWrite(3, HIGH);
```

The community example sets brightness with:

```cpp
lcd_brightness(200); // 0-255
```

For LVGL, keep the first demo simple: initialize the display, fill the screen, then add touch. Full LVGL demos can hide display-driver, memory, and touch problems behind one large failure.

## Useful Source Links

Additional local notes:

| Local doc | Purpose |
| --- | --- |
| [docs/lilygo/pin-reference.md](docs/lilygo/pin-reference.md) | Focused Pro pin and peripheral reference |
| [docs/lilygo/first-tests.md](docs/lilygo/first-tests.md) | Minimal serial, buzzer, I2C, encoder, and display-enable tests |
| [docs/lilygo/sources.md](docs/lilygo/sources.md) | Source links and provenance notes |

| Source | Link |
| --- | --- |
| Your product listing | https://www.aliexpress.us/item/3256806936883686.html |
| Official LilyGO T-Encoder repo | https://github.com/Xinyuan-LilyGO/T-Encoder |
| Official T-Encoder schematic, older/non-Pro repo | https://github.com/Xinyuan-LilyGO/T-Encoder/blob/main/schematic.pdf |
| Pro community smart-watch example | https://github.com/nikthefix/Lilygo_Support_T_Encoder_Pro_Smart_Watch |
| Pro pin config used above | https://github.com/nikthefix/Lilygo_Support_T_Encoder_Pro_Smart_Watch/blob/main/sls_encoder_pro_watch/pins_config.h |
| ESP32 Arduino board package | https://github.com/espressif/arduino-esp32 |
| Ai Esp32 Rotary Encoder library | https://github.com/igorantolic/ai-esp32-rotary-encoder |
| LVGL | https://lvgl.io/ |

## Common Problems

| Symptom | Likely Cause | Fix |
| --- | --- | --- |
| Upload fails | Wrong USB mode, wrong cable, not in bootloader | Use a data cable, enable USB CDC, try manual boot/reset sequence. |
| Serial monitor is blank | Wrong baud or USB CDC disabled | Use `115200`; enable USB CDC on boot for Arduino. |
| Display stays black | Display VCI enable not set, wrong driver, wrong board revision | Set GPIO 3 high before display init; verify SH8601/QSPI pins. |
| Touch not detected | Wrong I2C pins or reset/int pins | Use SDA 5, SCL 6, RST 8, INT 9; run I2C scanner. |
| LVGL build errors | LVGL 9 installed or missing `lv_conf.h` settings | Use LVGL 8.3.11 for existing Pro examples. |
| Encoder counts backward | A/B pins reversed in software | Swap GPIO 1 and 2 in the encoder constructor or invert logic. |
| Encoder button affects boot | Button is on GPIO 0 | Avoid holding the encoder button during normal reset unless entering bootloader. |

## Practical Next Steps

When the board arrives:

1. Photograph both sides before soldering or mounting anything.
2. Record what firmware ships on it.
3. Run the encoder test sketch.
4. Run the I2C scanner.
5. Flash a display color-fill test.
6. Only then move to LVGL or a full UI project.

Keep the older official `T-Encoder` repo around for reference, but do not blindly copy its board target or pinout into Pro code.
