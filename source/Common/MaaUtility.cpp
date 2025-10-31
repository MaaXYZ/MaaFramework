#include "MaaFramework/MaaAPI.h"

#include "MaaUtils/Logger.h"

const char* MaaVersion()
{
#pragma message("MaaFramework MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}
