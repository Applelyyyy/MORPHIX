#include "globals.h"

static void UpdateTabVisibility() {
    int showDisplay = (g_activeTab == 0) ? SW_SHOW : SW_HIDE;
    int showColor   = (g_activeTab == 1) ? SW_SHOW : SW_HIDE;

    // Display tab controls
    ShowWindow(hComboResGame, showDisplay);
    ShowWindow(hComboResNormal, showDisplay);
    ShowWindow(hLblRes1, showDisplay);
    ShowWindow(hLblRes2, showDisplay);
    ShowWindow(hHotkeyGame,   showDisplay);
    ShowWindow(hHotkeyNormal, showDisplay);
    ShowWindow(hLblResHotkey1, showDisplay);
    ShowWindow(hLblResHotkey2, showDisplay);

    // Color tab controls (vibrance + hotkeys only)
    ShowWindow(hLblGVibrance,    showColor);
    ShowWindow(hEditGVibrance,   showColor);
    ShowWindow(hLblNVibrance,    showColor);
    ShowWindow(hEditNVibrance,   showColor);
    ShowWindow(hLblColorHotkey1, showColor);
    ShowWindow(hLblColorHotkey2, showColor);
    ShowWindow(hHotkeyColor1,    showColor);
    ShowWindow(hHotkeyColor2,    showColor);

    // Rename card titles based on active tab
    SetWindowTextA(hP1Title, g_activeTab == 0 ? "Resolution 1" : "COLOR 1");
    SetWindowTextA(hP2Title, g_activeTab == 0 ? "Resolution 2" : "COLOR 2");
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {

    // =========================================================
    // WM_CREATE — build all controls, GDI objects, register hotkeys
    // =========================================================
    case WM_CREATE:
    {
        DebugLog("Creating window and controls");

        // --- Fonts ---
        hLargeFont   = CreateFontA(-26, 0, 0, 0, FW_BOLD,     FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        hTitleFont   = CreateFontA(-14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        hFont        = CreateFontA(-14, 0, 0, 0, FW_NORMAL,   FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        hSmallFont   = CreateFontA(-11, 0, 0, 0, FW_NORMAL,   FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        hPresetFont  = CreateFontA(-14, 0, 0, 0, FW_BOLD,     FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH  | FF_DONTCARE, "JetBrains Mono");
        if (!hPresetFont)
            hPresetFont = CreateFontA(-14, 0, 0, 0, FW_BOLD,  FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_DONTCARE, "Consolas");
        hSmallULFont = CreateFontA(-11, 0, 0, 0, FW_NORMAL,   FALSE, TRUE,  FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        // --- Brushes & Pens ---
        hBrushBg     = CreateSolidBrush(COL_BG);
        hBrushPanel  = CreateSolidBrush(COL_PANEL);
        hBrushCtrl   = CreateSolidBrush(COL_CTRL);
        hPenAccent   = CreatePen(PS_SOLID, 2, COL_ACCENT2);
        hPenBorder   = CreatePen(PS_SOLID, 1, COL_BORDER2);
        hPenSep      = CreatePen(PS_SOLID, 1, COL_BORDER);

        // Pre-cached draw objects — created once, reused in all hot paint/draw paths
        hBrushAccent    = CreateSolidBrush(COL_ACCENT);
        hBrushAccent2   = CreateSolidBrush(COL_ACCENT2);
        hBrushGreen     = CreateSolidBrush(COL_GREEN);
        hBrushHotlight  = CreateSolidBrush(RGB(38, 38, 52));
        hBrushText2     = CreateSolidBrush(COL_TEXT2);
        hPenGreen       = CreatePen(PS_SOLID, 2, COL_GREEN);
        hPenAccent3     = CreatePen(PS_SOLID, 1, COL_ACCENT3);
        hPenAccent2Thin = CreatePen(PS_SOLID, 1, COL_ACCENT2);
        hPenText2       = CreatePen(PS_SOLID, 1, COL_TEXT2);

        // ==========================================
        // Header bar  (y = 0..56)
        // ==========================================
        hTitleLabel = CreateWindowA("STATIC", "MORPHIX",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            18, 8, 280, 32, hwnd, NULL, NULL, NULL);
        SendMessage(hTitleLabel, WM_SETFONT, (WPARAM)hLargeFont, TRUE);
        NoTheme(hTitleLabel);

        hSubtitleLabel = CreateWindowA("STATIC", "Resolution",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            18, 38, 260, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hSubtitleLabel, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hSubtitleLabel);

        // ==========================================
        // Sidebar tabs  (x = 0..120, y = 56..H)
        // ==========================================
        hBtnTabDisplay = CreateWindowA("BUTTON", "Display",
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            0, 64, 120, 48, hwnd, (HMENU)IDC_BTN_TAB_DISPLAY, NULL, NULL);
        hBtnTabColor = CreateWindowA("BUTTON", "Color",
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            0, 112, 120, 48, hwnd, (HMENU)IDC_BTN_TAB_COLOR, NULL, NULL);

        // ==========================================
        // Monitor section  (x=132.., y = 68..124)
        // ==========================================
        HWND hMonLabel = CreateWindowA("STATIC", "DISPLAY",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            140, 74, 100, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hMonLabel, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hMonLabel);

        hComboMon = CreateWindowA("COMBOBOX", "",
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP,
            140, 90, 366, 160, hwnd, (HMENU)IDC_COMBO_MONITOR, NULL, NULL);
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
            514, 90, 232, 28, hwnd, (HMENU)IDC_LABEL_CURRENT_HZ, NULL, NULL);
        SendMessage(hLabelCurrentHz, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
        NoTheme(hLabelCurrentHz);
        GetCurrentDisplaySettings(0);

        // ==========================================
        // PRESET 1 card  (x=132..450, y=130..304)
        // ==========================================
        hP1Title = CreateWindowA("STATIC", "Resolution 1",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            148, 146, 200, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hP1Title, WM_SETFONT, (WPARAM)hPresetFont, TRUE);
        NoTheme(hP1Title);

        hLblRes1 = CreateWindowA("STATIC", "Resolution",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            148, 178, 130, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hLblRes1, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hLblRes1);

        hComboResNormal = CreateWindowA("COMBOBOX", "",
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP,
            148, 194, 286, 400, hwnd, (HMENU)IDC_COMBO_RES_NORMAL, NULL, NULL);
        SendMessage(hComboResNormal, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hComboResNormal);
        SubclassCombo(hComboResNormal);
        PopulateResolutions(hComboResNormal, 0);

        // Vibrance control (Color tab, card 1)
        hLblNVibrance = CreateWindowA("STATIC", "Vibrance", WS_CHILD | SS_LEFT, 148, 178, 60, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hLblNVibrance, WM_SETFONT, (WPARAM)hSmallFont, TRUE); NoTheme(hLblNVibrance);
        hEditNVibrance = CreateWindowA("EDIT", "50", WS_CHILD | WS_BORDER | ES_NUMBER, 148, 194, 286, 24, hwnd, (HMENU)IDC_EDIT_N_VIBRANCE, NULL, NULL);
        SendMessage(hEditNVibrance, WM_SETFONT, (WPARAM)hFont, TRUE); NoTheme(hEditNVibrance);

        hLblResHotkey1 = CreateWindowA("STATIC", "Res Hotkey",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            148, 234, 80, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hLblResHotkey1, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hLblResHotkey1);

        hHotkeyNormal = CreateWindowA(HOTKEY_CLASSA, "",
            WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
            148, 250, 155, 28, hwnd, (HMENU)IDC_HOTKEY_NORMAL, NULL, NULL);
        SendMessage(hHotkeyNormal, HKM_SETHOTKEY, MAKEWORD(VK_F8, 0), 0);
        SendMessage(hHotkeyNormal, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hHotkeyNormal);
        SubclassHotkey(hHotkeyNormal);

        hLblColorHotkey1 = CreateWindowA("STATIC", "Color Hotkey",
            WS_CHILD | SS_LEFT,
            148, 234, 80, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hLblColorHotkey1, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hLblColorHotkey1);

        hHotkeyColor1 = CreateWindowA(HOTKEY_CLASSA, "",
            WS_CHILD | WS_BORDER | WS_TABSTOP,
            148, 250, 155, 28, hwnd, (HMENU)IDC_HOTKEY_COLOR1, NULL, NULL);
        SendMessage(hHotkeyColor1, HKM_SETHOTKEY, MAKEWORD(VK_F11, 0), 0);
        SendMessage(hHotkeyColor1, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hHotkeyColor1);
        SubclassHotkey(hHotkeyColor1);

        // ==========================================
        // PRESET 2 card  (x=458..W-12, y=130..304)
        // ==========================================
        hP2Title = CreateWindowA("STATIC", "Resolution 2",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            476, 146, 200, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hP2Title, WM_SETFONT, (WPARAM)hPresetFont, TRUE);
        NoTheme(hP2Title);

        hLblRes2 = CreateWindowA("STATIC", "Resolution",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            476, 178, 130, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hLblRes2, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hLblRes2);

        hComboResGame = CreateWindowA("COMBOBOX", "",
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP,
            476, 194, 286, 400, hwnd, (HMENU)IDC_COMBO_RES_GAME, NULL, NULL);
        SendMessage(hComboResGame, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hComboResGame);
        SubclassCombo(hComboResGame);
        PopulateResolutions(hComboResGame, 0);

        // Vibrance control (Color tab, card 2)
        hLblGVibrance = CreateWindowA("STATIC", "Vibrance", WS_CHILD | SS_LEFT, 476, 178, 60, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hLblGVibrance, WM_SETFONT, (WPARAM)hSmallFont, TRUE); NoTheme(hLblGVibrance);
        hEditGVibrance = CreateWindowA("EDIT", "50", WS_CHILD | WS_BORDER | ES_NUMBER, 476, 194, 286, 24, hwnd, (HMENU)IDC_EDIT_G_VIBRANCE, NULL, NULL);
        SendMessage(hEditGVibrance, WM_SETFONT, (WPARAM)hFont, TRUE); NoTheme(hEditGVibrance);

        hLblResHotkey2 = CreateWindowA("STATIC", "Res Hotkey",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            476, 234, 80, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hLblResHotkey2, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hLblResHotkey2);

        hHotkeyGame = CreateWindowA(HOTKEY_CLASSA, "",
            WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
            476, 250, 155, 28, hwnd, (HMENU)IDC_HOTKEY_GAME, NULL, NULL);
        SendMessage(hHotkeyGame, HKM_SETHOTKEY, MAKEWORD(VK_F7, 0), 0);
        SendMessage(hHotkeyGame, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hHotkeyGame);
        SubclassHotkey(hHotkeyGame);

        hLblColorHotkey2 = CreateWindowA("STATIC", "Color Hotkey",
            WS_CHILD | SS_LEFT,
            476, 234, 80, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hLblColorHotkey2, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hLblColorHotkey2);

        hHotkeyColor2 = CreateWindowA(HOTKEY_CLASSA, "",
            WS_CHILD | WS_BORDER | WS_TABSTOP,
            476, 250, 155, 28, hwnd, (HMENU)IDC_HOTKEY_COLOR2, NULL, NULL);
        SendMessage(hHotkeyColor2, HKM_SETHOTKEY, MAKEWORD(VK_F12, 0), 0);
        SendMessage(hHotkeyColor2, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hHotkeyColor2);
        SubclassHotkey(hHotkeyColor2);

        // ==========================================
        // Footer  (y = 310..408)
        // ==========================================
        HWND hInfo1 = CreateWindowA("STATIC", "Auto-selects the highest available refresh rate",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            134, 318, 636, 16, hwnd, NULL, NULL, NULL);
        SendMessage(hInfo1, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hInfo1);

        HWND hInfo2 = CreateWindowA("STATIC", "Reset to default:  Ctrl + Alt + Shift + R",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            134, 338, 636, 16, hwnd, NULL, NULL, NULL);
        SendMessage(hInfo2, WM_SETFONT, (WPARAM)hSmallULFont, TRUE);
        NoTheme(hInfo2);

        HWND hBtnSave = CreateWindowA("BUTTON", "Save && Apply",
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
            332, 364, 240, 40, hwnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);
        SendMessage(hBtnSave, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
        NoTheme(hBtnSave);

        // Small "Refresh Res" button — bottom right
        hBtnRefreshRes = CreateWindowA("BUTTON", "",
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
            610, 370, 120, 28, hwnd, (HMENU)IDC_BTN_REFRESH_RES, NULL, NULL);
        NoTheme(hBtnRefreshRes);

        // Tooltip for the refresh button
        HWND hTip = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, NULL,
            WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hwnd, NULL, NULL, NULL);
        SendMessageA(hTip, TTM_SETMAXTIPWIDTH, 0, 280);
        TOOLINFOA ti = {};
        ti.cbSize   = sizeof(TOOLINFOA);
        ti.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
        ti.hwnd     = hwnd;
        ti.uId      = (UINT_PTR)hBtnRefreshRes;
        ti.lpszText = (LPSTR)"Re-scan the display driver and refresh\nboth resolution dropdowns.";
        SendMessageA(hTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);

        RegisterHotKey(hwnd, HOTKEY_GAME_MODE,   MOD_NOREPEAT, VK_F7);
        RegisterHotKey(hwnd, HOTKEY_NORMAL_MODE,  MOD_NOREPEAT, VK_F8);
        RegisterHotKey(hwnd, HOTKEY_COLOR1,        MOD_NOREPEAT, VK_F11);
        RegisterHotKey(hwnd, HOTKEY_COLOR2,        MOD_NOREPEAT, VK_F12);
        RegisterHotKey(hwnd, HOTKEY_RESET, MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'R');

        InitTray(hwnd);
        LoadConfig();
        UpdateTabVisibility();

        // Apply saved color settings immediately on startup so the display
        // reflects the stored vibrance without needing a manual Save & Apply.
        {
            int monIdx = (int)SendMessage(hComboMon, CB_GETCURSEL, 0, 0);
            if (monIdx == CB_ERR) monIdx = 0;
            ApplyHotkeys(hwnd);
            if (g_activeColorPreset == 1)
                ApplyColorSettings(gameVibrance, monIdx);
            else
                ApplyColorSettings(normalVibrance, monIdx);
        }
        break;
    }

    // =========================================================
    // WM_ERASEBKGND — suppress default erase; WM_PAINT handles everything
    // =========================================================
    case WM_ERASEBKGND:
        return 1;

    // =========================================================
    // WM_PAINT — draw background, header, monitor card, preset cards
    // =========================================================
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int W = rc.right;

        // Full background
        FillRect(hdc, &rc, hBrushBg);

        // Sidebar background
        RECT rcSide = {0, 64, 120, rc.bottom};
        FillRect(hdc, &rcSide, hBrushPanel);

        // Header bar (y 0..64)
        RECT rcHdr = {0, 0, W, 64};
        FillRect(hdc, &rcHdr, hBrushPanel);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPenAccent);
        MoveToEx(hdc, 0, 63, NULL); LineTo(hdc, W, 63);

        // Monitor card (x 132..W-12, y 68..124)
        RECT rcMon = {132, 68, W - 12, 124};
        FillRect(hdc, &rcMon, hBrushPanel);
        RECT   rcAccL = {132, 68, 135, 124};
        FillRect(hdc, &rcAccL, hBrushAccent);
        SelectObject(hdc, hPenBorder);
        HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rcMon.left, rcMon.top, rcMon.right, rcMon.bottom);

        // Thin separator between monitor card and preset cards
        SelectObject(hdc, hPenSep);
        MoveToEx(hdc, 132, 130, NULL); LineTo(hdc, W - 12, 130);

        // PRESET 1 card (x 132..450, y 132..304)
        // active state depends on which tab is shown
        int activeCard = (g_activeTab == 0) ? g_activeResPreset : g_activeColorPreset;

        RECT rcC1 = {132, 132, 450, 304};
        FillRect(hdc, &rcC1, hBrushPanel);
        if (activeCard == 2) {
            // Full green border: top, bottom, left, right strips (3 px each)
            RECT rcT1 = {132, 132, 450, 135}; FillRect(hdc, &rcT1, hBrushGreen);
            RECT rcB1 = {132, 301, 450, 304}; FillRect(hdc, &rcB1, hBrushGreen);
            RECT rcL1 = {132, 132, 135, 304}; FillRect(hdc, &rcL1, hBrushGreen);
            RECT rcR1 = {447, 132, 450, 304}; FillRect(hdc, &rcR1, hBrushGreen);
        } else {
            SelectObject(hdc, hPenBorder);
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rcC1.left, rcC1.top, rcC1.right, rcC1.bottom);
            // Accent top strip drawn AFTER Rectangle so it fully overwrites the top border line
            RECT rcAt1 = {132, 132, 450, 135};
            FillRect(hdc, &rcAt1, hBrushAccent);
        }

        // PRESET 2 card (x 458..W-12, y 132..304)
        RECT rcC2 = {458, 132, W - 12, 304};
        FillRect(hdc, &rcC2, hBrushPanel);
        if (activeCard == 1) {
            RECT rcT2 = {458, 132, W-12, 135}; FillRect(hdc, &rcT2, hBrushGreen);
            RECT rcB2 = {458, 301, W-12, 304}; FillRect(hdc, &rcB2, hBrushGreen);
            RECT rcL2 = {458, 132, 461, 304}; FillRect(hdc, &rcL2, hBrushGreen);
            RECT rcR2 = {W-15, 132, W-12, 304}; FillRect(hdc, &rcR2, hBrushGreen);
        } else {
            SelectObject(hdc, hPenBorder);
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rcC2.left, rcC2.top, rcC2.right, rcC2.bottom);
            // Accent top strip drawn AFTER Rectangle so it fully overwrites the top border line
            RECT rcAt2 = {458, 132, W-12, 135};
            FillRect(hdc, &rcAt2, hBrushAccent);
        }

        // Bottom separator above footer
        SelectObject(hdc, hPenSep);
        MoveToEx(hdc, 132, 310, NULL); LineTo(hdc, W - 12, 310);

        // Draw vertical separator for sidebar
        MoveToEx(hdc, 120, 64, NULL); LineTo(hdc, 120, rc.bottom);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBr);
        EndPaint(hwnd, &ps);
        return 0;
    }

    // =========================================================
    // WM_CTLCOLOR* — colour static/edit/listbox/button controls
    // =========================================================
    case WM_CTLCOLORSTATIC:
    {
        HDC  hdc  = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        SetBkMode(hdc, TRANSPARENT);

        if (hCtrl == hTitleLabel)    { SetTextColor(hdc, COL_TEXT);    return (INT_PTR)hBrushPanel; }
        if (hCtrl == hSubtitleLabel) { SetTextColor(hdc, COL_ACCENT3); return (INT_PTR)hBrushPanel; }
        if (hCtrl == hLabelCurrentHz){ SetTextColor(hdc, COL_GREEN);   return (INT_PTR)hBrushPanel; }

        RECT rcCtrl;
        GetWindowRect(hCtrl, &rcCtrl);
        MapWindowPoints(HWND_DESKTOP, hwnd, (POINT*)&rcCtrl, 2);

        if (rcCtrl.top < 306) { SetTextColor(hdc, COL_TEXT2); return (INT_PTR)hBrushPanel; }
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

    // =========================================================
    // WM_MEASUREITEM — set combobox row height
    // =========================================================
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT* mis = (MEASUREITEMSTRUCT*)lParam;
        if (mis->CtlType == ODT_COMBOBOX) { mis->itemHeight = 22; return TRUE; }
        break;
    }

    // =========================================================
    // WM_DRAWITEM — owner-draw comboboxes and buttons
    // =========================================================
    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;

        // Combobox items
        if (dis->CtlType == ODT_COMBOBOX && dis->CtlID != IDC_BTN_SAVE) {
            HDC  hdc       = dis->hDC;
            RECT rc        = dis->rcItem;
            bool isEditArea = (dis->itemState & ODS_COMBOBOXEDIT) != 0;
            bool selected   = (dis->itemState & ODS_SELECTED)     != 0 && !isEditArea;

            FillRect(hdc, &rc, selected ? hBrushAccent : hBrushCtrl);

            if (dis->itemID != (UINT)-1) {
                char text[256] = {0};
                SendMessageA(dis->hwndItem, CB_GETLBTEXT, dis->itemID, (LPARAM)text);
                bool incompat = g_incompatRes.count(dis->hwndItem) &&
                                g_incompatRes[dis->hwndItem].count(std::string(text)) > 0;
                COLORREF textCol = selected    ? RGB(255, 255, 255)
                                 : incompat    ? RGB(220, 80,  80)
                                               : COL_GREEN;
                SetTextColor(hdc, textCol);
                SetBkMode(hdc, TRANSPARENT);
                HFONT hf    = (HFONT)SendMessage(dis->hwndItem, WM_GETFONT, 0, 0);
                HFONT hfOld = hf ? (HFONT)SelectObject(hdc, hf) : NULL;
                RECT  rcT   = rc; rcT.left += 6;
                DrawTextA(hdc, text, -1, &rcT, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                if (hfOld) SelectObject(hdc, hfOld);
            }
            if (dis->itemState & ODS_FOCUS) DrawFocusRect(hdc, &rc);
            return TRUE;
        }

        // Save & Apply button
        if (dis->CtlID == IDC_BTN_SAVE) {
            HDC  hdc    = dis->hDC;
            RECT rc     = dis->rcItem;
            bool pressed = (dis->itemState & ODS_SELECTED) != 0;

            FillRect(hdc, &rc, pressed ? hBrushAccent : hBrushAccent2);

            HPEN hBPO = (HPEN)SelectObject(hdc, hPenAccent3);
            HBRUSH hBBO = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
            SelectObject(hdc, hBPO); SelectObject(hdc, hBBO);

            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, hTitleFont);
            DrawTextA(hdc, "Save && Apply", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            if (dis->itemState & ODS_FOCUS) { InflateRect(&rc, -3, -3); DrawFocusRect(hdc, &rc); }
            return TRUE;
        }

        // Sidebar Tabs
        if (dis->CtlID == IDC_BTN_TAB_DISPLAY || dis->CtlID == IDC_BTN_TAB_COLOR) {
            HDC  hdc      = dis->hDC;
            RECT rc       = dis->rcItem;
            bool hotlight = (dis->itemState & ODS_HOTLIGHT) != 0;
            bool active   = (dis->CtlID == IDC_BTN_TAB_DISPLAY && g_activeTab == 0) ||
                            (dis->CtlID == IDC_BTN_TAB_COLOR   && g_activeTab == 1);

            FillRect(hdc, &rc, active ? hBrushCtrl : hotlight ? hBrushHotlight : hBrushPanel);

            if (active) {
                RECT rcActiveInd = {0, 0, 4, rc.bottom};
                FillRect(hdc, &rcActiveInd, hBrushAccent);
            }

            SetTextColor(hdc, active ? COL_TEXT : COL_TEXT2);
            SetBkMode(hdc, TRANSPARENT);
            HFONT hfOld = (HFONT)SelectObject(hdc, hFont);
            const char* txt = dis->CtlID == IDC_BTN_TAB_DISPLAY ? "Display" : "Color";
            rc.left += 20; // Padding
            DrawTextA(hdc, txt, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, hfOld);
            return TRUE;
        }

        // Refresh Res ghost button
        if (dis->CtlID == IDC_BTN_REFRESH_RES) {
            HDC  hdc      = dis->hDC;
            RECT rc       = dis->rcItem;
            bool pressed  = (dis->itemState & ODS_SELECTED) != 0;
            bool hotlight = (dis->itemState & ODS_HOTLIGHT) != 0;

            FillRect(hdc, &rc, pressed  ? hBrushCtrl
                             : hotlight ? hBrushHotlight
                                        : hBrushPanel);

            HPEN hBPO = (HPEN)SelectObject(hdc, hotlight ? hPenAccent2Thin : hPenBorder);
            HBRUSH hBBO = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
            SelectObject(hdc, hBPO); SelectObject(hdc, hBBO);

            SetTextColor(hdc, hotlight ? COL_TEXT : COL_TEXT2);
            SetBkMode(hdc, TRANSPARENT);
            HFONT hfOld = (HFONT)SelectObject(hdc, hSmallFont);
            DrawTextA(hdc, "Refresh Res", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, hfOld);
            return TRUE;
        }
        break;
    }

    // =========================================================
    // WM_COMMAND — button clicks and combo selection changes
    // =========================================================
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_TAB_DISPLAY && g_activeTab != 0) {
            g_activeTab = 0;
            UpdateTabVisibility();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        if (LOWORD(wParam) == IDC_BTN_TAB_COLOR && g_activeTab != 1) {
            g_activeTab = 1;
            UpdateTabVisibility();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        if (LOWORD(wParam) == IDC_BTN_REFRESH_RES) {
            int idx = SendMessage(hComboMon, CB_GETCURSEL, 0, 0);
            if (idx == CB_ERR) idx = 0;
            PopulateResolutions(hComboResGame,   idx);
            PopulateResolutions(hComboResNormal,  idx);
            GetCurrentDisplaySettings(idx);
            // Restore saved selections from in-memory cache (no disk access)
            if (g_savedRes1[0]) {
                int n = SendMessage(hComboResGame, CB_GETCOUNT, 0, 0);
                for (int i = 0; i < n; i++) {
                    char item[64]; SendMessage(hComboResGame, CB_GETLBTEXT, i, (LPARAM)item);
                    if (strcmp(item, g_savedRes1) == 0) { SendMessage(hComboResGame, CB_SETCURSEL, i, 0); break; }
                }
            }
            if (g_savedRes2[0]) {
                int n = SendMessage(hComboResNormal, CB_GETCOUNT, 0, 0);
                for (int i = 0; i < n; i++) {
                    char item[64]; SendMessage(hComboResNormal, CB_GETLBTEXT, i, (LPARAM)item);
                    if (strcmp(item, g_savedRes2) == 0) { SendMessage(hComboResNormal, CB_SETCURSEL, i, 0); break; }
                }
            }
        }
        if (LOWORD(wParam) == IDC_BTN_SAVE) {
            ApplyHotkeys(hwnd);
            SaveConfig();
            // Immediately apply color settings so the user sees the effect right away
            int monIdx = (int)SendMessage(hComboMon, CB_GETCURSEL, 0, 0);
            if (monIdx == CB_ERR) monIdx = 0;
            if (g_activeColorPreset == 1)
                ApplyColorSettings(gameVibrance, monIdx);
            else
                ApplyColorSettings(normalVibrance, monIdx);
            DebugLog("USER ACTION: Applied hotkeys, saved config, applied colors");
            MessageBoxA(hwnd, "Configuration saved and applied!", "MORPHIX", MB_ICONINFORMATION);
        }
        // Warn when an incompatible resolution is selected
        if ((LOWORD(wParam) == IDC_COMBO_RES_GAME || LOWORD(wParam) == IDC_COMBO_RES_NORMAL)
            && HIWORD(wParam) == CBN_SELCHANGE) {
            HWND hSel   = (HWND)lParam;
            int  selIdx = SendMessage(hSel, CB_GETCURSEL, 0, 0);
            if (selIdx != CB_ERR) {
                char selText[64] = {0};
                SendMessageA(hSel, CB_GETLBTEXT, selIdx, (LPARAM)selText);
                if (g_incompatRes.count(hSel) &&
                    g_incompatRes[hSel].count(std::string(selText)) > 0) {
                    MessageBoxA(hwnd,
                        "This resolution was reported incompatible by your display driver.\n"
                        "Applying it may result in a blank screen or display error.\n\n"
                        "Proceed with caution.",
                        "MORPHIX  \xe2\x80\x94  Incompatible Resolution",
                        MB_ICONWARNING | MB_OK);
                }
            }
        }
        if (LOWORD(wParam) == IDC_COMBO_MONITOR && HIWORD(wParam) == CBN_SELCHANGE) {
            int idx = SendMessage(hComboMon, CB_GETCURSEL, 0, 0);
            PopulateResolutions(hComboResGame,  idx);
            PopulateResolutions(hComboResNormal, idx);
            GetCurrentDisplaySettings(idx);
        }
        break;

    // =========================================================
    // WM_HOTKEY — global hotkeys for preset switching and reset
    // =========================================================
    case WM_HOTKEY:
    {
        DebugLog("Hotkey pressed: ID=%d", wParam);
        char buf[64];
        int  w = 0, h = 0;
        int  monIdx = SendMessage(hComboMon, CB_GETCURSEL, 0, 0);

        if (wParam == HOTKEY_GAME_MODE) {
            int idx = SendMessage(hComboResGame, CB_GETCURSEL, 0, 0);
            SendMessage(hComboResGame, CB_GETLBTEXT, idx, (LPARAM)buf);
            sscanf(buf, "%dx%d", &w, &h);
            g_activeResPreset = 1;
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (wParam == HOTKEY_NORMAL_MODE) {
            int idx = SendMessage(hComboResNormal, CB_GETCURSEL, 0, 0);
            SendMessage(hComboResNormal, CB_GETLBTEXT, idx, (LPARAM)buf);
            sscanf(buf, "%dx%d", &w, &h);
            g_activeResPreset = 2;
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (wParam == HOTKEY_RESET) {
            if (originalWidth > 0 && originalHeight > 0) {
                ChangeRes(originalWidth, originalHeight, monIdx);
                g_activeResPreset   = 0;
                g_activeColorPreset = 0;
                ResetColorSettings(monIdx);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        } else if (wParam == HOTKEY_COLOR1) {
            g_activeColorPreset = 2;
            ApplyColorSettings(normalVibrance, monIdx);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (wParam == HOTKEY_COLOR2) {
            g_activeColorPreset = 1;
            ApplyColorSettings(gameVibrance, monIdx);
            InvalidateRect(hwnd, NULL, FALSE);
        }

        if (w > 0 && h > 0) ChangeRes(w, h, monIdx);
        break;
    }

    // =========================================================
    // WM_TRAYICON — right-click context menu, double-click show
    // =========================================================
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            POINT curPoint;
            GetCursorPos(&curPoint);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuA(hMenu, MF_STRING,   ID_TRAY_SHOW,  "Show Settings");
            AppendMenuA(hMenu, MF_STRING,   ID_TRAY_RESET, "Reset to Default");
            AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuA(hMenu, MF_STRING,   ID_TRAY_EXIT,  "Exit");
            SetForegroundWindow(hwnd);
            int sel = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                                    curPoint.x, curPoint.y, 0, hwnd, NULL);
            if (sel == ID_TRAY_EXIT)  PostQuitMessage(0);
            if (sel == ID_TRAY_SHOW)  { ShowWindow(hwnd, SW_SHOW); SetForegroundWindow(hwnd); }
            if (sel == ID_TRAY_RESET) {
                int monIdx = SendMessage(hComboMon, CB_GETCURSEL, 0, 0);
                if (originalWidth > 0 && originalHeight > 0) {
                    ChangeRes(originalWidth, originalHeight, monIdx);
                    g_activeResPreset   = 0;
                    g_activeColorPreset = 0;
                    ResetColorSettings(monIdx);
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            DestroyMenu(hMenu);
        } else if (lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hwnd, SW_SHOW);
        }
        break;

    // =========================================================
    // WM_CLOSE — save config, hide to tray (don't destroy)
    // =========================================================
    case WM_CLOSE:
        SaveConfig();
        UnregisterHotKey(hwnd, HOTKEY_RESET);
        UnregisterHotKey(hwnd, HOTKEY_COLOR1);
        UnregisterHotKey(hwnd, HOTKEY_COLOR2);
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    // =========================================================
    // WM_DESTROY — release GDI objects, quit
    // =========================================================
    case WM_DESTROY:
        DebugLog("Destroying window and cleaning up");
        Shell_NotifyIcon(NIM_DELETE, &nid);
        ShutdownNVAPI();
        UnregisterHotKey(hwnd, HOTKEY_GAME_MODE);
        UnregisterHotKey(hwnd, HOTKEY_NORMAL_MODE);
        UnregisterHotKey(hwnd, HOTKEY_RESET);
        UnregisterHotKey(hwnd, HOTKEY_COLOR1);
        UnregisterHotKey(hwnd, HOTKEY_COLOR2);
        if (hFont)       DeleteObject(hFont);
        if (hTitleFont)  DeleteObject(hTitleFont);
        if (hSmallFont)  DeleteObject(hSmallFont);
        if (hLargeFont)  DeleteObject(hLargeFont);
        if (hPresetFont) DeleteObject(hPresetFont);
        if (hSmallULFont)DeleteObject(hSmallULFont);
        if (hBrushBg)    DeleteObject(hBrushBg);
        if (hBrushPanel) DeleteObject(hBrushPanel);
        if (hBrushCtrl)  DeleteObject(hBrushCtrl);
        if (hPenAccent)  DeleteObject(hPenAccent);
        if (hPenBorder)  DeleteObject(hPenBorder);
        if (hPenSep)     DeleteObject(hPenSep);
        if (hBrushAccent)    DeleteObject(hBrushAccent);
        if (hBrushAccent2)   DeleteObject(hBrushAccent2);
        if (hBrushGreen)     DeleteObject(hBrushGreen);
        if (hBrushHotlight)  DeleteObject(hBrushHotlight);
        if (hBrushText2)     DeleteObject(hBrushText2);
        if (hPenGreen)       DeleteObject(hPenGreen);
        if (hPenAccent3)     DeleteObject(hPenAccent3);
        if (hPenAccent2Thin) DeleteObject(hPenAccent2Thin);
        if (hPenText2)       DeleteObject(hPenText2);
        if (DEBUG_MODE) FreeConsole();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
