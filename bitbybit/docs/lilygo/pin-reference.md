# T-Encoder-Pro Pin Reference

Source: Pro-targeted community firmware pin map:

https://github.com/nikthefix/Lilygo_Support_T_Encoder_Pro_Smart_Watch/blob/main/sls_encoder_pro_watch/pins_config.h

## Display

The T-Encoder-Pro display is configured as a 390 x 390 QSPI panel. The community firmware uses an SH8601-compatible driver.

| Signal | GPIO | Notes |
| --- | --- | --- |
| QSPI CS | 10 | Display chip select |
| QSPI SCK | 12 | Display clock |
| QSPI D0 | 11 | QSPI data |
| QSPI D1 | 13 | QSPI data |
| QSPI D2 | 7 | QSPI data |
| QSPI D3 | 14 | QSPI data |
| Reset | 4 | Display reset |
| VCI enable | 3 | Must be driven high before display init |
| TE | -1 | Not used in the referenced config |

Display constants:

```cpp
#define TFT_WIDTH 390
#define TFT_HEIGHT 390
#define SPI_FREQUENCY 40000000
```

## Touch

The Pro board in the referenced firmware uses a CHSC5816 capacitive touch controller.

| Signal | GPIO | Notes |
| --- | --- | --- |
| SDA | 5 | I2C data |
| SCL | 6 | I2C clock |
| INT | 9 | Touch interrupt |
| RST | 8 | Touch reset |

## Rotary Encoder

| Signal | GPIO | Notes |
| --- | --- | --- |
| Encoder A | 1 | Quadrature input |
| Encoder B | 2 | Quadrature input |
| Push button | 0 | Also ESP32 boot strapping pin; do not hold during normal reset unless entering bootloader |

If the encoder counts in the wrong direction, swap A/B in software.

## Buzzer

| Signal | GPIO | Notes |
| --- | --- | --- |
| Buzzer data | 17 | Use PWM/LEDC for tones |

## Boot And Flashing Notes

GPIO 0 is used by the encoder push button. On ESP32 boards, GPIO 0 is commonly involved in bootloader entry. If uploads fail, try this sequence:

1. Hold the encoder button.
2. Tap reset, or reconnect USB.
3. Start upload.
4. Release the encoder button once upload begins.

If your physical board has separate `BOOT` and `RST` buttons, use those first.
