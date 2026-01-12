#include "MaaFramework/MaaAPI.h"

#include "MaaUtils/Logger.h"

MaaController* MaaAdbControllerCreate(const char*, const char*, MaaAdbScreencapMethod, MaaAdbInputMethod, const char*, const char*)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
    return nullptr;
}

MaaController* MaaWin32ControllerCreate(void*, MaaWin32ScreencapMethod, MaaWin32InputMethod, MaaWin32InputMethod)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
    return nullptr;
}

MaaController* MaaCustomControllerCreate(MaaCustomControllerCallbacks*, void*)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
    return nullptr;
}

MaaController* MaaDbgControllerCreate(const char*, const char*, MaaDbgControllerType, const char*)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
    return nullptr;
}

MaaController* MaaPlayCoverControllerCreate(const char*, const char*)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
    return nullptr;
}

MaaController* MaaGamepadControllerCreate(void*, MaaWin32ScreencapMethod)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
    return nullptr;
}

void MaaControllerDestroy(MaaController*)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
}

MaaResource* MaaResourceCreate()
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
    return nullptr;
}

void MaaResourceDestroy(MaaResource*)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
}

MaaTasker* MaaTaskerCreate()
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
    return nullptr;
}

void MaaTaskerDestroy(MaaTasker*)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
}

MaaBool MaaGlobalSetOption(MaaGlobalOption, MaaOptionValue, MaaOptionValueSize)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
    return false;
}

MaaBool MaaGlobalLoadPlugin(const char*)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
    return false;
}

MaaBool MaaSetGlobalOption(MaaGlobalOption, MaaOptionValue, MaaOptionValueSize)
{
    LogError << "MaaAgentServer Not implement this API, Please use MaaFramework";
    return false;
}
