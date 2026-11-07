#include "Timer.h"
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
EFIAPI
TimerCreate (
  OUT EFI_EVENT *TimerEvent
  )
{
  if (TimerEvent == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return gBS->CreateEvent (
                EVT_TIMER,
                TPL_CALLBACK,
                NULL,
                NULL,
                TimerEvent
                );
}

EFI_STATUS
EFIAPI
TimerStartPeriodic (
  IN EFI_EVENT TimerEvent,
  IN UINT64 Milliseconds
  )
{
  UINT64 TriggerTime;

  if (TimerEvent == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TriggerTime = Milliseconds * 10000ULL;

  return gBS->SetTimer (
                TimerEvent,
                TimerPeriodic,
                TriggerTime
                );
}

EFI_STATUS
EFIAPI
TimerStop (
  IN EFI_EVENT TimerEvent
  )
{
  if (TimerEvent == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return gBS->SetTimer (
                TimerEvent,
                TimerCancel,
                0
                );
}

VOID
EFIAPI
TimerClose (
  IN EFI_EVENT TimerEvent
  )
{
  if (TimerEvent == NULL) {
    return;
  }

  gBS->SetTimer (TimerEvent, TimerCancel, 0);
  gBS->CloseEvent (TimerEvent);
}
