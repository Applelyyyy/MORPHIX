#include "globals.h"

// ---- NVAPI Digital Vibrance Control (DVC) via dynamic DLL loading ----
// Function IDs from nvapi_interface.h — same IDs used by VibranceGUI / RTSS.
// NVAPI DVC range is 0..63 on modern drivers (0 = no extra saturation, neutral).
// We map user vibrance 0-100 linearly onto [minLevel..maxLevel].
// "50" always corresponds to the driver's neutral (minLevel for additive DVC).

#define NVAPI_INIT_ID                   0x0150E828u
#define NVAPI_ENUM_NVIDIA_DISP_ID       0x9ABDD40Du
#define NVAPI_GET_ASSOC_DISP_HANDLE_ID  0x35C29134u
#define NVAPI_GET_DVC_INFO_EX_ID        0x0e45002Du  // NvAPI_GetDVCInfoEx  (preferred)
#define NVAPI_GET_DVC_INFO_ID           0x4085DE45u  // NvAPI_GetDVCInfo    (fallback)
#define NVAPI_SET_DVC_LEVEL_EX_ID       0x4A82C2B1u  // NvAPI_SetDVCLevelEx (preferred)
#define NVAPI_SET_DVC_LEVEL_ID          0x172409B4u  // NvAPI_SetDVCLevel   (fallback)

typedef void* (__cdecl *PFN_QueryInterface)(unsigned int);
typedef int   (__cdecl *PFN_Initialize)();
typedef int   (__cdecl *PFN_EnumNVDisp)(unsigned int, void**);
typedef int   (__cdecl *PFN_GetAssocDispHandle)(const char*, void**);

// NvAPI version macro: size in low 16 bits, version number in high 16 bits
#define NVAPI_VER1(T) ((NvU32)(sizeof(T) | (1u << 16)))
typedef unsigned int NvU32;

// DVC info/level structs — must match exactly what the driver expects
#pragma pack(push, 8)
struct NV_DVC_INFO_EX {
    NvU32 version;
    int   currentLevel;
    int   minLevel;
    int   maxLevel;
    int   defaultLevel;  // present in real NvAPI struct (PrivateDisplayDVCInfoEx)
};
struct NV_DVC_INFO {
    NvU32 version;
    int   currentLevel;
    int   minLevel;
    int   maxLevel;
};
#pragma pack(pop)

typedef int (__cdecl *PFN_GetDVCInfoEx) (void*, NvU32, NV_DVC_INFO_EX*);
typedef int (__cdecl *PFN_GetDVCInfo)   (void*, NvU32, NV_DVC_INFO*);
typedef int (__cdecl *PFN_SetDVCLevelEx)(void*, NvU32, NV_DVC_INFO_EX*);
typedef int (__cdecl *PFN_SetDVCLevel)  (void*, NvU32, int);

static HMODULE              g_hNvapi         = NULL;
static PFN_QueryInterface   g_QueryIface     = NULL;
static PFN_EnumNVDisp       g_EnumDisp       = NULL;
static PFN_GetAssocDispHandle g_GetAssocDisp  = NULL;
static PFN_GetDVCInfoEx     g_GetDVCInfoEx   = NULL;
static PFN_GetDVCInfo       g_GetDVCInfo     = NULL;
static PFN_SetDVCLevelEx    g_SetDVCLevelEx  = NULL;
static PFN_SetDVCLevel      g_SetDVCLevel    = NULL;
static bool                 g_nvapiReady     = false;

void InitNVAPI() {
    g_hNvapi = LoadLibraryA("nvapi64.dll");
    if (!g_hNvapi) g_hNvapi = LoadLibraryA("nvapi.dll");
    if (!g_hNvapi) { DebugLog("NVAPI: DLL not found - vibrance disabled"); return; }

    g_QueryIface = (PFN_QueryInterface)GetProcAddress(g_hNvapi, "nvapi_QueryInterface");
    if (!g_QueryIface) { DebugLog("NVAPI: nvapi_QueryInterface missing"); return; }

    PFN_Initialize fnInit = (PFN_Initialize)g_QueryIface(NVAPI_INIT_ID);
    if (!fnInit || fnInit() != 0) { DebugLog("NVAPI: NvAPI_Initialize failed"); return; }

    g_EnumDisp      = (PFN_EnumNVDisp)        g_QueryIface(NVAPI_ENUM_NVIDIA_DISP_ID);
    g_GetAssocDisp  = (PFN_GetAssocDispHandle) g_QueryIface(NVAPI_GET_ASSOC_DISP_HANDLE_ID);
    g_GetDVCInfoEx  = (PFN_GetDVCInfoEx)       g_QueryIface(NVAPI_GET_DVC_INFO_EX_ID);
    g_GetDVCInfo    = (PFN_GetDVCInfo)         g_QueryIface(NVAPI_GET_DVC_INFO_ID);
    g_SetDVCLevelEx = (PFN_SetDVCLevelEx)      g_QueryIface(NVAPI_SET_DVC_LEVEL_EX_ID);
    g_SetDVCLevel   = (PFN_SetDVCLevel)        g_QueryIface(NVAPI_SET_DVC_LEVEL_ID);

    g_nvapiReady = (g_EnumDisp != NULL) &&
                   ((g_GetDVCInfoEx && g_SetDVCLevelEx) ||
                    (g_GetDVCInfo   && g_SetDVCLevel));
    DebugLog("NVAPI: loaded, DVC %s", g_nvapiReady ? "ready" : "unavailable");
}

void ShutdownNVAPI() {
    if (g_hNvapi) { FreeLibrary(g_hNvapi); g_hNvapi = NULL; }
    g_QueryIface = NULL; g_EnumDisp = NULL; g_GetAssocDisp = NULL;
    g_GetDVCInfoEx = NULL; g_GetDVCInfo = NULL;
    g_SetDVCLevelEx = NULL; g_SetDVCLevel = NULL;
    g_nvapiReady = false;
}

// Map user vibrance 0-100 to driver DVC level.
// vibrance=0   → defL (factory default / neutral — no saturation change)
// vibrance=1-100 → linear 1%..100% of maxL
// Using maxL as the 100% reference (not maxL-defL) ensures MORPHIX 50% == NVCP 50%
// on systems where defL != 0 (e.g., older driver 0..63 range with defL=32).
static int VibranceToLevel(int vibrance, int defL, int maxL) {
    if (vibrance <= 0) return defL;
    if (vibrance >= 100) return maxL;
    return (maxL * vibrance + 50) / 100;  // integer rounding, no FP
}

// Apply DVC level to one display handle. vibrance 0-100.
static bool SetDVCOnHandle(void* hDisp, int vibrance) {
    // Try Ex variant first (preferred — correct 20-byte struct with defaultLevel)
    if (g_GetDVCInfoEx && g_SetDVCLevelEx) {
        NV_DVC_INFO_EX info = {};
        info.version = NVAPI_VER1(NV_DVC_INFO_EX);
        if (g_GetDVCInfoEx(hDisp, 0, &info) == 0) {
            int defL = info.defaultLevel;
            int maxL = info.maxLevel;
            if (maxL <= defL) maxL = defL + 63; // safety
            info.currentLevel = VibranceToLevel(vibrance, defL, maxL);
            int ret = g_SetDVCLevelEx(hDisp, 0, &info);
            DebugLog("NVAPI DVCEx: vibrance=%d level=%d [def=%d max=%d] ret=%d",
                     vibrance, info.currentLevel, defL, maxL, ret);
            if (ret == 0) return true;
        }
    }
    // Fallback: simple GetDVCInfo/SetDVCLevel (non-Ex, 0..maxLevel range, defaultLevel=0)
    if (g_GetDVCInfo && g_SetDVCLevel) {
        NV_DVC_INFO info = {};
        info.version = NVAPI_VER1(NV_DVC_INFO);
        if (g_GetDVCInfo(hDisp, 0, &info) == 0) {
            int maxL = info.maxLevel;
            if (maxL <= 0) maxL = 63;
            int level = VibranceToLevel(vibrance, 0, maxL);
            int ret = g_SetDVCLevel(hDisp, 0, level);
            DebugLog("NVAPI DVC: vibrance=%d level=%d [0..%d] ret=%d", vibrance, level, maxL, ret);
            if (ret == 0) return true;
        }
    }
    // Last resort: direct SetDVCLevel with hardcoded 0..63 range
    if (g_SetDVCLevel) {
        int level = VibranceToLevel(vibrance, 0, 63);
        int ret = g_SetDVCLevel(hDisp, 0, level);
        DebugLog("NVAPI DVC fallback: vibrance=%d level=%d ret=%d", vibrance, level, ret);
        return (ret == 0);
    }
    return false;
}

// Apply digital vibrance to the correct NVIDIA display for monIdx.
static void ApplyNVAPIVibrance(int vibrance, int monIdx) {
    if (!g_nvapiReady) return;

    // Try targeted lookup by DeviceName first (e.g. "\\.\DISPLAY1")
    if (g_GetAssocDisp && monIdx >= 0 && monIdx < (int)monitors.size()) {
        void* hDisp = NULL;
        if (g_GetAssocDisp(monitors[monIdx].DeviceName, &hDisp) == 0 && hDisp) {
            if (SetDVCOnHandle(hDisp, vibrance)) return;
        }
    }
    // Fallback: apply to all enumerated NVIDIA displays
    for (int i = 0; i < 8; i++) {
        void* hDisp = NULL;
        if (g_EnumDisp(i, &hDisp) != 0) break;
        SetDVCOnHandle(hDisp, vibrance);
    }
}

// ---- Main color application (vibrance only) ----
// brightness/contrast/gamma are intentionally removed — SetDeviceGammaRamp
// requires the Windows display driver to be active; since the driver is disabled,
// only NVAPI DVC (digital vibrance) is applied.
void ApplyColorSettings(int vibrance, int monIdx) {
    if (vibrance < 0) vibrance = 0;
    if (vibrance > 100) vibrance = 100;
    if (monIdx < 0) monIdx = 0;
    ApplyNVAPIVibrance(vibrance, monIdx);
    DebugLog("ApplyColor: vibrance=%d mon=%d", vibrance, monIdx);
}

void ResetColorSettings(int monIdx) {
    ApplyColorSettings(0, monIdx);  // 0 → defaultLevel (neutral)
}
