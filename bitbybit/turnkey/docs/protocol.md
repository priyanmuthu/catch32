# TurnKey Serial Protocol

TurnKey uses newline-delimited JSON over USB serial at `115200` baud.

## Rules

- Every message is one JSON object followed by `\n`.
- Every message has a `type`.
- Unknown message types are ignored.
- The firmware never executes shell commands.
- The companion only runs commands declared in local config.

## Device To Host

```json
{"type":"hello","device":"t-encoder-pro","fw":"0.1.0"}
{"type":"mode.enter","mode":"focus"}
{"type":"input.rotate","mode":"command","delta":1}
{"type":"input.press","mode":"meeting","action":"toggle_mute"}
{"type":"focus.update","state":"running","remaining_s":1200}
{"type":"macro.run","id":"git_status"}
```

## Host To Device

```json
{"type":"hello","app":"turnkey-companion","version":"0.1.0"}
{"type":"state.command","profile":"audio","value":42,"muted":false}
{"type":"state.meeting","status":"in_meeting","muted":true}
{"type":"state.macro","id":"git_status","status":"running"}
{"type":"state.macro","id":"git_status","status":"ok","label":"Done"}
{"type":"error","message":"Unknown macro"}
```

