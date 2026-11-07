#ifndef INPUT_H
#define INPUT_H

#include <Uefi.h>

typedef enum {
  KeyActionNone = 0,
  KeyActionUp,
  KeyActionDown,
  KeyActionSelect,
  KeyActionOther
} KEY_ACTION;

EFI_STATUS
EFIAPI
InputInit (
  VOID
  );

EFI_STATUS
EFIAPI
InputPollKey (
  OUT KEY_ACTION *Action
  );

VOID
EFIAPI
InputWaitAnyKey (
  VOID
  );

EFI_EVENT
EFIAPI
InputGetWaitEvent (
  VOID
  );

#endif
