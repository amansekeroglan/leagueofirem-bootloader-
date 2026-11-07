#ifndef WINDOWS_BOOT_H
#define WINDOWS_BOOT_H

#include <Uefi.h>

EFI_STATUS
EFIAPI
WindowsBootLaunch (
  IN EFI_HANDLE ParentImageHandle
  );

#endif
