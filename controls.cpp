#include "globals.h"

// ---- File-private state for subclassing ----

static WNDPROC g_OldHkProc    = NULL;
static WNDPROC g_OldComboProc = NULL;

// ---- Format a virtual key + modifiers into a human-readable string ----

static void FormatHotkeyText(BYTE vk, BYTE mod, char* buf, int bufLen) {
    buf[0] = 0;
    if (!vk) { strcpy(buf, "None"); return; }
    if (mod & HOTKEYF_CONTROL) strcat(buf, "Ctrl + ");
    if (mod & HOTKEYF_ALT)     strcat(buf, "Alt + ");
    if (mod & HOTKEYF_SHIFT)   strcat(buf, "Shift + ");
    char name[32] = {0};
    if      (vk >= VK_F1 && vk <= VK_F24)                        sprintf(name, "F%d", vk - VK_F1 + 1);
    else if (vk == VK_SPACE)                                      strcpy(name, "Space");
    else if (vk == VK_RETURN)                                     strcpy(name, "Enter");
    else if (vk == VK_TAB)                                        strcpy(name, "Tab");
    else if (vk == VK_ESCAPE)                                     strcpy(name, "Esc");
    else if (vk == VK_DELETE)                                     strcpy(name, "Del");
    else if (vk == VK_INSERT)                                     strcpy(name, "Ins");
    else if (vk == VK_HOME)                                       strcpy(name, "Home");
    else if (vk == VK_END)                                        strcpy(name, "End");
    else if (vk == VK_PRIOR)                                      strcpy(name, "PgUp");
    else if (vk == VK_NEXT)                                       strcpy(name, "PgDn");
    else if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) { name[0] = (char)vk; name[1] = 0; }
    else {
        UINT sc = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        if (!GetKeyNameTextA((LONG)(sc << 16), name, sizeof(name)))
            sprintf(name, "0x%02X", vk);
    }
    strncat(buf, name, bufLen - (int)strlen(buf) - 1);
}

// ---- Hotkey control subclass — dark background + custom text rendering ----

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
            HFONT hf    = (HFONT)SendMessage(h, WM_GETFONT, 0, 0);
            HFONT hfOld = hf ? (HFONT)SelectObject(hdc, hf) : NULL;
            RECT rcT = {6, 0, rc.right - 4, rc.bottom};
            DrawTextA(hdc, buf, -1, &rcT, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            if (hfOld) SelectObject(hdc, hfOld);
            ReleaseDC(h, hdc);
        }
        return r;
    }
    if (msg == WM_SETFOCUS) {
        // Unregister hotkeys while editing so they don't fire as the user types
        HWND parent = GetParent(h);
        UnregisterHotKey(parent, HOTKEY_GAME_MODE);
        UnregisterHotKey(parent, HOTKEY_NORMAL_MODE);
        LRESULT r = CallWindowProcA(g_OldHkProc, h, msg, w, l);
        InvalidateRect(h, NULL, TRUE);
        return r;
    }
    if (msg == WM_KILLFOCUS) {
        // Re-register the *saved* hotkeys (new one takes effect only after Save & Apply)
        HWND parent = GetParent(h);
        RegisterHotKey(parent, HOTKEY_GAME_MODE,   MOD_NOREPEAT, gameHotkey);
        RegisterHotKey(parent, HOTKEY_NORMAL_MODE, MOD_NOREPEAT, normalHotkey);
        LRESULT r = CallWindowProcA(g_OldHkProc, h, msg, w, l);
        InvalidateRect(h, NULL, TRUE);
        return r;
    }
    return CallWindowProcA(g_OldHkProc, h, msg, w, l);
}

void SubclassHotkey(HWND h) {
    // Allow ALL combinations including bare Fn keys (0 = no invalid combos)
    SendMessage(h, HKM_SETRULES, 0, 0);
    WNDPROC old = (WNDPROC)SetWindowLongPtrA(h, GWLP_WNDPROC, (LONG_PTR)HotkeySubclassProc);
    if (!g_OldHkProc) g_OldHkProc = old;
}

// ---- Combobox subclass — overpaints the native white dropdown button ----

LRESULT CALLBACK ComboSubclassProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    LRESULT r = CallWindowProcA(g_OldComboProc, h, msg, w, l);
    if (msg == WM_PAINT || msg == WM_NCPAINT) {
        HDC hdc = GetDC(h);
        if (hdc) {
            RECT rc; GetClientRect(h, &rc);
            // Button zone: fixed 18px, inset 1px from border on right/top/bottom
            RECT rcBtn = {rc.right - 18, rc.top + 1, rc.right - 1, rc.bottom - 1};
            FillRect(hdc, &rcBtn, hBrushCtrl);
            // Thin left separator
            HPEN hOldP = (HPEN)SelectObject(hdc, hPenBorder);
            MoveToEx(hdc, rcBtn.left, rcBtn.top + 2, NULL);
            LineTo(hdc,   rcBtn.left, rcBtn.bottom - 2);
            SelectObject(hdc, hOldP);
            // Triangle chevron centered in button zone
            int ax = rcBtn.left + (rcBtn.right - rcBtn.left) / 2;
            int ay = (rcBtn.top + rcBtn.bottom) / 2;
            POINT tri[3] = {{ax - 4, ay - 2}, {ax + 4, ay - 2}, {ax, ay + 3}};
            HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hBrushText2);
            HPEN   hOldPn = (HPEN)  SelectObject(hdc, hPenText2);
            Polygon(hdc, tri, 3);
            SelectObject(hdc, hOldBr); SelectObject(hdc, hOldPn);
            ReleaseDC(h, hdc);
        }
    }
    return r;
}

void SubclassCombo(HWND h) {
    WNDPROC old = (WNDPROC)SetWindowLongPtrA(h, GWLP_WNDPROC, (LONG_PTR)ComboSubclassProc);
    if (!g_OldComboProc) g_OldComboProc = old;
}
