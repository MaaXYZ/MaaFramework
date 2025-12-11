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
#define MaaNullSize UINT64_MAX

typedef int64_t MaaId;
typedef MaaId MaaCtrlId;
typedef MaaId MaaResId;
typedef MaaId MaaTaskId;
typedef MaaId MaaRecoId;
typedef MaaId MaaActId;
typedef MaaId MaaNodeId;
typedef MaaId MaaSinkId;
#define MaaInvalidId ((MaaId)0)

typedef struct MaaStringBuffer MaaStringBuffer;
typedef struct MaaImageBuffer MaaImageBuffer;
typedef struct MaaStringListBuffer MaaStringListBuffer;
typedef struct MaaImageListBuffer MaaImageListBuffer;

typedef struct MaaResource MaaResource;
typedef struct MaaController MaaController;
typedef struct MaaTasker MaaTasker;
typedef struct MaaContext MaaContext;

typedef int32_t MaaStatus;

enum MaaStatusEnum
{
    MaaStatus_Invalid = 0,
    MaaStatus_Pending = 1000,
    MaaStatus_Running = 2000,
    MaaStatus_Succeeded = 3000,
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

    // Deprecated
    // Dump all screenshots and actions
    //
    // Recording will evaluate to true if any of this or MaaCtrlOptionEnum::MaaCtrlOption_Recording is true.
    // value: bool, eg: true; val_size: sizeof(bool)
    // MaaGlobalOption_Recording = 3,

    /// The level of log output to stdout
    ///
    /// value: MaaLoggingLevel, val_size: sizeof(MaaLoggingLevel)
    /// default value is MaaLoggingLevel_Error
    MaaGlobalOption_StdoutLevel = 4,

    // Deprecated
    // Whether to show hit draw
    //
    // value: bool, eg: true; val_size: sizeof(bool)
    // MaaGlobalOption_ShowHitDraw = 5,

    /// Whether to debug
    ///
    /// value: bool, eg: true; val_size: sizeof(bool)
    MaaGlobalOption_DebugMode = 6,

    /// Whether to save screenshot on error
    ///
    /// value: bool, eg: true; val_size: sizeof(bool)
    MaaGlobalOption_SaveOnError = 7,

    /// Image quality for draw images
    ///
    /// value: int, eg: 85; val_size: sizeof(int)
    /// default value is 85, range: [0, 100]
    MaaGlobalOption_DrawQuality = 8,

    /// Recognition image cache limit
    ///
    /// value: size_t, eg: 4096; val_size: sizeof(size_t)
    /// default value is 4096
    MaaGlobalOption_RecoImageCacheLimit = 9,
};

typedef MaaOption MaaResOption;

typedef int32_t MaaInferenceDevice;
typedef int32_t MaaInferenceExecutionProvider;

enum MaaInferenceDeviceEnum
{
    MaaInferenceDevice_CPU = -2,
    MaaInferenceDevice_Auto = -1,
    MaaInferenceDevice_0 = 0,
    MaaInferenceDevice_1 = 1,
    // and more gpu id or flag...
};

enum MaaInferenceExecutionProviderEnum
{
    // I don't recommend setting up MaaResOption_InferenceDevice in this case,
    // because you don't know which EP will be used on different user devices.
    MaaInferenceExecutionProvider_Auto = 0,

    // MaaResOption_InferenceDevice will not work.
    MaaInferenceExecutionProvider_CPU = 1,

    // MaaResOption_InferenceDevice will be used to set adapter id,
    // It's from Win32 API `EnumAdapters1`.
    MaaInferenceExecutionProvider_DirectML = 2,

    // MaaResOption_InferenceDevice will be used to set coreml_flag,
    // Reference to
    // https://github.com/microsoft/onnxruntime/blob/main/include/onnxruntime/core/providers/coreml/coreml_provider_factory.h
    // But you need to pay attention to the onnxruntime version we use, the latest flag may not be supported.
    MaaInferenceExecutionProvider_CoreML = 3,

    // MaaResOption_InferenceDevice will be used to set NVIDIA GPU ID
    // TODO!
    MaaInferenceExecutionProvider_CUDA = 4,
};

enum MaaResOptionEnum
{
    MaaResOption_Invalid = 0,

    /// Use the specified inference device.
    /// Please set this option before loading the model.
    ///
    /// value: MaaInferenceDevice, eg: 0; val_size: sizeof(MaaInferenceDevice)
    /// default value is MaaInferenceDevice_Auto
    MaaResOption_InferenceDevice = 1,

    /// Use the specified inference execution provider
    /// Please set this option before loading the model.
    ///
    /// value: MaaInferenceExecutionProvider, eg: 0; val_size: sizeof(MaaInferenceExecutionProvider)
    /// default value is MaaInferenceExecutionProvider_Auto
    MaaResOption_InferenceExecutionProvider = 2,
};

typedef MaaOption MaaCtrlOption;

/**
 * @brief Option keys for controller instance options. See MaaControllerSetOption().
 *
 */
enum MaaCtrlOptionEnum
{
    MaaCtrlOption_Invalid = 0,

    /// Only one of long and short side can be set, and the other is automatically scaled according to the aspect ratio.
    ///
    /// value: int, eg: 1280; val_size: sizeof(int)
    MaaCtrlOption_ScreenshotTargetLongSide = 1,

    /// Only one of long and short side can be set, and the other is automatically scaled according to the aspect ratio.
    ///
    /// value: int, eg: 720; val_size: sizeof(int)
    MaaCtrlOption_ScreenshotTargetShortSide = 2,

    /// Screenshot use raw size without scaling.
    /// Please note that this option may cause incorrect coordinates on user devices with different resolutions if scaling is not performed.
    ///
    /// value: bool, eg: true; val_size: sizeof(bool)
    MaaCtrlOption_ScreenshotUseRawSize = 3,

    // Deprecated
    // Dump all screenshots and actions
    //
    // Recording will evaluate to true if any of this or
    // MaaGlobalOptionEnum::MaaGlobalOption_Recording is true.
    //
    // value: bool, eg: true; val_size: sizeof(bool)
    // MaaCtrlOption_Recording = 5,
};

typedef MaaOption MaaTaskerOption;

enum MaaTaskerOptionEnum
{
    MaaTaskerOption_Invalid = 0,
};

// MaaAdbScreencapMethod:
/**
 * @brief Adb screencap method flags
 *
 * Use bitwise OR to set the methods you need.
 * MaaFramework will test all provided methods and use the fastest available one.
 *
 * Default: All methods except RawByNetcat, MinicapDirect, MinicapStream
 *
 * Note: MinicapDirect and MinicapStream use lossy JPEG encoding, which may
 * significantly reduce template matching accuracy. Not recommended.
 *
 * | Method                | Speed      | Compatibility | Encoding | Notes                             |
 * |-----------------------|------------|---------------|----------|-----------------------------------|
 * | EncodeToFileAndPull   | Slow       | High          | Lossless |                                   |
 * | Encode                | Slow       | High          | Lossless |                                   |
 * | RawWithGzip           | Medium     | High          | Lossless |                                   |
 * | RawByNetcat           | Fast       | Low           | Lossless |                                   |
 * | MinicapDirect         | Fast       | Low           | Lossy    |                                   |
 * | MinicapStream         | Very Fast  | Low           | Lossy    |                                   |
 * | EmulatorExtras        | Very Fast  | Low           | Lossless | Emulators only: MuMu 12, LDPlayer 9 |
 */
typedef uint64_t MaaAdbScreencapMethod;
#define MaaAdbScreencapMethod_EncodeToFileAndPull 1ULL
#define MaaAdbScreencapMethod_Encode (1ULL << 1)
#define MaaAdbScreencapMethod_RawWithGzip (1ULL << 2)
#define MaaAdbScreencapMethod_RawByNetcat (1ULL << 3)
#define MaaAdbScreencapMethod_MinicapDirect (1ULL << 4)
#define MaaAdbScreencapMethod_MinicapStream (1ULL << 5)
#define MaaAdbScreencapMethod_EmulatorExtras (1ULL << 6)

#define MaaAdbScreencapMethod_None 0ULL
#define MaaAdbScreencapMethod_All (~MaaAdbScreencapMethod_None)
#define MaaAdbScreencapMethod_Default                                                                          \
    (MaaAdbScreencapMethod_All & (~MaaAdbScreencapMethod_RawByNetcat) & (~MaaAdbScreencapMethod_MinicapDirect) \
     & (~MaaAdbScreencapMethod_MinicapStream))

// MaaAdbInputMethod:
/**
 * @brief Adb input method flags
 *
 * Use bitwise OR to set the methods you need.
 * MaaFramework will select the first available method according to priority.
 *
 * Priority (high to low): EmulatorExtras > Maatouch > MinitouchAndAdbKey > AdbShell
 *
 * Default: All methods except EmulatorExtras
 *
 * | Method               | Speed | Compatibility | Notes                                 |
 * |----------------------|-------|---------------|---------------------------------------|
 * | AdbShell             | Slow  | High          |                                       |
 * | MinitouchAndAdbKey   | Fast  | Medium        | Key press still uses AdbShell         |
 * | Maatouch             | Fast  | Medium        |                                       |
 * | EmulatorExtras       | Fast  | Low           | Emulators only: MuMu 12               |
 */
typedef uint64_t MaaAdbInputMethod;
#define MaaAdbInputMethod_AdbShell 1ULL
#define MaaAdbInputMethod_MinitouchAndAdbKey (1ULL << 1)
#define MaaAdbInputMethod_Maatouch (1ULL << 2)
#define MaaAdbInputMethod_EmulatorExtras (1ULL << 3)

#define MaaAdbInputMethod_None 0ULL
#define MaaAdbInputMethod_All (~MaaAdbInputMethod_None)
#define MaaAdbInputMethod_Default (MaaAdbInputMethod_All & (~MaaAdbInputMethod_EmulatorExtras))

// MaaWin32ScreencapMethod:
/**
 * @brief Win32 screencap method
 *
 * No bitwise OR, select ONE method only.
 *
 * No default value. Client should choose one as default.
 *
 * Different applications use different rendering methods, there is no universal solution.
 *
 * | Method                  | Speed     | Compatibility | Require Admin | Background Support | Notes                            |
 * |-------------------------|-----------|---------------|---------------|--------------------|----------------------------------|
 * | GDI                     | Fast      | Medium        | No            | No                 |                                  |
 * | FramePool               | Very Fast | Medium        | No            | Yes                | Requires Windows 10 1903+        |
 * | DXGI_DesktopDup         | Very Fast | Low           | No            | No                 | Desktop duplication (full screen)|
 * | DXGI_DesktopDup_Window  | Very Fast | Low           | No            | No                 | Desktop duplication then crop    |
 * | PrintWindow             | Medium    | Medium        | No            | Yes                |                                  |
 * | ScreenDC                | Fast      | High          | No            | No                 |                                  |
 *
 * Note: When a window is minimized on Windows, all screencap methods will fail.
 * Avoid minimizing the target window.
 */
typedef uint64_t MaaWin32ScreencapMethod;
#define MaaWin32ScreencapMethod_None 0ULL
#define MaaWin32ScreencapMethod_GDI 1ULL
#define MaaWin32ScreencapMethod_FramePool (1ULL << 1)
#define MaaWin32ScreencapMethod_DXGI_DesktopDup (1ULL << 2)
#define MaaWin32ScreencapMethod_DXGI_DesktopDup_Window (1ULL << 3)
#define MaaWin32ScreencapMethod_PrintWindow (1ULL << 4)
#define MaaWin32ScreencapMethod_ScreenDC (1ULL << 5)

// MaaWin32InputMethod:
/**
 * @brief Win32 input method
 *
 * No bitwise OR, select ONE method only.
 *
 * No default value. Client should choose one as default.
 *
 * Different applications process input differently, there is no universal solution.
 *
 * | Method                       | Compatibility | Require Admin | Seize Mouse | Background Support | Notes |
 * |------------------------------|---------------|---------------|--------------|--------------------|-------------------------------------------------------------|
 * | Seize                        | High          | No            | Yes          | No                 | | | SendMessage                  |
 * Medium        | Maybe         | No           | Yes                |                                                             | |
 * PostMessage                  | Medium        | Maybe         | No           | Yes                | | | LegacyEvent                  | Low
 * | No            | Yes          | No                 |                                                             | | PostThreadMessage
 * | Low           | Maybe         | No           | Yes                |                                                             | |
 * SendMessageWithCursorPos     | Medium        | Maybe         | Briefly      | Yes                | Designed for apps that check real
 * cursor position           | | PostMessageWithCursorPos     | Medium        | Maybe         | Briefly      | Yes                | Designed
 * for apps that check real cursor position           |
 *
 * Note:
 * - Admin rights mainly depend on the target application's privilege level.
 *   If the target runs as admin, MaaFramework should also run as admin for compatibility.
 * - "WithCursorPos" methods briefly move the cursor to target position, send message,
 *   then restore cursor position. This "briefly" seizes the mouse but won't block user operations.
 */
typedef uint64_t MaaWin32InputMethod;
#define MaaWin32InputMethod_None 0ULL
#define MaaWin32InputMethod_Seize 1ULL
#define MaaWin32InputMethod_SendMessage (1ULL << 1)
#define MaaWin32InputMethod_PostMessage (1ULL << 2)
#define MaaWin32InputMethod_LegacyEvent (1ULL << 3)
#define MaaWin32InputMethod_PostThreadMessage (1ULL << 4)
#define MaaWin32InputMethod_SendMessageWithCursorPos (1ULL << 5)
#define MaaWin32InputMethod_PostMessageWithCursorPos (1ULL << 6)
#define MaaWin32InputMethod_Gamepad (1ULL << 7)

// MaaDbgControllerType:
/**
 * No bitwise OR, just set it
 */
typedef uint64_t MaaDbgControllerType;
#define MaaDbgControllerType_None 0
#define MaaDbgControllerType_CarouselImage 1ULL
#define MaaDbgControllerType_ReplayRecording (1ULL << 1)

typedef uint64_t MaaControllerFeature;
#define MaaControllerFeature_None 0
#define MaaControllerFeature_UseMouseDownAndUpInsteadOfClick 1ULL
#define MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick (1ULL << 1)

typedef struct MaaRect
{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} MaaRect;

/*
 * See MaaMsg.h
 */

// Deprecated, use MaaEventCallback instead
typedef void(MAA_CALL* MaaNotificationCallback)(const char* message, const char* details_json, void* notify_trans_arg);

/// void* handle:
/// - MaaTasker* for MaaTasker event
/// - MaaResource* for MaaResource event
/// - MaaController* for MaaController event
/// - MaaContext* for MaaContext event
typedef void(MAA_CALL* MaaEventCallback)(void* handle, const char* message, const char* details_json, void* trans_arg);

typedef MaaBool(MAA_CALL* MaaCustomRecognitionCallback)(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_recognition_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    const MaaRect* roi,
    void* trans_arg,
    /* out */ MaaRect* out_box,
    /* out */ MaaStringBuffer* out_detail);

typedef MaaBool(MAA_CALL* MaaCustomActionCallback)(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg);
