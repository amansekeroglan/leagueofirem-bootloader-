#include "BootMgr.h"
#include "Graphics.h"
#include "Input.h"
#include "Timer.h"
#include "OsBoot.h"
#include "FirmwareSetup.h"
#include "Helpers.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>

STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorCardBg = { 0x1A, 0x14, 0x12, 0x00 };
STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorCardBorder = { 0x58, 0x4E, 0x46, 0x00 };

STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorSelectedBg = { 0x32, 0x26, 0x20, 0x00 };
STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorSelectedBorder = { 0x50, 0xBD, 0xFF, 0x00 };
STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorSelectedFg = { 0xFF, 0xFF, 0xFF, 0x00 };

STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorUnselectedBg = { 0x18, 0x12, 0x10, 0x00 };
STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorUnselectedBorder = { 0x4C, 0x42, 0x3C, 0x00 };
STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorUnselectedFg = { 0xCC, 0xCC, 0xCC, 0x00 };

STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorCountdownBorder = { 0x60, 0xA8, 0xE8, 0x00 };
STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorCountdownFg = { 0x80, 0xD0, 0xFF, 0x00 };

STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorFooterBg = { 0x14, 0x10, 0x0E, 0x00 };
STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorFooterBorder = { 0x44, 0x3A, 0x34, 0x00 };
STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorFooterFg = { 0xB8, 0xB8, 0xB8, 0x00 };

STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorErrorBorder = { 0x40, 0x40, 0xE0, 0x00 };
STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorErrorFg = { 0x70, 0x70, 0xFF, 0x00 };

STATIC OS_BOOT_ENTRY mOsEntries[MAX_OS_ENTRIES];
STATIC UINTN         mOsCount = 0;

STATIC
VOID
GetButtonMetrics (
  IN  UINTN ScreenWidth,
  IN  UINTN ScreenHeight,
  IN  UINTN Scale,
  OUT UINTN *OutItemWidth,
  OUT UINTN *OutItemHeight,
  OUT UINTN *OutItemSpacing,
  OUT UINTN *OutItem0Y,
  OUT UINTN *OutItemX
  )
{
  UINTN CharWidth;
  UINTN CharHeight;
  UINTN LineSpacing;
  UINTN ItemHeight;
  UINTN ItemSpacing;
  UINTN MaxChars;
  UINTN Index;
  UINTN Len;
  UINTN TotalMenuHeight;
  UINTN ItemWidth;
  UINTN Item0Y;
  UINTN ItemX;

  CharWidth = 8 * Scale;
  CharHeight = 16 * Scale;
  LineSpacing = 24 * Scale;
  ItemHeight = CharHeight + 14 * Scale;
  ItemSpacing = 10 * Scale;
  MaxChars = 17; // Minimum width: "Firmware Settings"

  for (Index = 0; Index < mOsCount; Index++) {
    Len = StrLen (mOsEntries[Index].Name);
    if (Len > MaxChars) {
      MaxChars = Len;
    }
  }

  ItemWidth = (MaxChars + 4) * CharWidth + 20 * Scale;
  if (ItemWidth < 23 * CharWidth + 20 * Scale) {
    ItemWidth = 23 * CharWidth + 20 * Scale;
  }

  TotalMenuHeight = (mOsCount * ItemHeight) + ((mOsCount > 0 ? (mOsCount - 1) : 0) * ItemSpacing);

  if (ScreenHeight > TotalMenuHeight + (LineSpacing * 4)) {
    Item0Y = (ScreenHeight / 2) - (TotalMenuHeight / 2);
  } else {
    Item0Y = LineSpacing * 2;
  }

  ItemX = (ScreenWidth > ItemWidth) ? ((ScreenWidth - ItemWidth) / 2) : 0;

  *OutItemWidth = ItemWidth;
  *OutItemHeight = ItemHeight;
  *OutItemSpacing = ItemSpacing;
  *OutItem0Y = Item0Y;
  *OutItemX = ItemX;
}

STATIC
VOID
RenderMenuItem (
  IN UINTN Index,
  IN BOOLEAN Selected,
  IN UINTN ScreenWidth,
  IN UINTN ScreenHeight,
  IN UINTN Scale
  )
{
  UINTN ItemWidth;
  UINTN ItemHeight;
  UINTN ItemSpacing;
  UINTN Item0Y;
  UINTN ItemX;
  UINTN ItemY;
  UINTN BorderThickness;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL BgColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL BorderColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL FgColor;
  CHAR16 Buffer[64];

  if (Index >= mOsCount) {
    return;
  }

  GetButtonMetrics (
    ScreenWidth,
    ScreenHeight,
    Scale,
    &ItemWidth,
    &ItemHeight,
    &ItemSpacing,
    &Item0Y,
    &ItemX
    );

  ItemY = Item0Y + (Index * (ItemHeight + ItemSpacing));

  if (Selected) {
    BgColor = mColorSelectedBg;
    BorderColor = mColorSelectedBorder;
    FgColor = mColorSelectedFg;
    BorderThickness = (Scale > 1) ? 2 : 1;
    UnicodeSPrint (Buffer, sizeof (Buffer), L"> %s", mOsEntries[Index].Name);
  } else {
    BgColor = mColorUnselectedBg;
    BorderColor = mColorUnselectedBorder;
    FgColor = mColorUnselectedFg;
    BorderThickness = 1;
    UnicodeSPrint (Buffer, sizeof (Buffer), L"  %s", mOsEntries[Index].Name);
  }

  // Restore wallpaper background behind button before drawing card
  GraphicsRestoreRegion (
    ItemX,
    ItemY,
    ItemWidth,
    ItemHeight
    );

  // Card background
  GraphicsFillRect (
    ItemX,
    ItemY,
    ItemWidth,
    ItemHeight,
    BgColor
    );

  // Crisp border outlining corners and edges
  GraphicsDrawRectOutline (
    ItemX,
    ItemY,
    ItemWidth,
    ItemHeight,
    BorderThickness,
    BorderColor
    );

  // Button text
  GraphicsDrawString (
    ItemX + 10 * Scale,
    ItemY + 7 * Scale,
    Buffer,
    FgColor,
    BgColor,
    Scale
    );
}

STATIC
VOID
RenderMenu (
  IN UINTN SelectedIndex,
  IN UINTN ScreenWidth,
  IN UINTN ScreenHeight,
  IN UINTN Scale
  )
{
  UINTN CharWidth;
  UINTN CharHeight;
  UINTN ItemWidth;
  UINTN ItemHeight;
  UINTN ItemSpacing;
  UINTN Item0Y;
  UINTN ItemX;
  UINTN MarginTop;
  UINTN MarginBottom;
  UINTN MaxLogoW;
  UINTN MaxLogoH;
  UINTN LogoW;
  UINTN LogoH;
  UINTN LogoCenterX;
  UINTN LogoCenterY;
  UINTN FooterWidth;
  UINTN FooterHeight;
  UINTN FooterX;
  UINTN FooterY;
  UINTN Index;
  CONST CHAR16 *FooterText;

  CharWidth = 8 * Scale;
  CharHeight = 16 * Scale;

  // Draw full-screen wallpaper covering entire display
  GraphicsDrawWallpaper ();

  GetButtonMetrics (
    ScreenWidth,
    ScreenHeight,
    Scale,
    &ItemWidth,
    &ItemHeight,
    &ItemSpacing,
    &Item0Y,
    &ItemX
    );

  // Calculate dynamic logo size and position strictly scaled to screen size
  MarginTop = 16 * Scale;
  MarginBottom = 16 * Scale;

  // Symmetrically size logo: up to 38% of screen width, perfectly fitting above menu buttons
  MaxLogoW = (ScreenWidth * 38) / 100;
  if (Item0Y > (MarginTop + MarginBottom)) {
    MaxLogoH = Item0Y - MarginTop - MarginBottom;
    LogoCenterY = MarginTop + (MaxLogoH / 2);
  } else {
    MaxLogoH = Item0Y / 2;
    LogoCenterY = Item0Y / 2;
  }

  // Preserve 666:375 aspect ratio
  LogoW = MaxLogoW;
  LogoH = (LogoW * 375) / 666;

  if (LogoH > MaxLogoH) {
    LogoH = MaxLogoH;
    LogoW = (LogoH * 666) / 375;
  }

  if (LogoW == 0) LogoW = 1;
  if (LogoH == 0) LogoH = 1;

  LogoCenterX = ScreenWidth / 2;

  // Render dynamically scaled logo with alpha-blending
  GraphicsDrawLogo (
    LogoCenterX,
    LogoCenterY,
    LogoW,
    LogoH
    );

  // Render All Detected OS Options
  for (Index = 0; Index < mOsCount; Index++) {
    RenderMenuItem (Index, SelectedIndex == Index, ScreenWidth, ScreenHeight, Scale);
  }

  // Render Footer Card
  FooterText = L"[Up/Down] Navigate    [Enter] Select";
  FooterWidth = StrLen (FooterText) * CharWidth + 24 * Scale;
  FooterHeight = CharHeight + 10 * Scale;
  FooterX = (ScreenWidth > FooterWidth) ? (ScreenWidth - FooterWidth) / 2 : 0;
  FooterY = (ScreenHeight > (CharHeight * 3)) ? (ScreenHeight - (CharHeight * 3)) : (ScreenHeight - CharHeight);

  GraphicsFillRect (
    FooterX,
    FooterY,
    FooterWidth,
    FooterHeight,
    mColorFooterBg
    );

  GraphicsDrawRectOutline (
    FooterX,
    FooterY,
    FooterWidth,
    FooterHeight,
    1,
    mColorFooterBorder
    );

  GraphicsDrawCenteredString (
    FooterY + 5 * Scale,
    FooterText,
    mColorFooterFg,
    mColorFooterBg,
    Scale
    );
}

STATIC
VOID
RenderCountdown (
  IN UINTN Seconds,
  IN UINTN SelectedIndex,
  IN UINTN ScreenWidth,
  IN UINTN ScreenHeight,
  IN UINTN Scale
  )
{
  UINTN CharWidth;
  UINTN CharHeight;
  UINTN LineSpacing;
  UINTN ItemWidth;
  UINTN ItemHeight;
  UINTN ItemSpacing;
  UINTN Item0Y;
  UINTN ItemX;
  UINTN LastItemY;
  UINTN CountdownY;
  UINTN PillWidth;
  UINTN PillHeight;
  UINTN PillX;
  CHAR16 Buffer[80];
  CONST CHAR16 *TargetName;

  CharWidth = 8 * Scale;
  CharHeight = 16 * Scale;
  LineSpacing = 24 * Scale;

  GetButtonMetrics (
    ScreenWidth,
    ScreenHeight,
    Scale,
    &ItemWidth,
    &ItemHeight,
    &ItemSpacing,
    &Item0Y,
    &ItemX
    );

  LastItemY = Item0Y + ((mOsCount > 0 ? (mOsCount - 1) : 0) * (ItemHeight + ItemSpacing));
  CountdownY = LastItemY + ItemHeight + (LineSpacing * 2);

  PillWidth = 42 * CharWidth + 24 * Scale;
  PillHeight = CharHeight + 12 * Scale;
  PillX = (ScreenWidth > PillWidth) ? (ScreenWidth - PillWidth) / 2 : 0;

  // Restore wallpaper behind previous countdown
  GraphicsRestoreRegion (
    PillX - 2 * Scale,
    CountdownY - 2 * Scale,
    PillWidth + 4 * Scale,
    PillHeight + 4 * Scale
    );

  if (SelectedIndex < mOsCount) {
    TargetName = mOsEntries[SelectedIndex].Name;
  } else {
    TargetName = L"OS";
  }

  if (Seconds == 1) {
    UnicodeSPrint (
      Buffer,
      sizeof (Buffer),
      L"Booting %s in 1 second...",
      TargetName
      );
  } else {
    UnicodeSPrint (
      Buffer,
      sizeof (Buffer),
      L"Booting %s in %u seconds...",
      TargetName,
      Seconds
      );
  }

  // Draw countdown card with border
  GraphicsFillRect (
    PillX,
    CountdownY,
    PillWidth,
    PillHeight,
    mColorCardBg
    );

  GraphicsDrawRectOutline (
    PillX,
    CountdownY,
    PillWidth,
    PillHeight,
    1,
    mColorCountdownBorder
    );

  GraphicsDrawCenteredString (
    CountdownY + 6 * Scale,
    Buffer,
    mColorCountdownFg,
    mColorCardBg,
    Scale
    );
}

STATIC
VOID
ClearCountdown (
  IN UINTN ScreenWidth,
  IN UINTN ScreenHeight,
  IN UINTN Scale
  )
{
  UINTN CharWidth;
  UINTN CharHeight;
  UINTN LineSpacing;
  UINTN ItemWidth;
  UINTN ItemHeight;
  UINTN ItemSpacing;
  UINTN Item0Y;
  UINTN ItemX;
  UINTN LastItemY;
  UINTN CountdownY;
  UINTN PillWidth;
  UINTN PillHeight;
  UINTN PillX;

  CharWidth = 8 * Scale;
  CharHeight = 16 * Scale;
  LineSpacing = 24 * Scale;

  GetButtonMetrics (
    ScreenWidth,
    ScreenHeight,
    Scale,
    &ItemWidth,
    &ItemHeight,
    &ItemSpacing,
    &Item0Y,
    &ItemX
    );

  LastItemY = Item0Y + ((mOsCount > 0 ? (mOsCount - 1) : 0) * (ItemHeight + ItemSpacing));
  CountdownY = LastItemY + ItemHeight + (LineSpacing * 2);

  PillWidth = 42 * CharWidth + 24 * Scale;
  PillHeight = CharHeight + 12 * Scale;
  PillX = (ScreenWidth > PillWidth) ? (ScreenWidth - PillWidth) / 2 : 0;

  GraphicsRestoreRegion (
    PillX - 2 * Scale,
    CountdownY - 2 * Scale,
    PillWidth + 4 * Scale,
    PillHeight + 4 * Scale
    );
}

STATIC
VOID
DisplayError (
  IN CONST CHAR16 *Message
  )
{
  UINTN ScreenWidth;
  UINTN ScreenHeight;
  UINTN Scale;
  UINTN LineSpacing;
  UINTN DialogWidth;
  UINTN DialogHeight;
  UINTN DialogX;
  UINTN DialogY;

  ScreenWidth = GraphicsGetScreenWidth ();
  ScreenHeight = GraphicsGetScreenHeight ();
  Scale = GraphicsGetScale ();
  LineSpacing = 24 * Scale;

  GraphicsDrawWallpaper ();

  DialogWidth = (ScreenWidth > 600) ? 580 : (ScreenWidth - 40);
  DialogHeight = 6 * LineSpacing;
  DialogX = (ScreenWidth > DialogWidth) ? (ScreenWidth - DialogWidth) / 2 : 0;
  DialogY = (ScreenHeight > DialogHeight) ? (ScreenHeight - DialogHeight) / 2 : 0;

  GraphicsFillRect (
    DialogX,
    DialogY,
    DialogWidth,
    DialogHeight,
    mColorCardBg
    );

  GraphicsDrawRectOutline (
    DialogX,
    DialogY,
    DialogWidth,
    DialogHeight,
    2,
    mColorErrorBorder
    );

  GraphicsDrawCenteredString (
    DialogY + LineSpacing,
    Message,
    mColorErrorFg,
    mColorCardBg,
    Scale
    );

  GraphicsDrawCenteredString (
    DialogY + LineSpacing * 3,
    L"Press any key to return...",
    mColorFooterFg,
    mColorCardBg,
    Scale
    );

  InputWaitAnyKey ();
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  UINTN SelectedIndex;
  UINTN PreviousIndex;
  UINTN CountdownSeconds;
  BOOLEAN CountdownActive;
  UINTN ScreenWidth;
  UINTN ScreenHeight;
  UINTN Scale;
  EFI_EVENT TimerEvent;
  EFI_EVENT WaitEvents[2];
  UINTN EventIndex;
  KEY_ACTION KeyAction;

  // Initialize global UEFI system pointers for freestanding build
  gST = SystemTable;
  gBS = SystemTable->BootServices;
  gRT = SystemTable->RuntimeServices;

  Status = GraphicsInit ();
  if (EFI_ERROR (Status)) {
    if (gST->ConOut != NULL) {
      gST->ConOut->OutputString (
                     gST->ConOut,
                     L"Error: Graphics Output Protocol (GOP) is not supported.\r\n"
                     );
    }
    return Status;
  }

  Status = InputInit ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Scan all storage devices and partitions for installed OS bootloaders
  Status = OsScanAll (mOsEntries, MAX_OS_ENTRIES, &mOsCount);
  if (EFI_ERROR (Status) && mOsCount == 0) {
    DisplayError (L"No bootable operating systems found on any disk partition.");
    return Status;
  }

  SelectedIndex = 0;
  CountdownSeconds = BOOT_TIMEOUT_SECONDS;
  CountdownActive = TRUE;
  TimerEvent = NULL;

  ScreenWidth = GraphicsGetScreenWidth ();
  ScreenHeight = GraphicsGetScreenHeight ();
  Scale = GraphicsGetScale ();

  RenderMenu (SelectedIndex, ScreenWidth, ScreenHeight, Scale);
  RenderCountdown (CountdownSeconds, SelectedIndex, ScreenWidth, ScreenHeight, Scale);

  Status = TimerCreate (&TimerEvent);
  if (!EFI_ERROR (Status)) {
    Status = TimerStartPeriodic (TimerEvent, 1000);
    if (EFI_ERROR (Status)) {
      TimerClose (TimerEvent);
      TimerEvent = NULL;
      CountdownActive = FALSE;
      ClearCountdown (ScreenWidth, ScreenHeight, Scale);
    }
  } else {
    CountdownActive = FALSE;
    ClearCountdown (ScreenWidth, ScreenHeight, Scale);
  }

  while (TRUE) {
    EventIndex = 0;
    WaitEvents[0] = InputGetWaitEvent ();

    if (CountdownActive && TimerEvent != NULL) {
      WaitEvents[1] = TimerEvent;
      Status = gBS->WaitForEvent (2, WaitEvents, &EventIndex);
    } else {
      Status = gBS->WaitForEvent (1, WaitEvents, &EventIndex);
    }

    if (EFI_ERROR (Status)) {
      BootMgrCpuPause ();
      continue;
    }

    if (EventIndex == 0) {
      Status = InputPollKey (&KeyAction);
      if (!EFI_ERROR (Status)) {
        if (KeyAction == KeyActionUp) {
          PreviousIndex = SelectedIndex;
          SelectedIndex = (SelectedIndex == 0) ? (mOsCount - 1) : (SelectedIndex - 1);

          RenderMenuItem (PreviousIndex, FALSE, ScreenWidth, ScreenHeight, Scale);
          RenderMenuItem (SelectedIndex, TRUE, ScreenWidth, ScreenHeight, Scale);

          if (CountdownActive) {
            RenderCountdown (CountdownSeconds, SelectedIndex, ScreenWidth, ScreenHeight, Scale);
          }
        } else if (KeyAction == KeyActionDown) {
          PreviousIndex = SelectedIndex;
          SelectedIndex = (SelectedIndex + 1 >= mOsCount) ? 0 : (SelectedIndex + 1);

          RenderMenuItem (PreviousIndex, FALSE, ScreenWidth, ScreenHeight, Scale);
          RenderMenuItem (SelectedIndex, TRUE, ScreenWidth, ScreenHeight, Scale);

          if (CountdownActive) {
            RenderCountdown (CountdownSeconds, SelectedIndex, ScreenWidth, ScreenHeight, Scale);
          }
        } else if (KeyAction == KeyActionSelect) {
          if (CountdownActive) {
            CountdownActive = FALSE;
            if (TimerEvent != NULL) {
              TimerStop (TimerEvent);
            }
          }

          Status = OsLaunch (&mOsEntries[SelectedIndex], ImageHandle);
          if (EFI_ERROR (Status)) {
            if (Status == EFI_NOT_FOUND) {
              DisplayError (L"Selected operating system loader not found on disk.");
            } else if (Status == EFI_UNSUPPORTED) {
              DisplayError (L"Operation is not supported by this motherboard firmware.");
            } else {
              DisplayError (L"Failed to launch selected operating system.");
            }
            RenderMenu (SelectedIndex, ScreenWidth, ScreenHeight, Scale);
          }
        }
      }
    } else if (EventIndex == 1 && CountdownActive) {
      if (CountdownSeconds > 1) {
        CountdownSeconds--;
        RenderCountdown (CountdownSeconds, SelectedIndex, ScreenWidth, ScreenHeight, Scale);
      } else {
        CountdownActive = FALSE;
        if (TimerEvent != NULL) {
          TimerStop (TimerEvent);
        }
        ClearCountdown (ScreenWidth, ScreenHeight, Scale);

        Status = OsLaunch (&mOsEntries[SelectedIndex], ImageHandle);
        if (EFI_ERROR (Status)) {
          if (Status == EFI_NOT_FOUND) {
            DisplayError (L"Selected operating system loader not found on disk.");
          } else if (Status == EFI_UNSUPPORTED) {
            DisplayError (L"Operation is not supported by this motherboard firmware.");
          } else {
            DisplayError (L"Failed to launch selected operating system.");
          }
          RenderMenu (SelectedIndex, ScreenWidth, ScreenHeight, Scale);
        }
      }
    }
  }

  if (TimerEvent != NULL) {
    TimerClose (TimerEvent);
  }

  return EFI_SUCCESS;
}
