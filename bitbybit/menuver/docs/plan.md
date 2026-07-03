# Menuver Plan

## Goal

Build a contextual radial menu for the LILYGO T-Encoder-Pro. The menu changes based on the currently active desktop app, and each menu item runs a local desktop action.

## Product Shape

| Part | Responsibility |
| --- | --- |
| ESP32-S3 firmware | Render radial menu, handle encoder/touch/buzzer, send selected action |
| Desktop companion | Detect active app, choose menu, execute actions, report success/failure |
| Config file | Define app matchers, menu items, icons, labels, and commands |

## Interaction Model

| Input | Action |
| --- | --- |
| Rotate | Select previous/next menu item |
| Press | Run selected action |
| Long-press | Show fallback/global menu |
| Double-press | Repeat last action |
| Touch center | Toggle details/label view |
| Touch edge | Switch menu page when an app has more than one page |

## Device UI

Use an icon-first radial menu:

```text
          Reload

   Back     Browser     Dev

          Copy URL
```

Rules:

- Show 4 to 6 actions at a time.
- Use icons as the primary signal.
- Show one short label for the selected action.
- Show the app name or icon in the center.
- Use color for action state: normal, selected, unavailable, success, error.
- Keep unavailable actions visible but dimmed when useful.

## Architecture

```text
macOS foreground app watcher
  -> active app event
  -> context resolver
  -> menu model
  -> serial JSON to device

device
  -> renders menu
  -> sends action.run

companion
  -> executes configured command/shortcut
  -> sends action result
```

## Protocol

Use newline-delimited JSON over USB serial.

Host to device:

```json
{
  "type": "menu.update",
  "app": "browser",
  "title": "Safari",
  "items": [
    {"id": "back", "label": "Back", "icon": "left"},
    {"id": "reload", "label": "Reload", "icon": "refresh"},
    {"id": "copy_url", "label": "Copy URL", "icon": "copy"}
  ]
}
```

Device to host:

```json
{"type":"action.run","app":"browser","id":"copy_url"}
{"type":"action.repeat","id":"copy_url"}
```

Host to device result:

```json
{"type":"action.result","id":"copy_url","status":"ok","label":"Copied"}
{"type":"action.result","id":"copy_url","status":"error","label":"No URL"}
```

## Desktop Companion

Recommended MVP: Node.js companion on macOS.

Responsibilities:

1. Detect the foreground app.
2. Match foreground app to a configured scenario.
3. Send the current menu to the device.
4. Execute actions using keyboard shortcuts, AppleScript, shell commands, or app-specific CLIs.
5. Return result state to the device.

Foreground app detection options:

| Approach | Use |
| --- | --- |
| AppleScript/System Events | Easiest MVP on macOS |
| Swift helper | More robust later |
| Accessibility API | Needed for deeper context and selected text/window title |

MVP active-app command:

```sh
osascript -e 'tell application "System Events" to get name of first application process whose frontmost is true'
```

## Config Model

Example:

```json
{
  "serial": {"baud": 115200},
  "pollMs": 500,
  "apps": [
    {
      "id": "browser",
      "match": ["Safari", "Google Chrome", "Arc", "Firefox"],
      "title": "Browser",
      "items": [
        {"id": "back", "label": "Back", "icon": "left", "command": "osascript actions/browser-back.scpt"},
        {"id": "reload", "label": "Reload", "icon": "refresh", "keys": ["cmd", "r"]},
        {"id": "copy_url", "label": "Copy URL", "icon": "copy", "command": "osascript actions/copy-browser-url.scpt"}
      ]
    }
  ],
  "fallback": {
    "id": "global",
    "title": "Global",
    "items": [
      {"id": "volume", "label": "Volume", "icon": "audio"},
      {"id": "mute", "label": "Mute", "icon": "mute"}
    ]
  }
}
```

## Starter Scenarios

### Scenario 1: Browser

Apps: Safari, Chrome, Arc, Firefox.

Menu:

| Action | Why |
| --- | --- |
| Back | Very common, low risk |
| Forward | Pairs with Back |
| Reload | Common and easy shortcut |
| Copy URL | Useful for notes/chat |
| Reader / Focus | Useful when supported |
| Dev Tools | Useful for development |

MVP implementation:

- Shortcuts for back, forward, reload, dev tools.
- AppleScript per browser for copy URL.
- Reader mode can be app-specific later.

### Scenario 2: VS Code / Cursor

Apps: Visual Studio Code, Cursor.

Menu:

| Action | Why |
| --- | --- |
| Command Palette | Universal escape hatch |
| Toggle Terminal | Common dev action |
| Run Tests | Useful when configured |
| Format File | Safe local action |
| Git Status | Pairs well with device feedback |
| Problems | Quick diagnostics |

MVP implementation:

- Keyboard shortcuts for command palette, terminal, format, problems.
- Configurable shell command for tests.
- Optional integration with `code` CLI later.

### Scenario 3: Terminal

Apps: Terminal, iTerm2, Warp.

Menu:

| Action | Why |
| --- | --- |
| Clear | Simple and common |
| Rerun | Quick repeat loop |
| Interrupt | Useful but should require confirmation |
| New Tab | Common |
| Copy CWD | Great contextual action |
| Open Folder | Opens current directory in editor/Finder |

MVP implementation:

- Keyboard shortcuts for clear/new tab.
- Rerun via up-enter.
- Interrupt should be dimmed or require long-press.
- Copy CWD requires shell integration later; start with app-level shortcut only.

## Firmware Dependencies

Reuse the same board setup as TurnKey:

- Arduino-ESP32 3.x
- LVGL 8.3.x
- ArduinoJson
- SH8601 display driver
- CHSC5816 touch driver

The firmware should be simpler than TurnKey because it only needs one screen model:

```cpp
struct MenuItem {
  const char* id;
  const char* label;
  const char* icon;
  bool enabled;
};

struct MenuModel {
  const char* app;
  const char* title;
  MenuItem items[6];
  uint8_t itemCount;
  uint8_t selected;
};
```

## Milestones

### Milestone 1: Static Radial Menu

Goal: draw one hardcoded menu on the device.

Tasks:

1. Bring up display and encoder.
2. Draw 4 to 6 icon slots around the circle.
3. Rotate selection.
4. Press emits `action.run`.

Exit criteria:

- Device shows a radial menu.
- Selection is obvious.
- Press prints JSON over serial.

### Milestone 2: Companion App Watcher

Goal: companion can detect active macOS app and send menus.

Tasks:

1. Poll active app with AppleScript.
2. Match app name to config.
3. Send `menu.update` when app changes.
4. Log selected actions from device.

Exit criteria:

- Switching between Browser, VS Code/Cursor, and Terminal changes the device menu.

### Milestone 3: Browser Actions

Goal: first useful contextual menu.

Tasks:

1. Implement back/forward/reload shortcuts.
2. Implement copy URL for at least one browser.
3. Send success/error result.

Exit criteria:

- Device can control the active browser.

### Milestone 4: Dev Actions

Goal: useful developer workflow.

Tasks:

1. Implement VS Code/Cursor shortcuts.
2. Add configurable `run_tests`.
3. Add result state for long-running commands.

Exit criteria:

- Device can trigger command palette, terminal, format, and a test command.

### Milestone 5: Terminal Actions

Goal: terminal helper menu.

Tasks:

1. Implement clear/new tab/rerun shortcuts.
2. Add confirmation for interrupt.
3. Explore shell integration for cwd-aware actions.

Exit criteria:

- Device controls common terminal actions without accidental destructive behavior.

## Safety And UX Rules

- Dangerous actions need long-press confirmation or a second press.
- Do not execute commands sent from the device directly.
- Actions must be declared in local config.
- Show unavailable actions as dimmed rather than hiding too much.
- Use a global fallback menu when the active app has no scenario.

## Open Questions

| Question | Default |
| --- | --- |
| First OS | macOS |
| Host app language | Node.js MVP, Swift later if needed |
| Menu size | 4 primary items, up to 6 max |
| Icons | LVGL symbols first, custom icon font later |
| Deep context | App name first, window title/selection later |
