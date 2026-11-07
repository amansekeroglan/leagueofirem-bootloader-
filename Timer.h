#ifndef TIMER_H
#define TIMER_H

#include <Uefi.h>

EFI_STATUS
EFIAPI
TimerCreate (
  OUT EFI_EVENT *TimerEvent
  );

EFI_STATUS
EFIAPI
TimerStartPeriodic (
  IN EFI_EVENT TimerEvent,
  IN UINT64 Milliseconds
  );

EFI_STATUS
EFIAPI
TimerStop (
  IN EFI_EVENT TimerEvent
  );

VOID
EFIAPI
TimerClose (
  IN EFI_EVENT TimerEvent
  );

#endif
