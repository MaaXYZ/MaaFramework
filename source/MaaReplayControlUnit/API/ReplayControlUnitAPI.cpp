#include "MaaControlUnit/ReplayControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "ReplayRecording/ReplayControllerMgr.h"

const char* MaaReplayControlUnitGetVersion()
{
#pragma message("MaaReplayControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaReplayControlUnitHandle MaaReplayControlUnitCreate(const char* recording_path)
{
    LogFunc << VAR(recording_path);

    if (!recording_path) {
        LogError << "recording_path is null";
        return nullptr;
    }

    auto handle = MAA_CTRL_UNIT_NS::create_replay_controller(MAA_NS::path(recording_path));

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
