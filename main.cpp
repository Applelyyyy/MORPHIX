#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <stdio.h>
#include <cmath>

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

// Debug mode - set to true for console output
#define DEBUG_MODE false

// IDs
#define ID_TRAY_APP_ICON    1001
#define ID_TRAY_EXIT        1002
#define ID_TRAY_SHOW        1003
#define WM_TRAYICON         (WM_USER + 1)
#define HOTKEY_GAME_MODE    1
#define HOTKEY_NORMAL_MODE  2
#define HOTKEY_RESET        3

#define IDC_BTN_APPLY       2001
#define IDC_EDIT_W_GAME     2003
#define IDC_EDIT_H_GAME     2004
#define IDC_EDIT_W_NORM     2005
#define IDC_EDIT_H_NORM     2006
#define IDC_COMBO_MONITOR   2007
#define IDC_HOTKEY_GAME     2008
#define IDC_HOTKEY_NORMAL   2009
#define IDC_BTN_APPLY_HOTKEY 2010
#define IDC_COMBO_RES_GAME  2011
#define IDC_COMBO_RES_NORMAL 2012
#define IDC_LABEL_CURRENT_HZ 2013
#define IDC_BTN_APPLY_RES1  2014
#define IDC_BTN_APPLY_RES2  2015
#define IDC_BTN_RESET       2016
#define IDC_BTN_SAVE        2017

// Global variables
NOTIFYICONDATA nid;
HWND hEditW_Game, hEditH_Game, hEditW_Norm, hEditH_Norm, hComboMon;
HWND hHotkeyGame, hHotkeyNormal;
HWND hComboResGame, hComboResNormal, hLabelCurrentHz;
std::vector<DISPLAY_DEVICE> monitors;
UINT gameHotkey = VK_F7;
UINT normalHotkey = VK_F8;

// Dialog dark mode brushes
HBRUSH hBrushDarkBg = NULL;
HBRUSH hBrushEditBg = NULL;
HFONT hFont, hTitleFont;
HBRUSH hBrushBg;
int currentHz = 0;
char configFile[MAX_PATH];
int originalWidth = 0, originalHeight = 0, originalHz = 0;

// Forward declarations
void SaveConfig();

// Custom dialog window procedure for dark mode
LRESULT CALLBACK CustomDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, RGB(220, 220, 220));
            SetBkColor(hdcStatic, RGB(32, 32, 32));
            return (INT_PTR)hBrushDarkBg;
        }
        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, RGB(220, 220, 220));
            SetBkColor(hdcEdit, RGB(45, 45, 45));
            return (INT_PTR)hBrushEditBg;
        }
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, hBrushDarkBg);
            return 1;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Debug logging function
void DebugLog(const char* format, ...) {
    if (DEBUG_MODE) {
        char buffer[512];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        // Output to debug string
        OutputDebugStringA(buffer);
        OutputDebugStringA("\n");
        
        // Also print to console if allocated
        printf("%s\n", buffer);
    }
}

// Check if running as administrator
bool IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    return isAdmin == TRUE;
}

// Save configuration to INI file
void SaveConfig() {
    char buf[64];
    
    // Save monitor selection
    int monIdx = SendMessage(hComboMon, CB_GETCURSEL, 0, 0);
    sprintf(buf, "%d", monIdx);
    WritePrivateProfileString("Settings", "Monitor", buf, configFile);
    
    // Save Resolution 1 selection
    int res1Idx = SendMessage(hComboResGame, CB_GETCURSEL, 0, 0);
    SendMessage(hComboResGame, CB_GETLBTEXT, res1Idx, (LPARAM)buf);
    WritePrivateProfileString("Settings", "Resolution1", buf, configFile);
    
    // Save Resolution 2 selection
    int res2Idx = SendMessage(hComboResNormal, CB_GETCURSEL, 0, 0);
    SendMessage(hComboResNormal, CB_GETLBTEXT, res2Idx, (LPARAM)buf);
    WritePrivateProfileString("Settings", "Resolution2", buf, configFile);
    
    // Save hotkeys
    sprintf(buf, "%d", gameHotkey);
    WritePrivateProfileString("Settings", "Hotkey1", buf, configFile);
    sprintf(buf, "%d", normalHotkey);
    WritePrivateProfileString("Settings", "Hotkey2", buf, configFile);
    
    DebugLog("Configuration saved to %s", configFile);
}

// Load configuration from INI file
void LoadConfig() {
    char buf[64];
    
    DebugLog("Loading configuration from INI file");
    
    // Load monitor selection
    GetPrivateProfileString("Settings", "Monitor", "0", buf, 64, configFile);
    int monIdx = atoi(buf);
    if (monIdx >= 0 && monIdx < SendMessage(hComboMon, CB_GETCOUNT, 0, 0)) {
        SendMessage(hComboMon, CB_SETCURSEL, monIdx, 0);
        DebugLog("Loaded monitor index: %d", monIdx);
    }
    
    // Load Resolution 1
    GetPrivateProfileString("Settings", "Resolution1", "", buf, 64, configFile);
    if (strlen(buf) > 0) {
        int count = SendMessage(hComboResGame, CB_GETCOUNT, 0, 0);
        for (int i = 0; i < count; i++) {
            char item[64];
            SendMessage(hComboResGame, CB_GETLBTEXT, i, (LPARAM)item);
            if (strcmp(item, buf) == 0) {
                SendMessage(hComboResGame, CB_SETCURSEL, i, 0);
                DebugLog("Loaded Resolution 1: %s", buf);
                break;
            }
        }
    }
    
    // Load Resolution 2
    GetPrivateProfileString("Settings", "Resolution2", "", buf, 64, configFile);
    if (strlen(buf) > 0) {
        int count = SendMessage(hComboResNormal, CB_GETCOUNT, 0, 0);
        for (int i = 0; i < count; i++) {
            char item[64];
            SendMessage(hComboResNormal, CB_GETLBTEXT, i, (LPARAM)item);
            if (strcmp(item, buf) == 0) {
                SendMessage(hComboResNormal, CB_SETCURSEL, i, 0);
                DebugLog("Loaded Resolution 2: %s", buf);
                break;
            }
        }
    }
    
    // Load hotkeys
    GetPrivateProfileString("Settings", "Hotkey1", "118", buf, 64, configFile);
    gameHotkey = atoi(buf);
    SendMessage(hHotkeyGame, HKM_SETHOTKEY, MAKEWORD(gameHotkey, 0), 0);
    DebugLog("Loaded Hotkey 1: 0x%X", gameHotkey);
    
    GetPrivateProfileString("Settings", "Hotkey2", "119", buf, 64, configFile);
    normalHotkey = atoi(buf);
    SendMessage(hHotkeyNormal, HKM_SETHOTKEY, MAKEWORD(normalHotkey, 0), 0);
    DebugLog("Loaded Hotkey 2: 0x%X", normalHotkey);
    
    DebugLog("Configuration loaded from %s", configFile);
}

// Monitor enumeration
void EnumerateMonitors() {
    monitors.clear();
    DISPLAY_DEVICE dd;
    dd.cb = sizeof(dd);
    DWORD deviceNum = 0;
    while (EnumDisplayDevices(NULL, deviceNum, &dd, 0)) {
        if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
            monitors.push_back(dd);
            DebugLog("Found monitor %d: %s", deviceNum, dd.DeviceString);
        }
        deviceNum++;
    }
    DebugLog("Total monitors found: %d", monitors.size());
}

// Get current display settings
void GetCurrentDisplaySettings(int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= monitors.size()) return;
    
    DEVMODE dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);
    
    if (EnumDisplaySettingsEx(monitors[monitorIndex].DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0)) {
        currentHz = dm.dmDisplayFrequency;
        
        // Store original resolution on first call
        if (originalWidth == 0) {
            originalWidth = dm.dmPelsWidth;
            originalHeight = dm.dmPelsHeight;
            originalHz = dm.dmDisplayFrequency;
            DebugLog("Stored original resolution: %dx%d @ %dHz", originalWidth, originalHeight, originalHz);
        }
        
        char hzText[64];
        sprintf(hzText, "Current: %dx%d @ %dHz", dm.dmPelsWidth, dm.dmPelsHeight, currentHz);
        SetWindowText(hLabelCurrentHz, hzText);
        DebugLog("Current display: %s", hzText);
    }
}

// Populate resolution dropdown with available resolutions and refresh rates
void PopulateResolutions(HWND hCombo, int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= monitors.size()) return;
    
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
    
    DEVMODE dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);
    
    // Map to store resolution -> highest Hz
    std::map<std::string, int> resolutionMaxHz;
    std::vector<std::pair<int, std::string>> resolutions; // For sorting
    
    // Read all available modes from GPU driver using both standard and raw modes
    for (DWORD i = 0; EnumDisplaySettingsEx(monitors[monitorIndex].DeviceName, i, &dm, EDS_RAWMODE); i++) {
        // Accept both 32-bit and 24-bit color depth for maximum compatibility
        if (dm.dmBitsPerPel >= 24 && dm.dmPelsWidth >= 640 && dm.dmPelsHeight >= 480) {
            char resText[64];
            sprintf(resText, "%dx%d", dm.dmPelsWidth, dm.dmPelsHeight);
            std::string resKey(resText);
            
            // Track highest Hz for this resolution
            if (resolutionMaxHz.find(resKey) == resolutionMaxHz.end() || 
                dm.dmDisplayFrequency > resolutionMaxHz[resKey]) {
                resolutionMaxHz[resKey] = dm.dmDisplayFrequency;
            }
            
            // Check if resolution already added
            bool exists = false;
            for (const auto& res : resolutions) {
                if (res.second == resKey) {
                    exists = true;
                    break;
                }
            }
            
            if (!exists) {
                // Create sort key: width*10000 + height for sorting
                int sortKey = dm.dmPelsWidth * 10000 + dm.dmPelsHeight;
                resolutions.push_back(std::make_pair(sortKey, resKey));
            }
        }
    }
    
    // Sort by resolution (largest first)
    std::sort(resolutions.rbegin(), resolutions.rend());
    
    // Add sorted resolutions to combobox (showing just resolution, but we know max Hz)
    for (const auto& res : resolutions) {
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)res.second.c_str());
    }
    
    SendMessage(hCombo, CB_SETCURSEL, 0, 0);
    DebugLog("Populated %d unique resolutions with auto highest Hz", resolutions.size());
}

// Get maximum refresh rate for resolution
int GetMaxRefreshRate(int width, int height, int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= monitors.size()) {
        DebugLog("Invalid monitor index: %d", monitorIndex);
        return 60;
    }
    
    int maxHz = 60;
    DEVMODE dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);
    
    for (DWORD i = 0; EnumDisplaySettingsEx(monitors[monitorIndex].DeviceName, i, &dm, 0); i++) {
        if (dm.dmPelsWidth == width && dm.dmPelsHeight == height) {
            if (dm.dmDisplayFrequency > maxHz) {
                maxHz = dm.dmDisplayFrequency;
            }
        }
    }
    DebugLog("Max refresh rate for %dx%d: %dHz", width, height, maxHz);
    return maxHz;
}

// Change resolution with highest available refresh rate
void ChangeRes(int width, int height, int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= monitors.size()) {
        DebugLog("ChangeRes: Invalid monitor index %d", monitorIndex);
        return;
    }

    // Find highest Hz for this resolution
    int maxHz = GetMaxRefreshRate(width, height, monitorIndex);
    DebugLog("Changing resolution to %dx%d @ %dHz (auto-highest) on monitor %d", width, height, maxHz, monitorIndex);

    DEVMODE dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);
    dm.dmPelsWidth = width;
    dm.dmPelsHeight = height;
    dm.dmDisplayFrequency = maxHz;
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

    LONG result = ChangeDisplaySettingsEx(monitors[monitorIndex].DeviceName, &dm, NULL, CDS_UPDATEREGISTRY, NULL);

    if (result == DISP_CHANGE_SUCCESSFUL) {
        DebugLog("Resolution changed successfully to %dx%d @ %dHz", width, height, maxHz);
        GetCurrentDisplaySettings(monitorIndex);
    } else {
        DebugLog("Failed to change resolution. Error code: %d", result);
        if (DEBUG_MODE) {
            MessageBox(NULL, "Failed to change resolution. Check if your monitor supports this.", "MORPHIX Error", MB_ICONERROR | MB_TOPMOST);
        }
    }
}

// Initialize system tray icon
void InitTray(HWND hwnd) {
    DebugLog("Initializing system tray");
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_APP_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(100));
    if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);
    nid.hIcon = hIcon;
    strcpy(nid.szTip, "MORPHIX - Resolution Changer");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

// Apply hotkey settings
void ApplyHotkeys(HWND hwnd) {
    DebugLog("Applying new hotkeys");
    UnregisterHotKey(hwnd, HOTKEY_GAME_MODE);
    UnregisterHotKey(hwnd, HOTKEY_NORMAL_MODE);
    
    WORD modGame = LOWORD(SendMessage(hHotkeyGame, HKM_GETHOTKEY, 0, 0));
    WORD modNormal = LOWORD(SendMessage(hHotkeyNormal, HKM_GETHOTKEY, 0, 0));
    
    if (modGame > 0) gameHotkey = modGame;
    if (modNormal > 0) normalHotkey = modNormal;
    
    RegisterHotKey(hwnd, HOTKEY_GAME_MODE, MOD_NOREPEAT, gameHotkey);
    RegisterHotKey(hwnd, HOTKEY_NORMAL_MODE, MOD_NOREPEAT, normalHotkey);
    DebugLog("Hotkeys applied: Game=0x%X, Normal=0x%X", gameHotkey, normalHotkey);
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HBRUSH hBrushPanel;
    
    switch (uMsg) {
    case WM_CREATE:
    {
        DebugLog("Creating window and controls");
        // Use JetBrains Mono font
        hFont = CreateFont(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "JetBrains Mono");
        hTitleFont = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "JetBrains Mono");
        hBrushBg = CreateSolidBrush(RGB(32, 32, 32));
        hBrushPanel = CreateSolidBrush(RGB(45, 45, 45));
        
        HWND hGroupMonitor = CreateWindow("BUTTON", "Monitor Selection",
            WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 15, 15, 570, 70, hwnd, NULL, NULL, NULL);
        SendMessage(hGroupMonitor, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        CreateWindow("STATIC", "Select Monitor:", WS_VISIBLE | WS_CHILD, 30, 40, 110, 22, hwnd, NULL, NULL, NULL);
        hComboMon = CreateWindow("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
            150, 38, 420, 150, hwnd, (HMENU)IDC_COMBO_MONITOR, NULL, NULL);
        SendMessage(hComboMon, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        EnumerateMonitors();
        for (size_t i = 0; i < monitors.size(); i++) {
            char buf[256];
            sprintf(buf, "Monitor %d: %s", (int)i + 1, monitors[i].DeviceString);
            SendMessage(hComboMon, CB_ADDSTRING, 0, (LPARAM)buf);
        }
        SendMessage(hComboMon, CB_SETCURSEL, 0, 0);

        // Current Hz Display
        hLabelCurrentHz = CreateWindow("STATIC", "Current: Detecting...",
            WS_VISIBLE | WS_CHILD | SS_CENTER, 15, 90, 570, 25, hwnd, (HMENU)IDC_LABEL_CURRENT_HZ, NULL, NULL);
        SendMessage(hLabelCurrentHz, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
        GetCurrentDisplaySettings(0);

        // Resolution - 1
        HWND hGroupGame = CreateWindow("BUTTON", "Resolution - 1",
            WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 15, 125, 280, 150, hwnd, NULL, NULL, NULL);
        SendMessage(hGroupGame, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        CreateWindow("STATIC", "Resolution:", WS_VISIBLE | WS_CHILD, 30, 155, 80, 22, hwnd, NULL, NULL, NULL);
        hComboResGame = CreateWindow("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
            115, 152, 170, 400, hwnd, (HMENU)IDC_COMBO_RES_GAME, NULL, NULL);
        SendMessage(hComboResGame, WM_SETFONT, (WPARAM)hFont, TRUE);
        PopulateResolutions(hComboResGame, 0);
        
        CreateWindow("STATIC", "Hotkey:", WS_VISIBLE | WS_CHILD, 30, 195, 60, 22, hwnd, NULL, NULL, NULL);
        hHotkeyGame = CreateWindow(HOTKEY_CLASS, "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
            95, 192, 120, 24, hwnd, (HMENU)IDC_HOTKEY_GAME, NULL, NULL);
        SendMessage(hHotkeyGame, HKM_SETHOTKEY, MAKEWORD(VK_F7, 0), 0);
        SendMessage(hHotkeyGame, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        HWND hBtnApplyHotkey1 = CreateWindow("BUTTON", "Apply", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
            30, 230, 255, 30, hwnd, (HMENU)IDC_BTN_APPLY_RES1, NULL, NULL);
        SendMessage(hBtnApplyHotkey1, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Resolution - 2
        HWND hGroupNormal = CreateWindow("BUTTON", "Resolution - 2",
            WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 305, 125, 280, 150, hwnd, NULL, NULL, NULL);
        SendMessage(hGroupNormal, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        CreateWindow("STATIC", "Resolution:", WS_VISIBLE | WS_CHILD, 320, 155, 80, 22, hwnd, NULL, NULL, NULL);
        hComboResNormal = CreateWindow("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
            405, 152, 170, 400, hwnd, (HMENU)IDC_COMBO_RES_NORMAL, NULL, NULL);
        SendMessage(hComboResNormal, WM_SETFONT, (WPARAM)hFont, TRUE);
        PopulateResolutions(hComboResNormal, 0);
        
        CreateWindow("STATIC", "Hotkey:", WS_VISIBLE | WS_CHILD, 320, 195, 60, 22, hwnd, NULL, NULL, NULL);
        hHotkeyNormal = CreateWindow(HOTKEY_CLASS, "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
            385, 192, 120, 24, hwnd, (HMENU)IDC_HOTKEY_NORMAL, NULL, NULL);
        SendMessage(hHotkeyNormal, HKM_SETHOTKEY, MAKEWORD(VK_F8, 0), 0);
        SendMessage(hHotkeyNormal, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        HWND hBtnApplyHotkey2 = CreateWindow("BUTTON", "Apply", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
            320, 230, 255, 30, hwnd, (HMENU)IDC_BTN_APPLY_RES2, NULL, NULL);
        SendMessage(hBtnApplyHotkey2, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Info text
        HWND hInfo1 = CreateWindow("STATIC", "Use hotkeys to switch Resolution - Auto uses highest Hz",
            WS_VISIBLE | WS_CHILD | SS_CENTER, 15, 290, 570, 25, hwnd, NULL, NULL, NULL);
        SendMessage(hInfo1, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hInfo2 = CreateWindow("STATIC", "Reset to Default: Ctrl+Alt+Shift+R",
            WS_VISIBLE | WS_CHILD | SS_CENTER, 15, 320, 570, 25, hwnd, NULL, NULL, NULL);
        SendMessage(hInfo2, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Save button centered at bottom
        HWND hBtnSave = CreateWindow("BUTTON", "Save Config",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
            225, 365, 150, 35, hwnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);
        SendMessage(hBtnSave, WM_SETFONT, (WPARAM)hFont, TRUE);

        RegisterHotKey(hwnd, HOTKEY_GAME_MODE, MOD_NOREPEAT, VK_F7);
        RegisterHotKey(hwnd, HOTKEY_NORMAL_MODE, MOD_NOREPEAT, VK_F8);
        RegisterHotKey(hwnd, HOTKEY_RESET, MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'R');

        InitTray(hwnd);
        
        LoadConfig();
        
        break;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(220, 220, 220));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (INT_PTR)hBrushBg;
    }

    case WM_CTLCOLORBTN:
    {
        return (INT_PTR)hBrushBg;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_SAVE) {
            // Save configuration
            SaveConfig();
            DebugLog("USER ACTION: Manual save configuration");
            MessageBox(hwnd, "Configuration saved!", "MORPHIX", MB_ICONINFORMATION);
        }
        else if (LOWORD(wParam) == IDC_BTN_APPLY_RES1) {
            // Apply hotkey for Resolution 1
            ApplyHotkeys(hwnd);
            DebugLog("USER ACTION: Applied Resolution 1 hotkey settings");
            MessageBox(hwnd, "Hotkey applied for Resolution - 1! Use Save Config to persist.", "MORPHIX", MB_ICONINFORMATION);
        }
        else if (LOWORD(wParam) == IDC_BTN_APPLY_RES2) {
            // Apply hotkey for Resolution 2
            ApplyHotkeys(hwnd);
            DebugLog("USER ACTION: Applied Resolution 2 hotkey settings");
            MessageBox(hwnd, "Hotkey applied for Resolution - 2! Use Save Config to persist.", "MORPHIX", MB_ICONINFORMATION);
        }

        break;

    case WM_HOTKEY:
    {
        DebugLog("Hotkey pressed: ID=%d", wParam);
        char buf[64];
        int w = 0, h = 0;
        int monIdx = SendMessage(hComboMon, CB_GETCURSEL, 0, 0);

        if (wParam == HOTKEY_GAME_MODE) {
            DebugLog("Game mode hotkey activated");
            int idx = SendMessage(hComboResGame, CB_GETCURSEL, 0, 0);
            SendMessage(hComboResGame, CB_GETLBTEXT, idx, (LPARAM)buf);
            sscanf(buf, "%dx%d", &w, &h);
        }
        else if (wParam == HOTKEY_NORMAL_MODE) {
            DebugLog("Normal mode hotkey activated");
            int idx = SendMessage(hComboResNormal, CB_GETCURSEL, 0, 0);
            SendMessage(hComboResNormal, CB_GETLBTEXT, idx, (LPARAM)buf);
            sscanf(buf, "%dx%d", &w, &h);
        }
        else if (wParam == HOTKEY_RESET) {
            DebugLog("Reset hotkey activated - resetting to %dx%d", originalWidth, originalHeight);
            // Reset to original default resolution
            if (originalWidth > 0 && originalHeight > 0) {
                ChangeRes(originalWidth, originalHeight, monIdx);
            } else {
                DebugLog("ERROR: No original resolution stored");
            }
            return 0;
        }

        if (w > 0 && h > 0) {
            ChangeRes(w, h, monIdx);
        } else {
            DebugLog("Invalid resolution: %dx%d", w, h);
        }
    }
    break;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            POINT curPoint;
            GetCursorPos(&curPoint);
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, "Show Settings");
            AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");
            SetForegroundWindow(hwnd);
            int selection = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hwnd, NULL);
            if (selection == ID_TRAY_EXIT) PostQuitMessage(0);
            if (selection == ID_TRAY_SHOW) ShowWindow(hwnd, SW_SHOW);
            DestroyMenu(hMenu);
        }
        else if (lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hwnd, SW_SHOW);
        }
        break;

    case WM_CLOSE:
        SaveConfig();
        UnregisterHotKey(hwnd, HOTKEY_RESET);
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        
        DebugLog("Destroying window and cleaning up");
        Shell_NotifyIcon(NIM_DELETE, &nid);
        UnregisterHotKey(hwnd, HOTKEY_GAME_MODE);
        UnregisterHotKey(hwnd, HOTKEY_NORMAL_MODE);
        UnregisterHotKey(hwnd, HOTKEY_RESET);
        if (hFont) DeleteObject(hFont);
        if (hTitleFont) DeleteObject(hTitleFont);
        if (hBrushBg) DeleteObject(hBrushBg);
        if (DEBUG_MODE) FreeConsole();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Application entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize debug console if DEBUG_MODE is enabled
    if (DEBUG_MODE) {
        AllocConsole();
        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        SetConsoleTitle("MORPHIX Debug Console");
    }
    
    DebugLog("MORPHIX starting up");
    
    // Initialize config file path
    GetModuleFileName(NULL, configFile, MAX_PATH);
    char* lastSlash = strrchr(configFile, '\\');
    if (lastSlash) {
        strcpy(lastSlash + 1, "config.ini");
    }
    
    // Initialize common controls for hotkey control
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_HOTKEY_CLASS;
    InitCommonControlsEx(&icex);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ACRClass";
    wc.hbrBackground = CreateSolidBrush(RGB(32, 32, 32));
    
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100));
    if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIcon = hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow("ACRClass", "MORPHIX - Multi-Optical Resize & Pixel Hue Interface X",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 615, 460, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}