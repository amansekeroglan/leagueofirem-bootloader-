#include <Uefi.h>
#include <intrin.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>
#include "Helpers.h"

EFI_BOOT_SERVICES    *gBS = NULL;
EFI_SYSTEM_TABLE     *gST = NULL;
EFI_RUNTIME_SERVICES *gRT = NULL;

EFI_GUID gEfiGraphicsOutputProtocolGuid = 
    { 0x9042A9DE, 0x23DC, 0x4A38, { 0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A } };

EFI_GUID gEfiSimpleFileSystemProtocolGuid = 
    { 0x964E5B22, 0x6459, 0x11D2, { 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };

EFI_GUID gEfiGlobalVariableGuid = 
    { 0x8BE4DF61, 0x93CA, 0x11D2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C } };

EFI_GUID gEfiDevicePathProtocolGuid = 
    { 0x09576E91, 0x6D3F, 0x11D2, { 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };

#pragma function(memcpy, memset)
void* __cdecl memcpy(void* dest, const void* src, size_t count)
{
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (count--) {
        *d++ = *s++;
    }
    return dest;
}

void* __cdecl memset(void* dest, int c, size_t count)
{
    unsigned char* d = (unsigned char*)dest;
    while (count--) {
        *d++ = (unsigned char)c;
    }
    return dest;
}

VOID EFIAPI BootMgrCpuPause(VOID)
{
    _mm_pause();
}

VOID EFIAPI BootMgrMemoryBarrier(VOID)
{
    _mm_mfence();
}

UINT64 EFIAPI BootMgrReadTsc(VOID)
{
    return __rdtsc();
}

UINTN EFIAPI StrLen(CONST CHAR16 *String)
{
    UINTN len = 0;
    if (!String) return 0;
    while (String[len] != 0) {
        len++;
    }
    return len;
}

INTN EFIAPI StrCmp(CONST CHAR16 *FirstString, CONST CHAR16 *SecondString)
{
    if (!FirstString && !SecondString) return 0;
    if (!FirstString) return -1;
    if (!SecondString) return 1;
    while (*FirstString != 0 && *FirstString == *SecondString) {
        FirstString++;
        SecondString++;
    }
    return (INTN)(*FirstString - *SecondString);
}

VOID EFIAPI StrCpy(CHAR16 *Destination, CONST CHAR16 *Source)
{
    if (!Destination || !Source) return;
    while (*Source != 0) {
        *Destination++ = *Source++;
    }
    *Destination = 0;
}

VOID EFIAPI FreePool(IN VOID *Buffer)
{
    if (Buffer != NULL && gBS != NULL) {
        gBS->FreePool(Buffer);
    }
}

UINTN EFIAPI UnicodeSPrint(OUT CHAR16 *StartOfBuffer, IN UINTN BufferSize, IN CONST CHAR16 *FormatString, ...)
{
    va_list args;
    va_start(args, FormatString);

    UINTN maxChars = BufferSize / sizeof(CHAR16);
    if (maxChars == 0) return 0;

    UINTN outIdx = 0;
    for (UINTN i = 0; FormatString[i] != 0 && outIdx + 1 < maxChars; i++) {
        if (FormatString[i] == L'%' && FormatString[i + 1] != 0) {
            i++;
            if (FormatString[i] == L's') {
                CONST CHAR16 *s = va_arg(args, CONST CHAR16*);
                if (!s) s = L"(null)";
                while (*s != 0 && outIdx + 1 < maxChars) {
                    StartOfBuffer[outIdx++] = *s++;
                }
            } else if (FormatString[i] == L'u') {
                UINTN val = va_arg(args, UINTN);
                CHAR16 tmp[32];
                UINTN tmpIdx = 0;
                if (val == 0) {
                    tmp[tmpIdx++] = L'0';
                } else {
                    while (val > 0 && tmpIdx < 30) {
                        tmp[tmpIdx++] = L'0' + (val % 10);
                        val /= 10;
                    }
                }
                while (tmpIdx > 0 && outIdx + 1 < maxChars) {
                    StartOfBuffer[outIdx++] = tmp[--tmpIdx];
                }
            } else if (FormatString[i] == L'%') {
                StartOfBuffer[outIdx++] = L'%';
            }
        } else {
            StartOfBuffer[outIdx++] = FormatString[i];
        }
    }
    StartOfBuffer[outIdx] = 0;
    va_end(args);
    return outIdx;
}

#pragma pack(push, 1)
typedef struct {
    UINT8 Type;
    UINT8 SubType;
    UINT8 Length[2];
    CHAR16 PathName[1];
} FILEPATH_DEVICE_PATH;
#pragma pack(pop)

static UINTN GetDevicePathSize(CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath)
{
    if (!DevicePath) return 0;
    CONST EFI_DEVICE_PATH_PROTOCOL *node = DevicePath;
    while (1) {
        UINTN nodeLen = (UINTN)node->Length[0] | ((UINTN)node->Length[1] << 8);
        if (nodeLen < 4) break;
        if (node->Type == END_DEVICE_PATH_TYPE && node->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE) {
            return ((UINTN)node - (UINTN)DevicePath) + 4;
        }
        node = (CONST EFI_DEVICE_PATH_PROTOCOL*)((UINTN)node + nodeLen);
    }
    return 4;
}

EFI_DEVICE_PATH_PROTOCOL* EFIAPI FileDevicePath(IN EFI_HANDLE Device OPTIONAL, IN CONST CHAR16 *FileName)
{
    if (!FileName || !gBS) return NULL;

    EFI_DEVICE_PATH_PROTOCOL *devDp = NULL;
    UINTN devDpSize = 0;

    if (Device != NULL) {
        gBS->HandleProtocol(Device, &gEfiDevicePathProtocolGuid, (void**)&devDp);
        if (devDp != NULL) {
            devDpSize = GetDevicePathSize(devDp) - 4; // strip existing end node
        }
    }

    UINTN fnChars = StrLen(FileName) + 1;
    UINTN fnSize = sizeof(EFI_DEVICE_PATH_PROTOCOL) + (fnChars * sizeof(CHAR16));
    UINTN totalSize = devDpSize + fnSize + 4;

    EFI_DEVICE_PATH_PROTOCOL *newDp = NULL;
    EFI_STATUS status = gBS->AllocatePool(EfiBootServicesData, totalSize, (void**)&newDp);
    if (EFI_ERROR(status) || !newDp) return NULL;

    UINT8 *ptr = (UINT8*)newDp;
    if (devDpSize > 0) {
        memcpy(ptr, devDp, devDpSize);
        ptr += devDpSize;
    }

    FILEPATH_DEVICE_PATH *fileNode = (FILEPATH_DEVICE_PATH*)ptr;
    fileNode->Type = 4; // MEDIA_DEVICE_PATH
    fileNode->SubType = 4; // MEDIA_FILEPATH_DP
    fileNode->Length[0] = (UINT8)(fnSize & 0xFF);
    fileNode->Length[1] = (UINT8)((fnSize >> 8) & 0xFF);
    memcpy(fileNode->PathName, FileName, fnChars * sizeof(CHAR16));
    ptr += fnSize;

    EFI_DEVICE_PATH_PROTOCOL *endNode = (EFI_DEVICE_PATH_PROTOCOL*)ptr;
    endNode->Type = END_DEVICE_PATH_TYPE;
    endNode->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
    endNode->Length[0] = 4;
    endNode->Length[1] = 0;

    return newDp;
}
