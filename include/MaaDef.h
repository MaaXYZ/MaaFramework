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
typedef const char* MaaString;

typedef int64_t MaaId;
typedef MaaId MaaMsgId;
typedef MaaId MaaCtrlId;
typedef MaaId MaaTaskId;

typedef void(MAA_CALL* MaaApiCallback)(MaaMsgId msg, const char* details_json, void* callback_arg);
typedef MaaApiCallback MaaResourceCallback;
typedef MaaApiCallback MaaControllerCallback;
typedef MaaApiCallback MaaInstanceCallback;

struct MaaCustomControllerAPI;
typedef MaaCustomControllerAPI* MaaCustomControllerHandle;
