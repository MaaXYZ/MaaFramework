#pragma once

#include "MaaPort.h"
#include <stdint.h>

struct MaaResourceAPI;
typedef struct MaaResourceAPI* MaaResourceHandle;
struct MaaControllerAPI;
typedef struct MaaControllerAPI* MaaControllerHandle;
struct MaaInstanceAPI;
typedef struct MaaInstanceAPI* MaaInstanceHandle;

typedef uint8_t MaaBool;
typedef uint64_t MaaSize;
#define MaaNullSize ((MaaSize)-1)

typedef const char* MaaString;
typedef MaaString MaaJsonString;

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

typedef int64_t MaaId;
typedef MaaId MaaCtrlId;
typedef MaaId MaaResId;
typedef MaaId MaaTaskId;
#define MaaInvalidId ((MaaId)0)

typedef int32_t MaaOption;
typedef MaaOption MaaGlobalOption;
enum MaaGlobalOptionEnum
{
    MaaGlobalOption_Invalid = 0,
    MaaGlobalOption_Logging = 1,
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
};

typedef MaaOption MaaInstOption;
enum MaaInstOptionEnum
{
    MaaInstOption_Invalid = 0,
};

typedef int32_t MaaTaskType;
enum MaaTaskTypeEnum
{
    MaaTaskType_Invalid = 0,
    MaaTaskType_Pipeline = 1,
};

#define MaaTaskParam_Empty "{}"

typedef int32_t MaaAdbControllerType;
enum MaaAdbControllerTypeEnum
{
    MaaAdbControllerType_Touch_Adb = 1,
    MaaAdbControllerType_Touch_MiniTouch = 2,
    MaaAdbControllerType_Touch_MaaTouch = 3,
    MaaAdbControllerType_Touch_Mask = 0xFF,

    MaaAdbControllerType_Key_Adb = 1 << 8,
    MaaAdbControllerType_Key_MaaTouch = 2 << 8,
    MaaAdbControllerType_Key_Mask = 0xFF00,

    // same as Screencap::Method
    MaaAdbControllerType_Screencap_Auto = 1 << 16,
    MaaAdbControllerType_Screencap_RawByNetcat = 2 << 16,
    MaaAdbControllerType_Screencap_RawWithGzip = 3 << 16,
    MaaAdbControllerType_Screencap_Encode = 4 << 16,
    MaaAdbControllerType_Screencap_EncodeToFile = 5 << 16,
    MaaAdbControllerType_Screencap_MinicapDirect = 6 << 16,
    MaaAdbControllerType_Screencap_MinicapStream = 7 << 16,
    MaaAdbControllerType_Screencap_Mask = 0xFF0000,

    MaaAdbControllerType_Input_Preset_Adb = MaaAdbControllerType_Touch_Adb | MaaAdbControllerType_Key_Adb,
    MaaAdbControllerType_Input_Preset_Minitouch = MaaAdbControllerType_Touch_MiniTouch | MaaAdbControllerType_Key_Adb,
    MaaAdbControllerType_Input_Preset_Maatouch =
        MaaAdbControllerType_Touch_MaaTouch | MaaAdbControllerType_Key_MaaTouch,
};

typedef void(* MAA_APICallback)(MaaString msg, MaaString details_json, void* callback_arg);
typedef MAA_APICallback MaaResourceCallback;
typedef MAA_APICallback MaaControllerCallback;
typedef MAA_APICallback MaaInstanceCallback;

struct MaaCustomControllerAPI;
typedef MaaCustomControllerAPI* MaaCustomControllerHandle;
