# AMI Aptio V Native UEFI NVRAM Boot Option Injector for UEFI Boot Manager

$ErrorActionPreference = "Stop"

Write-Host "[INFO] Starting AMI Aptio V Native UEFI NVRAM installation for BootMgr..." -ForegroundColor Cyan

# 1. Locate Source Binary
$searchPaths = @(
    "$PSScriptRoot\BootMgr.efi",
    "$PSScriptRoot\uefi bootloader\BootMgr.efi",
    "C:\Users\elf\Desktop\uefi bootloader\BootMgr.efi",
    "C:\Users\elf\Desktop\uefi bootloader\uefi bootloader\BootMgr.efi"
)

$src = $null
foreach ($path in $searchPaths) {
    if (Test-Path $path) {
        $src = (Get-Item $path).FullName
        break
    }
}

if (-not $src) {
    Write-Host "[ERROR] BootMgr.efi binary not found in project directories." -ForegroundColor Red
    exit 1
}

Write-Host "[INFO] Source binary located: $src"

# 2. Mount ESP & Deploy File
Write-Host "[INFO] Mounting EFI System Partition to drive S:..."
mountvol S: /S 2>$null
if (-not (Test-Path "S:\")) {
    Write-Host "[ERROR] Failed to mount EFI System Partition to drive S:." -ForegroundColor Red
    exit 1
}

$destDir = "S:\EFI\BootMgr"
if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
}

$destFile = "$destDir\BootMgr.efi"
Copy-Item -Path $src -Destination $destFile -Force
mountvol S: /D 2>$null
Write-Host "[SUCCESS] Deployed binary to \EFI\BootMgr\BootMgr.efi on ESP." -ForegroundColor Green

# 3. Native Win32 Firmware API Definition
Write-Host "[INFO] Initializing Windows firmware environment API..."

$cSharpCode = @"
using System;
using System.Runtime.InteropServices;

public class NativeUefiBootMgr {
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
        tp.Attributes = 0x00000002; // SE_PRIVILEGE_ENABLED
        return AdjustTokenPrivileges(hToken, false, ref tp, 0, IntPtr.Zero, IntPtr.Zero);
    }
}
"@

Add-Type -TypeDefinition $cSharpCode -ErrorAction SilentlyContinue

if (-not [NativeUefiBootMgr]::EnableSystemEnvironmentPrivilege()) {
    Write-Host "[ERROR] Failed to acquire SeSystemEnvironmentPrivilege. Please run as Administrator." -ForegroundColor Red
    exit 1
}

$efiGlobalGuid = "{8be4df61-93ca-11d2-aa0d-00e098032b8c}"

# 4. Read BootOrder
Write-Host "[INFO] Querying motherboard BootOrder variable..."
$orderBuf = New-Object byte[] 512
$orderLen = [NativeUefiBootMgr]::GetFirmwareEnvironmentVariableW("BootOrder", $efiGlobalGuid, $orderBuf, 512)

if ($orderLen -eq 0) {
    $err = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
    Write-Host "[ERROR] Unable to read BootOrder from NVRAM (Win32 Error: $err)." -ForegroundColor Red
    exit 1
}

$bootList = New-Object System.Collections.Generic.List[uint16]
for ($i = 0; $i -lt $orderLen; $i += 2) {
    $bootList.Add([BitConverter]::ToUInt16($orderBuf, $i))
}

$orderStr = ($bootList | ForEach-Object { "Boot" + $_.ToString("X4") }) -join " -> "
Write-Host "[INFO] Current BootOrder: $orderStr"

# 5. Check for Existing BootMgr Entry
$existingIndex = $null
foreach ($b in $bootList) {
    $bName = "Boot" + $b.ToString("X4")
    $bBuf = New-Object byte[] 4096
    $bLen = [NativeUefiBootMgr]::GetFirmwareEnvironmentVariableW($bName, $efiGlobalGuid, $bBuf, 4096)
    if ($bLen -gt 6) {
        $str = [System.Text.Encoding]::Unicode.GetString($bBuf, 6, $bLen - 6)
        if ($str -match "UEFI Boot Manager" -or $str -match "\\EFI\\BootMgr\\BootMgr.efi") {
            $existingIndex = $b
            Write-Host "[INFO] Found existing BootMgr entry at $bName. Updating..." -ForegroundColor Yellow
            break
        }
    }
}

# 6. Clone Hard Drive Media Device Path from Primary Boot Entry
$hdNode = $null
foreach ($b in $bootList) {
    $entryName = "Boot" + $b.ToString("X4")
    $entryBuf = New-Object byte[] 4096
    $entryLen = [NativeUefiBootMgr]::GetFirmwareEnvironmentVariableW($entryName, $efiGlobalGuid, $entryBuf, 4096)
    if ($entryLen -lt 42) { continue }

    $descOffset = 6
    while ($descOffset -lt ($entryLen - 1) -and ($entryBuf[$descOffset] -ne 0 -or $entryBuf[$descOffset + 1] -ne 0)) {
        $descOffset += 2
    }
    $descOffset += 2

    if ($descOffset + 4 -le $entryLen) {
        $dpType = $entryBuf[$descOffset]
        $dpSubType = $entryBuf[$descOffset + 1]
        $dpLen = [BitConverter]::ToUInt16($entryBuf, $descOffset + 2)

        if ($dpType -eq 4 -and $dpSubType -eq 1 -and $dpLen -eq 42) {
            $hdNode = New-Object byte[] 42
            [Array]::Copy($entryBuf, $descOffset, $hdNode, 0, 42)
            Write-Host "[INFO] Extracted motherboard Hard Drive Device Path from $entryName."
            break
        }
    }
}

if (-not $hdNode) {
    $firstBootName = "Boot" + $bootList[0].ToString("X4")
    $firstBuf = New-Object byte[] 4096
    $firstLen = [NativeUefiBootMgr]::GetFirmwareEnvironmentVariableW($firstBootName, $efiGlobalGuid, $firstBuf, 4096)
    $descOffset = 6
    while ($descOffset -lt ($firstLen - 1) -and ($firstBuf[$descOffset] -ne 0 -or $firstBuf[$descOffset + 1] -ne 0)) {
        $descOffset += 2
    }
    $descOffset += 2
    $hdNode = New-Object byte[] 42
    [Array]::Copy($firstBuf, $descOffset, $hdNode, 0, 42)
}

# 7. Construct FilePath Node: \EFI\BootMgr\BootMgr.efi
$filePathStr = "\EFI\BootMgr\BootMgr.efi"
$filePathBytes = [System.Text.Encoding]::Unicode.GetBytes($filePathStr + [char]0)
$fileNodeLen = 4 + $filePathBytes.Length
$fileNode = New-Object byte[] $fileNodeLen
$fileNode[0] = 4 # MEDIA_DEVICE_PATH
$fileNode[1] = 4 # MEDIA_FILEPATH_DP
[BitConverter]::GetBytes([uint16]$fileNodeLen).CopyTo($fileNode, 2)
$filePathBytes.CopyTo($fileNode, 4)

# End Entire Device Path Node (Type=0x7F, SubType=0xFF, Length=4)
$endNode = [byte[]]@(0x7F, 0xFF, 0x04, 0x00)
$totalDpLen = $hdNode.Length + $fileNodeLen + 4

# 8. Construct EFI_LOAD_OPTION Structure
$loadOpt = New-Object System.Collections.Generic.List[byte]
$loadOpt.AddRange([BitConverter]::GetBytes([uint32]1)) # LOAD_OPTION_ACTIVE
$loadOpt.AddRange([BitConverter]::GetBytes([uint16]$totalDpLen))
$descBytes = [System.Text.Encoding]::Unicode.GetBytes("UEFI Boot Manager" + [char]0)
$loadOpt.AddRange($descBytes)
$loadOpt.AddRange($hdNode)
$loadOpt.AddRange($fileNode)
$loadOpt.AddRange($endNode)

$loadOptBytes = $loadOpt.ToArray()

# 9. Determine Boot Index
$targetIndex = $existingIndex
if (-not $targetIndex) {
    $targetIndex = 16 # Start from Boot0010
    while ($bootList.Contains($targetIndex)) {
        $targetIndex++
    }
}
$targetBootName = "Boot" + $targetIndex.ToString("X4")

# 10. Write Load Option to NVRAM (Attributes = 7: NON_VOLATILE | BOOTSERVICE | RUNTIME)
$setSuccess = [NativeUefiBootMgr]::SetFirmwareEnvironmentVariableExW($targetBootName, $efiGlobalGuid, $loadOptBytes, $loadOptBytes.Length, 7)

if (-not $setSuccess) {
    $err = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
    Write-Host "[ERROR] Failed to write $targetBootName to NVRAM (Win32 Error: $err)." -ForegroundColor Red
    exit 1
}

Write-Host "[SUCCESS] Registered NVRAM boot option: $targetBootName (UEFI Boot Manager)" -ForegroundColor Green

# 11. Update Motherboard BootOrder (Set BootMgr as #1 Priority)
$newOrderList = New-Object System.Collections.Generic.List[uint16]
$newOrderList.Add($targetIndex)
foreach ($b in $bootList) {
    if ($b -ne $targetIndex) {
        $newOrderList.Add($b)
    }
}

$newOrderBytes = New-Object byte[] ($newOrderList.Count * 2)
for ($i = 0; $i -lt $newOrderList.Count; $i++) {
    [BitConverter]::GetBytes($newOrderList[$i]).CopyTo($newOrderBytes, $i * 2)
}

$orderSetSuccess = [NativeUefiBootMgr]::SetFirmwareEnvironmentVariableExW("BootOrder", $efiGlobalGuid, $newOrderBytes, $newOrderBytes.Length, 7)

if ($orderSetSuccess) {
    $newOrderStr = ($newOrderList | ForEach-Object { "Boot" + $_.ToString("X4") }) -join " -> "
    Write-Host "[SUCCESS] Motherboard BootOrder updated successfully: $newOrderStr" -ForegroundColor Green
    Write-Host "[INFO] UEFI Boot Manager is now set as the primary boot target." -ForegroundColor Green
} else {
    Write-Host "[ERROR] Failed to update BootOrder variable in NVRAM." -ForegroundColor Red
    exit 1
}
