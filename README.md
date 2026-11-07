# Custom UEFI Boot Manager - Inspired As My Love "İrem Aras"

A lightweight, high-performance stand-alone x86-64 UEFI Boot Manager distributed as a PE32+ `.efi` executable. Features dynamic resolution scaling, custom full-screen wallpaper, transparent logo rendering, event-driven countdown, and direct Windows bootchain integration.

Inspired by the design philosophy of `systemd-boot`.

---

## 1. Features

- **Dynamic GOP Graphics Engine**:
  - Full-screen wallpaper support with 16.16 fixed-point bilinear aspect scaling (auto-adapts to 1080p, 1440p, 4K, 768p, and custom VM resolutions).
  - Transparent logo rendering with per-pixel alpha-blending and dynamic proportional scaling (symmetrically positioned above menu entries without overlapping).
  - Modern card/pill UI design with high-contrast borders and sharp corners for crystal-clear readability over any background image.
  - Zero-flicker sub-region restoring (`GraphicsRestoreRegion`) during menu navigation and countdown ticks.

- **Multi-OS Discovery Engine (rEFInd-Style)**:
  - Scans all storage devices and partitions across NVMe, SATA, and USB media via `EFI_SIMPLE_FILE_SYSTEM_PROTOCOL`.
  - Automatically identifies installed OS loaders: Windows, Ubuntu, Arch Linux, Linux Mint, Fedora, Debian, Pop!_OS, openSUSE, systemd-boot, and GRUB.
  - Dynamically renders uniform button cards for all detected operating systems.
  - Symmetrically places "Firmware Settings" at the bottom of the list.

- **Fluid Instant Boot (systemd-boot style)**:
  - Direct execution of the selected OS image without slow GOP re-initialization or scanline curtain wipe effects.

- **Persistent Auto-Boot Countdown**:
  - Automatically launches the selected entry upon timer expiration (default: 10 seconds).
  - Continuous timer state: navigating options with arrow keys updates the target description without interrupting or resetting the countdown.
  - Non-blocking UEFI timer events (`CreateEvent` + `SetTimer` + `WaitForEvent`) with CPU pause instructions (`_mm_pause`) for low power consumption.

- **1-Click AMI Aptio V NVRAM Integration**:
  - Native Windows batch and PowerShell scripts using Win32 Firmware APIs (`GetFirmwareEnvironmentVariableW` / `SetFirmwareEnvironmentVariableExW`).
  - Clones hardware media device paths, creates boot options, and prioritizes BootMgr in `BootOrder` without requiring third-party tools.
  - Includes full clean-up script to safely revert NVRAM entries.

- **Dual Build Support**:
  - **Native Windows MSVC Build**: Compiles freestanding UEFI binaries using MSVC `cl.exe` and `link.exe` (no EDK2 installation required).
  - **Linux EDK2 Build**: Tianocore EDK2 standard build via GCC and NASM.

---

## 2. Project Structure

```
.
├── bat/                       # Windows batch automation scripts
│   ├── AMI_NVRAM_BootMgr_Kur.bat # 1-Click installer for AMI Aptio V NVRAM
│   ├── AMI_NVRAM_BootMgr_Sil.bat # 1-Click uninstaller for AMI Aptio V NVRAM
│   └── build_efi.bat             # Native Windows MSVC 1-click build script
│
├── picture/                   # Graphical assets
│   ├── tatlimirem.png            # Full-screen background wallpaper asset
│   └── leagueofirem.png          # Transparent logo overlay asset
│
├── ps/                        # PowerShell helper and asset conversion scripts
│   ├── ami_nvram.ps1             # NVRAM injection implementation
│   ├── ami_nvram_clean.ps1       # NVRAM removal implementation
│   ├── gen_logo.ps1              # Asset converter: PNG logo -> COFF object
│   └── gen_wallpaper_1080.ps1    # Asset converter: PNG wallpaper -> COFF object
│
├── src/                       # Source code and build definitions
│   ├── BootMgr.c                 # Application entry point & main UI event loop
│   ├── BootMgr.h                 # Boot options and configuration definitions
│   ├── BootMgr.inf               # EDK2 module definition
│   ├── BootMgr.dsc               # EDK2 platform description
│   ├── Graphics.c                # GOP blitting, scaling engine, alpha blending & fonts
│   ├── Graphics.h                # Graphics subsystem interface
│   ├── Input.c                   # Non-blocking keyboard input handler
│   ├── Input.h                   # Input interface declarations
│   ├── Timer.c                   # UEFI event timer management
│   ├── Timer.h                   # Timer interface declarations
│   ├── OsBoot.c                  # Multi-OS detection & chainloading engine
│   ├── OsBoot.h                  # OS bootloader structures and signatures
│   ├── FirmwareSetup.c           # UEFI OsIndications variable management & reboot
│   ├── FirmwareSetup.h           # Firmware setup interface
│   ├── Helpers.h                 # Hardware intrinsic prototypes
│   ├── Helpers.nasm              # x86-64 NASM low-level routines (EDK2 build)
│   ├── msvc_support.c            # Freestanding runtime support & intrinsics for MSVC
│   ├── msvc_include/             # Freestanding UEFI standard headers for MSVC
│   └── build.sh                  # Linux EDK2 compilation script
│
├── BootMgr.efi                # Precompiled x64 PE32+ UEFI executable (~5.68 MB)
├── LICENSE                    # MIT License
├── .gitignore                 # Git ignore rules
└── README.md                  # Project documentation
```

---

## 3. Building

### Option A: Windows (MSVC - Recommended)

Requires **Visual Studio 2022** or **Visual Studio 2026** (Community / Professional / Build Tools) with Desktop Development with C++. No EDK2 setup is required.

Run the build script inside `bat/`:
```cmd
bat\build_efi.bat
```

The script will:
1. Initialize the x64 developer environment via `vcvars64.bat`.
2. Automatically convert images in `picture/` into linkable COFF object files (`wallpaper.obj`, `logo.obj`) inside `src/`.
3. Compile all C source files inside `src/` with freestanding flags (`/GS-`, `/O2`, `/TC`).
4. Link `BootMgr.efi` at the project root via `link.exe /subsystem:efi_application`.
5. Clean up temporary build artifacts automatically.

### Option B: Linux (Tianocore EDK2)

Requires `gcc`, `nasm`, `python3`, and an EDK2 workspace:
```bash
./src/build.sh
```

Alternatively, link the package manually inside your EDK2 workspace:
```bash
export WORKSPACE=/path/to/edk2
ln -sfn "$(pwd)/src" "$WORKSPACE/BootMgrPkg"
cd "$WORKSPACE"
source edksetup.sh
build -a X64 -t GCC -p BootMgrPkg/BootMgr.dsc -b RELEASE
```

---

## 4. Deployment

### Method A: AMI Aptio V Motherboards (Automated 1-Click)

On Windows (run as Administrator):
- **To Install & Set as Primary Boot**: Double-click `bat\AMI_NVRAM_BootMgr_Kur.bat`.
  - Automatically mounts the EFI System Partition (ESP) to `S:`.
  - Copies `BootMgr.efi` to `\EFI\BootMgr\BootMgr.efi`.
  - Reads Windows Boot Manager's Media Device Path node, creates a new NVRAM entry (`BootXXXX`), and prepends it to `BootOrder`.
- **To Uninstall & Restore**: Double-click `bat\AMI_NVRAM_BootMgr_Sil.bat`.
  - Safely deletes the NVRAM entry, updates `BootOrder`, and removes files from the ESP.

### Method B: Manual EFI System Partition Deployment

1. Mount your EFI System Partition (ESP):
   - **Windows**: `mountvol S: /S`
   - **Linux**: `sudo mount /dev/nvme0n1p1 /boot/efi`
2. Copy `BootMgr.efi`:
   ```cmd
   mkdir S:\EFI\BootMgr
   copy BootMgr.efi S:\EFI\BootMgr\BootMgr.efi
   ```
3. Register using `efibootmgr` on Linux:
   ```bash
   sudo efibootmgr --create --disk /dev/nvme0n1 --part 1 \
     --label "Boot Manager" \
     --loader '\EFI\BootMgr\BootMgr.efi'
   ```

### Method C: Bootable USB Drive

Format a flash drive with FAT32 and copy the binary to the default fallback path:
```
USB:\EFI\BOOT\BOOTX64.EFI
```

---

## 5. Secure Boot

Standalone binaries compiled from source are unsigned.
- If Secure Boot is enabled in your UEFI firmware, either:
  1. Disable Secure Boot in motherboard settings, or
  2. Sign `BootMgr.efi` using custom keys (`sbsign`) and enroll your key in the firmware DB / MOK.

---

## 6. License

This project is licensed under the [MIT License](LICENSE).
