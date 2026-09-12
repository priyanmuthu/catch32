# Iris

A touch weather clock named Iris for the Waveshare ESP32-S3-Touch-AMOLED-1.75C. The firmware uses the official Waveshare managed BSP and LVGL 9, keeps the clock usable without Wi-Fi, and fetches a three-day forecast from [Open-Meteo](https://open-meteo.com/).

## What is implemented

- 466×466 AMOLED bring-up through `waveshare/esp32_s3_touch_amoled_1_75c`.
- LVGL clock screen with local time, date, location, and connection state.
- Tap anywhere to switch between the clock and weather screens.
- Wi-Fi station mode with automatic reconnect.
- SNTP time synchronization and configurable IANA timezone.
- HTTPS Open-Meteo request with certificate-bundle validation.
- Current temperature, humidity, wind, condition text, and three-day min/max forecast.
- Refresh button, 30-minute periodic refresh, and retry/offline/stale-data messages.
- On-device Wi-Fi setup: scan nearby networks, select one, and enter its password using the touch keyboard.
- 32 MB flash, 8 MB octal PSRAM, 240 MHz CPU, and a custom partition table for this board.

## Configure, build, and flash

From `iris/firmware`, activate the ESP-IDF environment once in each new shell, then set the target and configure the app:

```sh
source "$HOME/.espressif/v6.0.2/esp-idf/export.sh"
idf.py set-target esp32s3
idf.py menuconfig
```

Under **Iris**, you can set the Wi-Fi SSID/password, location label, latitude, longitude, timezone, and brightness. The password is stored in `sdkconfig`; keep that file private.

If the SSID is left blank (the default), Iris scans nearby networks on first boot. Select one, enter its password on the touchscreen, and Iris saves the settings to NVS. Use the **Wi-Fi** button at any time to choose a different network. A local `sdkconfig.private.defaults` file, if present, overrides saved Wi-Fi settings; it is intended for your private build-time credentials and is ignored by Git. The location, coordinates, and timezone still come from **Iris** in `menuconfig` until a location editor is added to the device UI.

Build, flash, and monitor the board (replace the port if macOS assigns another one):

```sh
idf.py build
idf.py -p /dev/cu.usbmodem1101 flash monitor
```

The first build downloads the official BSP and its LVGL/display/touch dependencies. The board's USB-Serial/JTAG connection supplies both flashing and logs.

## Operation

- The clock screen appears first. It shows `--:--` until SNTP has supplied a valid time.
- Tap the display to switch to weather; tap again to return to the clock.
- Press **Refresh** to request weather immediately. Normal refreshes happen every 30 minutes.
- If Wi-Fi or Open-Meteo is unavailable, the last successful weather reading remains visible and the status explains what is happening.
- On first boot, wait for the nearby-network list, select your network, enter the password, and tap **Connect**. Use the keyboard's confirmation key as a shortcut for **Connect**.

## Source layout

- `firmware/main/main.c` — board startup, LVGL UI, Wi-Fi/SNTP, HTTP client, JSON parsing, and refresh task.
- `firmware/main/Kconfig.projbuild` — user-facing settings.
- `firmware/main/idf_component.yml` — official Waveshare BSP and LVGL 9 dependencies.
- `firmware/sdkconfig.defaults` — 32 MB flash, 8 MB PSRAM, display, LVGL, and TLS defaults.
- `firmware/partitions.csv` — NVS, factory app, and storage partitions.

## Next hardware experiments

The app deliberately leaves audio, IMU, battery telemetry, brightness controls, and sleep/wake behavior isolated from the core clock path. The official BSP exposes those peripherals when you are ready to add them. Before changing power or GPIO behavior, use the board notes and schematic in `devices/Waveshare-ESP32-S3-Touch-AMOLED-1.75C/README.md`.

## Board reference

See the local device notes and official documentation links in `../devices/Waveshare-ESP32-S3-Touch-AMOLED-1.75C/README.md`.
