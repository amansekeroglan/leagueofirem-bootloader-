#include "WindowsBoot.h"
#include "Graphics.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/SimpleFileSystem.h>

STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorBlack = { 0x00, 0x00, 0x00, 0x00 };

EFI_STATUS
EFIAPI
WindowsBootLaunch (
  IN EFI_HANDLE ParentImageHandle
  )
{
  EFI_STATUS Status;
  UINTN HandleCount;
  EFI_HANDLE *HandleBuffer;
  UINTN Index;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
  EFI_FILE_PROTOCOL *RootDir;
  EFI_FILE_PROTOCOL *BootFile;
  EFI_HANDLE TargetHandle;
  EFI_DEVICE_PATH_PROTOCOL *FilePath;
  EFI_HANDLE LoadedImage;

  HandleCount = 0;
  HandleBuffer = NULL;
  TargetHandle = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
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

    Status = RootDir->Open (
                        RootDir,
                        &BootFile,
                        L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi",
                        EFI_FILE_MODE_READ,
                        0
                        );
    if (!EFI_ERROR (Status)) {
      BootFile->Close (BootFile);
      RootDir->Close (RootDir);
      TargetHandle = HandleBuffer[Index];
      break;
    }

    RootDir->Close (RootDir);
  }

  FreePool (HandleBuffer);

  if (TargetHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  FilePath = FileDevicePath (
               TargetHandle,
               L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi"
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

  // Instant seamless transition: clear screen to pure black and hide cursor
  // Matches systemd-boot handoff behavior without any curtain scanline artifacts
  GraphicsClear (mColorBlack);

  if (gST->ConOut != NULL) {
    gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  }

  Status = gBS->StartImage (LoadedImage, NULL, NULL);

  return Status;
}
