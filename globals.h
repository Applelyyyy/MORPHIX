#pragma once
#define _WIN32_WINNT 0x0601
#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY 5
#endif
#pragma comment(lib, "uxtheme.lib")

// ---- Includes ----
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <stdio.h>
#include <cmath>

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

// ---- Debug ----
#define DEBUG_MODE false

// ---- Color palette ----
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

// ---- Tray / message IDs ----
#define ID_TRAY_APP_ICON    1001
#define ID_TRAY_EXIT        1002
#define ID_TRAY_SHOW        1003
#define ID_TRAY_RESET       1004
#define WM_TRAYICON         (WM_USER + 1)

// ---- Hotkey IDs ----
#define HOTKEY_GAME_MODE    1
#define HOTKEY_NORMAL_MODE  2
#define HOTKEY_RESET        3
#define HOTKEY_COLOR1       4
#define HOTKEY_COLOR2       5

// ---- Control IDs ----
#define IDC_COMBO_MONITOR     2007
#define IDC_HOTKEY_GAME       2008
#define IDC_HOTKEY_NORMAL     2009
#define IDC_HOTKEY_COLOR1     2029
#define IDC_HOTKEY_COLOR2     2030
#define IDC_COMBO_RES_GAME    2011
#define IDC_COMBO_RES_NORMAL  2012
#define IDC_LABEL_CURRENT_HZ  2013
#define IDC_BTN_SAVE          2017
#define IDC_BTN_REFRESH_RES   2018
#define IDC_BTN_TAB_DISPLAY   2019
#define IDC_BTN_TAB_COLOR     2020
#define IDC_EDIT_G_VIBRANCE   2024
#define IDC_EDIT_N_VIBRANCE   2028

// ---- Strip visual styles so WM_CTLCOLOR* messages are honoured ----
static inline void NoTheme(HWND h) { SetWindowTheme(h, L"", L""); }

// ---- Global variable declarations ----
extern NOTIFYICONDATA             nid;
extern HWND                       hComboMon;
extern HWND                       hHotkeyGame, hHotkeyNormal;
extern HWND                       hHotkeyColor1, hHotkeyColor2;
extern HWND                       hLblColorHotkey1, hLblColorHotkey2;
extern HWND                       hComboResGame, hComboResNormal, hLabelCurrentHz;
extern HWND                       hBtnRefreshRes;
extern HWND                       hBtnTabDisplay, hBtnTabColor;
extern HWND                       hEditGVibrance;
extern HWND                       hEditNVibrance;
extern HWND                       hLblGVibrance;
extern HWND                       hLblNVibrance;
extern HWND                       hP1Title, hP2Title;         // card title statics (renamed per tab)
extern HWND                       hLblResHotkey1, hLblResHotkey2; // "Res Hotkey" labels (Display tab)
extern HWND                       hLblRes1, hLblRes2;  // "Resolution" labels (Display tab only)
extern std::vector<DISPLAY_DEVICE> monitors;
extern UINT                       gameHotkey;
extern UINT                       normalHotkey;
extern UINT                       colorHotkey1;
extern UINT                       colorHotkey2;

extern int                        gameVibrance;
extern int                        normalVibrance;

// GDI resources
extern HBRUSH hBrushBg, hBrushPanel, hBrushCtrl;
extern HPEN   hPenAccent, hPenBorder, hPenSep;
extern HFONT  hFont, hTitleFont, hSmallFont, hLargeFont, hPresetFont, hSmallULFont;

// Pre-created draw GDI objects (never allocated in hot paint/draw paths)
extern HBRUSH hBrushAccent;    // COL_ACCENT    — monitor bar, combo selected bg
extern HBRUSH hBrushAccent2;   // COL_ACCENT2   — save button normal bg
extern HBRUSH hBrushGreen;     // COL_GREEN     — preset active indicators
extern HBRUSH hBrushHotlight;  // RGB(38,38,52) — refresh button hover bg
extern HBRUSH hBrushText2;     // COL_TEXT2     — combo chevron fill
extern HPEN   hPenGreen;       // 2px green     — preset active card border
extern HPEN   hPenAccent3;     // 1px accent3   — save button border
extern HPEN   hPenAccent2Thin; // 1px accent2   — refresh button hover border
extern HPEN   hPenText2;       // 1px text2     — combo chevron outline

// Cached last-saved resolution strings (avoids INI disk reads in hot paths)
extern char g_savedRes1[64];
extern char g_savedRes2[64];

// Named controls used for per-control colour overrides
extern HWND hTitleLabel, hSubtitleLabel;

// Application state
extern int  g_activeResPreset;   // 0=none, 1=gaming res, 2=normal res   (Display tab)
extern int  g_activeColorPreset; // 0=none, 1=gaming color, 2=normal color (Color tab)
extern int  g_activeTab;         // 0=Display, 1=Color
extern std::map<HWND, std::set<std::string>> g_incompatRes;
extern int  currentHz;
extern char configFile[MAX_PATH];
extern int  originalWidth, originalHeight, originalHz;

// ---- Function declarations ----

// utils.cpp
void DebugLog(const char* format, ...);

// display.cpp
void EnumerateMonitors();
void GetCurrentDisplaySettings(int monitorIndex);
void PopulateResolutions(HWND hCombo, int monitorIndex);
int  GetMaxRefreshRate(int width, int height, int monitorIndex);
void ChangeRes(int width, int height, int monitorIndex);

// config.cpp
void SaveConfig();
void LoadConfig();

// color.cpp
void InitNVAPI();
void ShutdownNVAPI();
void ApplyColorSettings(int vibrance, int monIdx = -1);
void ResetColorSettings(int monIdx = -1);

// font.cpp
void EnsureJetBrainsMono();

// controls.cpp
void SubclassHotkey(HWND h);
void SubclassCombo(HWND h);
LRESULT CALLBACK HotkeySubclassProc(HWND h, UINT msg, WPARAM w, LPARAM l);
LRESULT CALLBACK ComboSubclassProc(HWND h, UINT msg, WPARAM w, LPARAM l);

// main.cpp
void InitTray(HWND hwnd);
void ApplyHotkeys(HWND hwnd);

// wndproc.cpp
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
