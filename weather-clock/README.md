# Weather Clock

A learning project for the Waveshare ESP32-S3-Touch-AMOLED-1.75C.

The first goal is a simple, readable clock. We will add weather only after the display, touch input, Wi-Fi, and time synchronization are understood independently.

## Learning goals

- Bring up the AMOLED display.
- Render text and simple shapes.
- Read touch input.
- Connect to Wi-Fi without blocking the user interface.
- Synchronize time from the network.
- Fetch and display weather data from a web API.
- Handle loading, offline, error, and stale-data states.
- Explore brightness, sleep, and battery-aware behavior.

## Planned stages

1. **Display probe**: show a static screen with the project name and a few test colors.
2. **Local clock**: maintain and display time using a known starting value.
3. **Touch interaction**: add a screen tap or swipe that changes the view.
4. **Wi-Fi connection**: connect to a network and expose connection status.
5. **Network time**: obtain the current time and display the local timezone.
6. **Weather request**: fetch a small weather response and show temperature and conditions.
7. **Polish**: add refresh timing, offline behavior, brightness, and sleep/wake behavior.

## Learning plan

We will build the project in small checkpoints and understand each layer before combining it with the next one.

1. **Set up the tools**: install VS Code, the Espressif ESP-IDF extension, ESP-IDF `v5.5.x`, and the required toolchain.
2. **Create a native ESP-IDF project**: establish the standard project structure and set the target to `esp32s3`.
3. **Flash the stock example**: build and flash `hello_world`, then confirm that serial logging works.
4. **Understand the project**: inspect `CMakeLists.txt`, `main`, component dependencies, logging, and the build configuration.
5. **Add the Waveshare display component**: use the board's official component and examples to initialize the AMOLED display.
6. **Display a static screen**: render a project title, a test color, and basic text before attempting a clock.
7. **Build the local clock**: add timekeeping and render a readable clock without Wi-Fi.
8. **Add touch input**: detect a tap or swipe and use it to change a simple screen state.
9. **Add Wi-Fi**: connect without blocking the UI and display connection progress or failure.
10. **Synchronize network time**: configure the timezone and replace the local test time with current time.
11. **Add weather as a separate layer**: make an HTTP request, parse a small JSON response, and display temperature and conditions.
12. **Handle real-world states**: add loading, offline, request failure, stale-data, retry, and refresh behavior.
13. **Explore power behavior**: adjust brightness and investigate sleep/wake behavior and battery status.

Each stage should leave the board in a buildable, flashable state. We will pause at each checkpoint to inspect what changed and why before continuing.

## Questions we will answer as we build

- Which ESP-IDF project structure and component boundaries should we use?
- Should the UI use LVGL, or should we start with direct graphics calls?
- How should Wi-Fi credentials be supplied during development?
- Which weather service and response format should we use?
- How often should the display refresh weather data?
- What should the device show when Wi-Fi or the weather service is unavailable?

## Current status

Project created. No firmware implementation has been added yet.

## Board reference

See the local device notes for hardware details and official documentation:

`../devices/Waveshare-ESP32-S3-Touch-AMOLED-1.75C/README.md`
