#include "globals.h"

// ---- Monitor enumeration ----

void EnumerateMonitors() {
    monitors.clear();
    DISPLAY_DEVICE dd;
    dd.cb = sizeof(dd);
    for (DWORD i = 0; EnumDisplayDevices(NULL, i, &dd, 0); i++) {
        if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
            monitors.push_back(dd);
            DebugLog("Found monitor %d: %s", i, dd.DeviceString);
        }
    }
    DebugLog("Total monitors: %d", (int)monitors.size());
}

// ---- Read current settings and update the Hz label ----

void GetCurrentDisplaySettings(int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= (int)monitors.size()) return;

    DEVMODE dm = {};
    dm.dmSize = sizeof(dm);
    if (!EnumDisplaySettingsEx(monitors[monitorIndex].DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0))
        return;

    currentHz = dm.dmDisplayFrequency;

    // Snapshot original resolution on first call
    if (originalWidth == 0) {
        originalWidth  = dm.dmPelsWidth;
        originalHeight = dm.dmPelsHeight;
        originalHz     = dm.dmDisplayFrequency;
        DebugLog("Original resolution: %dx%d @ %dHz", originalWidth, originalHeight, originalHz);
    }

    char label[64];
    sprintf(label, "Current: %dx%d @ %dHz", dm.dmPelsWidth, dm.dmPelsHeight, currentHz);
    SetWindowTextA(hLabelCurrentHz, label);
    DebugLog("%s", label);
}

// ---- Resolution dropdown population ----

void PopulateResolutions(HWND hCombo, int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= (int)monitors.size()) return;

    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);

    std::unordered_set<std::string>         seen;
    std::vector<std::pair<int, std::string>> resList;

    // Merge standard and raw-driver modes; skip tiny resolutions
    auto AcceptMode = [&](const DEVMODE& d) {
        if (d.dmPelsWidth < 640 || d.dmPelsHeight < 480) return;
        char key[64];
        sprintf(key, "%dx%d", d.dmPelsWidth, d.dmPelsHeight);
        if (!seen.insert(key).second) return;  // O(1) duplicate check
        resList.push_back({ (int)(d.dmPelsWidth * 10000 + d.dmPelsHeight), key });
    };

    DEVMODE dm = {};
    dm.dmSize = sizeof(dm);

    // Pass 1: Windows-validated standard modes
    for (DWORD i = 0; ; i++) {
        ZeroMemory(&dm, sizeof(dm)); dm.dmSize = sizeof(dm);
        if (!EnumDisplaySettingsEx(monitors[monitorIndex].DeviceName, i, &dm, 0)) break;
        AcceptMode(dm);
    }
    // Pass 2: Raw driver modes (may include extra resolutions not in pass 1)
    for (DWORD i = 0; ; i++) {
        ZeroMemory(&dm, sizeof(dm)); dm.dmSize = sizeof(dm);
        if (!EnumDisplaySettingsEx(monitors[monitorIndex].DeviceName, i, &dm, EDS_RAWMODE)) break;
        AcceptMode(dm);
    }

    // Sort largest first
    std::sort(resList.rbegin(), resList.rend());

    // Test each resolution for driver compatibility (width+height only — no Hz/bpp
    // to avoid false rejections for resolutions like 1080x1080)
    g_incompatRes[hCombo].clear();
    for (const auto& res : resList) {
        int rw = 0, rh = 0;
        sscanf(res.second.c_str(), "%dx%d", &rw, &rh);

        DEVMODE dmTest = {};
        dmTest.dmSize       = sizeof(dmTest);
        dmTest.dmPelsWidth  = rw;
        dmTest.dmPelsHeight = rh;
        dmTest.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

        LONG result = ChangeDisplaySettingsEx(
            monitors[monitorIndex].DeviceName, &dmTest, NULL, CDS_TEST, NULL);
        if (result != DISP_CHANGE_SUCCESSFUL)
            g_incompatRes[hCombo].insert(res.second);

        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)res.second.c_str());
    }

    SendMessage(hCombo, CB_SETCURSEL, 0, 0);
    DebugLog("Populated %d resolutions for monitor %d", (int)resList.size(), monitorIndex);
}

// ---- Find the highest refresh rate the driver supports for a given resolution ----

int GetMaxRefreshRate(int width, int height, int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= (int)monitors.size()) return 60;

    int maxHz = 60;
    DEVMODE dm = {};
    dm.dmSize = sizeof(dm);
    for (DWORD i = 0; EnumDisplaySettingsEx(monitors[monitorIndex].DeviceName, i, &dm, 0); i++) {
        if ((int)dm.dmPelsWidth == width && (int)dm.dmPelsHeight == height)
            if ((int)dm.dmDisplayFrequency > maxHz)
                maxHz = dm.dmDisplayFrequency;
    }
    DebugLog("Max Hz for %dx%d: %d", width, height, maxHz);
    return maxHz;
}

// ---- Apply a resolution change (always uses highest available Hz) ----

void ChangeRes(int width, int height, int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= (int)monitors.size()) return;

    int maxHz = GetMaxRefreshRate(width, height, monitorIndex);
    DebugLog("ChangeRes: %dx%d @ %dHz on monitor %d", width, height, maxHz, monitorIndex);

    DEVMODE dm = {};
    dm.dmSize             = sizeof(dm);
    dm.dmPelsWidth        = width;
    dm.dmPelsHeight       = height;
    dm.dmDisplayFrequency = maxHz;
    dm.dmFields           = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

    LONG result = ChangeDisplaySettingsEx(
        monitors[monitorIndex].DeviceName, &dm, NULL, CDS_UPDATEREGISTRY, NULL);

    if (result == DISP_CHANGE_SUCCESSFUL) {
        GetCurrentDisplaySettings(monitorIndex);
        DebugLog("Resolution changed successfully");
    } else {
        DebugLog("ChangeRes failed, code %ld", result);
        if (DEBUG_MODE)
            MessageBoxA(NULL, "Failed to change resolution.", "MORPHIX Error", MB_ICONERROR | MB_TOPMOST);
    }
}
