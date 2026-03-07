#include "globals.h"

// ---- Global variable definitions ----

NOTIFYICONDATA              nid;
HWND                        hComboMon;
HWND                        hHotkeyGame, hHotkeyNormal;
HWND                        hHotkeyColor1    = NULL;
HWND                        hHotkeyColor2    = NULL;
HWND                        hLblColorHotkey1 = NULL;
HWND                        hLblColorHotkey2 = NULL;
HWND                        hComboResGame, hComboResNormal, hLabelCurrentHz;
HWND                        hBtnRefreshRes = NULL;
HWND                        hBtnTabDisplay = NULL, hBtnTabColor = NULL;
HWND                        hEditGVibrance = NULL;
HWND                        hEditNVibrance = NULL;
HWND                        hLblGVibrance = NULL;
HWND                        hLblNVibrance = NULL;
HWND                        hP1Title       = NULL;
HWND                        hP2Title       = NULL;
HWND                        hLblResHotkey1 = NULL;
HWND                        hLblResHotkey2 = NULL;
std::vector<DISPLAY_DEVICE> monitors;
UINT                        gameHotkey  = VK_F7;
UINT                        normalHotkey = VK_F8;
UINT                        colorHotkey1 = VK_F11;
UINT                        colorHotkey2 = VK_F12;

int                         gameVibrance   = 50;
int                         normalVibrance   = 50;

HBRUSH hBrushBg     = NULL;
HBRUSH hBrushPanel  = NULL;
HBRUSH hBrushCtrl   = NULL;
HPEN   hPenAccent   = NULL;
HPEN   hPenBorder   = NULL;
HPEN   hPenSep      = NULL;
HFONT  hFont        = NULL;
HFONT  hTitleFont   = NULL;
HFONT  hSmallFont   = NULL;
HFONT  hLargeFont   = NULL;
HFONT  hPresetFont  = NULL;
HFONT  hSmallULFont = NULL;

// Pre-cached draw objects
HBRUSH hBrushAccent    = NULL;
HBRUSH hBrushAccent2   = NULL;
HBRUSH hBrushGreen     = NULL;
HBRUSH hBrushHotlight  = NULL;
HBRUSH hBrushText2     = NULL;
HPEN   hPenGreen       = NULL;
HPEN   hPenAccent3     = NULL;
HPEN   hPenAccent2Thin = NULL;
HPEN   hPenText2       = NULL;

// Resolution string cache
char   g_savedRes1[64] = {};
char   g_savedRes2[64] = {};

HWND hTitleLabel    = NULL;
HWND hSubtitleLabel = NULL;
HWND hLblRes1       = NULL;   // "Resolution" label - Preset 1 card (Display tab)
HWND hLblRes2       = NULL;   // "Resolution" label - Preset 2 card (Display tab)

int  g_activeResPreset   = 0;
int  g_activeColorPreset = 0;
int  g_activeTab         = 0;
std::map<HWND, std::set<std::string>> g_incompatRes;
int  currentHz      = 0;
char configFile[MAX_PATH];
int  originalWidth  = 0;
int  originalHeight = 0;
int  originalHz     = 0;

// ---- Debug logging ----

void DebugLog(const char* format, ...) {
    if (!DEBUG_MODE) return;
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
    printf("%s\n", buffer);
}


