#pragma once

#include "MaaPort.h"
#include <stdint.h>

struct MaaStringBuffer;
typedef struct MaaStringBuffer* MaaStringBufferHandle;
struct MaaImageBuffer;
typedef struct MaaImageBuffer* MaaImageBufferHandle;

struct MaaResourceAPI;
typedef struct MaaResourceAPI* MaaResourceHandle;
struct MaaControllerAPI;
typedef struct MaaControllerAPI* MaaControllerHandle;
struct MaaInstanceAPI;
typedef struct MaaInstanceAPI* MaaInstanceHandle;

typedef uint8_t MaaBool;
typedef uint64_t MaaSize;
#define MaaNullSize ((MaaSize)-1)

typedef const char* MaaStringView;

typedef int32_t MaaStatus;
enum MaaStatusEnum
{
    MaaStatus_Invalid = 0,
    MaaStatus_Pending = 1000,
    MaaStatus_Running = 2000,
    MaaStatus_Success = 3000,
    MaaStatus_Failed = 4000,
    // MaaStatus_Timeout = 5000,
};
typedef int32_t MaaLoggingLevel;
enum MaaLoggingLevelEunm
{
    MaaLoggingLevel_Off = 0,
    MaaLoggingLevel_Fatal = 1,
    MaaLoggingLevel_Error = 2,
    MaaLoggingLevel_Warn = 3,
    MaaLoggingLevel_Info = 4,
    MaaLoggingLevel_Debug = 5,
    MaaLoggingLevel_Trace = 6,
    MaaLoggingLevel_All = 7,
};

typedef int64_t MaaId;
typedef MaaId MaaCtrlId;
typedef MaaId MaaResId;
typedef MaaId MaaTaskId;
#define MaaInvalidId ((MaaId)0)

typedef int32_t MaaOption;
typedef void* MaaOptionValue;
typedef uint64_t MaaOptionValueSize;

typedef MaaOption MaaGlobalOption;
enum MaaGlobalOptionEnum
{
    MaaGlobalOption_Invalid = 0,

    // value: string, log dir, eg: "C:\\Users\\Administrator\\Desktop\\log"; val_size: string length
    MaaGlobalOption_LogDir = 1,

    // value: bool, eg: true; val_size: sizeof(bool)
    MaaGlobalOption_SaveDraw = 2,

    // Dump all screenshots and actions
    // this option will || with MaaCtrlOption_Recording
    // value: bool, eg: true; val_size: sizeof(bool)
    MaaGlobalOption_Recording = 3,

    // value: MaaLoggingLevel, val_size: sizeof(MaaLoggingLevel), default by MaaLoggingLevel_Error
    MaaGlobalOption_StdoutLevel = 4,

    // value: bool, eg: true; val_size: sizeof(bool)
    MaaGlobalOption_ShowHitDraw = 5,
};

typedef MaaOption MaaResOption;
enum MaaResOptionEnum
{
    MaaResOption_Invalid = 0,
};

typedef MaaOption MaaCtrlOption;
enum MaaCtrlOptionEnum
{
    MaaCtrlOption_Invalid = 0,

    // Only one of long and short side can be set, and the other is automatically scaled according to the aspect ratio.
    // value: int, eg: 1920; val_size: sizeof(int)
    MaaCtrlOption_ScreenshotTargetLongSide = 1,

    // Only one of long and short side can be set, and the other is automatically scaled according to the aspect ratio.
    // value: int, eg: 1080; val_size: sizeof(int)
    MaaCtrlOption_ScreenshotTargetShortSide = 2,

    // For StartApp
    // value: string, eg: "com.hypergryph.arknights/com.u8.sdk.U8UnityContext"; val_size: string length
    MaaCtrlOption_DefaultAppPackageEntry = 3,

    // For StopApp
    // value: string, eg: "com.hypergryph.arknights"; val_size: string length
    MaaCtrlOption_DefaultAppPackage = 4,

    // Dump all screenshots and actions
    // this option will || with MaaGlobalOption_Recording
    // value: bool, eg: true; val_size: sizeof(bool)
    MaaCtrlOption_Recording = 5,
};

typedef MaaOption MaaInstOption;
enum MaaInstOptionEnum
{
    MaaInstOption_Invalid = 0,
};

#define MaaTaskParam_Empty "{}"

typedef int32_t MaaAdbControllerType;
enum MaaAdbControllerTypeEnum
{
    MaaAdbControllerType_Invalid = 0,

    MaaAdbControllerType_Touch_Adb = 1,
    MaaAdbControllerType_Touch_MiniTouch = 2,
    MaaAdbControllerType_Touch_MaaTouch = 3,
    MaaAdbControllerType_Touch_Mask = 0xFF,

    MaaAdbControllerType_Key_Adb = 1 << 8,
    MaaAdbControllerType_Key_MaaTouch = 2 << 8,
    MaaAdbControllerType_Key_Mask = 0xFF00,

    MaaAdbControllerType_Input_Preset_Adb = MaaAdbControllerType_Touch_Adb | MaaAdbControllerType_Key_Adb,
    MaaAdbControllerType_Input_Preset_Minitouch = MaaAdbControllerType_Touch_MiniTouch | MaaAdbControllerType_Key_Adb,
    MaaAdbControllerType_Input_Preset_Maatouch =
        MaaAdbControllerType_Touch_MaaTouch | MaaAdbControllerType_Key_MaaTouch,

    MaaAdbControllerType_Screencap_FastestWay = 1 << 16,
    MaaAdbControllerType_Screencap_RawByNetcat = 2 << 16,
    MaaAdbControllerType_Screencap_RawWithGzip = 3 << 16,
    MaaAdbControllerType_Screencap_Encode = 4 << 16,
    MaaAdbControllerType_Screencap_EncodeToFile = 5 << 16,
    MaaAdbControllerType_Screencap_MinicapDirect = 6 << 16,
    MaaAdbControllerType_Screencap_MinicapStream = 7 << 16,
    MaaAdbControllerType_Screencap_Mask = 0xFF0000,
};

typedef int32_t MaaDbgControllerType;
enum MaaDbgControllerTypeEnum
{
    MaaDbgController_Invalid = 0,

    MaaDbgControllerType_CarouselImage = 1,
    MaaDbgControllerType_ReplayRecording = 2,
};

typedef int32_t MaaThriftControllerType;
enum MaaThriftControllerTypeEnum
{
    MaaThriftController_Invalid = 0,

    MaaThriftControllerType_Socket = 1,
    MaaThriftControllerType_UnixDomainSocket = 2,
};

typedef int32_t MaaWin32ControllerType;
enum MaaWin32ControllerTypeEnum
{
    MaaWin32Controller_Invalid = 0,

    MaaWin32ControllerType_Touch_SendMessage = 1,
    MaaWin32ControllerType_Touch_Mask = 0xFF,

    MaaWin32ControllerType_Key_SendMessage = 1 << 8,
    MaaWin32ControllerType_Key_Mask = 0xFF00,

    MaaWin32ControllerType_Screencap_GDI = 1 << 16,
    MaaWin32ControllerType_Screencap_DXGI_DesktopDup = 2 << 16,
    // MaaWin32ControllerType_Screencap_DXGI_BackBuffer = 3 << 16,
    MaaWin32ControllerType_Screencap_DXGI_FramePool = 4 << 16,
    MaaWin32ControllerType_Screencap_Mask = 0xFF0000,
};
typedef void* MaaWin32Hwnd;

typedef void* MaaTransparentArg;
typedef MaaTransparentArg MaaCallbackTransparentArg;

typedef void (*MaaAPICallback)(MaaStringView msg, MaaStringView details_json, MaaTransparentArg callback_arg);
typedef MaaAPICallback MaaResourceCallback;
typedef MaaAPICallback MaaControllerCallback;
typedef MaaAPICallback MaaInstanceCallback;

struct MaaCustomControllerAPI;
typedef struct MaaCustomControllerAPI* MaaCustomControllerHandle;

struct MaaCustomRecognizerAPI;
typedef struct MaaCustomRecognizerAPI* MaaCustomRecognizerHandle;

struct MaaCustomActionAPI;
typedef struct MaaCustomActionAPI* MaaCustomActionHandle;

struct MaaSyncContextAPI;
typedef struct MaaSyncContextAPI* MaaSyncContextHandle;

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct MaaRect
    {
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
    }* MaaRectHandle;

#ifdef __cplusplus
}
#endif
