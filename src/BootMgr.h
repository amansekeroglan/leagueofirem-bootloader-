#ifndef BOOTMGR_H
#define BOOTMGR_H

#include <Uefi.h>
#include "OsBoot.h"

#define BOOT_TIMEOUT_SECONDS 10

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  );

#endif
