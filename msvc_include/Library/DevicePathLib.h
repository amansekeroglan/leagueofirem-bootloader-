#pragma once
#include <Uefi.h>

EFI_DEVICE_PATH_PROTOCOL*
EFIAPI
FileDevicePath (
  IN EFI_HANDLE    Device OPTIONAL,
  IN CONST CHAR16  *FileName
  );

