#pragma once

#include <stdint.h>

typedef uint8_t MaaBool;
typedef uint64_t MaaSize;
#define MaaNullSize ((MaaSize)-1)

typedef const char* MaaString;
typedef MaaString MaaJsonString;

struct MaaToolKitConfigAPI;
typedef struct MaaToolKitConfigAPI* MaaToolKitConfigHandle;

struct MaaToolKitTaskAPI;
typedef struct MaaToolKitTaskAPI* MaaToolKitTaskHandle;