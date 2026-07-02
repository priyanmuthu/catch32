# TurnKey Implementation Notes

This folder now has a first implementation scaffold for the plan.

## What Works In This Scaffold

- App mode framework for Home, Command, Focus, Meeting, and Coding.
- Icon-first LVGL UI screens.
- Encoder rotation, press, double-press, and long-press handling.
- Focus timer that runs locally on the ESP32.
- Newline-delimited JSON protocol over USB serial.
- Node.js companion that listens for device events and sends state back.
- Config-driven macro runner with dry-run safety for disabled commands.

## Hardware Driver Gap

`firmware/src/display.cpp` intentionally leaves the SH8601 display and CHSC5816 touch driver hookup as a TODO. Use the board bring-up examples from `../docs/lilygo/first-tests.md` and move the known-good panel/touch setup into that file.

The rest of the firmware should not need to change when the display driver is attached.

## First Hardware Bring-Up Flow

1. Run the first-tests guide for serial, encoder, buzzer, I2C, and display.
2. Copy the working SH8601 display init into `firmware/src/display.cpp`.
3. Build with PlatformIO.
4. Upload and confirm the Home screen draws.
5. Start the companion with an explicit serial port.

## Companion Safety

Macros and meeting commands only execute if their config entry has `"enabled": true`. Keep `config.local.json` private and review every command before enabling it.

