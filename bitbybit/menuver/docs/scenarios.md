# Menuver Scenario Sketches

## Browser

Primary apps: Safari, Chrome, Arc, Firefox.

```text
          Reload

    Back  Browser  Forward

          Copy URL
```

Second page:

```text
          Reader

   DevTools Browser  Inspect

          New Tab
```

Notes:

- Copy URL is the most distinctive action.
- Reader mode and DevTools are browser-specific.
- Browser is a good first test because keyboard shortcuts are stable.

## VS Code / Cursor

```text
          Palette

 Terminal   Code    Problems

          Format
```

Second page:

```text
          Test

    Git     Code     Build

          Run
```

Notes:

- Use keyboard shortcuts for the first page.
- Use config commands for Test/Build/Run.
- The device should show running/ok/error for long-running commands.

## Terminal

```text
          Clear

  New Tab  Terminal  Rerun

          Copy CWD
```

Danger zone page:

```text
          Interrupt

   Kill?   Terminal  Stop

          Confirm
```

Notes:

- Interrupt should not be a normal single-press action.
- Copy CWD is more powerful after shell integration.
- Rerun can be implemented with up-arrow + enter, but that may be surprising if shell history has changed.

## Meeting App

Apps: Zoom, Google Meet in browser, Teams.

```text
          Camera

    Mute   Meeting  Hand

          Leave
```

Notes:

- Leave should require confirmation.
- Browser-based Meet is harder because the foreground app is only the browser; deeper context needs URL or window title.

## Finder

```text
          Rename

 QuickLook Finder   New Folder

          Copy Path
```

Notes:

- Finder is a strong later scenario because selected file context matters.
- Copy Path and Open In Terminal become very useful if the companion can read Finder selection.
