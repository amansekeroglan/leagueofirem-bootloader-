#ifndef HELPERS_H
#define HELPERS_H

#include <Uefi.h>

VOID
EFIAPI
BootMgrCpuPause (
  VOID
  );

VOID
EFIAPI
BootMgrMemoryBarrier (
  VOID
  );

UINT64
EFIAPI
BootMgrReadTsc (
  VOID
  );

#endif
