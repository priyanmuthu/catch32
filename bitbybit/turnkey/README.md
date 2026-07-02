# TurnKey

TurnKey is a starter project for the LILYGO T-Encoder-Pro ESP32-S3 rotary encoder. It combines four desk-tool modes in one firmware:

- Command knob
- Focus timer
- Meeting status button
- Coding macro pad

The project is split into:

- `firmware/`: PlatformIO Arduino firmware for the device.
- `companion/`: Node.js serial companion that maps device events to desktop actions.
- `docs/`: bring-up and implementation notes.

Start with the hardware guide in `../docs/lilygo.md`, then build the firmware shell.

## Firmware

```sh
cd bitbybit/turnkey/firmware
pio run
pio run --target upload
pio device monitor --baud 115200
```

The firmware currently includes the app framework, input handling, JSON serial protocol, and UI hooks. The display file uses LVGL object creation but keeps the panel/touch driver initialization isolated in `src/display.cpp` so it can be wired to the exact SH8601/CHSC5816 driver from the board test project.

## Companion

```sh
cd bitbybit/turnkey/companion
npm install
cp config.example.json config.local.json
npm start -- --config config.local.json
```

To choose a serial port explicitly:

```sh
npm start -- --port /dev/cu.usbmodemXXXX --config config.local.json
```

By default, shell commands are dry-run unless a config entry sets `"enabled": true`.

