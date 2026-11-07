#include "FirmwareSetup.h"
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/GlobalVariable.h>

EFI_STATUS
EFIAPI
FirmwareSetupEnter (
  VOID
  )
{
  EFI_STATUS Status;
  UINT64 SupportedIndications;
  UINTN DataSize;
  UINT64 OsIndications;
  UINT32 Attributes;

  SupportedIndications = 0;
  DataSize = sizeof (UINT64);

  Status = gRT->GetVariable (
                  L"OsIndicationsSupported",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  &SupportedIndications
                  );
  if (!EFI_ERROR (Status)) {
    if ((SupportedIndications != 0) &&
        ((SupportedIndications & EFI_OS_INDICATIONS_BOOT_TO_FW_UI) == 0)) {
      return EFI_UNSUPPORTED;
    }
  }

  OsIndications = 0;
  DataSize = sizeof (UINT64);
  Attributes = 0;

  Status = gRT->GetVariable (
                  L"OsIndications",
                  &gEfiGlobalVariableGuid,
                  &Attributes,
                  &DataSize,
                  &OsIndications
                  );
  if (EFI_ERROR (Status)) {
    OsIndications = 0;
    Attributes = EFI_VARIABLE_NON_VOLATILE |
                 EFI_VARIABLE_BOOTSERVICE_ACCESS |
                 EFI_VARIABLE_RUNTIME_ACCESS;
  } else {
    Attributes |= EFI_VARIABLE_NON_VOLATILE |
                  EFI_VARIABLE_BOOTSERVICE_ACCESS |
                  EFI_VARIABLE_RUNTIME_ACCESS;
  }

  OsIndications |= EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
  DataSize = sizeof (UINT64);

  Status = gRT->SetVariable (
                  L"OsIndications",
                  &gEfiGlobalVariableGuid,
                  Attributes,
                  DataSize,
                  &OsIndications
                  );
  if (EFI_ERROR (Status)) {
    gRT->SetVariable (
           L"OsIndications",
           &gEfiGlobalVariableGuid,
           Attributes,
           0,
           NULL
           );
    gRT->SetVariable (
           L"OsIndications",
           &gEfiGlobalVariableGuid,
           0,
           0,
           NULL
           );

    Attributes = EFI_VARIABLE_NON_VOLATILE |
                 EFI_VARIABLE_BOOTSERVICE_ACCESS |
                 EFI_VARIABLE_RUNTIME_ACCESS;

    Status = gRT->SetVariable (
                    L"OsIndications",
                    &gEfiGlobalVariableGuid,
                    Attributes,
                    DataSize,
                    &OsIndications
                    );
    if (EFI_ERROR (Status)) {
      Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS |
                   EFI_VARIABLE_RUNTIME_ACCESS;

      Status = gRT->SetVariable (
                      L"OsIndications",
                      &gEfiGlobalVariableGuid,
                      Attributes,
                      DataSize,
                      &OsIndications
                      );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
  gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);

  return EFI_SUCCESS;
}
