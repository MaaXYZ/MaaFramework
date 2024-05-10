/**
 * @file MaaDef.h
 * @author
 * @brief The type definitions.
 *
 * @copyright Copyright (c) 2024
 *
 */

// IWYU pragma: private, include <MaaFramework/MaaAPI.h>

#pragma once

#include "MaaPort.h" // IWYU pragma: export
#include <stdint.h>

struct MaaStringBuffer;
typedef struct MaaStringBuffer* MaaStringBufferHandle;
struct MaaImageBuffer;
typedef struct MaaImageBuffer* MaaImageBufferHandle;
struct MaaStringListBuffer;
typedef struct MaaStringListBuffer* MaaStringListBufferHandle;
struct MaaImageListBuffer;
typedef struct MaaImageListBuffer* MaaImageListBufferHandle;

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
typedef MaaId MaaRecoId;
typedef MaaId MaaNodeId;
#define MaaInvalidId ((MaaId)0)

typedef int32_t MaaOption;
typedef void* MaaOptionValue;
typedef uint64_t MaaOptionValueSize;

typedef MaaOption MaaGlobalOption;

enum MaaGlobalOptionEnum
{
    MaaGlobalOption_Invalid = 0,

    /// Log dir
    ///
    /// value: string, eg: "C:\\Users\\Administrator\\Desktop\\log"; val_size: string length
    MaaGlobalOption_LogDir = 1,

    /// Whether to save draw
    ///
    /// value: bool, eg: true; val_size: sizeof(bool)
    MaaGlobalOption_SaveDraw = 2,

    /// Dump all screenshots and actions
    ///
    /// Recording will evaluate to true if any of this or MaaCtrlOptionEnum::MaaCtrlOption_Recording
    /// is true. value: bool, eg: true; val_size: sizeof(bool)
    MaaGlobalOption_Recording = 3,

    /// The level of log output to stdout
    ///
    /// value: MaaLoggingLevel, val_size: sizeof(MaaLoggingLevel)
    /// default value is MaaLoggingLevel_Error
    MaaGlobalOption_StdoutLevel = 4,

    /// Whether to show hit draw
    ///
    /// value: bool, eg: true; val_size: sizeof(bool)
    MaaGlobalOption_ShowHitDraw = 5,

    /// Whether to callback debug message
    ///
    /// value: bool, eg: true; val_size: sizeof(bool)
    MaaGlobalOption_DebugMessage = 6,
};

typedef MaaOption MaaResOption;

enum MaaResOptionEnum
{
    MaaResOption_Invalid = 0,
};

typedef MaaOption MaaCtrlOption;

/**
 * @brief Option keys for controller instance options. See MaaControllerSetOption().
 *
 */
enum MaaCtrlOptionEnum
{
    MaaCtrlOption_Invalid = 0,

    /// Only one of long and short side can be set, and the other is automatically scaled according
    /// to the aspect ratio.
    ///
    /// value: int, eg: 1920; val_size: sizeof(int)
    MaaCtrlOption_ScreenshotTargetLongSide = 1,

    /// Only one of long and short side can be set, and the other is automatically scaled according
    /// to the aspect ratio.
    ///
    /// value: int, eg: 1080; val_size: sizeof(int)
    MaaCtrlOption_ScreenshotTargetShortSide = 2,

    /// For StartApp
    ///
    /// value: string, eg: "com.hypergryph.arknights/com.u8.sdk.U8UnityContext"; val_size: string
    /// length
    MaaCtrlOption_DefaultAppPackageEntry = 3,

    /// For StopApp
    ///
    /// value: string, eg: "com.hypergryph.arknights"; val_size: string length
    MaaCtrlOption_DefaultAppPackage = 4,

    /// Dump all screenshots and actions
    ///
    /// Recording will evaluate to true if any of this or
    /// MaaGlobalOptionEnum::MaaGlobalOption_Recording is true.
    ///
    /// value: bool, eg: true; val_size: sizeof(bool)
    MaaCtrlOption_Recording = 5,
};

typedef MaaOption MaaInstOption;

enum MaaInstOptionEnum
{
    MaaInstOption_Invalid = 0,
};

#define MaaTaskParam_Empty "{}"

typedef int32_t MaaAdbControllerType;
#define MaaAdbControllerType_Touch_Mask 0xFF
#define MaaAdbControllerType_Key_Mask 0xFF00
#define MaaAdbControllerType_Screencap_Mask 0xFF0000

/**
 * @brief ADB controller type
 *
 * The ADB controller type consists of three parts: touch, key, and screencap.
 * The touch part is used to control the touch events on the device.
 * The key part is used to control the key events on the device.
 * The screencap part is used to capture the screen of the device.
 * The final value is the combination of the three parts as follows:
 *
 * touch_type | key_type | screencap_type
 *
 */
enum MaaAdbControllerTypeEnum
{
    MaaAdbControllerType_Invalid = 0,

    MaaAdbControllerType_Touch_Adb = 1,
    MaaAdbControllerType_Touch_MiniTouch = 2,
    MaaAdbControllerType_Touch_MaaTouch = 3,
    MaaAdbControllerType_Touch_EmulatorExtras = 4,
    MaaAdbControllerType_Touch_AutoDetect = MaaAdbControllerType_Touch_Mask - 1,

    MaaAdbControllerType_Key_Adb = 1 << 8,
    MaaAdbControllerType_Key_MaaTouch = 2 << 8,
    MaaAdbControllerType_Key_EmulatorExtras = 3 << 8,
    MaaAdbControllerType_Key_AutoDetect = MaaAdbControllerType_Key_Mask - (1 << 8),

    MaaAdbControllerType_Input_Preset_Adb =
        MaaAdbControllerType_Touch_Adb | MaaAdbControllerType_Key_Adb,
    MaaAdbControllerType_Input_Preset_Minitouch =
        MaaAdbControllerType_Touch_MiniTouch | MaaAdbControllerType_Key_Adb,
    MaaAdbControllerType_Input_Preset_Maatouch =
        MaaAdbControllerType_Touch_MaaTouch | MaaAdbControllerType_Key_MaaTouch,
    MaaAdbControllerType_Input_Preset_AutoDetect =
        MaaAdbControllerType_Touch_AutoDetect | MaaAdbControllerType_Key_AutoDetect,
    MaaAdbControllerType_Input_Preset_EmulatorExtras =
        MaaAdbControllerType_Touch_EmulatorExtras | MaaAdbControllerType_Key_EmulatorExtras,

    /// \deprecated
    MaaAdbControllerType_Screencap_FastestWay_Compatible = 1 << 16,
    MaaAdbControllerType_Screencap_RawByNetcat = 2 << 16,
    MaaAdbControllerType_Screencap_RawWithGzip = 3 << 16,
    MaaAdbControllerType_Screencap_Encode = 4 << 16,
    MaaAdbControllerType_Screencap_EncodeToFile = 5 << 16,
    MaaAdbControllerType_Screencap_MinicapDirect = 6 << 16,
    MaaAdbControllerType_Screencap_MinicapStream = 7 << 16,
    MaaAdbControllerType_Screencap_EmulatorExtras = 8 << 16,
    MaaAdbControllerType_Screencap_FastestLosslessWay =
        MaaAdbControllerType_Screencap_Mask - (2 << 16),
    MaaAdbControllerType_Screencap_FastestWay = MaaAdbControllerType_Screencap_Mask - (1 << 16),
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
#define MaaWin32ControllerType_Touch_Mask 0xFF
#define MaaWin32ControllerType_Key_Mask 0xFF00
#define MaaWin32ControllerType_Screencap_Mask 0xFF0000

/**
 * @brief Win32 controller type
 *
 * See AdbControllerTypeEnum to know how the value is composed.
 *
 */
enum MaaWin32ControllerTypeEnum
{
    MaaWin32Controller_Invalid = 0,

    MaaWin32ControllerType_Touch_SendMessage = 1,
    MaaWin32ControllerType_Touch_Seize = 2,

    MaaWin32ControllerType_Key_SendMessage = 1 << 8,
    MaaWin32ControllerType_Key_Seize = 2 << 8,

    MaaWin32ControllerType_Screencap_GDI = 1 << 16,
    MaaWin32ControllerType_Screencap_DXGI_DesktopDup = 2 << 16,
    // MaaWin32ControllerType_Screencap_DXGI_BackBuffer = 3 << 16,
    MaaWin32ControllerType_Screencap_DXGI_FramePool = 4 << 16,
};

typedef void* MaaWin32Hwnd;

typedef void* MaaTransparentArg;
typedef MaaTransparentArg MaaCallbackTransparentArg;

/**
 * @brief The callback function type.
 *
 * @param msg The message. See MaaMsg.h
 * @param details_json The details in JSON format. See doc in MaaMsg.h
 */
typedef void (
    *MaaAPICallback)(MaaStringView msg, MaaStringView details_json, MaaTransparentArg callback_arg);
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
