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

#include <stdint.h>

#include "MaaPort.h" // IWYU pragma: export

typedef uint8_t MaaBool;
typedef uint64_t MaaSize;
#define MaaNullSize ((MaaSize)-1)

typedef int64_t MaaId;
typedef MaaId MaaCtrlId;
typedef MaaId MaaResId;
typedef MaaId MaaTaskId;
typedef MaaId MaaRecoId;
typedef MaaId MaaNodeId;
#define MaaInvalidId ((MaaId)0)

struct MaaStringBuffer;
struct MaaImageBuffer;
struct MaaStringListBuffer;
struct MaaImageListBuffer;

struct MaaResource;
struct MaaController;
struct MaaTasker;
struct MaaContext;

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

enum MaaLoggingLevelEnum
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

typedef MaaOption MaaTaskerOption;

enum MaaTaskerOptionEnum
{
    MaaTaskerOption_Invalid = 0,
};

// MaaAdbScreencapMethod:
// Use bitwise OR to set the method you need, MaaFramework will test their speed and use the fastest one.
typedef uint64_t MaaAdbScreencapMethod;
const MaaAdbScreencapMethod MaaAdbScreencapMethod_None = 0ULL;
const MaaAdbScreencapMethod MaaAdbScreencapMethod_EncodeToFileAndPull = 1ULL;
const MaaAdbScreencapMethod MaaAdbScreencapMethod_Encode = 1ULL << 1;
const MaaAdbScreencapMethod MaaAdbScreencapMethod_RawWithGzip = 1ULL << 2;
const MaaAdbScreencapMethod MaaAdbScreencapMethod_RawByNetcat = 1ULL << 3;
const MaaAdbScreencapMethod MaaAdbScreencapMethod_MinicapDirect = 1ULL << 4;
const MaaAdbScreencapMethod MaaAdbScreencapMethod_MinicapStream = 1ULL << 5;
const MaaAdbScreencapMethod MaaAdbScreencapMethod_EmulatorExtras = 1ULL << 6;

const MaaAdbScreencapMethod MaaAdbScreencapMethod_All = ~MaaAdbScreencapMethod_None;
const MaaAdbScreencapMethod MaaAdbScreencapMethod_Default =
    MaaAdbScreencapMethod_All & (~MaaAdbScreencapMethod_MinicapDirect) & (~MaaAdbScreencapMethod_MinicapStream);

// MaaAdbInputMethod:
// Use bitwise OR to set the method you need, MaaFramework will select the available ones according to priority.
// The priority is: EmulatorExtras > Maatouch > MinitouchAndAdbKey > AdbShell
typedef uint64_t MaaAdbInputMethod;
const MaaAdbInputMethod MaaAdbInputMethod_None = 0ULL;
const MaaAdbInputMethod MaaAdbInputMethod_AdbShell = 1ULL;
const MaaAdbInputMethod MaaAdbInputMethod_MinitouchAndAdbKey = 1ULL << 1;
const MaaAdbInputMethod MaaAdbInputMethod_Maatouch = 1ULL << 2;
const MaaAdbInputMethod MaaAdbInputMethod_EmulatorExtras = 1ULL << 3;

const MaaAdbInputMethod MaaAdbInputMethod_All = ~MaaAdbInputMethod_None;
const MaaAdbInputMethod MaaAdbInputMethod_Default = MaaAdbInputMethod_All & (~MaaAdbInputMethod_EmulatorExtras);

// MaaWin32ScreencapMethod:
// No bitwise OR, just set it
typedef uint64_t MaaWin32ScreencapMethod;
const MaaWin32ScreencapMethod MaaWin32ScreencapMethod_None = 0ULL;
const MaaWin32ScreencapMethod MaaWin32ScreencapMethod_GDI = 1ULL;
const MaaWin32ScreencapMethod MaaWin32ScreencapMethod_FramePool = 1ULL << 1;
const MaaWin32ScreencapMethod MaaWin32ScreencapMethod_DXGI_DesktopDup = 1ULL << 2;

// MaaWin32InputMethod:
// No bitwise OR, just set it
typedef uint64_t MaaWin32InputMethod;
const MaaWin32InputMethod MaaWin32InputMethod_None = 0ULL;
const MaaWin32InputMethod MaaWin32InputMethod_Seize = 1ULL;
const MaaWin32InputMethod MaaWin32InputMethod_SendMessage = 1ULL << 1;

// MaaDbgControllerType:
// No bitwise OR, just set it
typedef uint64_t MaaDbgControllerType;
const MaaDbgControllerType MaaDbgControllerType_None = 0;
const MaaDbgControllerType MaaDbgControllerType_CarouselImage = 1ULL;
const MaaDbgControllerType MaaDbgControllerType_ReplayRecording = 1ULL << 1;

struct MaaRect
{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

typedef void (*MaaNotificationCallback)(const char* message, const char* details_json, void* callback_arg);

typedef MaaBool (*MaaCustomRecognizerCallback)(
    MaaContext* context,
    MaaTaskId task_id,
    const char* recognizer_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    void* trans_arg,
    /* out */ MaaRect* out_box,
    /* out */ MaaStringBuffer* out_detail);

typedef MaaBool (*MaaCustomActionCallback)(
    MaaContext* context,
    MaaTaskId task_id,
    const char* action_name,
    const char* custom_action_param,
    const MaaRect* box,
    const char* reco_detail,
    void* trans_arg);
