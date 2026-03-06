#include "globals.h"

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
        hBrushDarkBg = hBrushBg;
        hBrushEditBg = hBrushCtrl;
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

        hSubtitleLabel = CreateWindowA("STATIC", "Resolution Manager",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            21, 38, 260, 14, hwnd, NULL, NULL, NULL);
        SendMessage(hSubtitleLabel, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hSubtitleLabel);

        // ==========================================
        // Monitor section  (y = 68..124)
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

        hHotkeyGame = CreateWindowA(HOTKEY_CLASSA, "",
            WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
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

        hHotkeyNormal = CreateWindowA(HOTKEY_CLASSA, "",
            WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
            356, 256, 155, 28, hwnd, (HMENU)IDC_HOTKEY_NORMAL, NULL, NULL);
        SendMessage(hHotkeyNormal, HKM_SETHOTKEY, MAKEWORD(VK_F8, 0), 0);
        SendMessage(hHotkeyNormal, WM_SETFONT, (WPARAM)hFont, TRUE);
        NoTheme(hHotkeyNormal);
        SubclassHotkey(hHotkeyNormal);

        // ==========================================
        // Footer  (y = 310..408)
        // ==========================================
        HWND hInfo1 = CreateWindowA("STATIC", "Auto-selects the highest available refresh rate",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            14, 318, 636, 16, hwnd, NULL, NULL, NULL);
        SendMessage(hInfo1, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        NoTheme(hInfo1);

        HWND hInfo2 = CreateWindowA("STATIC", "Reset to default:  Ctrl + Alt + Shift + R",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            14, 338, 636, 16, hwnd, NULL, NULL, NULL);
        SendMessage(hInfo2, WM_SETFONT, (WPARAM)hSmallULFont, TRUE);
        NoTheme(hInfo2);

        HWND hBtnSave = CreateWindowA("BUTTON", "Save && Apply",
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
            212, 364, 240, 40, hwnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);
        SendMessage(hBtnSave, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
        NoTheme(hBtnSave);

        // Small "Refresh Res" button — bottom right
        hBtnRefreshRes = CreateWindowA("BUTTON", "",
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
            490, 370, 120, 28, hwnd, (HMENU)IDC_BTN_REFRESH_RES, NULL, NULL);
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
        RegisterHotKey(hwnd, HOTKEY_RESET, MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'R');

        InitTray(hwnd);
        LoadConfig();
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

        // Header bar (y 0..56)
        RECT rcHdr = {0, 0, W, 56};
        FillRect(hdc, &rcHdr, hBrushPanel);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPenAccent);
        MoveToEx(hdc, 0, 55, NULL); LineTo(hdc, W, 55);

        // Monitor card (y 68..124)
        RECT rcMon = {12, 68, W - 12, 124};
        FillRect(hdc, &rcMon, hBrushPanel);
        RECT   rcAccL = {12, 68, 15, 124};
        FillRect(hdc, &rcAccL, hBrushAccent);
        SelectObject(hdc, hPenBorder);
        HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rcMon.left, rcMon.top, rcMon.right, rcMon.bottom);

        // Thin separator between monitor card and preset cards
        SelectObject(hdc, hPenSep);
        MoveToEx(hdc, 12, 130, NULL); LineTo(hdc, W - 12, 130);

        // PRESET 1 card (x 12..330, y 132..304)
        RECT rcC1 = {12, 132, 330, 304};
        FillRect(hdc, &rcC1, hBrushPanel);
        RECT   rcAt1 = {12, 132, 330, 135};
        FillRect(hdc, &rcAt1, g_activePreset == 1 ? hBrushGreen : hBrushAccent);
        SelectObject(hdc, g_activePreset == 1 ? hPenGreen : hPenBorder);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rcC1.left, rcC1.top, rcC1.right, rcC1.bottom);
        if (g_activePreset == 1) {
            RECT rcBot1 = {12, 301, 330, 304};
            FillRect(hdc, &rcBot1, hBrushGreen);
        }

        // PRESET 2 card (x 338..W-12, y 132..304)
        RECT rcC2 = {338, 132, W - 12, 304};
        FillRect(hdc, &rcC2, hBrushPanel);
        RECT   rcAt2 = {338, 132, W - 12, 135};
        FillRect(hdc, &rcAt2, g_activePreset == 2 ? hBrushGreen : hBrushAccent);
        SelectObject(hdc, g_activePreset == 2 ? hPenGreen : hPenBorder);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rcC2.left, rcC2.top, rcC2.right, rcC2.bottom);
        if (g_activePreset == 2) {
            RECT rcBot2 = {338, 301, W - 12, 304};
            FillRect(hdc, &rcBot2, hBrushGreen);
        }

        // Bottom separator above footer
        SelectObject(hdc, hPenSep);
        MoveToEx(hdc, 12, 310, NULL); LineTo(hdc, W - 12, 310);

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

        if (rcCtrl.top < 56)                              { SetTextColor(hdc, COL_TEXT2); return (INT_PTR)hBrushPanel; }
        if (rcCtrl.top >= 56  && rcCtrl.top < 126)        { SetTextColor(hdc, COL_TEXT2); return (INT_PTR)hBrushPanel; }
        if (rcCtrl.top >= 126 && rcCtrl.top < 306)        { SetTextColor(hdc, COL_TEXT2); return (INT_PTR)hBrushPanel; }
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
            DebugLog("USER ACTION: Applied hotkeys and saved configuration");
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

        static const RECT rcPresets = {12, 132, 654, 304};
        if (wParam == HOTKEY_GAME_MODE) {
            int idx = SendMessage(hComboResGame, CB_GETCURSEL, 0, 0);
            SendMessage(hComboResGame, CB_GETLBTEXT, idx, (LPARAM)buf);
            sscanf(buf, "%dx%d", &w, &h);
            g_activePreset = 1;
            InvalidateRect(hwnd, &rcPresets, FALSE);
        } else if (wParam == HOTKEY_NORMAL_MODE) {
            int idx = SendMessage(hComboResNormal, CB_GETCURSEL, 0, 0);
            SendMessage(hComboResNormal, CB_GETLBTEXT, idx, (LPARAM)buf);
            sscanf(buf, "%dx%d", &w, &h);
            g_activePreset = 2;
            InvalidateRect(hwnd, &rcPresets, FALSE);
        } else if (wParam == HOTKEY_RESET) {
            if (originalWidth > 0 && originalHeight > 0) {
                ChangeRes(originalWidth, originalHeight, monIdx);
                g_activePreset = 0;
                InvalidateRect(hwnd, &rcPresets, FALSE);
            }
            return 0;
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
                    g_activePreset = 0;
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
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    // =========================================================
    // WM_DESTROY — release GDI objects, quit
    // =========================================================
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
