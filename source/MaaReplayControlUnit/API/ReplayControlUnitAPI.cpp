#include "ControlUnit/ReplayControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "ReplayRecording/ReplayControllerMgr.h"

const char* MaaReplayControlUnitGetVersion()
{
#pragma message("MaaReplayControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaReplayControlUnitHandle MaaReplayControlUnitCreate(const char* read_path)
{
    LogFunc << VAR(read_path);

    if (!read_path) {
        LogError << "read_path is null";
        return nullptr;
    }

    auto handle = MAA_CTRL_UNIT_NS::create_replay_controller(MAA_NS::path(read_path));

    LogDebug << VAR_VOIDP(handle);

    return handle;
}

void MaaReplayControlUnitDestroy(MaaReplayControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
