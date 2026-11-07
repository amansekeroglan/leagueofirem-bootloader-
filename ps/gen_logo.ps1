Add-Type -AssemblyName System.Drawing

$scriptDir = $PSScriptRoot
if (-not $scriptDir) { $scriptDir = (Get-Location).Path }

$possibleImgPaths = @(
    (Join-Path $scriptDir "..\picture\leagueofirem.png"),
    (Join-Path $scriptDir "picture\leagueofirem.png"),
    (Join-Path $scriptDir "leagueofirem.png")
)

$imgPath = $null
foreach ($p in $possibleImgPaths) {
    if (Test-Path $p) {
        $imgPath = (Get-Item $p).FullName
        break
    }
}

$srcDir = Join-Path $scriptDir "..\src"
if (-not (Test-Path $srcDir)) { $srcDir = $scriptDir }

$binPath = Join-Path $srcDir "logo.bin"
$objPath = Join-Path $srcDir "logo.obj"

if (-not $imgPath) {
    Write-Host "[ERROR] Logo image not found in picture directory." -ForegroundColor Red
    exit 1
}

$img = [System.Drawing.Bitmap]::FromFile($imgPath)
$w = $img.Width
$h = $img.Height
Write-Host "Logo dimensions: ${w}x${h}"

# Extract raw 32-bit BGRA pixel data
$bytes = New-Object byte[] ($w * $h * 4)
$rect = New-Object System.Drawing.Rectangle(0, 0, $w, $h)
$bmpData = $img.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
[System.Runtime.InteropServices.Marshal]::Copy($bmpData.Scan0, $bytes, 0, $bytes.Length)
$img.UnlockBits($bmpData)
$img.Dispose()

[System.IO.File]::WriteAllBytes($binPath, $bytes)
Write-Host "logo.bin written: $($bytes.Length) bytes"

function New-CoffObjectFile {
    param(
        [string]$BinaryPath,
        [string]$SymbolName,
        [string]$OutputPath
    )

    $rawBytes = [System.IO.File]::ReadAllBytes($BinaryPath)
    $dataLen = $rawBytes.Length

    $hdrSize = 60
    $dataOffset = $hdrSize
    $symOffset = $hdrSize + $dataLen

    $pad = (4 - ($dataLen % 4)) % 4
    $symOffset += $pad

    $ms = New-Object System.IO.MemoryStream
    $bw = New-Object System.IO.BinaryWriter($ms)

    $bw.Write([uint16]0x8664)          # Machine = AMD64
    $bw.Write([uint16]1)               # NumberOfSections = 1
    $bw.Write([uint32]0)               # TimeDateStamp = 0
    $bw.Write([uint32]$symOffset)      # PointerToSymbolTable
    $bw.Write([uint32]1)               # NumberOfSymbols = 1
    $bw.Write([uint16]0)               # SizeOfOptionalHeader = 0
    $bw.Write([uint16]0)               # Characteristics = 0

    $secName = [System.Text.Encoding]::ASCII.GetBytes(".rdata`0`0")
    $bw.Write($secName, 0, 8)          # Name
    $bw.Write([uint32]0)               # VirtualSize = 0
    $bw.Write([uint32]0)               # VirtualAddress = 0
    $bw.Write([uint32]$dataLen)        # SizeOfRawData
    $bw.Write([uint32]$dataOffset)     # PointerToRawData
    $bw.Write([uint32]0)               # PointerToRelocations = 0
    $bw.Write([uint32]0)               # PointerToLinenumbers = 0
    $bw.Write([uint16]0)               # NumberOfRelocations = 0
    $bw.Write([uint16]0)               # NumberOfLinenumbers = 0
    $bw.Write([uint32]0x40500040)      # Initialized, Read-Only, 16-byte align

    $bw.Write($rawBytes, 0, $dataLen)
    for ($i = 0; $i -lt $pad; $i++) { $bw.Write([byte]0) }

    $symBytes = [System.Text.Encoding]::ASCII.GetBytes($SymbolName)
    if ($symBytes.Length -le 8) {
        $bw.Write($symBytes, 0, $symBytes.Length)
        for ($i = $symBytes.Length; $i -lt 8; $i++) { $bw.Write([byte]0) }
    } else {
        $bw.Write([uint32]0)
        $bw.Write([uint32]4)
    }

    $bw.Write([uint32]0)
    $bw.Write([int16]1)
    $bw.Write([uint16]0)
    $bw.Write([byte]2)                 # External
    $bw.Write([byte]0)

    if ($symBytes.Length -gt 8) {
        $strTableLen = 4 + $symBytes.Length + 1
        $bw.Write([uint32]$strTableLen)
        $bw.Write($symBytes, 0, $symBytes.Length)
        $bw.Write([byte]0)
    } else {
        $bw.Write([uint32]4)
    }

    [System.IO.File]::WriteAllBytes($OutputPath, $ms.ToArray())
    $bw.Close()
    $ms.Close()
    Write-Host "COFF Object generated: $OutputPath ($([math]::Round((Get-Item $OutputPath).Length / 1KB, 1)) KB)"

    # Clean temporary raw bin file
    if (Test-Path $BinaryPath) {
        Remove-Item -Path $BinaryPath -Force -ErrorAction SilentlyContinue
    }
}

New-CoffObjectFile -BinaryPath $binPath -SymbolName "g_LogoData" -OutputPath $objPath
