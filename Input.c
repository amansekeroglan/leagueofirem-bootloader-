#include "Input.h"
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
EFIAPI
InputInit (
  VOID
  )
{
  EFI_INPUT_KEY DummyKey;

  if (gST->ConIn == NULL) {
    return EFI_DEVICE_ERROR;
  }

  while (!EFI_ERROR (gST->ConIn->ReadKeyStroke (gST->ConIn, &DummyKey))) {
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InputPollKey (
  OUT KEY_ACTION *Action
  )
{
  EFI_STATUS Status;
  EFI_INPUT_KEY Key;

  if (Action == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Action = KeyActionNone;

  if (gST->ConIn == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Key.ScanCode == SCAN_UP) {
    *Action = KeyActionUp;
  } else if (Key.ScanCode == SCAN_DOWN) {
    *Action = KeyActionDown;
  } else if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN || Key.UnicodeChar == CHAR_LINEFEED) {
    *Action = KeyActionSelect;
  } else {
    *Action = KeyActionOther;
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
InputWaitAnyKey (
  VOID
  )
{
  UINTN Index;
  EFI_INPUT_KEY Key;

  if (gST->ConIn == NULL) {
    return;
  }

  while (!EFI_ERROR (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key))) {
  }

  gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
  gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
}

EFI_EVENT
EFIAPI
InputGetWaitEvent (
  VOID
  )
{
  if (gST->ConIn == NULL) {
    return NULL;
  }
  return gST->ConIn->WaitForKey;
}
