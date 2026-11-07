#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>

EFI_STATUS
EFIAPI
GraphicsInit (
  VOID
  );

VOID
EFIAPI
GraphicsDrawWallpaper (
  VOID
  );

VOID
EFIAPI
GraphicsDrawLogo (
  IN UINTN CenterX,
  IN UINTN CenterY,
  IN UINTN TargetWidth,
  IN UINTN TargetHeight
  );

VOID
EFIAPI
GraphicsRestoreRegion (
  IN UINTN X,
  IN UINTN Y,
  IN UINTN Width,
  IN UINTN Height
  );

VOID
EFIAPI
GraphicsClear (
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color
  );

VOID
EFIAPI
GraphicsFillRect (
  IN UINTN X,
  IN UINTN Y,
  IN UINTN Width,
  IN UINTN Height,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color
  );

VOID
EFIAPI
GraphicsDrawRectOutline (
  IN UINTN X,
  IN UINTN Y,
  IN UINTN Width,
  IN UINTN Height,
  IN UINTN Thickness,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color
  );

VOID
EFIAPI
GraphicsDrawString (
  IN UINTN X,
  IN UINTN Y,
  IN CONST CHAR16 *String,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL FgColor,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL BgColor,
  IN UINTN Scale
  );

VOID
EFIAPI
GraphicsDrawCenteredString (
  IN UINTN Y,
  IN CONST CHAR16 *String,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL FgColor,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL BgColor,
  IN UINTN Scale
  );

UINTN
EFIAPI
GraphicsGetScreenWidth (
  VOID
  );

UINTN
EFIAPI
GraphicsGetScreenHeight (
  VOID
  );

UINTN
EFIAPI
GraphicsGetScale (
  VOID
  );

#endif
