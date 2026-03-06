// INCLUDE
#define _WIN32_WINNT 0x0601
#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY 5
#endif
#pragma comment(lib, "uxtheme.lib")

// Dark Theme Color Palette
#define COL_BG          RGB(11,  11,  14)
#define COL_PANEL       RGB(18,  18,  24)
#define COL_CTRL        RGB(28,  28,  38)
#define COL_ACCENT      RGB(124, 58,  237)
#define COL_ACCENT2     RGB(139, 92,  246)
#define COL_ACCENT3     RGB(167, 139, 250)
#define COL_TEXT        RGB(250, 250, 255)
#define COL_TEXT2       RGB(148, 148, 168)
#define COL_TEXT3       RGB(72,  74,  92)
#define COL_BORDER      RGB(34,  36,  52)
#define COL_BORDER2     RGB(52,  54,  72)
#define COL_GREEN       RGB(52,  211, 153)
#define COL_HEADER_BG   RGB(15,  15,  20)
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <uxtheme.h>

// Strip Visual Styles so WM_CTLCOLOR* messages are honoured
static inline void NoTheme(HWND h) { SetWindowTheme(h, L"", L""); }
#include <string>
#include <vector>
#include <map>
#include <set>
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

// Brushes, Pens, Fonts
HBRUSH hBrushDarkBg  = NULL;
HBRUSH hBrushEditBg  = NULL;
HBRUSH hBrushBg      = NULL;
HBRUSH hBrushPanel   = NULL;
HBRUSH hBrushCtrl    = NULL;
HPEN   hPenAccent    = NULL;
HPEN   hPenBorder    = NULL;
HPEN   hPenSep       = NULL;
HFONT  hFont         = NULL;
HFONT  hTitleFont    = NULL;
HFONT  hSmallFont    = NULL;
HFONT  hLargeFont    = NULL;
HFONT  hPresetFont   = NULL;  // JetBrains Mono for PRESET labels
HFONT  hSmallULFont  = NULL;  // Small underline font for reset hint

// Header control handles (for per-control coloring)
HWND hTitleLabel     = NULL;
HWND hSubtitleLabel  = NULL;

// Active preset tracking (0=none, 1=game/preset1, 2=normal/preset2)
int g_activePreset = 0;

// Per-combo set of resolution strings that failed CDS_TEST
std::map<HWND, std::set<std::string>> g_incompatRes;

int currentHz = 0;
char configFile[MAX_PATH];
int originalWidth = 0, originalHeight = 0, originalHz = 0;

// Revert-after-change state
#define ID_TRAY_RESET 1004

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
    WritePrivateProfileStringA("Settings", "Monitor", buf, configFile);
    
    // Save Resolution 1 selection
    int res1Idx = SendMessage(hComboResGame, CB_GETCURSEL, 0, 0);
    SendMessage(hComboResGame, CB_GETLBTEXT, res1Idx, (LPARAM)buf);
    WritePrivateProfileStringA("Settings", "Resolution1", buf, configFile);
    
    // Save Resolution 2 selection
    int res2Idx = SendMessage(hComboResNormal, CB_GETCURSEL, 0, 0);
    SendMessage(hComboResNormal, CB_GETLBTEXT, res2Idx, (LPARAM)buf);
    WritePrivateProfileStringA("Settings", "Resolution2", buf, configFile);
    
    // Save hotkeys
    sprintf(buf, "%d", gameHotkey);
    WritePrivateProfileStringA("Settings", "Hotkey1", buf, configFile);
    sprintf(buf, "%d", normalHotkey);
    WritePrivateProfileStringA("Settings", "Hotkey2", buf, configFile);
    
    DebugLog("Configuration saved to %s", configFile);
}

// Load configuration from INI file
void LoadConfig() {
    char buf[64];
    
    DebugLog("Loading configuration from INI file");
    
    // Load monitor selection
    GetPrivateProfileStringA("Settings", "Monitor", "0", buf, 64, configFile);
    int monIdx = atoi(buf);
    if (monIdx >= 0 && monIdx < SendMessage(hComboMon, CB_GETCOUNT, 0, 0)) {
        SendMessage(hComboMon, CB_SETCURSEL, monIdx, 0);
        DebugLog("Loaded monitor index: %d", monIdx);
    }
    
    // Load Resolution 1
    GetPrivateProfileStringA("Settings", "Resolution1", "", buf, 64, configFile);
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
    GetPrivateProfileStringA("Settings", "Resolution2", "", buf, 64, configFile);
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
    GetPrivateProfileStringA("Settings", "Hotkey1", "118", buf, 64, configFile);
    gameHotkey = atoi(buf);
    SendMessage(hHotkeyGame, HKM_SETHOTKEY, MAKEWORD(gameHotkey, 0), 0);
    DebugLog("Loaded Hotkey 1: 0x%X", gameHotkey);
    
    GetPrivateProfileStringA("Settings", "Hotkey2", "119", buf, 64, configFile);
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
        SetWindowTextA(hLabelCurrentHz, hzText);
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

    // Reset compatibility set for this combo
    g_incompatRes[hCombo].clear();

    // Test each resolution via CDS_TEST and add to combobox
    for (const auto& res : resolutions) {
        int rw = 0, rh = 0;
        sscanf(res.second.c_str(), "%dx%d", &rw, &rh);
        int hz = resolutionMaxHz[res.second];

        // Test with width+height only — no forced Hz/bpp so valid modes like 1080x1080
        // aren't falsely rejected by the driver's strict frequency matching
        DEVMODE dmTest;
        ZeroMemory(&dmTest, sizeof(dmTest));
        dmTest.dmSize      = sizeof(dmTest);
        dmTest.dmPelsWidth  = rw;
        dmTest.dmPelsHeight = rh;
        dmTest.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

        LONG compat = ChangeDisplaySettingsEx(
            monitors[monitorIndex].DeviceName, &dmTest, NULL, CDS_TEST, NULL);
        if (compat != DISP_CHANGE_SUCCESSFUL) {
            g_incompatRes[hCombo].insert(res.second);
        }
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
            MessageBoxA(NULL, "Failed to change resolution. Check if your monitor supports this.", "MORPHIX Error", MB_ICONERROR | MB_TOPMOST);
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
    lstrcpyA((LPSTR)nid.szTip, "MORPHIX - Resolution Changer");
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

// Format hotkey virtual-key + modifiers into human-readable string
static void FormatHotkeyText(BYTE vk, BYTE mod, char* buf, int bufLen) {
    buf[0] = 0;
    if (!vk) { strcpy(buf, "None"); return; }
    if (mod & HOTKEYF_CONTROL) strcat(buf, "Ctrl + ");
    if (mod & HOTKEYF_ALT)     strcat(buf, "Alt + ");
    if (mod & HOTKEYF_SHIFT)   strcat(buf, "Shift + ");
    char name[32] = {0};
    if (vk >= VK_F1 && vk <= VK_F24)          sprintf(name, "F%d", vk - VK_F1 + 1);
    else if (vk == VK_SPACE)                   strcpy(name, "Space");
    else if (vk == VK_RETURN)                  strcpy(name, "Enter");
    else if (vk == VK_TAB)                     strcpy(name, "Tab");
    else if (vk == VK_ESCAPE)                  strcpy(name, "Esc");
    else if (vk == VK_DELETE)                  strcpy(name, "Del");
    else if (vk == VK_INSERT)                  strcpy(name, "Ins");
    else if (vk == VK_HOME)                    strcpy(name, "Home");
    else if (vk == VK_END)                     strcpy(name, "End");
    else if (vk == VK_PRIOR)                   strcpy(name, "PgUp");
    else if (vk == VK_NEXT)                    strcpy(name, "PgDn");
    else if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) { name[0]=(char)vk; name[1]=0; }
    else {
        UINT sc = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        if (!GetKeyNameTextA((LONG)(sc << 16), name, sizeof(name))) sprintf(name, "0x%02X", vk);
    }
    strncat(buf, name, bufLen - (int)strlen(buf) - 1);
}

// Hotkey subclass proc — dark background + custom text rendering
static WNDPROC g_OldHkProc = NULL;
LRESULT CALLBACK HotkeySubclassProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_ERASEBKGND) {
        RECT rc; GetClientRect(h, &rc);
        FillRect((HDC)w, &rc, hBrushCtrl);
        return 1;
    }
    if (msg == WM_PAINT) {
        LRESULT r = CallWindowProcA(g_OldHkProc, h, msg, w, l);
        HDC hdc = GetDC(h);
        if (hdc) {
            RECT rc; GetClientRect(h, &rc);
            FillRect(hdc, &rc, hBrushCtrl);
            WORD hk = (WORD)SendMessage(h, HKM_GETHOTKEY, 0, 0);
            BYTE vk = LOBYTE(hk), mod = HIBYTE(hk);
            char buf[64] = {0};
            FormatHotkeyText(vk, mod, buf, sizeof(buf));
            SetTextColor(hdc, vk ? COL_TEXT : COL_TEXT3);
            SetBkMode(hdc, TRANSPARENT);
            HFONT hf = (HFONT)SendMessage(h, WM_GETFONT, 0, 0);
            HFONT hfOld = hf ? (HFONT)SelectObject(hdc, hf) : NULL;
            RECT rcT = {6, 0, rc.right - 4, rc.bottom};
            DrawTextA(hdc, buf, -1, &rcT, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            if (hfOld) SelectObject(hdc, hfOld);
            ReleaseDC(h, hdc);
        }
        return r;
    }
    if (msg == WM_SETFOCUS) {
        // Unregister hotkeys while editing so they don\'t fire while the user types
        HWND parent = GetParent(h);
        UnregisterHotKey(parent, HOTKEY_GAME_MODE);
        UnregisterHotKey(parent, HOTKEY_NORMAL_MODE);
        LRESULT r = CallWindowProcA(g_OldHkProc, h, msg, w, l);
        InvalidateRect(h, NULL, TRUE);
        return r;
    }
    if (msg == WM_KILLFOCUS) {
        // Re-register the SAVED hotkeys (new hotkey only takes effect after Save & Apply)
        HWND parent = GetParent(h);
        RegisterHotKey(parent, HOTKEY_GAME_MODE,   MOD_NOREPEAT, gameHotkey);
        RegisterHotKey(parent, HOTKEY_NORMAL_MODE, MOD_NOREPEAT, normalHotkey);
        LRESULT r = CallWindowProcA(g_OldHkProc, h, msg, w, l);
        InvalidateRect(h, NULL, TRUE);
        return r;
    }
    return CallWindowProcA(g_OldHkProc, h, msg, w, l);
}
static inline void SubclassHotkey(HWND h) {
    // Allow ALL combinations incl. bare Fn keys (wParam = 0 means no invalid combos)
    SendMessage(h, HKM_SETRULES, 0, 0);
    WNDPROC old = (WNDPROC)SetWindowLongPtrA(h, GWLP_WNDPROC, (LONG_PTR)HotkeySubclassProc);
    if (!g_OldHkProc) g_OldHkProc = old;
}

// Combobox subclass proc — overpaints the native white dropdown button
static WNDPROC g_OldComboProc = NULL;
LRESULT CALLBACK ComboSubclassProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    LRESULT r = CallWindowProcA(g_OldComboProc, h, msg, w, l);
    if (msg == WM_PAINT || msg == WM_NCPAINT) {
        HDC hdc = GetDC(h);
        if (hdc) {
            RECT rc; GetClientRect(h, &rc);
            // Button zone: fixed 18px, inset 1px from border on right/top/bottom
            RECT rcBtn = {rc.right - 18, rc.top + 1, rc.right - 1, rc.bottom - 1};
            FillRect(hdc, &rcBtn, hBrushCtrl);
            // Thin left separator inside the button zone
            HPEN hSepPen = CreatePen(PS_SOLID, 1, COL_BORDER2);
            HPEN hOldP   = (HPEN)SelectObject(hdc, hSepPen);
            MoveToEx(hdc, rcBtn.left, rcBtn.top + 2, NULL);
            LineTo(hdc,   rcBtn.left, rcBtn.bottom - 2);
            SelectObject(hdc, hOldP);
            DeleteObject(hSepPen);
            // Triangle centered inside button zone
            int ax = rcBtn.left + (rcBtn.right - rcBtn.left) / 2;
            int ay = (rcBtn.top + rcBtn.bottom) / 2;
            POINT tri[3] = {{ax - 4, ay - 2}, {ax + 4, ay - 2}, {ax, ay + 3}};
            HBRUSH hTriBr  = CreateSolidBrush(COL_TEXT2);
            HPEN   hTriPen = CreatePen(PS_SOLID, 1, COL_TEXT2);
            HBRUSH hOldBr  = (HBRUSH)SelectObject(hdc, hTriBr);
            HPEN   hOldPn  = (HPEN)SelectObject(hdc, hTriPen);
            Polygon(hdc, tri, 3);
            SelectObject(hdc, hOldBr); SelectObject(hdc, hOldPn);
            DeleteObject(hTriBr); DeleteObject(hTriPen);
            ReleaseDC(h, hdc);
        }
    }
    return r;
}
static inline void SubclassCombo(HWND h) {
    WNDPROC old = (WNDPROC)SetWindowLongPtrA(h, GWLP_WNDPROC, (LONG_PTR)ComboSubclassProc);
    if (!g_OldComboProc) g_OldComboProc = old;
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
    {
        DebugLog("Creating window and controls");

        // --- Fonts (negative height = character height in pixels) ---
        hLargeFont  = CreateFontA(-26, 0, 0, 0, FW_BOLD,    FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        hTitleFont  = CreateFontA(-14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        hFont       = CreateFontA(-14, 0, 0, 0, FW_NORMAL,   FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        hSmallFont  = CreateFontA(-11, 0, 0, 0, FW_NORMAL,   FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        // JetBrains Mono for PRESET 1 / PRESET 2 titles (falls back to Consolas if not installed)
        hPresetFont = CreateFontA(-14, 0, 0, 0, FW_BOLD,     FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH  | FF_DONTCARE, "JetBrains Mono");
        if (!hPresetFont) hPresetFont = CreateFontA(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_DONTCARE, "Consolas");
        // Small underline font for the reset hint
        hSmallULFont = CreateFontA(-11, 0, 0, 0, FW_NORMAL,  FALSE, TRUE,  FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        // --- Brushes & Pens ---
        hBrushBg     = CreateSolidBrush(COL_BG);
        hBrushPanel  = CreateSolidBrush(COL_PANEL);
        hBrushCtrl   = CreateSolidBrush(COL_CTRL);
        hBrushDarkBg = hBrushBg;
        hBrushEditBg = hBrushCtrl;
        hPenAccent   = CreatePen(PS_SOLID, 2, COL_ACCENT2);
        hPenBorder   = CreatePen(PS_SOLID, 1, COL_BORDER2);
        hPenSep      = CreatePen(PS_SOLID, 1, COL_BORDER);

        // ==========================================
        // Header bar  (y = 0..64)
        // ==========================================
        hTitleLabel = CreateWindowA("STATIC", "MORPHIX",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            18, 8, 280, 32, hwnd, NULL, NULL, NULL);
        SendMessage(hTitleLabel, WM_SETFONT, (WPARAM)hLargeFont, TRUE);
        NoTheme(hTitleLabel);

        hSubtitleLabel = CreateWindowA("STATIC", "Resolution Manager",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            21, 38, 260, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hSubtitleLabel, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hSubtitleLabel);

        // ==========================================
        // Monitor section  (y = 68..124)  — 10px gap below header
        // ==========================================
        HWND hMonLabel = CreateWindowA("STATIC", "DISPLAY",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            20, 74, 100, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hMonLabel, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hMonLabel);

        hComboMon = CreateWindowA("COMBOBOX", "",
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP,
            20, 90, 366, 160, hwnd, (HMENU)IDC_COMBO_MONITOR, NULL, NULL);
        SendMessage(hComboMon, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hComboMon);
        SubclassCombo(hComboMon);

        EnumerateMonitors();
        for (size_t i = 0; i < monitors.size(); i++) {
            char buf[256];
            sprintf(buf, "Monitor %d: %s", (int)i + 1, monitors[i].DeviceString);
            SendMessage(hComboMon, CB_ADDSTRING, 0, (LPARAM)buf);
        }
        SendMessage(hComboMon, CB_SETCURSEL, 0, 0);

        hLabelCurrentHz = CreateWindowA("STATIC", "Detecting...",
            WS_VISIBLE | WS_CHILD | SS_RIGHT | SS_CENTERIMAGE,
            394, 90, 232, 28, hwnd, (HMENU)IDC_LABEL_CURRENT_HZ, NULL, NULL);
        SendMessage(hLabelCurrentHz, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
        NoTheme(hLabelCurrentHz);
        GetCurrentDisplaySettings(0);

        // ==========================================
        // PRESET 1 card  (x=12..330, y=130..304)
        // ==========================================
        HWND hP1Title = CreateWindowA("STATIC", "PRESET 1",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            28, 146, 140, 18, hwnd, NULL, NULL, NULL);
        // JetBrains Mono for preset title
        SendMessage(hP1Title, WM_SETFONT, (WPARAM)hPresetFont, TRUE);
        NoTheme(hP1Title);

        HWND hR1Label = CreateWindowA("STATIC", "Resolution",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            28, 178, 130, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hR1Label, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hR1Label);

        hComboResGame = CreateWindowA("COMBOBOX", "",
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP,
            28, 194, 286, 400, hwnd, (HMENU)IDC_COMBO_RES_GAME, NULL, NULL);
        SendMessage(hComboResGame, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hComboResGame);
        SubclassCombo(hComboResGame);
        PopulateResolutions(hComboResGame, 0);

        HWND hK1Label = CreateWindowA("STATIC", "Hotkey",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            28, 238, 80, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hK1Label, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hK1Label);

        // Width = ~15 chars in JetBrains Mono (~9px/char) + padding = 155px
        hHotkeyGame = CreateWindowA(HOTKEY_CLASSA, "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
            28, 256, 155, 28, hwnd, (HMENU)IDC_HOTKEY_GAME, NULL, NULL);
        SendMessage(hHotkeyGame, HKM_SETHOTKEY, MAKEWORD(VK_F7, 0), 0);
        SendMessage(hHotkeyGame, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hHotkeyGame);
        SubclassHotkey(hHotkeyGame);

        // ==========================================
        // PRESET 2 card  (x=338..W-12, y=130..304)
        // ==========================================
        HWND hP2Title = CreateWindowA("STATIC", "PRESET 2",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            356, 146, 140, 18, hwnd, NULL, NULL, NULL);
        // JetBrains Mono for preset title
        SendMessage(hP2Title, WM_SETFONT, (WPARAM)hPresetFont, TRUE);
        NoTheme(hP2Title);

        HWND hR2Label = CreateWindowA("STATIC", "Resolution",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            356, 178, 130, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hR2Label, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hR2Label);

        hComboResNormal = CreateWindowA("COMBOBOX", "",
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP,
            356, 194, 286, 400, hwnd, (HMENU)IDC_COMBO_RES_NORMAL, NULL, NULL);
        SendMessage(hComboResNormal, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hComboResNormal);
        SubclassCombo(hComboResNormal);
        PopulateResolutions(hComboResNormal, 0);

        HWND hK2Label = CreateWindowA("STATIC", "Hotkey",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            356, 238, 80, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hK2Label, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hK2Label);

        hHotkeyNormal = CreateWindowA(HOTKEY_CLASSA, "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
            356, 256, 155, 28, hwnd, (HMENU)IDC_HOTKEY_NORMAL, NULL, NULL);
        SendMessage(hHotkeyNormal, HKM_SETHOTKEY, MAKEWORD(VK_F8, 0), 0);
        SendMessage(hHotkeyNormal, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hHotkeyNormal);
        SubclassHotkey(hHotkeyNormal);

        // ==========================================
        // Footer: info + button  (y=310..398)
        // ==========================================
        HWND hInfo1 = CreateWindowA("STATIC", "Auto-selects the highest available refresh rate",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            14, 318, 636, 16, hwnd, NULL, NULL, NULL);
        SendMessage(hInfo1, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hInfo1);

        HWND hInfo2 = CreateWindowA("STATIC", "Reset to default:  Ctrl + Alt + Shift + R",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            14, 338, 636, 16, hwnd, NULL, NULL, NULL);
        // Underlined font for the reset hint
        SendMessage(hInfo2, WM_SETFONT, (WPARAM)hSmallULFont, TRUE);
        NoTheme(hInfo2);

        HWND hBtnSave = CreateWindowA("BUTTON", "Save && Apply",
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
            212, 364, 240, 40, hwnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);
        SendMessage(hBtnSave, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
        NoTheme(hBtnSave);

        RegisterHotKey(hwnd, HOTKEY_GAME_MODE, MOD_NOREPEAT, VK_F7);
        RegisterHotKey(hwnd, HOTKEY_NORMAL_MODE, MOD_NOREPEAT, VK_F8);
        RegisterHotKey(hwnd, HOTKEY_RESET, MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'R');

        InitTray(hwnd);
        LoadConfig();
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int W = rc.right, H = rc.bottom;

        // --- whole background ---
        FillRect(hdc, &rc, hBrushBg);

        // --- header bar (y 0..56) — shrunk 10px to give Display more room ---
        RECT rcHdr = {0, 0, W, 56};
        FillRect(hdc, &rcHdr, hBrushPanel);

        // accent rule under header
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPenAccent);
        MoveToEx(hdc, 0, 55, NULL); LineTo(hdc, W, 55);

        // --- monitor section card (y 68..124) ---
        RECT rcMon = {12, 68, W - 12, 124};
        FillRect(hdc, &rcMon, hBrushPanel);
        // left accent bar
        HBRUSH hAccBr = CreateSolidBrush(COL_ACCENT);
        RECT rcAccL = {12, 68, 15, 124};
        FillRect(hdc, &rcAccL, hAccBr);
        DeleteObject(hAccBr);
        // card border
        SelectObject(hdc, hPenBorder);
        HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rcMon.left, rcMon.top, rcMon.right, rcMon.bottom);

        // --- thin separator ---
        SelectObject(hdc, hPenSep);
        MoveToEx(hdc, 12, 130, NULL); LineTo(hdc, W - 12, 130);

        // ---- Active-preset highlight: green 3px bottom bar + bright border ----
        HPEN hGreenPen = CreatePen(PS_SOLID, 2, COL_GREEN);
        HBRUSH hGreenBr = CreateSolidBrush(COL_GREEN);

        // --- PRESET 1 card (x 12..330, y 132..304) ---
        RECT rcC1 = {12, 132, 330, 304};
        FillRect(hdc, &rcC1, hBrushPanel);
        // top accent bar
        HBRUSH hA1 = CreateSolidBrush(g_activePreset == 1 ? COL_GREEN : COL_ACCENT);
        RECT rcAt1 = {12, 132, 330, 135};
        FillRect(hdc, &rcAt1, hA1);
        DeleteObject(hA1);
        // card border — green if active
        if (g_activePreset == 1) {
            SelectObject(hdc, hGreenPen);
        } else {
            SelectObject(hdc, hPenBorder);
        }
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rcC1.left, rcC1.top, rcC1.right, rcC1.bottom);
        // green bottom glow bar if active
        if (g_activePreset == 1) {
            RECT rcBot1 = {12, 301, 330, 304};
            FillRect(hdc, &rcBot1, hGreenBr);
        }

        // --- PRESET 2 card (x 338..W-12, y 132..304) ---
        RECT rcC2 = {338, 132, W - 12, 304};
        FillRect(hdc, &rcC2, hBrushPanel);
        HBRUSH hA2 = CreateSolidBrush(g_activePreset == 2 ? COL_GREEN : COL_ACCENT);
        RECT rcAt2 = {338, 132, W - 12, 135};
        FillRect(hdc, &rcAt2, hA2);
        DeleteObject(hA2);
        if (g_activePreset == 2) {
            SelectObject(hdc, hGreenPen);
        } else {
            SelectObject(hdc, hPenBorder);
        }
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rcC2.left, rcC2.top, rcC2.right, rcC2.bottom);
        if (g_activePreset == 2) {
            RECT rcBot2 = {338, 301, W - 12, 304};
            FillRect(hdc, &rcBot2, hGreenBr);
        }

        DeleteObject(hGreenPen);
        DeleteObject(hGreenBr);

        // --- bottom separator above footer ---
        SelectObject(hdc, hPenSep);
        MoveToEx(hdc, 12, 310, NULL); LineTo(hdc, W - 12, 310);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBr);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        SetBkMode(hdc, TRANSPARENT);

        // Special named controls
        if (hCtrl == hTitleLabel) {
            SetTextColor(hdc, COL_TEXT);
            return (INT_PTR)hBrushPanel;
        }
        if (hCtrl == hSubtitleLabel) {
            SetTextColor(hdc, COL_ACCENT3);
            return (INT_PTR)hBrushPanel;
        }
        if (hCtrl == hLabelCurrentHz) {
            SetTextColor(hdc, COL_GREEN);
            return (INT_PTR)hBrushPanel;
        }

        // Detect background by control position
        RECT rcCtrl;
        GetWindowRect(hCtrl, &rcCtrl);
        MapWindowPoints(HWND_DESKTOP, hwnd, (POINT*)&rcCtrl, 2);

        // Header zone (y < 56)
        if (rcCtrl.top < 56) {
            SetTextColor(hdc, COL_TEXT2);
            return (INT_PTR)hBrushPanel;
        }
        // Monitor card zone (y 68..124)
        if (rcCtrl.top >= 56 && rcCtrl.top < 126) {
            SetTextColor(hdc, COL_TEXT2);
            return (INT_PTR)hBrushPanel;
        }
        // Preset card zone (y 132..304)
        if (rcCtrl.top >= 126 && rcCtrl.top < 306) {
            SetTextColor(hdc, COL_TEXT2);
            return (INT_PTR)hBrushPanel;
        }

        // Footer
        SetTextColor(hdc, COL_TEXT3);
        return (INT_PTR)hBrushBg;
    }

    case WM_CTLCOLORLISTBOX:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COL_TEXT);
        SetBkColor(hdc, COL_PANEL);
        return (INT_PTR)hBrushPanel;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COL_TEXT);
        SetBkColor(hdc, COL_CTRL);
        return (INT_PTR)hBrushCtrl;
    }

    case WM_CTLCOLORBTN:
        return (INT_PTR)hBrushBg;

    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT* mis = (MEASUREITEMSTRUCT*)lParam;
        if (mis->CtlType == ODT_COMBOBOX) {
            mis->itemHeight = 22;  // dark row height
            return TRUE;
        }
        break;
    }

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;

        // --- Owner-draw COMBOBOX items ---
        if (dis->CtlType == ODT_COMBOBOX && dis->CtlID != IDC_BTN_SAVE) {
            HDC hdc = dis->hDC;
            RECT rc  = dis->rcItem;
            bool isEditArea = (dis->itemState & ODS_COMBOBOXEDIT) != 0;
            bool selected   = (dis->itemState & ODS_SELECTED) != 0 && !isEditArea;

            // Background
            COLORREF bgCol  = selected ? COL_ACCENT : COL_CTRL;
            HBRUSH hBr = CreateSolidBrush(bgCol);
            FillRect(hdc, &rc, hBr);
            DeleteObject(hBr);

            // Text
            if (dis->itemID != (UINT)-1) {
                char text[256] = {0};
                SendMessageA(dis->hwndItem, CB_GETLBTEXT, dis->itemID, (LPARAM)text);
                bool incompat = g_incompatRes.count(dis->hwndItem) &&
                                g_incompatRes[dis->hwndItem].count(std::string(text)) > 0;
                COLORREF textCol;
                if (selected)       textCol = RGB(255, 255, 255);
                else if (incompat)  textCol = RGB(220, 80,  80);   // red — incompatible
                else                textCol = COL_GREEN;            // green — compatible
                SetTextColor(hdc, textCol);
                SetBkMode(hdc, TRANSPARENT);
                HFONT hf = (HFONT)SendMessage(dis->hwndItem, WM_GETFONT, 0, 0);
                HFONT hfOld = hf ? (HFONT)SelectObject(hdc, hf) : NULL;
                RECT rcT = rc; rcT.left += 6;
                DrawTextA(hdc, text, -1, &rcT, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                if (hfOld) SelectObject(hdc, hfOld);
            }

            // Arrow button is handled entirely by ComboSubclassProc (WM_PAINT overpaint)

            // Focus rect
            if (dis->itemState & ODS_FOCUS) DrawFocusRect(hdc, &rc);
            return TRUE;
        }

        if (dis->CtlID == IDC_BTN_SAVE) {
            HDC hdc = dis->hDC;
            RECT rc = dis->rcItem;
            bool pressed = (dis->itemState & ODS_SELECTED) != 0;

            // Button fill
            COLORREF btnCol = pressed ? COL_ACCENT : COL_ACCENT2;
            HBRUSH hBtnBr = CreateSolidBrush(btnCol);
            FillRect(hdc, &rc, hBtnBr);
            DeleteObject(hBtnBr);

            // Border
            HPEN hBP = CreatePen(PS_SOLID, 1, COL_ACCENT3);
            HPEN hBPOld = (HPEN)SelectObject(hdc, hBP);
            HBRUSH hBBO = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
            SelectObject(hdc, hBPOld);
            SelectObject(hdc, hBBO);
            DeleteObject(hBP);

            // Label
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, hTitleFont);
            DrawTextA(hdc, "Save && Apply", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            if (dis->itemState & ODS_FOCUS) {
                InflateRect(&rc, -3, -3);
                DrawFocusRect(hdc, &rc);
            }
            return TRUE;
        }
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_SAVE) {
            ApplyHotkeys(hwnd);
            SaveConfig();
            DebugLog("USER ACTION: Applied hotkeys and saved configuration");
            MessageBoxA(hwnd, "Configuration saved and applied!", "MORPHIX", MB_ICONINFORMATION);
        }
        // Warn when an incompatible resolution is selected
        if ((LOWORD(wParam) == IDC_COMBO_RES_GAME || LOWORD(wParam) == IDC_COMBO_RES_NORMAL)
            && HIWORD(wParam) == CBN_SELCHANGE) {
            HWND hSel = (HWND)lParam;
            int selIdx = SendMessage(hSel, CB_GETCURSEL, 0, 0);
            if (selIdx != CB_ERR) {
                char selText[64] = {0};
                SendMessageA(hSel, CB_GETLBTEXT, selIdx, (LPARAM)selText);
                if (g_incompatRes.count(hSel) &&
                    g_incompatRes[hSel].count(std::string(selText)) > 0) {
                    MessageBoxA(hwnd,
                        "This resolution was reported incompatible by your display driver.\n"
                        "Applying it may result in a blank screen or display error.\n\n"
                        "Proceed with caution.",
                        "MORPHIX  —  Incompatible Resolution",
                        MB_ICONWARNING | MB_OK);
                }
            }
        }
        if (LOWORD(wParam) == IDC_COMBO_MONITOR && HIWORD(wParam) == CBN_SELCHANGE) {
            int idx = SendMessage(hComboMon, CB_GETCURSEL, 0, 0);
            PopulateResolutions(hComboResGame, idx);
            PopulateResolutions(hComboResNormal, idx);
            GetCurrentDisplaySettings(idx);
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
            g_activePreset = 1;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == HOTKEY_NORMAL_MODE) {
            DebugLog("Normal mode hotkey activated");
            int idx = SendMessage(hComboResNormal, CB_GETCURSEL, 0, 0);
            SendMessage(hComboResNormal, CB_GETLBTEXT, idx, (LPARAM)buf);
            sscanf(buf, "%dx%d", &w, &h);
            g_activePreset = 2;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == HOTKEY_RESET) {
            DebugLog("Reset hotkey activated - resetting to %dx%d", originalWidth, originalHeight);
            if (originalWidth > 0 && originalHeight > 0) {
                ChangeRes(originalWidth, originalHeight, monIdx);
                g_activePreset = 0;
                InvalidateRect(hwnd, NULL, FALSE);
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
            AppendMenuA(hMenu, MF_STRING, ID_TRAY_SHOW, "Show Settings");
            AppendMenuA(hMenu, MF_STRING, ID_TRAY_RESET, "Reset to Default");
            AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuA(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");
            SetForegroundWindow(hwnd);
            int selection = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hwnd, NULL);
            if (selection == ID_TRAY_EXIT)  PostQuitMessage(0);
            if (selection == ID_TRAY_SHOW)  { ShowWindow(hwnd, SW_SHOW); SetForegroundWindow(hwnd); }
            if (selection == ID_TRAY_RESET) {
                int monIdx2 = SendMessage(hComboMon, CB_GETCURSEL, 0, 0);
                if (originalWidth > 0 && originalHeight > 0) {
                    ChangeRes(originalWidth, originalHeight, monIdx2);
                    g_activePreset = 0;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
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
        if (hFont)       DeleteObject(hFont);
        if (hTitleFont)  DeleteObject(hTitleFont);
        if (hSmallFont)  DeleteObject(hSmallFont);
        if (hLargeFont)  DeleteObject(hLargeFont);
        if (hPresetFont) DeleteObject(hPresetFont);
        if (hSmallULFont)DeleteObject(hSmallULFont);
        if (hBrushBg)   DeleteObject(hBrushBg);
        if (hBrushPanel)DeleteObject(hBrushPanel);
        if (hBrushCtrl) DeleteObject(hBrushCtrl);
        if (hPenAccent) DeleteObject(hPenAccent);
        if (hPenBorder) DeleteObject(hPenBorder);
        if (hPenSep)    DeleteObject(hPenSep);
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
        SetConsoleTitleA("MORPHIX Debug Console");
    }
    
    DebugLog("MORPHIX starting up");
    
    // Initialize config file path
    GetModuleFileNameA(NULL, configFile, MAX_PATH);
    char* lastSlash = strrchr(configFile, '\\');
    if (lastSlash) {
        strcpy(lastSlash + 1, "config.ini");
    }
    
    // Initialize common controls for hotkey control
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_HOTKEY_CLASS;
    InitCommonControlsEx(&icex);

    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ACRClass";
    wc.hbrBackground = CreateSolidBrush(RGB(32, 32, 32));
    
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100));
    if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIcon = hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA("ACRClass", "MORPHIX",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 666, 448, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, SW_HIDE);
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}