#include "globals.h"

// ---- Persist current UI state to config.ini ----

void SaveConfig() {
    char buf[64];

    int monIdx = SendMessage(hComboMon, CB_GETCURSEL, 0, 0);
    sprintf(buf, "%d", monIdx);
    WritePrivateProfileStringA("Settings", "Monitor", buf, configFile);

    int res1Idx = SendMessage(hComboResGame, CB_GETCURSEL, 0, 0);
    SendMessage(hComboResGame, CB_GETLBTEXT, res1Idx, (LPARAM)buf);
    WritePrivateProfileStringA("Settings", "Resolution1", buf, configFile);
    strncpy(g_savedRes1, buf, sizeof(g_savedRes1) - 1);

    int res2Idx = SendMessage(hComboResNormal, CB_GETCURSEL, 0, 0);
    SendMessage(hComboResNormal, CB_GETLBTEXT, res2Idx, (LPARAM)buf);
    WritePrivateProfileStringA("Settings", "Resolution2", buf, configFile);
    strncpy(g_savedRes2, buf, sizeof(g_savedRes2) - 1);

    sprintf(buf, "%d", gameHotkey);
    WritePrivateProfileStringA("Settings", "Hotkey1", buf, configFile);

    sprintf(buf, "%d", normalHotkey);
    WritePrivateProfileStringA("Settings", "Hotkey2", buf, configFile);

    DebugLog("Config saved to %s", configFile);
}

// ---- Helper: restore a saved resolution string into a combo box ----

static void RestoreComboSelection(HWND hCombo, const char* saved) {
    if (!saved || !saved[0]) return;
    int count = SendMessage(hCombo, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        char item[64];
        SendMessage(hCombo, CB_GETLBTEXT, i, (LPARAM)item);
        if (strcmp(item, saved) == 0) {
            SendMessage(hCombo, CB_SETCURSEL, i, 0);
            return;
        }
    }
}

// ---- Load config.ini and apply to UI controls ----

void LoadConfig() {
    char buf[64];
    DebugLog("Loading config from %s", configFile);

    // Monitor
    GetPrivateProfileStringA("Settings", "Monitor", "0", buf, sizeof(buf), configFile);
    int monIdx = atoi(buf);
    if (monIdx >= 0 && monIdx < SendMessage(hComboMon, CB_GETCOUNT, 0, 0))
        SendMessage(hComboMon, CB_SETCURSEL, monIdx, 0);

    // Resolution presets
    GetPrivateProfileStringA("Settings", "Resolution1", "", buf, sizeof(buf), configFile);
    strncpy(g_savedRes1, buf, sizeof(g_savedRes1) - 1);
    RestoreComboSelection(hComboResGame, buf);

    GetPrivateProfileStringA("Settings", "Resolution2", "", buf, sizeof(buf), configFile);
    strncpy(g_savedRes2, buf, sizeof(g_savedRes2) - 1);
    RestoreComboSelection(hComboResNormal, buf);

    // Hotkeys
    GetPrivateProfileStringA("Settings", "Hotkey1", "118", buf, sizeof(buf), configFile);
    gameHotkey = atoi(buf);
    SendMessage(hHotkeyGame, HKM_SETHOTKEY, MAKEWORD(gameHotkey, 0), 0);

    GetPrivateProfileStringA("Settings", "Hotkey2", "119", buf, sizeof(buf), configFile);
    normalHotkey = atoi(buf);
    SendMessage(hHotkeyNormal, HKM_SETHOTKEY, MAKEWORD(normalHotkey, 0), 0);

    DebugLog("Config loaded");
}
