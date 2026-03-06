// main.cpp -- entry point, tray initialisation, hotkey apply
// All other logic lives in the following modules:
//   utils.cpp    -- globals, DebugLog, IsRunAsAdmin, CustomDialogProc
//   display.cpp  -- EnumerateMonitors, PopulateResolutions, ChangeRes
//   config.cpp   -- SaveConfig, LoadConfig
//   font.cpp     -- EnsureJetBrainsMono
//   controls.cpp -- SubclassHotkey, SubclassCombo, subclass procs
//   wndproc.cpp  -- WindowProc (all WM_ handlers)
#include "globals.h"

// ---- System tray setup ----

void InitTray(HWND hwnd) {
    DebugLog("Initializing system tray");
    nid.cbSize          = sizeof(NOTIFYICONDATA);
    nid.hWnd            = hwnd;
    nid.uID             = ID_TRAY_APP_ICON;
    nid.uFlags          = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(100));
    if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);
    nid.hIcon = hIcon;
    lstrcpyA((LPSTR)nid.szTip, "MORPHIX - Resolution Changer");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

// ---- Register the current hotkey values with the OS ----

void ApplyHotkeys(HWND hwnd) {
    DebugLog("Applying new hotkeys");
    UnregisterHotKey(hwnd, HOTKEY_GAME_MODE);
    UnregisterHotKey(hwnd, HOTKEY_NORMAL_MODE);

    WORD modGame   = LOWORD(SendMessage(hHotkeyGame,   HKM_GETHOTKEY, 0, 0));
    WORD modNormal = LOWORD(SendMessage(hHotkeyNormal, HKM_GETHOTKEY, 0, 0));

    if (modGame   > 0) gameHotkey   = modGame;
    if (modNormal > 0) normalHotkey = modNormal;

    RegisterHotKey(hwnd, HOTKEY_GAME_MODE,   MOD_NOREPEAT, gameHotkey);
    RegisterHotKey(hwnd, HOTKEY_NORMAL_MODE, MOD_NOREPEAT, normalHotkey);
    DebugLog("Hotkeys applied: Game=0x%X, Normal=0x%X", gameHotkey, normalHotkey);
}

// ---- Application entry point ----

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    EnsureJetBrainsMono();

    if (DEBUG_MODE) {
        AllocConsole();
        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        SetConsoleTitleA("MORPHIX Debug Console");
    }

    DebugLog("MORPHIX starting up");

    // Build path to config.ini next to the exe
    GetModuleFileNameA(NULL, configFile, MAX_PATH);
    char* lastSlash = strrchr(configFile, '\\');
    if (lastSlash) strcpy(lastSlash + 1, "config.ini");

    // Common controls (needed for the HotKey control class)
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_HOTKEY_CLASS };
    InitCommonControlsEx(&icex);

    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100));
    if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);

    WNDCLASSA wc    = {};
    wc.lpfnWndProc  = WindowProc;
    wc.hInstance    = hInstance;
    wc.lpszClassName = "ACRClass";
    wc.hbrBackground = CreateSolidBrush(RGB(32, 32, 32));
    wc.hIcon        = hIcon;
    wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA("ACRClass", "MORPHIX",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 666, 448,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, SW_HIDE);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
