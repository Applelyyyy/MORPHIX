# MORPHIX - Resolution Changer

A lightweight, fast Windows application for quick resolution switching with hotkeys.

---

## 🚀 Features

- **Dual Resolution Profiles** - Save two favorite resolutions for instant switching
- **Global Hotkeys** - Switch resolutions from anywhere (default: F7 & F8)
- **Auto Highest Refresh Rate** - Automatically uses the highest available Hz for each resolution
- **Multi-Monitor Support** - Configure different settings for each monitor
- **System Tray Integration** - Runs quietly in the background
---

## 📋 Quick Start

### Installation
1. Download the latest release from [GitHub Releases](https://github.com/Applelyyyy/MORPHIX/releases)
2. Run `MORPHIX.exe`

### First Time Setup
1. **Select Monitor** - Choose which display to control
2. **Set Resolution 1** - Pick your first resolution (e.g., gaming resolution)
3. **Set Resolution 2** - Pick your second resolution (e.g., desktop resolution)
4. **Configure Hotkeys** - Default F7 and F8, or customize
5. **Click Apply** to activate hotkeys
6. **Click Save Config** to remember your settings

---

## ⌨️ Default Hotkeys

| Hotkey | Action |
|--------|--------|
| **F7** | Switch to Resolution 1 |
| **F8** | Switch to Resolution 2 |
| **Ctrl+Alt+Shift+R** | Reset to original resolution |

## 🔧 Configuration

Settings are automatically saved to `config.ini`:

```ini
[Settings]
Monitor=0
Resolution1=1920x1080
Resolution2=1280x720
Hotkey1=118
Hotkey2=119
```

### Changing Hotkeys
1. Click in the hotkey field
2. Press your desired key combination
3. Click **Apply** button
4. Click **Save Config** to make permanent

---

## 💡 Tips & Tricks

- **System Tray Access**: Double-click tray icon to show window
- **Quick Exit**: Right-click tray icon → Exit
- **Auto-Start**: Create a shortcut to MORPHIX.exe in `shell:startup`
- **Portable**: Copy entire folder to USB drive
- **Reset If Stuck**: Press Ctrl+Alt+Shift+R to recover

---

## 🔨 Building from Source

Requirements:
- MinGW-w64 (g++)
- Windows SDK

Build:
```cmd
compile.bat
```

---

## ⚡ Performance

- **Memory**: ~3-5 MB RAM
- **CPU**: Minimal (idle when not switching)
- **Startup**: Instant
- **No Dependencies**: Standalone executable

---