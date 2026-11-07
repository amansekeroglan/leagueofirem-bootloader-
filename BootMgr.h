#ifndef BOOTMGR_H
#define BOOTMGR_H

#include <Uefi.h>

#define BOOT_TIMEOUT_SECONDS 10

#define BOOT_OPTION_WINDOWS          0
#define BOOT_OPTION_FIRMWARE_SETUP   1
#define BOOT_OPTION_COUNT            2

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  );

#endif
