@echo off
echo ====================================
echo   MORPHIX - Build and Run
echo ====================================
echo.

echo [1/3] Terminating existing MORPHIX process...
taskkill /IM MORPHIX.exe /F >nul 2>&1
timeout /t 1 /nobreak >nul

echo [2/3] Compiling with icon and manifest...
windres app.rc -o app_res.o 2>nul
if %errorlevel% neq 0 (
    echo Warning: Resource compilation failed, building without icon...
    g++ main.cpp -o MORPHIX.exe -lgdi32 -luser32 -lcomctl32 -mwindows 2>nul
) else (
    g++ main.cpp app_res.o -o MORPHIX.exe -lgdi32 -luser32 -lcomctl32 -mwindows 2>nul
    del app_res.o 2>nul
)

if %errorlevel% equ 0 (
    echo.
    echo ====================================
    echo   SUCCESS! MORPHIX.exe compiled
    echo ====================================
    echo.
    echo [3/3] Launching MORPHIX...
    start MORPHIX.exe
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

