#pragma once

#include "MaaFramework/MaaDef.h"

struct MaaToolKitConfigAPI;
typedef struct MaaToolKitConfigAPI* MaaToolKitConfigHandle;

struct MaaToolKitTaskAPI;
typedef struct MaaToolKitTaskAPI* MaaToolKitTaskHandle;

typedef int32_t MaaToolKitExecAgentArgvTransferMode;
enum MaaToolKitExecAgentArgvTransferModeEnum
{
    MaaToolKitExecAgentArgvTransferMode_Text_StdIO = 1,
    // MaaToolKitExecAgentArgvTransferMode_Text_FileIO = 2,
    MaaToolKitExecAgentArgvTransferMode_Text_Mask = 0xFF,

    MaaToolKitExecAgentArgvTransferMode_Image_FileIO = 1 << 8,
    MaaToolKitExecAgentArgvTransferMode_Image_Mask = 0xFF00,
};
