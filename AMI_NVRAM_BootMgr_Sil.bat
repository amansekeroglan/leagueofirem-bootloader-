@echo off
setlocal
chcp 65001 >nul
title UEFI Boot Manager - AMI Aptio V NVRAM Uninstaller

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [INFO] Requesting administrator privileges...
    powershell -NoProfile -Command "Start-Process cmd.exe -ArgumentList '/k \"\"%~f0\"\"' -Verb RunAs"
    exit /b
)

cd /d "%~dp0"

echo [INFO] Executing AMI Aptio V NVRAM cleanup...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0ami_nvram_clean.ps1"

echo.
echo Press any key to exit...
pause >nul
