#include "ControlUnit/DbgControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "ReplayRecording/ReplayRecordingMgr.h"

const char* MaaDbgControlUnitGetVersion()
{
#pragma message("MaaDbgControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaDbgControlUnitHandle MaaDbgControlUnitCreate(const char* read_path)
{
    LogFunc << VAR(read_path);

    if (!read_path) {
        LogError << "read_path is null";
        return nullptr;
    }

    auto handle = MAA_CTRL_UNIT_NS::create_replay_recording(MAA_NS::path(read_path));

    LogDebug << VAR_VOIDP(handle);

    return handle;
}

void MaaDbgControlUnitDestroy(MaaDbgControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
