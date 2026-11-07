#include "BootMgr.h"
#include "Graphics.h"
#include "Input.h"
#include "Timer.h"
#include "WindowsBoot.h"
#include "FirmwareSetup.h"
#include "Helpers.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>

STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorCardBg = { 0x1A, 0x14, 0x12, 0x00 };
STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorCardBorder = { 0x58, 0x4E, 0x46, 0x00 };
STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL mColorTitle = { 0xFF, 0xFF, 0xFF, 0x00 };

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

STATIC CONST CHAR16 *mOptionsSelected[BOOT_OPTION_COUNT] = {
  L"> Windows          ",
  L"> Firmware Settings"
};

STATIC CONST CHAR16 *mOptionsUnselected[BOOT_OPTION_COUNT] = {
  L"  Windows          ",
  L"  Firmware Settings"
};

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
  UINTN CharWidth;
  UINTN CharHeight;
  UINTN LineSpacing;
  UINTN ItemWidth;
  UINTN ItemHeight;
  UINTN ItemX;
  UINTN Item0Y;
  UINTN ItemY;
  UINTN BorderThickness;
  CONST CHAR16 *Text;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL BgColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL BorderColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL FgColor;

  CharWidth = 8 * Scale;
  CharHeight = 16 * Scale;
  LineSpacing = 24 * Scale;
  ItemWidth = 23 * CharWidth + 20 * Scale;
  ItemHeight = CharHeight + 14 * Scale;

  ItemX = (ScreenWidth > ItemWidth) ? (ScreenWidth - ItemWidth) / 2 : 0;
  Item0Y = (ScreenHeight / 2) > LineSpacing ? ((ScreenHeight / 2) - LineSpacing) : LineSpacing;
  ItemY = (Index == 0) ? Item0Y : (Item0Y + ItemHeight + 10 * Scale);

  if (Selected) {
    BgColor = mColorSelectedBg;
    BorderColor = mColorSelectedBorder;
    FgColor = mColorSelectedFg;
    BorderThickness = (Scale > 1) ? 2 : 1;
    Text = mOptionsSelected[Index];
  } else {
    BgColor = mColorUnselectedBg;
    BorderColor = mColorUnselectedBorder;
    FgColor = mColorUnselectedFg;
    BorderThickness = 1;
    Text = mOptionsUnselected[Index];
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
    Text,
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
  UINTN LineSpacing;
  UINTN Item0Y;
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
  CONST CHAR16 *FooterText;

  CharWidth = 8 * Scale;
  CharHeight = 16 * Scale;
  LineSpacing = 24 * Scale;

  // Draw full-screen wallpaper covering entire display
  GraphicsDrawWallpaper ();

  // Calculate dynamic logo size and position strictly scaled to screen size
  Item0Y = (ScreenHeight / 2) > LineSpacing ? ((ScreenHeight / 2) - LineSpacing) : LineSpacing;
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

  // Render Options
  RenderMenuItem (0, SelectedIndex == 0, ScreenWidth, ScreenHeight, Scale);
  RenderMenuItem (1, SelectedIndex == 1, ScreenWidth, ScreenHeight, Scale);

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
  UINTN ItemHeight;
  UINTN Item0Y;
  UINTN Item1Y;
  UINTN CountdownY;
  UINTN PillWidth;
  UINTN PillHeight;
  UINTN PillX;
  CHAR16 Buffer[64];
  CONST CHAR16 *TargetName;

  CharWidth = 8 * Scale;
  CharHeight = 16 * Scale;
  LineSpacing = 24 * Scale;
  ItemHeight = CharHeight + 14 * Scale;
  Item0Y = (ScreenHeight / 2) > LineSpacing ? ((ScreenHeight / 2) - LineSpacing) : LineSpacing;
  Item1Y = Item0Y + ItemHeight + 10 * Scale;
  CountdownY = Item1Y + ItemHeight + (LineSpacing * 2);

  PillWidth = 37 * CharWidth + 24 * Scale;
  PillHeight = CharHeight + 12 * Scale;
  PillX = (ScreenWidth > PillWidth) ? (ScreenWidth - PillWidth) / 2 : 0;

  // Restore wallpaper behind previous countdown
  GraphicsRestoreRegion (
    PillX - 2 * Scale,
    CountdownY - 2 * Scale,
    PillWidth + 4 * Scale,
    PillHeight + 4 * Scale
    );

  TargetName = (SelectedIndex == BOOT_OPTION_WINDOWS) ? L"Windows" : L"Firmware Settings";

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
  UINTN ItemHeight;
  UINTN Item0Y;
  UINTN Item1Y;
  UINTN CountdownY;
  UINTN PillWidth;
  UINTN PillHeight;
  UINTN PillX;

  CharWidth = 8 * Scale;
  CharHeight = 16 * Scale;
  LineSpacing = 24 * Scale;
  ItemHeight = CharHeight + 14 * Scale;
  Item0Y = (ScreenHeight / 2) > LineSpacing ? ((ScreenHeight / 2) - LineSpacing) : LineSpacing;
  Item1Y = Item0Y + ItemHeight + 10 * Scale;
  CountdownY = Item1Y + ItemHeight + (LineSpacing * 2);

  PillWidth = 37 * CharWidth + 24 * Scale;
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
  UINTN CharHeight;
  UINTN DialogWidth;
  UINTN DialogHeight;
  UINTN DialogX;
  UINTN DialogY;

  ScreenWidth = GraphicsGetScreenWidth ();
  ScreenHeight = GraphicsGetScreenHeight ();
  Scale = GraphicsGetScale ();
  LineSpacing = 24 * Scale;
  CharHeight = 16 * Scale;

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

  SelectedIndex = BOOT_OPTION_WINDOWS;
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
        if (KeyAction == KeyActionUp || KeyAction == KeyActionDown) {
          SelectedIndex = (SelectedIndex == BOOT_OPTION_WINDOWS) ?
                          BOOT_OPTION_FIRMWARE_SETUP :
                          BOOT_OPTION_WINDOWS;

          RenderMenuItem (0, SelectedIndex == 0, ScreenWidth, ScreenHeight, Scale);
          RenderMenuItem (1, SelectedIndex == 1, ScreenWidth, ScreenHeight, Scale);

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

          if (SelectedIndex == BOOT_OPTION_WINDOWS) {
            Status = WindowsBootLaunch (ImageHandle);
            if (EFI_ERROR (Status)) {
              if (Status == EFI_NOT_FOUND) {
                DisplayError (L"Windows Boot Manager not found on any system partition.");
              } else {
                DisplayError (L"Failed to launch Windows Boot Manager.");
              }
              RenderMenu (SelectedIndex, ScreenWidth, ScreenHeight, Scale);
            }
          } else if (SelectedIndex == BOOT_OPTION_FIRMWARE_SETUP) {
            Status = FirmwareSetupEnter ();
            if (EFI_ERROR (Status)) {
              if (Status == EFI_UNSUPPORTED) {
                DisplayError (L"Firmware settings entry is not supported by this motherboard.");
              } else {
                DisplayError (L"Failed to request firmware settings entry.");
              }
              RenderMenu (SelectedIndex, ScreenWidth, ScreenHeight, Scale);
            }
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

        if (SelectedIndex == BOOT_OPTION_WINDOWS) {
          Status = WindowsBootLaunch (ImageHandle);
          if (EFI_ERROR (Status)) {
            if (Status == EFI_NOT_FOUND) {
              DisplayError (L"Windows Boot Manager not found on any system partition.");
            } else {
              DisplayError (L"Failed to launch Windows Boot Manager.");
            }
            RenderMenu (SelectedIndex, ScreenWidth, ScreenHeight, Scale);
          }
        } else if (SelectedIndex == BOOT_OPTION_FIRMWARE_SETUP) {
          Status = FirmwareSetupEnter ();
          if (EFI_ERROR (Status)) {
            if (Status == EFI_UNSUPPORTED) {
              DisplayError (L"Firmware settings entry is not supported by this motherboard.");
            } else {
              DisplayError (L"Failed to request firmware settings entry.");
            }
            RenderMenu (SelectedIndex, ScreenWidth, ScreenHeight, Scale);
          }
        }
      }
    }
  }

  if (TimerEvent != NULL) {
    TimerClose (TimerEvent);
  }

  return EFI_SUCCESS;
}
