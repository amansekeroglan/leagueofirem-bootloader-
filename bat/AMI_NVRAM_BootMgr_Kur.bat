@echo off
setlocal
chcp 65001 >nul
title UEFI Boot Manager - AMI Aptio V NVRAM Installer

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [INFO] Requesting administrator privileges...
    powershell -NoProfile -Command "Start-Process cmd.exe -ArgumentList '/k \"\"%~f0\"\"' -Verb RunAs"
    exit /b
)

cd /d "%~dp0"

echo [INFO] Executing AMI Aptio V NVRAM installation...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\ps\ami_nvram.ps1"

echo.
echo Press any key to exit...
pause >nul
