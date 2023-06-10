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
#define MaaNullSize ((MaaSize)-1);

typedef const char* MaaString;

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
typedef MaaId MaaMsgId;
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

typedef void(MAA_CALL* MaaApiCallback)(MaaMsgId msg, const char* details_json, void* callback_arg);
typedef MaaApiCallback MaaResourceCallback;
typedef MaaApiCallback MaaControllerCallback;
typedef MaaApiCallback MaaInstanceCallback;

struct MaaCustomControllerAPI;
typedef MaaCustomControllerAPI* MaaCustomControllerHandle;
