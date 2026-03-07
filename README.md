# MORPHIX

A lightweight Win32 application for instant resolution switching via global hotkeys.

---

## Features

- **Dual Resolution Presets** — Save two resolutions and switch between them instantly
- **Global Hotkeys** — Trigger switches from any window, even in fullscreen
- **Auto Refresh Rate** — Always selects the highest available Hz for each resolution
- **Multi-Monitor Support** — Per-monitor configuration
- **System Tray** — Runs silently in the background, out of your way
- **Color tab and vibrance control** — Added a second tab (Color) with two independent vibrance presets
---

## Installation

1. Download `MORPHIX.exe` from [Releases](https://github.com/Applelyyyy/MORPHIX/releases)
2. Run it — no installer needed

---

## Setup

1. Select the target monitor from the dropdown
2. Set **Preset 1** and **Preset 2** resolutions
3. Assign hotkeys (default: F7 and F8)
4. Click **Apply & Save** to register the hotkeys and Save to Config

---

## Hotkeys

| Key | Action |
|-----|--------|
| F7 | Switch to Preset 1 |
| F8 | Switch to Preset 2 |
| Ctrl + Alt + Shift + R | Reset to original resolution |

---

## Configuration

Settings are saved automatically to `config.ini` next to the executable.

```ini
[Settings]
Monitor=0
Resolution1=1920x1080
Resolution2=1280x720
Hotkey1=118
Hotkey2=119
```

To change hotkeys: click the hotkey field, press a new key, then click **Apply** and **Save**.

---

## Building from Source

Requirements: MinGW-w64 (g++), Windows 7 SDK or later

```cmd
compile.bat
```

---

## Notes

- To run on startup, place a shortcut to `MORPHIX.exe` in `shell:startup`
- If the display gets stuck, press Ctrl + Alt + Shift + R to recover
- Right-click the tray icon to exit
- Memory usage: ~1–4 MB at idle
- No external dependencies — single standalone executable