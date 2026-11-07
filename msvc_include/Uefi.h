#pragma once

#include <intrin.h>
#include <stdarg.h>

typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef unsigned long long  UINT64;
typedef signed long long    INT64;
typedef signed int          INT32;
typedef UINT64              UINTN;
typedef INT64               INTN;
typedef UINT64              EFI_STATUS;
typedef void*               EFI_HANDLE;
typedef void*               EFI_EVENT;
typedef UINT64              EFI_PHYSICAL_ADDRESS;
typedef void*               PVOID;
typedef void                VOID;
typedef wchar_t             CHAR16;

#define CONST               const
#define STATIC              static
#define IN
#define OUT
#define OPTIONAL
#define UNREFERENCED_PARAMETER(P) ((void)(P))

typedef unsigned char       BOOLEAN;
#define TRUE                1
#define FALSE               0

#define EFIAPI

#define EFI_SUCCESS                 0ULL
#define EFI_LOAD_ERROR              (0x8000000000000000ULL | 1)
#define EFI_INVALID_PARAMETER       (0x8000000000000000ULL | 2)
#define EFI_UNSUPPORTED             (0x8000000000000000ULL | 3)
#define EFI_BUFFER_TOO_SMALL        (0x8000000000000000ULL | 5)
#define EFI_NOT_READY               (0x8000000000000000ULL | 6)
#define EFI_DEVICE_ERROR            (0x8000000000000000ULL | 7)
#define EFI_OUT_OF_RESOURCES        (0x8000000000000000ULL | 9)
#define EFI_NOT_FOUND               (0x8000000000000000ULL | 14)

#define EFI_ERROR(status) (((INT64)(status)) < 0)

#define EFI_FILE_MODE_READ          0x0000000000000001ULL

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8  Data4[8];
} EFI_GUID;

typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, const wchar_t *String);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN Attribute);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_QUERY_MODE)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN ModeNumber, UINTN *Columns, UINTN *Rows);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN Column, UINTN Row);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_ENABLE_CURSOR)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, BOOLEAN Visible);

typedef struct {
    INT32   MaxMode;
    INT32   Mode;
    INT32   Attribute;
    INT32   CursorColumn;
    INT32   CursorRow;
    BOOLEAN CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void*                           Reset;
    EFI_TEXT_STRING                 OutputString;
    void*                           TestString;
    EFI_TEXT_QUERY_MODE             QueryMode;
    void*                           SetMode;
    EFI_TEXT_SET_ATTRIBUTE          SetAttribute;
    EFI_TEXT_CLEAR_SCREEN           ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION    SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR          EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE*        Mode;
};

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
typedef struct {
    UINT16  ScanCode;
    CHAR16  UnicodeChar;
} EFI_INPUT_KEY;

#define SCAN_UP    0x0001
#define SCAN_DOWN  0x0002
#define CHAR_CARRIAGE_RETURN 0x000D
#define CHAR_LINEFEED        0x000A

typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, EFI_INPUT_KEY *Key);
struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    void*               Reset;
    EFI_INPUT_READ_KEY  ReadKeyStroke;
    EFI_EVENT           WaitForKey;
};

// Graphics Output Protocol
typedef struct {
    UINT8 Blue;
    UINT8 Green;
    UINT8 Red;
    UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef enum {
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

typedef struct {
    UINT32 Version;
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    UINT32 PixelFormat;
    UINT32 RedMask;
    UINT32 GreenMask;
    UINT32 BlueMask;
    UINT32 ReservedMask;
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT32 MaxMode;
    UINT32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo;
    EFI_PHYSICAL_ADDRESS FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
    EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    UINTN SourceX,
    UINTN SourceY,
    UINTN DestinationX,
    UINTN DestinationY,
    UINTN Width,
    UINTN Height,
    UINTN Delta
);

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    void*                               QueryMode;
    void*                               SetMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT    Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE*  Mode;
};

// Device Path Protocol
#pragma pack(push, 1)
typedef struct _EFI_DEVICE_PATH_PROTOCOL {
    UINT8 Type;
    UINT8 SubType;
    UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;
#pragma pack(pop)

#define END_DEVICE_PATH_TYPE                 0x7f
#define END_ENTIRE_DEVICE_PATH_SUBTYPE       0xff

// File Protocol
typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN)(
    EFI_FILE_PROTOCOL *This,
    EFI_FILE_PROTOCOL **NewHandle,
    CONST CHAR16 *FileName,
    UINT64 OpenMode,
    UINT64 Attributes
);
typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE)(EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS (EFIAPI *EFI_FILE_READ)(EFI_FILE_PROTOCOL *This, UINTN *BufferSize, void *Buffer);

struct _EFI_FILE_PROTOCOL {
    UINT64                  Revision;
    EFI_FILE_OPEN           Open;
    EFI_FILE_CLOSE          Close;
    void*                   Delete;
    EFI_FILE_READ           Read;
    void*                   Write;
    void*                   GetPosition;
    void*                   SetPosition;
    void*                   GetInfo;
    void*                   SetInfo;
    void*                   Flush;
};

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME)(
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
    EFI_FILE_PROTOCOL **Root
);
struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    UINT64                                          Revision;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME     OpenVolume;
};

// Loaded Image Protocol
typedef struct {
    UINT32              Revision;
    EFI_HANDLE          ParentHandle;
    void*               SystemTable;
    EFI_HANDLE          DeviceHandle;
    void*               FilePath;
    void*               Reserved;
    UINT32              LoadOptionsSize;
    void*               LoadOptions;
    void*               ImageBase;
    UINT64              ImageSize;
    EFI_MEMORY_TYPE     ImageCodeType;
    EFI_MEMORY_TYPE     ImageDataType;
    void*               Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

// Table Header
typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

// Timer Delay Types
typedef enum {
    TimerCancel,
    TimerPeriodic,
    TimerRelative
} EFI_TIMER_DELAY;

#define EVT_TIMER 0x80000000
#define TPL_CALLBACK 8

typedef enum {
    AllHandles,
    ByRegisterNotify,
    ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_PAGES)(EFI_ALLOCATE_TYPE Type, EFI_MEMORY_TYPE MemoryType, UINTN Pages, EFI_PHYSICAL_ADDRESS *Memory);
typedef EFI_STATUS (EFIAPI *EFI_FREE_PAGES)(EFI_PHYSICAL_ADDRESS Memory, UINTN Pages);
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(EFI_MEMORY_TYPE PoolType, UINTN Size, void **Buffer);
typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(void *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(EFI_HANDLE Handle, CONST EFI_GUID *Protocol, void **Interface);
typedef EFI_STATUS (EFIAPI *EFI_IMAGE_LOAD)(BOOLEAN BootPolicy, EFI_HANDLE ParentImageHandle, void *DevicePath, void *SourceBuffer, UINTN SourceSize, EFI_HANDLE *ImageHandle);
typedef EFI_STATUS (EFIAPI *EFI_IMAGE_START)(EFI_HANDLE ImageHandle, UINTN *ExitDataSize, CHAR16 **ExitData);
typedef EFI_STATUS (EFIAPI *EFI_CREATE_EVENT)(UINT32 Type, UINTN NotifyTpl, void *NotifyFunction, void *NotifyContext, EFI_EVENT *Event);
typedef EFI_STATUS (EFIAPI *EFI_SET_TIMER)(EFI_EVENT Event, EFI_TIMER_DELAY Type, UINT64 TriggerTime);
typedef EFI_STATUS (EFIAPI *EFI_CLOSE_EVENT)(EFI_EVENT Event);
typedef EFI_STATUS (EFIAPI *EFI_WAIT_FOR_EVENT)(UINTN NumberOfEvents, EFI_EVENT *Event, UINTN *Index);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(EFI_LOCATE_SEARCH_TYPE SearchType, CONST EFI_GUID *Protocol, void *SearchKey, UINTN *NoHandles, EFI_HANDLE **Buffer);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(CONST EFI_GUID *Protocol, void *Registration, void **Interface);
typedef EFI_STATUS (EFIAPI *EFI_CONNECT_CONTROLLER)(EFI_HANDLE ControllerHandle, EFI_HANDLE *DriverImageHandle, void *RemainingDevicePath, BOOLEAN Recursive);

// Forward declarations
typedef struct _EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;
typedef struct _EFI_BOOT_SERVICES EFI_BOOT_SERVICES;

// Boot Services
struct _EFI_BOOT_SERVICES {
    EFI_TABLE_HEADER            Hdr;
    void*                       RaiseTPL;
    void*                       RestoreTPL;
    EFI_ALLOCATE_PAGES          AllocatePages;
    EFI_FREE_PAGES              FreePages;
    void*                       GetMemoryMap;
    EFI_ALLOCATE_POOL           AllocatePool;
    EFI_FREE_POOL               FreePool;
    EFI_CREATE_EVENT            CreateEvent;
    EFI_SET_TIMER               SetTimer;
    EFI_WAIT_FOR_EVENT          WaitForEvent;
    void*                       SignalEvent;
    EFI_CLOSE_EVENT             CloseEvent;
    void*                       CheckEvent;
    void*                       InstallProtocolInterface;
    void*                       ReinstallProtocolInterface;
    void*                       UninstallProtocolInterface;
    EFI_HANDLE_PROTOCOL         HandleProtocol;
    void*                       Void;
    void*                       RegisterProtocolNotify;
    void*                       LocateHandle;
    void*                       LocateDevicePath;
    void*                       InstallConfigurationTable;
    EFI_IMAGE_LOAD              LoadImage;
    EFI_IMAGE_START             StartImage;
    void*                       Exit;
    void*                       UnloadImage;
    void*                       ExitBootServices;
    void*                       GetNextMonotonicCount;
    void*                       Stall;
    void*                       SetWatchdogTimer;
    EFI_CONNECT_CONTROLLER      ConnectController;
    void*                       DisconnectController;
    void*                       OpenProtocol;
    void*                       CloseProtocol;
    void*                       OpenProtocolInformation;
    void*                       ProtocolsPerHandle;
    EFI_LOCATE_HANDLE_BUFFER    LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL         LocateProtocol;
    void*                       InstallMultipleProtocolInterfaces;
    void*                       UninstallMultipleProtocolInterfaces;
    void*                       CalculateCrc32;
    void*                       CopyMem;
    void*                       SetMem;
};

// Reset Types
typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef EFI_STATUS (EFIAPI *EFI_GET_VARIABLE)(CHAR16 *VariableName, CONST EFI_GUID *VendorGuid, UINT32 *Attributes, UINTN *DataSize, void *Data);
typedef EFI_STATUS (EFIAPI *EFI_SET_VARIABLE)(CHAR16 *VariableName, CONST EFI_GUID *VendorGuid, UINT32 Attributes, UINTN DataSize, void *Data);
typedef VOID (EFIAPI *EFI_RESET_SYSTEM)(EFI_RESET_TYPE ResetType, EFI_STATUS ResetStatus, UINTN DataSize, void *ResetData);

typedef struct {
    EFI_TABLE_HEADER            Hdr;
    void*                       GetTime;
    void*                       SetTime;
    void*                       GetWakeupTime;
    void*                       SetWakeupTime;
    void*                       SetVirtualAddressMap;
    void*                       ConvertPointer;
    EFI_GET_VARIABLE            GetVariable;
    void*                       GetNextVariableName;
    EFI_SET_VARIABLE            SetVariable;
    void*                       GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM            ResetSystem;
} EFI_RUNTIME_SERVICES;

// System Table
struct _EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER                    Hdr;
    CHAR16*                             FirmwareVendor;
    UINT32                              FirmwareRevision;
    EFI_HANDLE                          ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL*     ConIn;
    EFI_HANDLE                          ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*    ConOut;
    EFI_HANDLE                          StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*    StdErr;
    EFI_RUNTIME_SERVICES*               RuntimeServices;
    EFI_BOOT_SERVICES*                  BootServices;
    UINTN                               NumberOfTableEntries;
    void*                               ConfigurationTable;
};

#define EFI_VARIABLE_NON_VOLATILE                           0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS                     0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS                         0x00000004
#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI                    0x0000000000000001ULL
