#include "globals.h"

// ---- Global variable definitions ----

NOTIFYICONDATA              nid;
HWND                        hEditW_Game, hEditH_Game, hEditW_Norm, hEditH_Norm;
HWND                        hComboMon;
HWND                        hHotkeyGame, hHotkeyNormal;
HWND                        hComboResGame, hComboResNormal, hLabelCurrentHz;
HWND                        hBtnRefreshRes = NULL;
std::vector<DISPLAY_DEVICE> monitors;
UINT                        gameHotkey  = VK_F7;
UINT                        normalHotkey = VK_F8;

HBRUSH hBrushDarkBg = NULL;
HBRUSH hBrushEditBg = NULL;
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

int  g_activePreset = 0;
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

// ---- Admin check ----

bool IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
}

// ---- Dark-mode dialog proc (used for any secondary dialogs) ----

LRESULT CALLBACK CustomDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkColor(hdc, RGB(32, 32, 32));
            return (INT_PTR)hBrushDarkBg;
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkColor(hdc, RGB(45, 45, 45));
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
