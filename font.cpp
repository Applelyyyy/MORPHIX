#include "globals.h"

// ---- Check if a font face is already installed / loaded ----

static bool IsFontInstalled(const char* faceName) {
    HDC hdc = GetDC(NULL);
    bool found = false;
    LOGFONTA lf = {};
    lf.lfCharSet = DEFAULT_CHARSET;
    strncpy(lf.lfFaceName, faceName, LF_FACESIZE - 1);
    struct CB {
        static int CALLBACK Proc(const LOGFONTA*, const TEXTMETRICA*, DWORD, LPARAM lp) {
            *(bool*)lp = true; return 0;
        }
    };
    EnumFontFamiliesExA(hdc, &lf, CB::Proc, (LPARAM)&found, 0);
    ReleaseDC(NULL, hdc);
    return found;
}

// ---- Permanently register a font file for the current user (no admin) ----

static void RegisterFontForUser(const char* fontPath) {
    AddFontResourceExA(fontPath, 0, NULL);
    HKEY hKey = NULL;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "JetBrains Mono Bold (TrueType)", 0, REG_SZ,
            (const BYTE*)fontPath, (DWORD)(strlen(fontPath) + 1));
        RegCloseKey(hKey);
    }
}

// ---- Load JetBrains Mono from embedded resource; install to HKCU if needed ----

void EnsureJetBrainsMono() {
    // Preferred install path: %LOCALAPPDATA%\Microsoft\Windows\Fonts\  (no admin required)
    char installDir[MAX_PATH] = {};
    char localApp[MAX_PATH]   = {};
    if (GetEnvironmentVariableA("LOCALAPPDATA", localApp, MAX_PATH) > 0) {
        sprintf(installDir, "%s\\Microsoft\\Windows\\Fonts", localApp);
        CreateDirectoryA(installDir, NULL);
        if (GetFileAttributesA(installDir) == INVALID_FILE_ATTRIBUTES)
            installDir[0] = '\0';
    }
    if (!installDir[0]) {
        // Fallback: directory of the running exe
        GetModuleFileNameA(NULL, installDir, MAX_PATH);
        char* sl = strrchr(installDir, '\\'); if (sl) *sl = '\0';
    }
    char installPath[MAX_PATH];
    sprintf(installPath, "%s\\JetBrainsMono-Bold.ttf", installDir);

    // Try embedded RCDATA resource (ID 200)
    HRSRC hRes = FindResourceA(NULL, MAKEINTRESOURCEA(200), RT_RCDATA);
    if (!hRes) {
        // No embedded resource — look for font file next to the exe
        char exeDir[MAX_PATH] = {};
        GetModuleFileNameA(NULL, exeDir, MAX_PATH);
        char* sl = strrchr(exeDir, '\\'); if (sl) *sl = '\0';
        char diskPath[MAX_PATH];
        sprintf(diskPath, "%s\\JetBrainsMono-Bold.ttf", exeDir);
        if (GetFileAttributesA(diskPath) != INVALID_FILE_ATTRIBUTES) {
            if (!IsFontInstalled("JetBrains Mono"))
                RegisterFontForUser(diskPath);
            else
                AddFontResourceExA(diskPath, 0, NULL);
        }
        return;
    }

    HGLOBAL hGlob = LoadResource(NULL, hRes);
    if (!hGlob) return;
    void*  pData = LockResource(hGlob);
    DWORD  size  = SizeofResource(NULL, hRes);
    if (!pData || !size) return;

    // Load into current GDI session instantly (no disk write needed for immediate use)
    DWORD nFonts = 0;
    AddFontMemResourceEx(pData, size, NULL, &nFonts);

    // Permanently install if the face is not already registered in the system
    if (!IsFontInstalled("JetBrains Mono")) {
        if (GetFileAttributesA(installPath) == INVALID_FILE_ATTRIBUTES) {
            HANDLE hf = CreateFileA(installPath, GENERIC_WRITE, 0, NULL,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hf != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
                WriteFile(hf, pData, size, &written, NULL);
                CloseHandle(hf);
            }
        }
        RegisterFontForUser(installPath);
    }
}
