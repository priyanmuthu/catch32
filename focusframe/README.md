# FocusFrame

FocusFrame is a simple Pomodoro timer for the [LILYGO T4-S3](../devices/T4S3/README.md). It uses the board's AMOLED touch display for a timer UI and the SY6970 power chip for basic battery/USB status.

## Project Name

Brainstormed names:

- FocusFrame
- Tiny Tomato
- TimeTile
- FlowPanel
- SprintScreen

Chosen name: **FocusFrame**. It fits the project because the T4-S3 is mostly a display-first device, and the timer is meant to frame one focused work session at a time.

## What You Will Build

The first version has:

- 25 minute focus timer.
- 5 minute break timer.
- Touch controls for `Start`, `Pause`, `Reset`, and `Skip`.
- Progress bar.
- Automatic switch from focus to break, and break back to focus.
- Battery or USB voltage status at the top-right of the screen.

This is intentionally small. The goal is to get one complete firmware project compiling and running before adding Wi-Fi, settings, sounds, storage, or nicer graphics.

## Hardware

Required:

- LILYGO T4-S3 ESP32-S3 board.
- USB-C data cable.
- Computer with VS Code and PlatformIO.

Optional:

- Single-cell 3.7 V Li-Po battery.
- microSD card for future versions.
- Qwiic sensors for future versions.

Device notes and pin references are in [../devices/T4S3/README.md](../devices/T4S3/README.md).

## Install Tools

1. Install VS Code.
2. Install the PlatformIO extension in VS Code.
3. Open this repository folder in VS Code:

   ```txt
   /Users/priyan/workspaces/catch32
   ```

4. Open the PlatformIO project folder:

   ```txt
   focusframe
   ```

PlatformIO will download the ESP32 platform and libraries the first time you build.

## Project Files

```txt
focusframe/
├── README.md
├── boards/
│   └── T-Display-AMOLED.json
├── platformio.ini
└── src/
    └── main.cpp
```

Important files:

- `boards/T-Display-AMOLED.json`: LILYGO's PlatformIO board definition for the AMOLED series.
- `platformio.ini`: board, build flags, upload speed, and library dependencies.
- `src/main.cpp`: the timer application.

## Build Settings

The project is configured for:

- ESP32-S3 Arduino framework.
- 16 MB flash.
- OPI PSRAM.
- USB CDC serial on boot.
- LVGL 8.4.0.
- LILYGO AMOLED Series library.

These settings match the T4-S3 guidance in the device docs.

## Upload The Firmware

1. Connect the T4-S3 with USB-C.
2. In VS Code, open PlatformIO.
3. Select the `focusframe` project.
4. Click `Build`.
5. Click `Upload`.
6. Click `Monitor` to view serial logs.

Equivalent terminal commands from this folder:

```sh
cd /Users/priyan/workspaces/catch32/focusframe
pio run
pio run --target upload
pio device monitor --baud 115200
```

Expected serial output:

```txt
FocusFrame starting...
```

The board screen should show:

- `Focus`
- `25:00`
- `Tap start when ready`
- battery or USB status
- Start, Reset, and Skip buttons

## If Upload Fails

Put the board into bootloader mode:

1. Hold `BOOT`.
2. Press and release `RST`.
3. Release `BOOT`.
4. Upload again.
5. Press `RST` once after upload.

If the USB port keeps appearing and disappearing, repeat the same bootloader sequence.

## How The Code Works

The firmware starts in `setup()`:

1. Starts USB serial.
2. Calls `amoled.beginAMOLED_241()` to initialize the T4-S3 display, touch, PMU, and SD support.
3. Calls `beginLvglHelper(amoled)` to connect LVGL to the display/touch driver.
4. Builds the screen using LVGL widgets.

The main loop does three things:

1. Calls `lv_task_handler()` so LVGL can process touch and redraws.
2. Calls `tickTimer()` to subtract time once per second while running.
3. Updates the battery/USB label every 10 seconds.

The timer state is deliberately simple:

- `TimerMode::Focus`: 25 minutes.
- `TimerMode::Break`: 5 minutes.
- `running`: whether the timer is counting down.
- `remainingSeconds`: current countdown value.

When a focus session reaches zero, the app switches to break mode and pauses. When a break reaches zero, it switches back to focus mode and pauses.

## First Changes To Try

Change timer lengths in `src/main.cpp`:

```cpp
constexpr uint32_t kFocusSeconds = 25 * 60;
constexpr uint32_t kBreakSeconds = 5 * 60;
```

For quick testing, use:

```cpp
constexpr uint32_t kFocusSeconds = 30;
constexpr uint32_t kBreakSeconds = 10;
```

Change the accent color:

```cpp
lv_color_hex(0x4FD1C5)
```

Useful next features:

- Add a long break every 4 focus sessions.
- Add a settings screen for timer lengths.
- Save settings to flash.
- Add screen dimming while idle.
- Add Wi-Fi time sync.
- Add a small completion sound if you attach a buzzer.
- Log completed sessions to microSD.

## Pin Safety

Do not reuse display, touch, PMU, SD, or BOOT pins until you understand the T4-S3 pin map. The display alone reserves several GPIOs. Start with the device docs before connecting anything to the 30-pin header:

[../devices/T4S3/README.md](../devices/T4S3/README.md)
