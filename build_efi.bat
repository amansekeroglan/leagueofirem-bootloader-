@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo [BootMgr] Building UEFI Bootloader on Windows
echo ===================================================

cd /d "%~dp0"

if not defined VCINSTALLDIR (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) else (
        echo [ERROR] Visual Studio x64 Build Tools not found.
        exit /b 1
    )
)

echo.
echo [1/3] Preparing embedded graphical assets...
if not exist "wallpaper.obj" (
    powershell.exe -ExecutionPolicy Bypass -File "%~dp0gen_wallpaper_1080.ps1"
)
if not exist "logo.obj" (
    powershell.exe -ExecutionPolicy Bypass -File "%~dp0gen_logo.ps1"
)

echo.
echo [2/3] Compiling C source files...
cl.exe /c /O2 /GS- /W3 /TC /I"msvc_include" /I"." ^
    BootMgr.c ^
    Graphics.c ^
    Input.c ^
    Timer.c ^
    WindowsBoot.c ^
    FirmwareSetup.c ^
    msvc_support.c

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed with error %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

echo.
echo [3/3] Linking BootMgr.efi...
link.exe /subsystem:efi_application /entry:UefiMain /nodefaultlib /out:BootMgr.efi ^
    BootMgr.obj ^
    Graphics.obj ^
    Input.obj ^
    Timer.obj ^
    WindowsBoot.obj ^
    FirmwareSetup.obj ^
    msvc_support.obj ^
    wallpaper.obj ^
    logo.obj

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Linking failed with error %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

echo.
echo Cleaning up intermediate object files...
del /q *.obj >nul 2>&1
del /q *.bin >nul 2>&1

echo.
echo ===================================================
echo [SUCCESS] BootMgr.efi built successfully!
echo Binary size:
dir BootMgr.efi | findstr /i "BootMgr.efi"
echo ===================================================
