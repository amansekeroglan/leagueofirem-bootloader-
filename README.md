# Custom UEFI Boot Manager

A lightweight, high-performance stand-alone x86-64 UEFI Boot Manager distributed as a PE32+ `.efi` executable. Features dynamic resolution scaling, custom full-screen wallpaper, transparent logo rendering, event-driven countdown, and direct Windows bootchain integration.

Inspired by the design philosophy of `systemd-boot`.

---

## 1. Features

- **Minimalist GUI**: Rendered directly via UEFI Graphics Output Protocol (GOP). Dark slate aesthetic, dynamic resolution scaling (auto-adapts to 1080p, 1440p, 4K, and 768p displays), centered menu with subtle selection highlighting.
- **Two Menu Entries**:
  - `Windows` (Default)
  - `Firmware Settings`

---

## 2. Project Structure

```
.
├── BootMgr.c                  # Application entry point & main UI event loop
├── BootMgr.h                  # Boot options and configuration definitions
├── BootMgr.inf                # EDK2 module definition
├── BootMgr.dsc                # EDK2 platform description
├── Graphics.c                 # GOP blitting, scaling engine, alpha blending & fonts
├── Graphics.h                 # Graphics subsystem interface
├── Input.c                    # Non-blocking keyboard input handler
├── Input.h                    # Input interface declarations
├── Timer.c                    # UEFI event timer management
├── Timer.h                    # Timer interface declarations
├── WindowsBoot.c              # Dynamic ESP partition discovery and instant launch
├── WindowsBoot.h              # Windows bootloader loader interface
├── FirmwareSetup.c            # UEFI OsIndications variable management & reboot
├── FirmwareSetup.h            # Firmware setup interface
├── Helpers.h                  # Hardware intrinsic prototypes
├── Helpers.nasm               # x86-64 NASM low-level routines (EDK2 build)
├── msvc_support.c             # Freestanding runtime support & intrinsics for MSVC
├── msvc_include/              # Freestanding UEFI standard headers for MSVC
├── build_efi.bat              # Native Windows MSVC 1-click build script
├── build.sh                   # Linux EDK2 compilation script
├── AMI_NVRAM_BootMgr_Kur.bat  # 1-Click installer for AMI Aptio V NVRAM (Windows)
├── AMI_NVRAM_BootMgr_Sil.bat  # 1-Click uninstaller for AMI Aptio V NVRAM (Windows)
├── ami_nvram.ps1              # NVRAM injection implementation
├── ami_nvram_clean.ps1        # NVRAM removal implementation
├── gen_wallpaper_1080.ps1     # Asset converter: PNG wallpaper -> COFF object
├── gen_logo.ps1               # Asset converter: PNG logo -> COFF object
├── tatlimirem.png             # Full-screen background wallpaper asset
├── leagueofirem.png           # Transparent logo overlay asset
├── BootMgr.efi                # Precompiled x64 PE32+ UEFI binary
├── LICENSE                    # MIT License
└── README.md                  # Project documentation
```

---

## 3. Building

### Option A: Windows (MSVC - Recommended)

Requires **Visual Studio 2022** or **Visual Studio 2026** (Community / Professional / Build Tools) with Desktop Development with C++. No EDK2 setup is required.

Simply run:
```cmd
build_efi.bat
```

The script will:
1. Initialize the x64 developer environment via `vcvars64.bat`.
2. Automatically convert `tatlimirem.png` and `leagueofirem.png` into linkable COFF object files (`wallpaper.obj`, `logo.obj`).
3. Compile all C source files with freestanding flags (`/GS-`, `/O2`, `/TC`).
4. Link `BootMgr.efi` via `link.exe /subsystem:efi_application`.
5. Clean up temporary build artifacts automatically.

### Option B: Linux (Tianocore EDK2)

Requires `gcc`, `nasm`, `python3`, and an EDK2 workspace:
```bash
./build.sh
```

Alternatively, link the package manually inside your EDK2 workspace:
```bash
export WORKSPACE=/path/to/edk2
ln -sfn "$(pwd)" "$WORKSPACE/BootMgrPkg"
cd "$WORKSPACE"
source edksetup.sh
build -a X64 -t GCC -p BootMgrPkg/BootMgr.dsc -b RELEASE
```

---

## 4. Deployment

### Method A: AMI Aptio V Motherboards (Automated 1-Click)

On Windows (run as Administrator):
- **To Install & Set as Primary Boot**: Double-click `AMI_NVRAM_BootMgr_Kur.bat`.
  - Automatically mounts the EFI System Partition (ESP) to `S:`.
  - Copies `BootMgr.efi` to `\EFI\BootMgr\BootMgr.efi`.
  - Reads Windows Boot Manager's Media Device Path node, creates a new NVRAM entry (`BootXXXX`), and prepends it to `BootOrder`.
- **To Uninstall & Restore**: Double-click `AMI_NVRAM_BootMgr_Sil.bat`.
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
