# AMI Aptio V Native UEFI NVRAM Boot Option Uninstaller for UEFI Boot Manager

$ErrorActionPreference = "Stop"

Write-Host "[INFO] Starting AMI Aptio V Native UEFI NVRAM cleanup for BootMgr..." -ForegroundColor Cyan

# 1. Native Win32 Firmware API Definition
$cSharpCode = @"
using System;
using System.Runtime.InteropServices;

public class NativeUefiBootMgrClean {
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern uint GetFirmwareEnvironmentVariableW(string lpName, string lpGuid, byte[] pBuffer, uint nSize);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern bool SetFirmwareEnvironmentVariableExW(string lpName, string lpGuid, byte[] pBuffer, uint nSize, uint dwAttributes);

    [DllImport("advapi32.dll", SetLastError = true)]
    public static extern bool OpenProcessToken(IntPtr ProcessHandle, uint DesiredAccess, out IntPtr TokenHandle);

    [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern bool LookupPrivilegeValue(string lpSystemName, string lpName, out LUID lpLuid);

    [DllImport("advapi32.dll", SetLastError = true)]
    public static extern bool AdjustTokenPrivileges(IntPtr TokenHandle, bool DisableAllPrivileges, ref TOKEN_PRIVILEGES NewState, uint BufferLength, IntPtr PreviousState, IntPtr ReturnLength);

    [StructLayout(LayoutKind.Sequential)]
    public struct LUID {
        public uint LowPart;
        public int HighPart;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct TOKEN_PRIVILEGES {
        public uint PrivilegeCount;
        public LUID Luid;
        public uint Attributes;
    }

    public static bool EnableSystemEnvironmentPrivilege() {
        IntPtr hToken;
        if (!OpenProcessToken(System.Diagnostics.Process.GetCurrentProcess().Handle, 0x0020 | 0x0008, out hToken)) return false;
        LUID luid;
        if (!LookupPrivilegeValue(null, "SeSystemEnvironmentPrivilege", out luid)) return false;
        TOKEN_PRIVILEGES tp = new TOKEN_PRIVILEGES();
        tp.PrivilegeCount = 1;
        tp.Luid = luid;
        tp.Attributes = 0x00000002;
        return AdjustTokenPrivileges(hToken, false, ref tp, 0, IntPtr.Zero, IntPtr.Zero);
    }
}
"@

Add-Type -TypeDefinition $cSharpCode -ErrorAction SilentlyContinue

if (-not [NativeUefiBootMgrClean]::EnableSystemEnvironmentPrivilege()) {
    Write-Host "[ERROR] Failed to acquire SeSystemEnvironmentPrivilege. Please run as Administrator." -ForegroundColor Red
    exit 1
}

$efiGlobalGuid = "{8be4df61-93ca-11d2-aa0d-00e098032b8c}"

# 2. Read BootOrder
Write-Host "[INFO] Querying motherboard BootOrder variable..."
$orderBuf = New-Object byte[] 512
$orderLen = [NativeUefiBootMgrClean]::GetFirmwareEnvironmentVariableW("BootOrder", $efiGlobalGuid, $orderBuf, 512)

$orders = New-Object System.Collections.Generic.List[uint16]
if ($orderLen -gt 0) {
    for ($i = 0; $i -lt $orderLen; $i += 2) {
        $orders.Add([BitConverter]::ToUInt16($orderBuf, $i))
    }
}

# 3. Identify and Remove BootMgr NVRAM Entries
$toRemove = New-Object System.Collections.Generic.List[uint16]

foreach ($b in $orders) {
    $bName = "Boot" + $b.ToString("X4")
    $bBuf = New-Object byte[] 4096
    $bLen = [NativeUefiBootMgrClean]::GetFirmwareEnvironmentVariableW($bName, $efiGlobalGuid, $bBuf, 4096)
    if ($bLen -gt 6) {
        $str = [System.Text.Encoding]::Unicode.GetString($bBuf, 6, $bLen - 6)
        if ($str -match "UEFI Boot Manager" -or $str -match "BootMgr" -or $str -match "\\EFI\\BootMgr") {
            Write-Host "[INFO] Removing NVRAM boot entry: $bName" -ForegroundColor Yellow
            [NativeUefiBootMgrClean]::SetFirmwareEnvironmentVariableExW($bName, $efiGlobalGuid, $null, 0, 0) | Out-Null
            $toRemove.Add($b)
        }
    }
}

# Scan common indices (0x0010 to 0x0030) for unlisted BootMgr entries
for ($idx = 16; $idx -le 48; $idx++) {
    if (-not $toRemove.Contains($idx)) {
        $candidateName = "Boot" + $idx.ToString("X4")
        $cBuf = New-Object byte[] 4096
        $cLen = [NativeUefiBootMgrClean]::GetFirmwareEnvironmentVariableW($candidateName, $efiGlobalGuid, $cBuf, 4096)
        if ($cLen -gt 6) {
            $str = [System.Text.Encoding]::Unicode.GetString($cBuf, 6, $cLen - 6)
            if ($str -match "UEFI Boot Manager" -or $str -match "BootMgr" -or $str -match "\\EFI\\BootMgr") {
                Write-Host "[INFO] Removing orphan NVRAM boot entry: $candidateName" -ForegroundColor Yellow
                [NativeUefiBootMgrClean]::SetFirmwareEnvironmentVariableExW($candidateName, $efiGlobalGuid, $null, 0, 0) | Out-Null
                $toRemove.Add($idx)
            }
        }
    }
}

# 4. Synchronize BootOrder
if ($toRemove.Count -gt 0) {
    foreach ($r in $toRemove) {
        $orders.Remove($r)
    }

    $newOrderBytes = New-Object byte[] ($orders.Count * 2)
    for ($i = 0; $i -lt $orders.Count; $i++) {
        [BitConverter]::GetBytes($orders[$i]).CopyTo($newOrderBytes, $i * 2)
    }

    $orderSetSuccess = [NativeUefiBootMgrClean]::SetFirmwareEnvironmentVariableExW("BootOrder", $efiGlobalGuid, $newOrderBytes, $newOrderBytes.Length, 7)
    if ($orderSetSuccess) {
        $newOrderStr = ($orders | ForEach-Object { "Boot" + $_.ToString("X4") }) -join " -> "
        Write-Host "[SUCCESS] NVRAM entries removed and BootOrder synchronized: $newOrderStr" -ForegroundColor Green
    } else {
        Write-Host "[WARN] Failed to write updated BootOrder to NVRAM." -ForegroundColor Yellow
    }
} else {
    Write-Host "[INFO] No BootMgr entries found in NVRAM."
}

# 5. Remove Files from EFI System Partition (ESP)
Write-Host "[INFO] Mounting EFI System Partition to drive S:..."
mountvol S: /S 2>$null
if (Test-Path "S:\") {
    $espTargetDir = "S:\EFI\BootMgr"
    if (Test-Path $espTargetDir) {
        Remove-Item -Path $espTargetDir -Recurse -Force
        Write-Host "[SUCCESS] Removed $espTargetDir from EFI System Partition." -ForegroundColor Green
    } else {
        Write-Host "[INFO] Directory $espTargetDir does not exist on ESP."
    }
    mountvol S: /D 2>$null
    Write-Host "[INFO] EFI System Partition dismounted."
} else {
    Write-Host "[WARN] Could not mount EFI System Partition to S:." -ForegroundColor Yellow
}

Write-Host "[SUCCESS] Cleanup operation completed successfully." -ForegroundColor Green
