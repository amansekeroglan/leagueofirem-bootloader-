#include "OsBoot.h"
#include "Graphics.h"
#include "FirmwareSetup.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseLib.h>
#include <Protocol/SimpleFileSystem.h>

STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorBlack = { 0x00, 0x00, 0x00, 0x00 };

typedef struct {
  CONST CHAR16  *Name;
  CONST CHAR16  *Path;
  OS_ENTRY_TYPE Type;
} OS_PROBE;

STATIC CONST OS_PROBE mKnownProbes[] = {
  { L"Windows",             L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi", OsTypeWindows },
  { L"Ubuntu",              L"\\EFI\\ubuntu\\shimx64.efi",           OsTypeLinux },
  { L"Ubuntu",              L"\\EFI\\ubuntu\\grubx64.efi",           OsTypeLinux },
  { L"Arch Linux",          L"\\EFI\\arch\\grubx64.efi",             OsTypeLinux },
  { L"Linux Mint",          L"\\EFI\\linuxmint\\shimx64.efi",        OsTypeLinux },
  { L"Linux Mint",          L"\\EFI\\linuxmint\\grubx64.efi",        OsTypeLinux },
  { L"Fedora",              L"\\EFI\\fedora\\shimx64.efi",           OsTypeLinux },
  { L"Fedora",              L"\\EFI\\fedora\\grubx64.efi",           OsTypeLinux },
  { L"Debian",              L"\\EFI\\debian\\shimx64.efi",           OsTypeLinux },
  { L"Debian",              L"\\EFI\\debian\\grubx64.efi",           OsTypeLinux },
  { L"Pop!_OS",             L"\\EFI\\Pop_OS\\systemd-bootx64.efi",   OsTypeLinux },
  { L"openSUSE",            L"\\EFI\\opensuse\\shim.efi",            OsTypeLinux },
  { L"openSUSE",            L"\\EFI\\opensuse\\grubx64.efi",         OsTypeLinux },
  { L"systemd-boot",        L"\\EFI\\systemd\\systemd-bootx64.efi",  OsTypeLinux },
  { L"GRUB Bootloader",     L"\\EFI\\grub\\grubx64.efi",             OsTypeLinux },
  { L"UEFI Fallback Boot",  L"\\EFI\\BOOT\\BOOTX64.EFI",             OsTypeLinux }
};

STATIC
BOOLEAN
HasDistroOnHandle (
  IN CONST OS_BOOT_ENTRY *Entries,
  IN UINTN               Count,
  IN EFI_HANDLE          DeviceHandle,
  IN CONST CHAR16        *Name
  )
{
  UINTN Index;

  for (Index = 0; Index < Count; Index++) {
    if (Entries[Index].DeviceHandle == DeviceHandle) {
      if (StrCmp (Entries[Index].Name, Name) == 0) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

EFI_STATUS
EFIAPI
OsScanAll (
  OUT OS_BOOT_ENTRY *Entries,
  IN  UINTN         MaxEntries,
  OUT UINTN         *Count
  )
{
  EFI_STATUS Status;
  UINTN HandleCount;
  EFI_HANDLE *HandleBuffer;
  UINTN HandleIdx;
  UINTN ProbeIdx;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
  EFI_FILE_PROTOCOL *RootDir;
  EFI_FILE_PROTOCOL *BootFile;
  EFI_FILE_PROTOCOL *SelfCheck;
  UINTN TotalFound;

  if (Entries == NULL || Count == NULL || MaxEntries < 2) {
    return EFI_INVALID_PARAMETER;
  }

  TotalFound = 0;
  HandleCount = 0;
  HandleBuffer = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    // If scanning fails, still append Firmware Settings
    StrCpy (Entries[TotalFound].Name, L"Firmware Settings");
    Entries[TotalFound].Path[0] = 0;
    Entries[TotalFound].DeviceHandle = NULL;
    Entries[TotalFound].Type = OsTypeFirmwareSettings;
    TotalFound++;
    *Count = TotalFound;
    return Status;
  }

  for (HandleIdx = 0; HandleIdx < HandleCount; HandleIdx++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[HandleIdx],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID **)&Fs
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = Fs->OpenVolume (Fs, &RootDir);
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (ProbeIdx = 0; ProbeIdx < (sizeof (mKnownProbes) / sizeof (mKnownProbes[0])); ProbeIdx++) {
      if (TotalFound >= (MaxEntries - 1)) {
        break;
      }

      // If we already added this OS name on this specific partition, skip fallback paths
      if (HasDistroOnHandle (Entries, TotalFound, HandleBuffer[HandleIdx], mKnownProbes[ProbeIdx].Name)) {
        continue;
      }

      // Check if file exists
      Status = RootDir->Open (
                          RootDir,
                          &BootFile,
                          (CHAR16 *)mKnownProbes[ProbeIdx].Path,
                          EFI_FILE_MODE_READ,
                          0
                          );
      if (EFI_ERROR (Status)) {
        continue;
      }

      BootFile->Close (BootFile);

      // Self-exclusion check for generic fallback (\EFI\BOOT\BOOTX64.EFI)
      if (StrCmp (mKnownProbes[ProbeIdx].Path, L"\\EFI\\BOOT\\BOOTX64.EFI") == 0) {
        // If this partition has our dedicated BootMgr directory, BOOTX64.EFI is either us or duplicate
        Status = RootDir->Open (
                            RootDir,
                            &SelfCheck,
                            L"\\EFI\\BootMgr\\BootMgr.efi",
                            EFI_FILE_MODE_READ,
                            0
                            );
        if (!EFI_ERROR (Status)) {
          SelfCheck->Close (SelfCheck);
          continue;
        }

        // If Windows bootmgrfw is already on this partition, skip fallback BOOTX64.EFI
        if (HasDistroOnHandle (Entries, TotalFound, HandleBuffer[HandleIdx], L"Windows")) {
          continue;
        }
      }

      // Add valid detected OS entry
      StrCpy (Entries[TotalFound].Name, mKnownProbes[ProbeIdx].Name);
      StrCpy (Entries[TotalFound].Path, mKnownProbes[ProbeIdx].Path);
      Entries[TotalFound].DeviceHandle = HandleBuffer[HandleIdx];
      Entries[TotalFound].Type = mKnownProbes[ProbeIdx].Type;
      TotalFound++;
    }

    RootDir->Close (RootDir);
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  // Always append Firmware Settings as the last entry
  StrCpy (Entries[TotalFound].Name, L"Firmware Settings");
  Entries[TotalFound].Path[0] = 0;
  Entries[TotalFound].DeviceHandle = NULL;
  Entries[TotalFound].Type = OsTypeFirmwareSettings;
  TotalFound++;

  *Count = TotalFound;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OsLaunch (
  IN OS_BOOT_ENTRY *Entry,
  IN EFI_HANDLE    ParentImageHandle
  )
{
  EFI_STATUS Status;
  EFI_DEVICE_PATH_PROTOCOL *FilePath;
  EFI_HANDLE LoadedImage;

  if (Entry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Entry->Type == OsTypeFirmwareSettings) {
    return FirmwareSetupEnter ();
  }

  if (Entry->DeviceHandle == NULL || Entry->Path[0] == 0) {
    return EFI_NOT_FOUND;
  }

  FilePath = FileDevicePath (
               Entry->DeviceHandle,
               Entry->Path
               );
  if (FilePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  LoadedImage = NULL;
  Status = gBS->LoadImage (
                  FALSE,
                  ParentImageHandle,
                  FilePath,
                  NULL,
                  0,
                  &LoadedImage
                  );
  FreePool (FilePath);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Instant seamless transition: pure black fill & hide cursor (matches systemd-boot)
  GraphicsClear (mColorBlack);

  if (gST->ConOut != NULL) {
    gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  }

  Status = gBS->StartImage (LoadedImage, NULL, NULL);

  return Status;
}

