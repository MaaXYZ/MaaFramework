#include "ControlUnit/ReplayControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "ReplayRecording/ReplayControllerMgr.h"

const char* MaaReplayControlUnitGetVersion()
{
#pragma message("MaaReplayControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaReplayControlUnitHandle MaaReplayControlUnitCreate(const char* dump_dir)
{
    LogFunc << VAR(dump_dir);

    if (!dump_dir) {
        LogError << "dump_dir is null";
        return nullptr;
    }

    auto handle = MAA_CTRL_UNIT_NS::create_replay_controller(MAA_NS::path(dump_dir));

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
