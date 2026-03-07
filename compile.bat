@echo off
echo ====================================
echo   MORPHIX - Build and Run
echo ====================================
echo.

echo [1/4] Terminating existing MORPHIX process...
taskkill /IM MORPHIX.exe /F >nul 2>&1
timeout /t 1 /nobreak >nul

echo [2/4] Checking JetBrains Mono font...
if not exist "JetBrainsMono-Bold.ttf" (
    echo   Font file not found. Downloading...
    powershell -NoProfile -Command "try { Invoke-WebRequest -Uri 'https://github.com/JetBrains/JetBrainsMono/raw/master/fonts/ttf/JetBrainsMono-Bold.ttf' -OutFile 'JetBrainsMono-Bold.ttf' -UseBasicParsing } catch { exit 1 }"
    if not exist "JetBrainsMono-Bold.ttf" (
        echo   Warning: Download failed. App will use fallback font.
    ) else (
        echo   Font downloaded successfully.
    )
)
if exist "JetBrainsMono-Bold.ttf" (
    if not exist "%LOCALAPPDATA%\Microsoft\Windows\Fonts\JetBrainsMono-Bold.ttf" (
        if not exist "%LOCALAPPDATA%\Microsoft\Windows\Fonts" mkdir "%LOCALAPPDATA%\Microsoft\Windows\Fonts"
        copy "JetBrainsMono-Bold.ttf" "%LOCALAPPDATA%\Microsoft\Windows\Fonts\" >nul 2>&1
        reg add "HKCU\Software\Microsoft\Windows NT\CurrentVersion\Fonts" /v "JetBrains Mono Bold (TrueType)" /t REG_SZ /d "%LOCALAPPDATA%\Microsoft\Windows\Fonts\JetBrainsMono-Bold.ttf" /f >nul 2>&1
        echo   Font installed for current user.
    ) else (
        echo   Font already installed.
    )
)

echo [3/4] Compiling with icon and manifest...
windres app.rc -o app_res.o 2>nul
if %errorlevel% neq 0 (
    echo Warning: Resource compilation failed, building without icon...
    g++ main.cpp utils.cpp display.cpp config.cpp font.cpp controls.cpp wndproc.cpp color.cpp -O2 -o MORPHIX.exe -lgdi32 -luser32 -lcomctl32 -luxtheme -mwindows 2>nul
) else (
    g++ main.cpp utils.cpp display.cpp config.cpp font.cpp controls.cpp wndproc.cpp color.cpp app_res.o -O2 -o MORPHIX.exe -lgdi32 -luser32 -lcomctl32 -luxtheme -mwindows 2>nul
    del app_res.o 2>nul
)

if %errorlevel% equ 0 (
    echo.
    echo ====================================
    echo   SUCCESS! MORPHIX.exe compiled
    echo ====================================
    echo.
    echo [4/4] Launching MORPHIX...
    start "" MORPHIX.exe
    timeout /t 1 /nobreak >nul
    echo.
    echo ====================================
    echo   MORPHIX is running!
    echo ====================================
) else (
    echo.
    echo ====================================
    echo   COMPILATION FAILED!
    echo ====================================
    pause
    exit /b 1
)

