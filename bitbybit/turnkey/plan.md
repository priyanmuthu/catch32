# Desktop Companion Knob Implementation Plan

## Implementation Status

Initial MVP scaffold is implemented in this folder:

- Firmware app shell: `firmware/`
- Desktop companion CLI: `companion/`
- Protocol notes: `docs/protocol.md`
- Bring-up notes: `docs/implementation-notes.md`

Implemented:

- Home, Command, Focus, Meeting, and Coding modes.
- Icon-first LVGL UI screens using built-in LVGL symbols.
- Encoder rotation, press, double-press, and long-press handling.
- Local Focus Timer behavior.
- USB serial newline JSON protocol.
- Node.js companion with serial parsing, command state, meeting state, and config-driven macro execution.

Still pending hardware bring-up:

- Attach the real SH8601 display flush driver in `firmware/src/display.cpp`.
- Attach the CHSC5816 touch driver.
- Confirm encoder direction and button timing on the physical device.
- Confirm macOS media/meeting automation commands.

## Goal

Build one firmware and one desktop companion app for the LILYGO T-Encoder-Pro that can switch between four useful modes:

1. Desk Command Knob
2. Focus Timer
3. Meeting Status Button
4. Coding Macro Pad

The device should feel like a small desktop appliance: rotate to navigate or adjust, press to act, long-press to go home, and touch the round screen for secondary controls.

## Product Shape

The project has two parts:

| Part | Responsibility |
| --- | --- |
| ESP32-S3 firmware | UI, display, rotary encoder, touch, buzzer, local timer state, mode switching |
| Desktop companion app | OS automation, app shortcuts, scripts, media controls, calendar/meeting integrations |

The firmware should not try to directly know how macOS, VS Code, Zoom, terminal commands, or calendar apps work. It sends simple intent messages. The desktop companion translates those intents into local actions.

## Recommended Architecture

```text
T-Encoder-Pro
  - LVGL UI
  - input handling
  - app mode state machine
  - serial/BLE transport
  - sends events
  - receives state updates

USB serial first
  - newline-delimited JSON
  - simple to debug in Serial Monitor
  - BLE HID/BLE serial can come later

Desktop companion
  - reads device events
  - runs local actions
  - returns current state
  - loads macro/config files
```

Start with USB serial. It is easier to debug, works without pairing, and lets the desktop app handle privileged OS behavior.

## Mode Model

Use a small app framework on the firmware:

```cpp
class AppMode {
public:
  virtual const char* id() = 0;
  virtual const char* label() = 0;
  virtual void onEnter() {}
  virtual void onExit() {}
  virtual void onRotate(int delta) {}
  virtual void onPress() {}
  virtual void onLongPress() {}
  virtual void onTouch(int x, int y) {}
  virtual void onHostMessage(JsonDocument& msg) {}
  virtual void tick(uint32_t nowMs) {}
  virtual void draw() {}
};
```

Initial modes:

| Mode | Firmware Behavior | Host Behavior |
| --- | --- | --- |
| Home | Shows mode launcher | Optional host connection status |
| Command | Sends volume/media/brightness commands | Runs OS media/volume actions |
| Focus | Runs timer locally, sends status events | Optional DND/focus mode integration |
| Meeting | Shows meeting/mute state, sends mute/status actions | Talks to OS/app shortcuts/calendar |
| Coding | Shows macro list, sends macro action IDs | Runs configured local commands |

Global controls:

| Input | Action |
| --- | --- |
| Rotate on home | Select mode |
| Press on home | Enter selected mode |
| Long-press anywhere | Return home |
| Double-press anywhere | Quick-cycle modes |
| Touch top/bottom/left/right | Mode-specific secondary actions |

## Device UI

The display is round, so keep the UI radial, icon-first, and glanceable. Text should be secondary: use it for the currently selected mode, current value, short status, or error messages.

## Icon Strategy

Use LVGL built-in symbols for the MVP. LVGL symbols render through `lv_label`, so they are easy to draw, recolor, resize, and align without managing bitmap assets.

Example:

```cpp
lv_obj_t *btn = lv_btn_create(parent);
lv_obj_t *icon = lv_label_create(btn);
lv_label_set_text(icon, LV_SYMBOL_HOME);
lv_obj_center(icon);
```

Icon implementation path:

| Phase | Approach | Why |
| --- | --- | --- |
| MVP | LVGL built-in symbols | Fastest, no asset pipeline, very low memory overhead |
| Polish | Custom icon font with only needed glyphs | Better icon coverage while staying compact and scalable |
| Later optional | Small PNG/logo assets | Useful for splash or branding, not ideal for every control |

Avoid PNGs for the first version. They add conversion work, flash/RAM pressure, and are less flexible for theme colors and sizing.

Suggested icon mapping:

| Mode | MVP Icon Ideas | Polish Icon Font Ideas |
| --- | --- | --- |
| Command | speaker, settings, plus/minus, arrows | speaker, sliders, monitor brightness, media controls |
| Focus | play, pause, stop, refresh | clock, timer, coffee/break, progress ring |
| Meeting | audio/mute, user, bell | microphone, calendar, video, presence dot |
| Coding | play, settings, list, upload/download | terminal, braces, git branch, test/check, build/hammer |

Home screen rule: show four large icons around the circle and one short text label for the selected mode. For example:

```text
          mic

  code  TurnKey  speaker

         clock

Selected: Focus
```

The selected icon should be visually distinct using size, color, ring highlight, or brightness. Keep all icons single-color for the MVP.

### Home Screen

Show four mode wedges or large icons:

```text
        Meeting

  Code    ●    Command

         Focus
```

Rotate moves selection around the circle. Press enters.

### Command Knob

Primary use: volume/media/brightness.

UI:

- Large center value, e.g. `42%`
- Ring gauge around edge
- Large profile icon, e.g. speaker or brightness
- Small label only for the selected profile, e.g. `Audio`

Controls:

| Input | Action |
| --- | --- |
| Rotate | Adjust active value |
| Press | Toggle mute or play/pause |
| Touch left/right | Previous/next command profile |
| Long-press | Home |

Initial profiles:

1. Audio volume
2. Media track
3. Screen brightness
4. Custom shortcut

### Focus Timer

Primary use: Pomodoro/focus sessions.

UI:

- Circular progress ring
- Remaining time in center
- Play/pause state icon
- Mode label only for the active preset: `Focus`, `Break`, `Long Break`

Controls:

| Input | Action |
| --- | --- |
| Rotate while stopped | Adjust duration |
| Rotate while running | Optional fine adjust |
| Press | Start/pause |
| Double-press | Reset |
| Touch left/right | Switch timer preset |
| Long-press | Home |

This mode should work even if the desktop companion is not running.

Default presets:

| Preset | Duration |
| --- | --- |
| Focus | 25 min |
| Short Break | 5 min |
| Long Break | 15 min |

### Meeting Status

Primary use: meeting/mute/status control.

UI:

- Large mic/status icon
- Current state: `Available`, `Focus`, `In Meeting`
- Mic state can be icon-first: muted mic vs live mic
- Optional next meeting time

Controls:

| Input | Action |
| --- | --- |
| Rotate | Select status |
| Press | Toggle mute |
| Touch left/right | Previous/next status |
| Touch center | Join/open meeting action, later |
| Long-press | Home |

MVP host actions:

- Toggle system/app mute via configurable shortcut.
- Set manual status.
- Show returned mute/status state.

Later host actions:

- Read calendar.
- Detect active meeting.
- Show next meeting.
- Trigger meeting join URL.

### Coding Macro Pad

Primary use: run developer commands and common shortcuts.

UI:

- Current macro group icon: git, build, test, run
- Selected macro label
- Last result as icon-first status: check, spinner, warning/error

Controls:

| Input | Action |
| --- | --- |
| Rotate | Select macro |
| Press | Run selected macro |
| Touch left/right | Switch macro group |
| Double-press | Repeat last macro |
| Long-press | Home |

Example macros:

| ID | Label | Command |
| --- | --- | --- |
| `git_status` | Git Status | `git status --short` |
| `test_project` | Run Tests | project-specific |
| `build_project` | Build | project-specific |
| `open_terminal` | Terminal | OS-specific |

## Device/Host Protocol

Use newline-delimited JSON over serial.

Device to host examples:

```json
{"type":"hello","device":"t-encoder-pro","fw":"0.1.0"}
{"type":"mode.enter","mode":"command"}
{"type":"input.rotate","mode":"command","delta":1}
{"type":"input.press","mode":"meeting","action":"toggle_mute"}
{"type":"focus.update","state":"running","remaining_s":1200}
{"type":"macro.run","id":"test_project"}
```

Host to device examples:

```json
{"type":"hello","app":"desktop-companion","version":"0.1.0"}
{"type":"state.command","profile":"audio","value":42,"muted":false}
{"type":"state.meeting","status":"in_meeting","muted":true}
{"type":"state.macro","id":"test_project","status":"running"}
{"type":"state.macro","id":"test_project","status":"ok","label":"Tests passed"}
{"type":"error","message":"Unknown macro"}
```

Rules:

- One JSON object per line.
- Keep messages small.
- Include `type` in every message.
- Firmware should ignore unknown message types.
- Host should ignore unknown fields.
- Add sequence IDs later if commands need acknowledgements.

## Desktop Companion App

Start with Node.js or Python. Node.js is a good fit if the app will grow into a menu bar UI later. Python is faster for a small local helper. Either is acceptable.

Recommended MVP: Node.js CLI helper.

Responsibilities:

1. Discover and connect to the serial port.
2. Parse newline-delimited JSON.
3. Map device events to local actions.
4. Load macros from config.
5. Send state updates back to the device.

Suggested config file:

```json
{
  "serial": {
    "baud": 115200
  },
  "commandProfiles": [
    {"id": "audio", "label": "Audio", "kind": "volume"},
    {"id": "media", "label": "Media", "kind": "media"},
    {"id": "brightness", "label": "Brightness", "kind": "brightness"}
  ],
  "meeting": {
    "toggleMuteCommand": "osascript scripts/toggle-mute.scpt"
  },
  "macros": [
    {
      "id": "git_status",
      "label": "Git Status",
      "group": "Git",
      "cwd": "/Users/priyan/workspaces/catch32",
      "command": "git status --short"
    },
    {
      "id": "test_project",
      "label": "Run Tests",
      "group": "Test",
      "cwd": "/Users/priyan/workspaces/catch32",
      "command": "npm test"
    }
  ]
}
```

Security note: a macro runner can execute arbitrary shell commands. Keep the config local, explicit, and not writable by untrusted processes.

## Firmware Dependencies

Likely firmware stack:

| Dependency | Use |
| --- | --- |
| Arduino-ESP32 3.x | ESP32-S3 support |
| LVGL 8.3.x | UI |
| LVGL symbols | MVP icon rendering through labels |
| Optional custom LVGL icon font | Polished icon set after MVP |
| Ai Esp32 Rotary Encoder | Encoder bring-up, or replace with direct interrupt handling |
| ArduinoJson | JSON protocol |
| Pro SH8601 driver | Display |
| Pro CHSC5816 driver | Touch |

Use the board details in:

- `../docs/lilygo.md`
- `../docs/lilygo/pin-reference.md`
- `../docs/lilygo/first-tests.md`

## Proposed Folder Layout

```text
bitbybit/turnkey/
  plan.md
  firmware/
    platformio.ini
    src/
      main.cpp
      app_mode.h
      app_home.cpp
      app_command.cpp
      app_focus.cpp
      app_meeting.cpp
      app_coding.cpp
      protocol.cpp
      display.cpp
      input.cpp
  companion/
    package.json
    src/
      index.js
      serial.js
      protocol.js
      actions/
        media.js
        meeting.js
        macros.js
    config.example.json
    scripts/
      toggle-mute.scpt
```

Do not build all of this at once. Create the folders as each milestone starts.

## Milestones

### Milestone 0: Board Bring-Up

Goal: prove the hardware works.

Tasks:

1. Run serial test.
2. Run encoder test.
3. Run buzzer test.
4. Run I2C scan.
5. Run display enable/display fill test.

Exit criteria:

- Serial works.
- Encoder rotates and presses.
- Buzzer works.
- Touch appears on I2C.
- Display can be initialized.

### Milestone 1: Firmware Shell

Goal: basic UI and mode switching on-device.

Tasks:

1. Initialize display, touch, encoder, buzzer.
2. Create `AppMode` interface.
3. Implement icon-first home screen with LVGL symbols.
4. Implement mode switching.
5. Implement global long-press home behavior.
6. Add selected-mode label as the only required home-screen text.

Exit criteria:

- Device boots into home.
- Rotate selects one of four modes.
- Selected mode is obvious from icon highlight plus short label.
- Press enters a mode.
- Long-press returns home.

### Milestone 2: Focus Timer

Goal: first complete standalone mode.

Tasks:

1. Implement preset selection.
2. Implement start/pause/reset.
3. Draw circular countdown.
4. Add buzzer notification.

Exit criteria:

- Focus Timer is usable without the desktop companion.

### Milestone 3: Serial Protocol

Goal: device and host can exchange state.

Tasks:

1. Add newline JSON sender on firmware.
2. Add parser for host messages.
3. Build tiny desktop CLI that logs device messages.
4. Add host `hello` reply.
5. Show connection state on device.

Exit criteria:

- Rotating/pressing in a mode prints structured events on host.
- Device receives and displays at least one host state message.

### Milestone 4: Command Knob

Goal: useful daily desktop control.

Tasks:

1. Implement command profile UI.
2. Send rotate/press events to host.
3. Host maps audio profile to volume/mute.
4. Host sends current value/mute state back.

Exit criteria:

- Rotating changes desktop volume.
- Press toggles mute.
- Device display reflects state.

### Milestone 5: Coding Macro Pad

Goal: run configured commands.

Tasks:

1. Load macros from companion config.
2. Send macro list or active macro state to device.
3. Firmware displays macro groups and selected macro.
4. Press runs selected macro.
5. Host reports running/ok/failed.

Exit criteria:

- A configured macro can run from the device.
- Result appears on the device.

### Milestone 6: Meeting Status

Goal: manual meeting controls first, calendar later.

Tasks:

1. Implement status selection UI.
2. Implement mute toggle event.
3. Host maps mute toggle to configured command/shortcut.
4. Host reports manual status/mute state.

Exit criteria:

- Device can toggle meeting mute/status through the companion.

### Milestone 7: Polish

Goal: make it feel like a finished desk tool.

Tasks:

1. Add icons and consistent visual language.
2. Evaluate a custom icon font if LVGL symbols feel too limited.
3. Add connection lost/reconnect behavior.
4. Add settings screen.
5. Add brightness control.
6. Add sleep/dim behavior.
7. Add error toasts.

Exit criteria:

- Device survives companion restarts.
- UI is readable at a glance.
- Common failures are visible and recoverable.

## MVP Scope

The first version should include:

- Home launcher
- Focus Timer fully on device
- USB serial protocol
- Command Knob for volume/mute
- Coding Macro Pad with 3 to 5 configured commands
- Meeting Status with manual mute/status control

Defer:

- BLE
- Calendar integration
- Menu bar app
- Auto app detection
- OTA updates
- Deep sleep/battery tuning

## Open Decisions

| Decision | Default Recommendation |
| --- | --- |
| Firmware framework | Arduino + LVGL first |
| Build system | PlatformIO once board settings are confirmed |
| Host language | Node.js CLI first |
| Transport | USB serial first |
| UI library | LVGL 8.3.x |
| Persistence | Host config JSON; firmware preferences later |
| Meeting integration | Manual shortcut first; calendar later |

## Risks

| Risk | Mitigation |
| --- | --- |
| Display driver instability | Start from Pro SH8601 example and keep first UI minimal |
| LVGL version mismatch | Use LVGL 8.3.x until the display/touch path is stable |
| USB serial port changes | Add host auto-discovery and reconnect |
| macOS automation permissions | Start with explicit scripts and document required permissions |
| Macro security | Require local config and avoid executing commands received directly from device |
| GPIO 0 button affects boot | Avoid holding encoder button during normal reset except bootloader entry |

## First Build Task

When the hardware arrives, do not start with the full project. Start with `../docs/lilygo/first-tests.md`, then create a minimal firmware shell that only shows:

```text
Home
Command | Focus | Meeting | Code
```

Once that works, build one mode at a time.
