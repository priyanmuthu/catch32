# Menuver

Menuver is a separate LILYGO T-Encoder-Pro project that turns the round screen and encoder into a contextual command menu for the currently active desktop application.

The device shows a small radial menu. The desktop companion watches the foreground app, sends the relevant menu to the device, and runs the selected action when the encoder is pressed.

## Core Idea

```text
Current app changes
  -> companion detects app + optional context
  -> companion sends a menu model to the device
  -> device renders icons around the round display
  -> rotate selects an action
  -> press runs the selected action
```

This is intentionally different from TurnKey. TurnKey is a multi-mode desk companion. Menuver is an adaptive command surface.

## Why This Fits The Hardware

- A round screen naturally supports radial menus.
- The rotary encoder is good for cycling through actions without looking down for long.
- Press is a strong commit action.
- Touch can expose secondary actions or page switching.
- The buzzer can confirm action success/failure without another notification.

## Starter Scenarios

The first version should support a few high-value apps with small, predictable menus:

| App / Context | Example Actions |
| --- | --- |
| Browser | Back, forward, reload, copy URL, reader mode, open dev tools |
| VS Code / Cursor | Run tests, command palette, git status, format file, toggle terminal |
| Terminal | Clear, rerun last command, interrupt, split/new tab, copy cwd |
| Slack / Discord | Mute, reply, mark read, switch channel, set status |
| Zoom / Meet | Mute, camera, raise hand, leave, copy meeting link |
| Finder | New folder, rename, copy path, quick look, AirDrop/share |
| Figma / Design | Zoom, hand/select, frame, export, toggle grid |

## MVP Recommendation

Start with these three scenarios:

1. Browser
2. VS Code / Cursor
3. Terminal

They are easy to test, useful every day, and do not require deep app APIs.

## Docs

- [Plan](docs/plan.md)
- [Scenario Menu Sketches](docs/scenarios.md)
