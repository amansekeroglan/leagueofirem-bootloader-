#pragma once
#include <Uefi.h>

UINTN
EFIAPI
StrLen (
  IN CONST CHAR16 *String
  );

INTN
EFIAPI
StrCmp (
  IN CONST CHAR16 *FirstString,
  IN CONST CHAR16 *SecondString
  );

VOID
EFIAPI
StrCpy (
  OUT CHAR16       *Destination,
  IN  CONST CHAR16 *Source
  );
