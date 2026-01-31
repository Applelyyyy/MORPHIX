# MORPHIX - Resolution Changer

**Multi-Optical Resize & Pixel Hue Interface X**

A lightweight, fast Windows application for quick resolution switching with hotkeys.

---

## 🚀 Features

- **Dual Resolution Profiles** - Save two favorite resolutions for instant switching
- **Global Hotkeys** - Switch resolutions from anywhere (default: F7 & F8)
- **Auto Highest Refresh Rate** - Automatically uses the highest available Hz for each resolution
- **Multi-Monitor Support** - Configure different settings for each monitor
- **System Tray Integration** - Runs quietly in the background
- **Quick Reset** - Return to default resolution with Ctrl+Alt+Shift+R
- **Dark Modern UI** - Clean, minimal interface

---

## 📋 Quick Start

### Installation
1. Extract all files to a folder (e.g., `C:\Program Files\MORPHIX\`)
2. Run `ACR.exe`
3. The app will minimize to system tray

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

---

## 🎮 Usage Examples

### Gaming Setup
- **Resolution 1**: `1280x720` (Performance mode)
- **Resolution 2**: `1920x1080` (Desktop mode)
- Press **F7** before gaming for better FPS
- Press **F8** after gaming to return to desktop

### Streaming Setup
- **Resolution 1**: `1920x1080` (Recording)
- **Resolution 2**: `2560x1440` (Editing)
- Quick switch between streaming and editing resolutions

---

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

## 📁 File Structure

```
ACR/
├── ACR.exe          # Main executable
├── config.ini       # Your settings (auto-created)
├── icon.ico         # Application icon
├── README.md        # This file
└── Source Files (optional):
    ├── main.cpp     # Source code
    ├── compile.bat  # Build script
    ├── app.rc       # Resources
    └── app.manifest # Windows manifest
```

---

## 💡 Tips & Tricks

- **System Tray Access**: Double-click tray icon to show window
- **Quick Exit**: Right-click tray icon → Exit
- **Auto-Start**: Create a shortcut to ACR.exe in `shell:startup`
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

## 🐛 Troubleshooting

### Hotkeys Don't Work
- Click **Apply** button after changing hotkeys
- Make sure no other app is using the same hotkey
- Try different key combinations

### Resolution Won't Change
- Check if monitor supports the resolution
- Update graphics drivers
- Try the **Reset** hotkey (Ctrl+Alt+Shift+R)

### App Won't Start
- Check system tray (it may already be running)
- Look for ACR icon near the clock
- Double-click tray icon to show window

### Config Not Saving
- Make sure ACR.exe has write permissions
- Check if config.ini is read-only
- Try running as administrator (right-click → Run as admin)

---

## ⚡ Performance

- **Memory**: ~3-5 MB RAM
- **CPU**: Minimal (idle when not switching)
- **Startup**: Instant
- **No Dependencies**: Standalone executable

---

## 📝 Version History

### v2.1 (Current)
- Simplified to pure resolution changer
- Removed color management features
- Improved UI layout
- Added custom icon
- Enhanced stability

### v1.0
- Initial release
- Basic resolution switching

---

## 🎯 Advanced Usage

### Command Line
ACR.exe runs normally with no command-line options. All configuration is done through the GUI.

### Multi-Monitor Example
1. Select **Monitor 1** from dropdown
2. Configure Resolution 1 and 2
3. Save config
4. Repeat for other monitors if needed

---

## 📞 Support

- **GitHub**: Check for updates and report issues
- **Email**: Contact developer for support
- **Config Reset**: Delete `config.ini` to reset all settings

---

## ⚖️ License

Free for personal and commercial use.

---

## 🙏 Credits

Created for quick resolution switching on Windows.

**Enjoy your optimized display experience!** 🎮🖥️
