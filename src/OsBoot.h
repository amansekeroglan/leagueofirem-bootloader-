#ifndef OS_BOOT_H
#define OS_BOOT_H

#include <Uefi.h>

#define MAX_OS_ENTRIES    16
#define MAX_OS_NAME_LEN   48
#define MAX_OS_PATH_LEN   128

typedef enum {
  OsTypeWindows,
  OsTypeLinux,
  OsTypeFirmwareSettings
} OS_ENTRY_TYPE;

typedef struct {
  CHAR16          Name[MAX_OS_NAME_LEN];
  CHAR16          Path[MAX_OS_PATH_LEN];
  EFI_HANDLE      DeviceHandle;
  OS_ENTRY_TYPE   Type;
} OS_BOOT_ENTRY;

EFI_STATUS
EFIAPI
OsScanAll (
  OUT OS_BOOT_ENTRY *Entries,
  IN  UINTN         MaxEntries,
  OUT UINTN         *Count
  );

EFI_STATUS
EFIAPI
OsLaunch (
  IN OS_BOOT_ENTRY *Entry,
  IN EFI_HANDLE    ParentImageHandle
  );

#endif

